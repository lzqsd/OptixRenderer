/* 
 * Copyright (c) 2017 NVIDIA CORPORATION. All rights reserved.
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

#include <optix_world.h>
#include "helpers.h"
#include "random.h"

using namespace optix;

struct PerRayData_pathtrace_shadow{
    bool inShadow;
};


struct PerRayData_pathtrace{
    float3 result;
    float3 radiance;
    float3 attenuation;
    float3 origin;
    float3 direction;
    unsigned int seed;
    int depth;
    int countEmitted;
    int done;
};

// The parameter of the camera
rtDeclareVariable(float3,        eye, , );
rtDeclareVariable(float3,        U, , );
rtDeclareVariable(float3,        V, , );
rtDeclareVariable(float3,        W, , );
rtDeclareVariable(float3,        plight, , );
rtDeclareVariable(float,         lightIntensity, , );
rtDeclareVariable(float3,        bad_color, , );
rtDeclareVariable(float,         scene_epsilon, , );
rtDeclareVariable(float,         infiniteFar, , );

rtBuffer<uchar4, 2>              output_buffer;
rtBuffer<float4, 2>              float_buffer;
rtBuffer<float, 2>               envcdfV;
rtBuffer<float, 2>               envcdfH;
rtBuffer<float, 2>               envmapPdf;

rtDeclareVariable(rtObject,      top_object, , );
rtDeclareVariable(unsigned int, pathtrace_ray_type, , );
rtDeclareVariable(unsigned int, shadow_ray_type, , );
rtDeclareVariable(unsigned int, sqrt_num_samples, , );
rtDeclareVariable(unsigned int, rr_begin_depth, , );
rtDeclareVariable(int, max_depth, , );
rtDeclareVariable(unsigned int, bounce_num, , );

rtDeclareVariable(uint2, launch_index, rtLaunchIndex, );
rtDeclareVariable(uint2, launch_dim,   rtLaunchDim, );
rtDeclareVariable(float, time_view_scale, , ) = 1e-6f;
rtDeclareVariable(optix::Ray, ray, rtCurrentRay, );

// Add the phong model for practice
rtDeclareVariable(PerRayData_pathtrace, current_prd, rtPayload, );
rtDeclareVariable(float3, geometric_normal, attribute geometric_normal, ); 
rtDeclareVariable(float3, shading_normal,   attribute shading_normal, ); 
rtDeclareVariable(float3, texcoord, attribute texcoord, );
rtDeclareVariable(float3, tangent_direction, attribute tangent_direction, );
rtDeclareVariable(float3, bitangent_direction, attribute bitangent_direction, );
rtDeclareVariable(float,      t_hit,        rtIntersectionDistance, );
rtDeclareVariable(float, F0, , );
rtDeclareVariable(float3, specularColor, , );
rtDeclareVariable(int,    mode, , );

// Environment map background
rtTextureSampler<float4, 2> envmap;

// Spatially varying BRDF
rtTextureSampler<float4, 2> albedoMap;
rtTextureSampler<float4, 2> normalMap;
rtTextureSampler<float4, 2> roughMap;


/********************************************/
/**** Function to sample environment map ****/
/********************************************/
RT_CALLABLE_PROGRAM float3 EnvUVToDirec(float u, float v){ 
    // Turn uv coordinate into direction
    float theta = 2 * (u - 0.5) * M_PIf;
    float phi = M_PIf * (1 - v); 
    return make_float3(
                sinf(phi) * sinf(theta),
                cosf(phi),
                sinf(phi) * cosf(theta)
            );
}

RT_CALLABLE_PROGRAM float2 EnvDirecToUV(const float3& direc){ 
    float theta = atan2f( direc.x, direc.z );
    float phi = M_PIf - acosf(direc.y );
    float u = theta * (0.5f * M_1_PIf) + 0.5;
    if(u > 1)
        u = u-1;
    float v     = phi / M_PIf;
    return make_float2(u, v);
}

RT_CALLABLE_PROGRAM float EnvDirecToPdf(const float3& direc){
    float2 uv = EnvDirecToUV(direc);
    size_t2 pdfSize = envmapPdf.size();
    float u = uv.x, v = (1 -uv.y);
    int rowId = int(v * pdfSize.y);
    int colId = int(u * pdfSize.x);
    return envmapPdf[make_uint2(colId, rowId ) ];
}

