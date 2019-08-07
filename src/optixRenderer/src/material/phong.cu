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
#include "light/envmap.h"
#include "structs/prd.h"
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

// Materials
rtDeclareVariable( float3, albedo, , );
rtTextureSampler<float4, 2> albedoMap;
rtDeclareVariable( int, isAlbedoTexture, , );
rtDeclareVariable( float3, specular, , );
rtTextureSampler<float4, 2> specularMap;
rtDeclareVariable( int, isSpecularTexture, , );
rtDeclareVariable( float, glossy, , );
rtTextureSampler<float4, 2> glossyMap;
rtDeclareVariable( int, isGlossyTexture, , );
rtTextureSampler<float4, 2> normalMap;
rtDeclareVariable(int, isNormalTexture, , );
rtDeclareVariable(float, F0, , );

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
    float NoL = fmaxf(dot(N, L), 1e-14);
    float pdf = NoL / M_PIf;
    return fmaxf(pdf, 1e-14f);
}
RT_CALLABLE_PROGRAM float SpecularPdf(const float3& L, const float3& N, const float3& R, 
        float glossyValue)
{ 
    float RoL = dot(R, L);
    if(RoL < 1e-14) RoL = 0;
    float pdf = (glossyValue + 2) / (2*M_PIf) * pow(RoL, fmaxf(glossyValue, 1e-14) );
    return fmaxf(pdf, 1e-14);
}
RT_CALLABLE_PROGRAM float pdf(const float3& L, const float3& N, const float3& R, const float3& albedoValue, const float3& specularValue, float glossyValue)
{
    float pdfLambertian = LambertianPdf(L, N);
    float pdfSpecular = SpecularPdf(L, N, R, glossyValue);
    float albedoStr = length(albedoValue );
    float specularStr = length(specularValue );
    float pdf = (albedoStr * pdfLambertian + specularStr * pdfLambertian) / fmaxf(albedoStr + specularStr, 1e-14);
    return fmaxf(pdf, 1e-14);
}

RT_CALLABLE_PROGRAM float3 evaluate(const float3& albedoValue, const float3& specularValue, const float3& N, const float glossyValue, 
        const float3& L, const float3& R, const float3& radiance)
{
    float NoL = fmaxf(dot(N, L), 1e-14);

    float RoL = dot(R, L);
    if(RoL < 1e-14) RoL = 0;

    float3 lambertianTerm = albedoValue / M_PIf;
    float3 specularTerm = specularValue / (2*M_PIf) * (glossyValue + 2) * pow(RoL, fmaxf(glossyValue, 1e-14) );
    return (lambertianTerm + specularTerm) * radiance * NoL;
}

