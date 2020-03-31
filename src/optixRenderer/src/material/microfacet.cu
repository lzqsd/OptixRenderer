/* 
 * Copyright (c) 2016, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <optix.h>
#include <optixu/optixu_math_namespace.h>
#include "helpers.h"
#include "structs/prd.h"
#include "light/envmap.h"
#include "random.h"
#include "commonStructs.h"
#include "lightStructs.h"
#include "light/areaLight.h"
#include <vector>

using namespace optix;


rtDeclareVariable( float3, shading_normal, attribute shading_normal, ); 
rtDeclareVariable( float3, geometric_normal, attribute geometric_normal, );
rtDeclareVariable(float3, tangent_direction, attribute tangent_direction, );
rtDeclareVariable(float3, bitangent_direction, attribute bitangent_direction, );
rtDeclareVariable(int, max_depth, , );

rtDeclareVariable( float3, texcoord, attribute texcoord, );
rtDeclareVariable( float, t_hit, rtIntersectionDistance, );

rtDeclareVariable(optix::Ray, ray,   rtCurrentRay, );
rtDeclareVariable(PerRayData_radiance, prd_radiance, rtPayload, );
rtDeclareVariable(PerRayData_shadow,   prd_shadow, rtPayload, );
rtDeclareVariable(float, scene_epsilon, , );

rtDeclareVariable( float, uvScale, , ); 

// Materials
rtDeclareVariable( float3, albedo, , );
rtTextureSampler<float4, 2> albedoMap;
rtDeclareVariable( int, isAlbedoTexture, , );
rtDeclareVariable( float, rough, , );
rtTextureSampler<float4, 2> roughMap;
rtDeclareVariable( int, isRoughTexture, , );
rtTextureSampler<float4, 2> normalMap;
rtDeclareVariable(int, isNormalTexture, , );
rtDeclareVariable(float, F0, , );
rtDeclareVariable( float, metallic, , );
rtDeclareVariable( int, isMetallicTexture, ,  );
rtTextureSampler<float4, 2> metallicMap;

// Area Light Buffer
rtDeclareVariable(int, isAreaLight, , );

// Environmental Lighting 
rtDeclareVariable(int, isEnvmap, , );
rtDeclareVariable(float, infiniteFar, , );

// Point lighting 
rtDeclareVariable(int, isPointLight, , );
rtDeclareVariable(int, pointLightNum, , );
rtBuffer<Point> pointLights;


// Geometry Group
rtDeclareVariable( rtObject, top_object, , );

rtDeclareVariable(
        rtCallableProgramX<void(unsigned int&, float3&, float3&, float&)>, 
        sampleEnvironmapLight, , );
rtDeclareVariable(
        rtCallableProgramX<void(unsigned int&, float3&, float3&, float3&, float&)>, 
        sampleAreaLight, , );


// Computing the pdfSolidAngle of BRDF giving a direction 
RT_CALLABLE_PROGRAM float LambertianPdf(const float3& L, const float3& N)
{
    float NoL = fmaxf(dot(N, L), 0);
    float pdf = NoL / M_PIf;
    return fmaxf(pdf, 1e-14f);
}
RT_CALLABLE_PROGRAM float SpecularPdf(const float3& L, const float3& V, const float3& N, float R)
{
    float a2 = R * R * R * R;
    float3 H = normalize( (L+V) / 2.0 );
    float NoH = fmaxf(dot(N, H), 0);
    float VoH = fmaxf(dot(V, H), 0);
    float pdf = (a2 * NoH) / fmaxf( (4 * M_PIf * (1 + (a2-1) * NoH)
            *(1 + (a2-1) * NoH) * VoH ), 1e-14f);
    return fmaxf(pdf, 1e-14f);
}
RT_CALLABLE_PROGRAM float pdf(const float3& L, const float3& V, const float3& N, float R)
{
    float pdfLambertian = LambertianPdf(L, N);
    float pdfSpecular = SpecularPdf(L, V, N, R);
    return pdfLambertian * 0.5 + pdfSpecular * 0.5;
}

RT_CALLABLE_PROGRAM float3 evaluate(const float3& albedoValue, const float3& N, const float rough, const float3& fresnel, 
        const float3& V, const float3& L, const float3& radiance)
{
    float alpha = rough * rough;
    float k = (alpha + 2 * rough + 1) / 8.0;
    float alpha2 = alpha * alpha;
    
    float3 H = normalize((L + V) / 2.0f );
    float NoL = fmaxf(dot(N, L), 0);
    float NoV = fmaxf(dot(N, V), 0);
    float NoH = fmaxf(dot(N, H), 0);
    float VoH = fmaxf(dot(V, H), 0);

    float FMi = (-5.55473 * VoH - 6.98316) * VoH;
    float3 frac0 = fresnel + (1 - fresnel) * pow(2.0f, FMi);
    float3 frac = frac0 * alpha2;
    float nom0 = NoH * NoH * (alpha2 - 1) + 1;
    float nom1 = NoV * (1 - k) + k;
    float nom2 = NoL * (1 - k) + k;
    float nom = fmaxf(4 * M_PIf * nom0 * nom0 * nom1 * nom2, 1e-14);
    float3 spec = frac / nom;
         
    float3 intensity = (albedoValue / M_PIf + spec) * NoL * radiance; 
    return intensity;
}

RT_CALLABLE_PROGRAM void sample(unsigned& seed, 
        const float3& albedoValue, const float3& N, const float rough, const float3& fresnel, const float3& V,  
        const float3& ffnormal, 
        optix::Onb onb, 
        float3& attenuation, float3& direction, float& pdfSolid)
{
    const float z1 = rnd( seed );
    const float z2 = rnd( seed );
    const float z = rnd( seed );
    
    float alpha = rough * rough;
    float k = (alpha + 2 * rough + 1) / 8.0;
    float alpha2 = alpha * alpha;
    
    float3 L;
    if(z < 0.5){
        cosine_sample_hemisphere(z1, z2, L);
        onb.inverse_transform(L);
        direction = L;
        attenuation =  2 * attenuation * albedoValue;
    }
    else{
        // Compute the half angle 
        float phi = 2 * M_PIf * z1;
        float cosTheta = sqrt( (1 - z2) / (1 + (alpha2 - 1) * z2) );
        float sinTheta = sqrt( 1 - cosTheta * cosTheta);

        float3 H = make_float3(
                sinTheta * cos(phi),
                sinTheta * sin(phi),
                cosTheta );
        onb.inverse_transform(H);
        L = 2 * dot(V, H) * H - V;
        direction = L;

        float NoV = fmaxf(dot(N, V), 0.0);
        float NoL = dot(N, L);
        float NoH = fmaxf(dot(N, H), 0.0);
        float VoH = fmaxf(dot(V, H), 0.0);

        if( dot(ffnormal, L) >= 0.05 ){
            float G1 = NoV / (NoV * (1-k) + k);
            float G2 = NoL / (NoL * (1-k) + k);
            float FMi = (-5.55473 * VoH - 6.98316) * VoH;
            float3 F = fresnel + (1 - fresnel) * pow(2.0f, FMi);
            float3 reflec = F * G1 * G2 * VoH / fmaxf(NoH * NoV, 1e-14);

            attenuation = 2 * attenuation * reflec;
        }
        else{
            attenuation = make_float3(0.0f);
        }
    }
    pdfSolid = pdf(L, V, N, rough);
}


RT_PROGRAM void closest_hit_radiance()
{
    const float3 world_shading_normal   = normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, shading_normal ) );
    const float3 world_geometric_normal = normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, geometric_normal ) );
    float3 ffnormal = faceforward( world_shading_normal, -ray.direction, world_geometric_normal );
 
    float3 albedoValue;
    if(isAlbedoTexture == 0){
        albedoValue = albedo;
    }
    else{
        albedoValue = make_float3(tex2D(albedoMap, texcoord.x * uvScale, texcoord.y * uvScale) );
        albedoValue.x = pow(albedoValue.x, 2.2);
        albedoValue.y = pow(albedoValue.y, 2.2);
        albedoValue.z = pow(albedoValue.z, 2.2);
    }

    float roughValue = (isRoughTexture == 0) ? rough :
        tex2D(roughMap, texcoord.x * uvScale, texcoord.y * uvScale).x;

    float metallicValue = (isMetallicTexture == 0) ? metallic :
        tex2D(metallicMap, texcoord.x * uvScale, texcoord.y * uvScale).x;

    float3 fresnel = F0 * (1 - metallicValue) + metallicValue * albedoValue;
    albedoValue = (1 - metallicValue) * albedoValue;
    
    float3 V = normalize(-ray.direction );    
    if(dot(ffnormal, V) < 0)
        ffnormal = -ffnormal;
    
    float3 N;
    if( isNormalTexture == 0){
        N = ffnormal;
    }
    else{
        N = make_float3(tex2D(normalMap, texcoord.x * uvScale, texcoord.y * uvScale) );
        N = normalize(2 * N - 1);
        N = N.x * tangent_direction 
            + N.y * bitangent_direction 
            + N.z * ffnormal;
    }
    N = normalize(N );
    optix::Onb onb(N );
 
    float3 hitPoint = ray.origin + t_hit * ray.direction;
    prd_radiance.origin = hitPoint;

    // Connect to the area Light
    {
        if(isAreaLight == 1){
            float3 position, radiance, normal;
            float pdfAreaLight;
            sampleAreaLight(prd_radiance.seed, radiance, position, normal, pdfAreaLight);
   
            float Dist = length(position - hitPoint);
            float3 L = normalize(position - hitPoint);

            if(fmaxf(dot(ffnormal, L), 0.0f) * fmaxf(dot(ffnormal, V), 0.0f) > 0.0025 ){
                float cosPhi = dot(L, normal);
                cosPhi = (cosPhi < 0) ? -cosPhi : cosPhi;

                Ray shadowRay = make_Ray(hitPoint, L, 1, scene_epsilon, Dist - scene_epsilon);
                PerRayData_shadow prd_shadow; 
                prd_shadow.inShadow = false;
                rtTrace(top_object, shadowRay, prd_shadow);
                if(prd_shadow.inShadow == false)
                {
                    float3 intensity = evaluate(albedoValue, N, roughValue, fresnel, V, L, radiance) * cosPhi / Dist / Dist;
                    
                    if(prd_radiance.depth == (max_depth - 1) ){
                    }
                    else{
                        float pdfAreaLight2 = pdfAreaLight * pdfAreaLight;
                        float pdfSolidBRDF = pdf(L, V, N, roughValue);
                        float pdfAreaBRDF = pdfSolidBRDF * cosPhi / Dist / Dist;
                        float pdfAreaBRDF2 = pdfAreaBRDF * pdfAreaBRDF;

                        prd_radiance.radiance += intensity * pdfAreaLight / 
                            fmaxf(pdfAreaBRDF2 + pdfAreaLight2, 1e-14) * prd_radiance.attenuation;            
                    }
                }
            }
        }   
    }

    
    // Connect to point light 
    {
        if(isPointLight == 1){
            // Connect to every point light 
            for(int i = 0; i < pointLightNum; i++){
                float3 position = pointLights[i].position;
                float3 radiance = pointLights[i].intensity;
                float3 L = normalize(position - hitPoint);
                float Dist = length(position - hitPoint);

                if( fmaxf(dot(ffnormal, L), 0.0f) * fmaxf(dot(ffnormal, V), 0.0f) > 0.0025){
                    Ray shadowRay = make_Ray(hitPoint + 0.1 * L * scene_epsilon, L, 1, scene_epsilon, Dist - scene_epsilon);
                    PerRayData_shadow prd_shadow; 
                    prd_shadow.inShadow = false;
                    rtTrace(top_object, shadowRay, prd_shadow);
                    if(prd_shadow.inShadow == false && prd_radiance.depth != (max_depth - 1) ){
                        float3 intensity = evaluate(albedoValue, N, roughValue, fresnel, V, L, radiance) / Dist/ Dist;
                        prd_radiance.radiance += intensity * prd_radiance.attenuation;
                    }
                }
            }
        }
    }

    // Connect to the environmental map 
    { 
        if(isEnvmap == 1){
            float3 L, radiance;
            float pdfSolidEnv;
            sampleEnvironmapLight(prd_radiance.seed, radiance, L, pdfSolidEnv);

            if( fmaxf(dot(L, ffnormal), 0.0f) * fmaxf(dot(V, ffnormal ), 0.0f) > 0.0025){
                Ray shadowRay = make_Ray(hitPoint + 0.1 * scene_epsilon, L, 1, scene_epsilon, infiniteFar);
                PerRayData_shadow prd_shadow;
                prd_shadow.inShadow = false;
                rtTrace(top_object, shadowRay, prd_shadow);
                if(prd_shadow.inShadow == false)
                {
                    float3 intensity = evaluate(albedoValue, N, roughValue, fresnel, V, L, radiance);
                    if(prd_radiance.depth == (max_depth - 1) ){
                    }
                    else{
                        float pdfSolidBRDF = pdf(L, V, N, roughValue);
                        float pdfSolidBRDF2 = pdfSolidBRDF * pdfSolidBRDF;
                        float pdfSolidEnv2 = pdfSolidEnv * pdfSolidEnv;
                        prd_radiance.radiance += intensity * pdfSolidEnv / 
                            fmaxf(pdfSolidEnv2 + pdfSolidBRDF2, 1e-14) * prd_radiance.attenuation; 
                    }
                }
            }
        }
    }


    // Sammple the new ray 
    sample(prd_radiance.seed, 
        albedoValue, N, fmaxf(roughValue, 0.02), fresnel, V, 
        ffnormal, onb, 
        prd_radiance.attenuation, prd_radiance.direction, prd_radiance.pdf );
}

// any_hit_shadow program for every material include the lighting should be the same
RT_PROGRAM void any_hit_shadow()
{
    prd_shadow.inShadow = true;
    rtTerminateRay();
}