RT_CALLABLE_PROGRAM float4 SampleEnvironmap(float z1, float z2)
{
    int ncols = envcdfH.size().x;
    int nrows = envcdfH.size().y;
    
    // Sample the row 
    float u = 0, v = 0;
    int rowId = 0;
    {
        int left = 0, right = nrows-1;
        while(right > left){
            int mid = (left + right) / 2;
            if(envcdfV[ make_uint2(0, mid) ] >= z1)
                right = mid;
            else if(envcdfV[ make_uint2(0, mid) ] < z1)
                left = mid + 1;
        }
        float up = envcdfV[make_uint2(0, left) ];
        float down = (left == 0) ? 0 : envcdfV[make_uint2(0, left-1) ];
        v = ( (z1 - down) / (up - down) + left) / float(nrows);
        rowId = left;
    }

    // Sample the column
    int colId = 0;
    {
        int left = 0; int right = ncols - 1;
        while(right > left){
            int mid = (left + right) / 2;
            if(envcdfH[ make_uint2(mid, rowId) ] >= z2)
                right = mid;
            else if(envcdfH[ make_uint2(mid, rowId) ] < z2)
                left = mid + 1;
        }
        float up = envcdfH[make_uint2(left, rowId) ];
        float down = (left == 0) ? 0 : envcdfH[make_uint2(left-1, rowId) ];
        u = ((z2 - down) / (up - down) + left) / float(ncols);
        colId = left;
    }

    // Turn uv coordinate into direction
    float3 direc = EnvUVToDirec(u, 1 - v);
    float pdf = envmapPdf[make_uint2(colId, rowId) ];

    float4 direcPdf = make_float4(
                direc, pdf
            );
    return direcPdf;
}
/*********************************************/


/*********************************************/
/**** Function to sample BRDF ****/
/*********************************************/
RT_CALLABLE_PROGRAM float LambDirecToPdf(const float3& L, const float3& N){
    float pdf = dot(L, N) / M_PIf;
    return fmaxf(pdf, 0.0f);
}
RT_CALLABLE_PROGRAM float SpecDirecToPdf(const float3& L, const float3& V, const float3& N, float R){
    float a2 = R * R * R * R;
    float3 H = normalize( (L+V) / 2.0 );
    float NoH = dot(N, H);
    float VoH = dot(V, H);
    float pdf = (a2 * NoH) / (4 * M_PIf * (1 + (a2-1) * NoH)
            *(1 + (a2-1) * NoH) * VoH );
    return fmaxf(pdf, 0.0f);
}
/*********************************************/


// The Camera Model
RT_PROGRAM void pinhole_camera()
{
    size_t2 screen = output_buffer.size();
    
    float2 inv_screen = 1.0f/make_float2(screen) * 2.f;
    float2 pixel = (make_float2(launch_index)) * inv_screen - 1.f;

    float2 jitter_scale = inv_screen / sqrt_num_samples;
    unsigned int samples_per_pixel = sqrt_num_samples*sqrt_num_samples;
    float3 result = make_float3(0.0f);

    unsigned int seed = tea<16>(screen.x*launch_index.y+launch_index.x,  0);
    do{
        // Sample pixel using jittering
        unsigned int x = samples_per_pixel%sqrt_num_samples;
        unsigned int y = samples_per_pixel/sqrt_num_samples;
        float2 jitter = make_float2(x-rnd(seed), y-rnd(seed) );
        float2 d = pixel + jitter*jitter_scale;
        float3 ray_origin = eye;
        float3 ray_direction = normalize(d.x*U + d.y*V + W);

        // Initialze per-ray data
        PerRayData_pathtrace prd;
        prd.result = make_float3(0.f);
        prd.attenuation = make_float3(1.f);
        prd.radiance = make_float3(0.f);
        prd.countEmitted = false;
        prd.done = false;
        prd.seed = tea<8>(seed, 0);
        prd.depth = 0;
        prd.direction = ray_direction;

        // Each iteration is a segment of the ray path.  The closest hit will
        // return new segments to be traced here.
        for(;;)
        {
            optix::Ray ray(ray_origin, ray_direction, pathtrace_ray_type, scene_epsilon);
            rtTrace(top_object, ray, prd);
 
            prd.depth++;

            if(mode == 9 || mode == 10 || mode == 11){
                if(prd.depth == bounce_num)
                    break;
            }
            
            // Hit the light source or exceed the max depth
            if(prd.done || prd.depth >= max_depth) 
                break;
            
            // Update ray data for the next path segment
            ray_origin = prd.origin;
            ray_direction = prd.direction;
        }
        result += prd.result;
        seed = prd.seed;
    } while (--samples_per_pixel);

    // Update the output buffer
    float3 pixel_color = result/(sqrt_num_samples*sqrt_num_samples);
    if(mode == 0 || mode == 1 || mode == 2 || mode == 9 || mode == 10 || mode == 11){
        pixel_color.x = powf(pixel_color.x, 1.0f/2.2f );
        pixel_color.y = powf(pixel_color.y, 1.0f/2.2f );
        pixel_color.z = powf(pixel_color.z, 1.0f/2.2f );
    }
    if (mode != 6)
        output_buffer[launch_index] = make_color(pixel_color);
    else
        float_buffer[launch_index] = make_float4(pixel_color, 1.0);
}
RT_PROGRAM void exception()
{
    const unsigned int code = rtGetExceptionCode();
    rtPrintf( "Caught exception 0x%X at launch index (%d,%d)\n", code, launch_index.x, launch_index.y );
    if(mode != 6)
        output_buffer[launch_index] = make_color( bad_color );
    else
        float_buffer[launch_index] = make_float4(bad_color, 1);
}