RT_CALLABLE_PROGRAM void sample(unsigned& seed, 
        const float3& albedoValue, const float3& specularValue, const float3& N, const float glossyValue, const float3& R, 
        optix::Onb onb, 
        float3& attenuation, float3& direction, float& pdfSolid)
{
    const float z1 = rnd( seed );
    const float z2 = rnd( seed );
    const float z = rnd( seed );
    
    float albedoStr = length(albedoValue );
    float specularStr = length(specularValue );

    float3 L;
    if(z <= albedoStr / fmaxf(albedoStr + specularStr, 1e-14) || (albedoStr + specularStr) < 1e-14 ){
        cosine_sample_hemisphere(z1, z2, L);
        onb.inverse_transform(L);
        attenuation = attenuation * albedoValue * (albedoStr + specularStr) / fmaxf(albedoStr, 1e-14);
    }
    else{
        float z1_1_nP1 = pow(z1, 1 / (glossyValue +1) );
        float z1_2_nP1 = z1_1_nP1 * z1_1_nP1;
        L = make_float3(
                sqrt(1 - z1_2_nP1) * cos(2 * M_PIf * z2), 
                sqrt(1 - z1_2_nP1) * sin(2 * M_PIf * z2),
                z1_1_nP1
                );
        optix::Onb ronb(R);
        ronb.inverse_transform(L);
        float NoL = fmaxf(dot(N, L), 1e-14);
        attenuation = attenuation * specularValue * NoL * (albedoStr + specularStr) / fmaxf(specularStr, 1e-14); 
    }
    direction = L;
    pdfSolid = pdf(L, N, R, albedoValue, specularValue, glossyValue);
    return;
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
        albedoValue = make_float3(tex2D(albedoMap, texcoord.x, texcoord.y) );
        albedoValue.x = pow(albedoValue.x, 2.2);
        albedoValue.y = pow(albedoValue.y, 2.2);
        albedoValue.z = pow(albedoValue.z, 2.2);
    }

    float3 specularValue;
    if(isSpecularTexture == 0){
        specularValue = specular;
    }
    else{
        specularValue = make_float3(tex2D(specularMap, texcoord.x, texcoord.y) );
        specularValue.x = pow(specularValue.x, 2.2);
        specularValue.y = pow(specularValue.y, 2.2);
        specularValue.z = pow(specularValue.z, 2.2);
    }



    float3 colorSum = fmaxf(albedoValue + specularValue, make_float3(1e-14f) );
    float colorMax= colorSum.x;
    if(colorMax < colorSum.y) colorMax = colorSum.y;
    if(colorMax < colorSum.z) colorMax = colorSum.z;
    colorMax = fmaxf(colorMax, 1e-14);

    if(colorMax > 1){
        specularValue = specularValue / colorMax;
        albedoValue = albedoValue / colorMax;
    }

    float glossyValue = (isGlossyTexture == 0) ? glossy :
        tex2D(glossyMap, texcoord.x, texcoord.y).x;
    
    float3 V = normalize(-ray.direction );    
    if(dot(ffnormal, V) < 0)
        ffnormal = -ffnormal;
    
    float3 N;
    if( isNormalTexture == 0){
        N = ffnormal;
    }
    else{
        N = make_float3(tex2D(normalMap, texcoord.x, texcoord.y) );
        N = normalize(2 * N - 1);
        N = N.x * tangent_direction 
            + N.y * bitangent_direction 
            + N.z * ffnormal;
    }
    N = normalize(N );
    optix::Onb onb(N );
    
    float3 hitPoint = ray.origin + t_hit * ray.direction;
    prd_radiance.origin = hitPoint;

    float3 R = 2 * dot(V, N) * N - V;

    // Connect to the area Light
    {
        if(isAreaLight == 1){
            float3 position, radiance, normal;
            float pdfAreaLight;
            sampleAreaLight(prd_radiance.seed, radiance, position, normal, pdfAreaLight);
   
            float Dist = length(position - hitPoint);
            float3 L = normalize(position - hitPoint);

            if(fmaxf(dot(N, L), 0.0) * fmaxf(dot(N, V), 0.0) > 0 ){
                float cosPhi = dot(L, normal);
                cosPhi = (cosPhi < 0) ? -cosPhi : cosPhi;

                Ray shadowRay = make_Ray(hitPoint, L, 1, scene_epsilon, Dist - scene_epsilon);
                PerRayData_shadow prd_shadow; 
                prd_shadow.inShadow = false;
                rtTrace(top_object, shadowRay, prd_shadow);
                if(prd_shadow.inShadow == false)
                {
                    float3 intensity = evaluate(albedoValue, specularValue, N, glossyValue, L, R, radiance) * cosPhi / Dist / Dist;
                    if(prd_radiance.depth == (max_depth - 1) ){
                    }
                    else{
                        float pdfSolidBRDF = pdf(L, N, R, albedoValue, specularValue, glossyValue);
                        float pdfAreaBRDF = pdfSolidBRDF * cosPhi / Dist / Dist;

                        float pdfAreaLight2 = pdfAreaLight * pdfAreaLight;
                        float pdfAreaBRDF2 = pdfAreaBRDF * pdfAreaBRDF;

                        prd_radiance.radiance += intensity * pdfAreaLight / (pdfAreaBRDF2 + pdfAreaLight2) * prd_radiance.attenuation;
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

                if(fmaxf(dot(N, L), 0.0) * fmaxf(dot(N, V), 0.0) > 0 ){
                    Ray shadowRay = make_Ray(hitPoint + 0.1 * L * scene_epsilon, L, 1, scene_epsilon, Dist - scene_epsilon);
                    PerRayData_shadow prd_shadow; 
                    prd_shadow.inShadow = false;
                    rtTrace(top_object, shadowRay, prd_shadow);
                    if(prd_shadow.inShadow == false && prd_radiance.depth != (max_depth - 1 ) )
                    {
                        float3 intensity = evaluate(albedoValue, specularValue, N, glossyValue, L, R, radiance) / Dist/ Dist;
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

            if( fmaxf(dot(L, N), 0.0) * fmaxf(dot(V, N), 0.0) > 0){
                Ray shadowRay = make_Ray(hitPoint + 0.1 * scene_epsilon, L, 1, scene_epsilon, infiniteFar);
                PerRayData_shadow prd_shadow;
                prd_shadow.inShadow = false;
                rtTrace(top_object, shadowRay, prd_shadow);
                if(prd_shadow.inShadow == false)
                {
                    float3 intensity = evaluate(albedoValue, specularValue, N, glossyValue, L, R, radiance);
                    if(prd_radiance.depth == (max_depth - 1) ){
                    }
                    else{
                        float pdfSolidBRDF = pdf(L, N, R, albedoValue, specularValue, glossyValue);
                        float pdfSolidBRDF2 = pdfSolidBRDF * pdfSolidBRDF;
                        float pdfSolidEnv2 = pdfSolidEnv * pdfSolidEnv;
                        prd_radiance.radiance += intensity * pdfSolidEnv / (pdfSolidEnv2 + pdfSolidBRDF2) * prd_radiance.attenuation; 
                    }
                }
            }
        }
    }

    // Sammple the new ray 
    sample(prd_radiance.seed, 
        albedoValue, specularValue, N, glossyValue, R, 
        onb, 
        prd_radiance.attenuation, prd_radiance.direction, prd_radiance.pdf);

}    

// any_hit_shadow program for every material include the lighting should be the same
RT_PROGRAM void any_hit_shadow()
{
    prd_shadow.inShadow = true;
    rtTerminateRay();
}