RT_PROGRAM void microfacetMIS()
{
    if(mode < 9)
    {
        // Russian roulette termination 
        if(current_prd.depth > rr_begin_depth){
            float pcont = fmaxf(current_prd.attenuation);
            if(rnd(current_prd.seed) >= pcont){
                current_prd.done = true;
                return;
            }
            current_prd.attenuation /= pcont;
        }
    }
    float3 world_shading_normal   = normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, shading_normal ) );
    float3 world_geometric_normal = normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, geometric_normal ) );
    float3 ffnormal = faceforward( world_shading_normal, -ray.direction, world_geometric_normal );

    float3 hitpoint = ray.origin + t_hit * ray.direction;
    float3 diffuse_color = make_float3(tex2D(albedoMap, texcoord.x, texcoord.y) ) * M_1_PIf;
    float3 normal_detail = make_float3(tex2D(normalMap, texcoord.x, texcoord.y) );
    normal_detail = normalize(2 * normal_detail - 1);
    float roughness  = tex2D(roughMap, texcoord.x, texcoord.y).x;

    // Upate Geometry information
    current_prd.origin = hitpoint;
    unsigned int seed = current_prd.seed;
    float z = rnd(seed);
    seed = tea<8>(seed, 2);
    float z1 = rnd(seed);
    seed = tea<8>(seed, 2);
    float z2 = rnd(seed);
    current_prd.seed = seed;

    normal_detail = normal_detail.x * tangent_direction
        + normal_detail.y * bitangent_direction 
        + normal_detail.z * ffnormal;
    normal_detail = normalize(normal_detail);
    float3 N = normal_detail;
    optix::Onb onb(N);
    float3 V = -ray.direction;

    // Data preparation for the computation
    float3 attenuationPre = current_prd.attenuation;
    float alpha = roughness * roughness;
    float k = (alpha + 2 * roughness + 1) / 8;
    float alpha2 = alpha * alpha;
   
    if(z < 0.5){
        float3 L;
        cosine_sample_hemisphere(z1, z2, L);
        onb.inverse_transform(L);
        current_prd.direction = L;
        current_prd.attenuation = 2 * M_PIf * current_prd.attenuation * diffuse_color;
    }
    else {
        // Compute the half angle 
        float phi = 2 * M_PIf * z1;
        float cosTheta = sqrt( (1 - z2) / (1 + (alpha2 - 1) * z2) );
        float sinTheta = sqrt( 1 - cosTheta * cosTheta);

        float3 H = make_float3(
                sinTheta * cos(phi),
                sinTheta * sin(phi),
                cosTheta);
        onb.inverse_transform(H);
        float3 L = 2 * dot(V, H) * H - V;
        current_prd.direction = L;

        float NoV = fmaxf(dot(N, V), 0.0000001f);
        float NoL = dot(N, L);
        float NoH = fmaxf(dot(N, H), 0.00000001f);
        float VoH = fmaxf(dot(V, H), 0.00000001f);

        if( NoL >= 0){
            float G1 = NoV / (NoV * (1-k) + k);
            float G2 = NoL / (NoL * (1-k) + k);
            float FMi = (-5.55473 * VoH - 6.98316) * VoH;
            float F = F0 + (1 - F0) * pow(2.0f, FMi);
            float reflec = F * G1 * G2 * VoH / NoH / NoV;

            current_prd.attenuation = 2 * current_prd.attenuation * reflec;
        }
        else
            current_prd.attenuation = make_float3(0.0f);
    }

    if(mode != 1 && mode != 10)
    { // Connect with the point light source
        float pLdist = length(plight - hitpoint);
        float3 L = normalize(plight - hitpoint);
        float3 H = normalize((L + V) / 2.0f );
        float NoL = dot(N, L);
        float NoV = fmaxf(dot(N, V), 1e-6);
        float NoH = fmaxf(dot(N, H), 1e-6);
        float VoH = fmaxf(dot(V, H), 1e-6);

        // Connect with the point light source and compute the direct contribution 
        if (NoL > 0.0f){
            PerRayData_pathtrace_shadow shadow_prd;
            shadow_prd.inShadow = false;
            Ray shadow_ray = make_Ray(hitpoint, L, shadow_ray_type, scene_epsilon, pLdist-scene_epsilon);
            rtTrace(top_object, shadow_ray, shadow_prd);
            if(!shadow_prd.inShadow){
                float FMi = (-5.55473 * VoH - 6.98316) * VoH;
                float frac0 = F0 + (1 - F0) * pow(2.0f, FMi);
                float frac = frac0 * alpha2;
                float nom0 = NoH * NoH * (alpha2 - 1) + 1;
                float nom1 = NoV * (1 - k) + k;
                float nom2 = NoL * (1 - k) + k;
                float nom = fmaxf(4 * M_PIf * nom0 * nom0 * nom1 * nom2, 1e-6);
                float spec = frac / nom;

                if(mode >= 9){
                    if(current_prd.depth == bounce_num-1){
                        current_prd.result += attenuationPre * (diffuse_color + spec) * 
                            NoL * lightIntensity / pLdist / pLdist;
                    }
                }
                else{
                    current_prd.result += attenuationPre * (diffuse_color + spec) * 
                        NoL * lightIntensity / pLdist / pLdist;
                }
            }
        }
    }
    

    if(mode != 0 && mode != 9)
    { 
        unsigned int seed = current_prd.seed;
        seed = tea<8>(seed, 2);
        float z3 = rnd(seed);
        seed = tea<8>(seed, 2);
        float z4 = rnd(seed);
        seed = tea<8>(seed, 2);
        float z5 = rnd(seed);
        current_prd.seed = seed;

        // Sample According to Environment Map
        {
            // Connect with Environment Map
            float4 directPdf = SampleEnvironmap(z3, z4);
            float3 L = normalize( make_float3(
                        directPdf.x, directPdf.y, directPdf.z) );
            float pdf = fmaxf(directPdf.w, 1e-6 );
            float3 H = normalize((L + V) / 2.0f );
            float NoL = dot(N, L);
            float NoV = fmaxf(dot(N, V), 1e-6);
            float NoH = fmaxf(dot(N, H), 1e-6);
            float VoH = fmaxf(dot(V, H), 1e-6);
     
            // Get the radiance of the environment map
            float2 uv = EnvDirecToUV(L);
            float3 radiance =  make_float3( tex2D(envmap, uv.x, uv.y) );
         
            // Connect with the environment and compute the direct contribution 
            if (NoL > 0.0f){
                PerRayData_pathtrace_shadow shadow_prd;
                shadow_prd.inShadow = false;
                Ray shadow_ray = make_Ray(hitpoint, L, shadow_ray_type, scene_epsilon, infiniteFar-scene_epsilon);
                rtTrace(top_object, shadow_ray, shadow_prd);
                if(!shadow_prd.inShadow){
                    float FMi = (-5.55473 * VoH - 6.98316) * VoH;
                    float frac0 = F0 + (1 - F0) * pow(2.0f, FMi);
                    float frac = frac0 * alpha2;
                    float nom0 = NoH * NoH * (alpha2 - 1) + 1;
                    float nom1 = NoV * (1 - k) + k;
                    float nom2 = NoL * (1 - k) + k;
                    float nom = fmaxf(4 * M_PIf * nom0 * nom0 * nom1 * nom2, 1e-6);
                    float spec = frac / nom;
         
                    float Lambpdf = LambDirecToPdf(L, N);
                    float Specpdf = SpecDirecToPdf(L, V, N, roughness);
                    float Envpdf = EnvDirecToPdf(L);
                    float W = Envpdf * Envpdf / (Lambpdf*Lambpdf 
                            + Envpdf*Envpdf + Specpdf*Specpdf); 
                    if(mode >= 9){
                        if(current_prd.depth == bounce_num - 1){
                            current_prd.result += W * attenuationPre * (diffuse_color + spec) * 
                                NoL * radiance / pdf; 
                        }
                    }
                    else{
                        current_prd.result += W * attenuationPre * (diffuse_color + spec) * 
                            NoL * radiance / pdf; 
                    }
                }
            }
        }

        // Sampling According to BRDF
        {
            if(z3 < 0.5){
                float3 L;
                cosine_sample_hemisphere(z4, z5, L);
                onb.inverse_transform(L);
                PerRayData_pathtrace_shadow shadow_prd;
                shadow_prd.inShadow = false;
                Ray shadow_ray = make_Ray(hitpoint, L, shadow_ray_type, scene_epsilon, infiniteFar-scene_epsilon);
                rtTrace(top_object, shadow_ray, shadow_prd);
                if( !shadow_prd.inShadow ){
                    float2 uv = EnvDirecToUV(L);
                    float3 radiance =  make_float3( tex2D(envmap, uv.x, uv.y) );
                    
                    float Lambpdf = LambDirecToPdf(L, N);
                    float Specpdf = SpecDirecToPdf(L, V, N, roughness);
                    float Envpdf = EnvDirecToPdf(L);
                    float W = 2 * (Lambpdf * Lambpdf + Specpdf * Specpdf) / (Lambpdf*Lambpdf 
                            + Envpdf*Envpdf + Specpdf*Specpdf);        
                    if(mode >= 9){
                        if(current_prd.depth == bounce_num - 1){
                            current_prd.result += W * radiance * M_PIf * attenuationPre * diffuse_color;
                        }
                    }
                    else{
                        current_prd.result += W * radiance * M_PIf * attenuationPre * diffuse_color;
                    }
                }
            }
            else {
                // Compute the half angle 
                float phi = 2 * M_PIf * z4;
                float cosTheta = sqrt( (1 - z5) / (1 + (alpha2 - 1) * z5) );
                float sinTheta = sqrt( 1 - cosTheta * cosTheta);
  
                float3 H = make_float3(
                        sinTheta * cos(phi),
                        sinTheta * sin(phi),
                        cosTheta);
                onb.inverse_transform(H);
                float3 L = 2 * dot(V, H) * H - V;
  
                float NoV = fmaxf(dot(N, V), 0.0000001f);
                float NoL = dot(N, L);
                float NoH = fmaxf(dot(N, H), 0.00000001f);
                float VoH = fmaxf(dot(V, H), 0.00000001f);
  
                if( NoL >= 0){
                    PerRayData_pathtrace_shadow shadow_prd;
                    shadow_prd.inShadow = false;
                    Ray shadow_ray = make_Ray(hitpoint, L, shadow_ray_type, scene_epsilon, infiniteFar-scene_epsilon);
                    rtTrace(top_object, shadow_ray, shadow_prd);
                    if(!shadow_prd.inShadow){
                        float G1 = NoV / (NoV * (1-k) + k);
                        float G2 = NoL / (NoL * (1-k) + k);
                        float FMi = (-5.55473 * VoH - 6.98316) * VoH;
                        float F = F0 + (1 - F0) * pow(2.0f, FMi);
                        float reflec = F * G1 * G2 * VoH / NoH / NoV;

                        float2 uv = EnvDirecToUV(L);
                        float3 radiance =  make_float3( tex2D(envmap, uv.x, uv.y) );
                        float Lambpdf = LambDirecToPdf(L, N);
                        float Specpdf = SpecDirecToPdf(L, V, N, roughness);
                        float Envpdf = EnvDirecToPdf(L);
                        float W = 2 * (Lambpdf * Lambpdf + Specpdf * Specpdf) / (Lambpdf*Lambpdf 
                                + Envpdf*Envpdf + Specpdf*Specpdf);        
                        if(mode >= 9){
                            if(current_prd.depth == bounce_num - 1){
                                current_prd.result +=  W * attenuationPre * radiance * reflec;
                            }
                        }
                        else{
                            current_prd.result +=  W * attenuationPre * radiance * reflec;
                        }
                    }
                }
            }
        }
    }
}

// Rendering BRDF parameter
RT_PROGRAM void albedo()
{
    float3 diffuse_color = make_float3(tex2D(albedoMap, texcoord.x, texcoord.y) );
    current_prd.result = diffuse_color;
}
RT_PROGRAM void roughness()
{
    float3 roughness = make_float3(tex2D(roughMap, texcoord.x, texcoord.y) );
    current_prd.result = roughness;
}
RT_PROGRAM void segmentation()
{
    current_prd.result = make_float3(1.0);
}
RT_PROGRAM void depth()
{
    float3 Z = normalize(W);
    current_prd.result = t_hit * make_float3(dot(Z, ray.direction) );
}
RT_PROGRAM void normal()
{
    float3 world_shading_normal   = normalize( rtTransformNormal(RT_OBJECT_TO_WORLD, shading_normal) );
    float3 world_geometric_normal = normalize( rtTransformNormal(RT_OBJECT_TO_WORLD, geometric_normal) );
    float3 ffnormal = faceforward(world_shading_normal, -ray.direction, world_geometric_normal );

    float3 normal_detail = make_float3(tex2D(normalMap, texcoord.x, texcoord.y) );
    normal_detail = normalize(2*normal_detail - 1);
    normal_detail = normal_detail.x * tangent_direction
        + normal_detail.y * bitangent_direction 
        + normal_detail.z * ffnormal;
    normal_detail = normalize(normal_detail);

    float3 X = normalize(U);
    float3 Y = normalize(V);
    float3 Z = -normalize(W);
    float xComp = 0.5 * dot(X, normal_detail) + 0.5;
    float yComp = 0.5 * dot(Y, normal_detail) + 0.5;
    float zComp = 0.5 * dot(Z, normal_detail) + 0.5;
    xComp = fminf(fmaxf(xComp, 0.0f), 1.0f);
    yComp = fminf(fmaxf(yComp, 0.0f), 1.0f);
    zComp = fminf(fmaxf(zComp, 0.0f), 1.0f);

    current_prd.result = make_float3(xComp, yComp, zComp);
}
RT_PROGRAM void normalCoarse()
{
    float3 world_shading_normal   = normalize( rtTransformNormal(RT_OBJECT_TO_WORLD, shading_normal) );
    float3 world_geometric_normal = normalize( rtTransformNormal(RT_OBJECT_TO_WORLD, geometric_normal) );
    float3 ffnormal = faceforward(world_shading_normal, -ray.direction, world_geometric_normal );

    float3 X = normalize(U);
    float3 Y = normalize(V);
    float3 Z = -normalize(W);
    float xComp = 0.5 * dot(X, ffnormal) + 0.5;
    float yComp = 0.5 * dot(Y, ffnormal) + 0.5;
    float zComp = 0.5 * dot(Z, ffnormal) + 0.5;
    xComp = fminf(fmaxf(xComp, 0.0f), 1.0f);
    yComp = fminf(fmaxf(yComp, 0.0f), 1.0f);
    zComp = fminf(fmaxf(zComp, 0.0f), 1.0f);
    current_prd.result = make_float3(xComp, yComp, zComp);
}
RT_PROGRAM void uvmap()
{
    current_prd.result = make_float3(texcoord.x, texcoord.y, 1.0);
}


RT_PROGRAM void envmap_miss() {
    float2 uv = EnvDirecToUV(current_prd.direction);
    if(current_prd.depth == 0){
        if(mode != 0 && mode != 9)
            current_prd.result =  make_float3( tex2D(envmap, uv.x, uv.y) );
        else 
            current_prd.result = make_float3(0.0f);
    }
    current_prd.done = true;
}

RT_PROGRAM void miss(){
    current_prd.radiance = make_float3(0.0f);
    current_prd.done = true;
}

// -------------------------------------------------------------
//  Shadow any-hit
// -------------------------------------------------------------
rtDeclareVariable(PerRayData_pathtrace_shadow, current_prd_shadow, rtPayload, );
RT_PROGRAM void shadow()
{
    current_prd_shadow.inShadow = true;
    rtTerminateRay();
}

