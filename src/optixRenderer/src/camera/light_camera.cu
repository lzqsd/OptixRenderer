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
#include "random.h"

using namespace optix;


rtDeclareVariable(float3,        eye, , );
rtDeclareVariable(float3,        cameraU, , );
rtDeclareVariable(float3,        cameraV, , );
rtDeclareVariable(float3,        cameraW, , );
rtDeclareVariable(float3,        bad_color, , );
rtDeclareVariable(float,         scene_epsilon, , );
rtDeclareVariable(int,           max_depth, , ); 

rtBuffer<float3, 2>              intensity_buffer;

rtDeclareVariable(rtObject,      top_object, , );
rtDeclareVariable(uint2,         launch_index, rtLaunchIndex, );
rtDeclareVariable(unsigned int, sqrt_num_samples, , );
rtDeclareVariable(unsigned int, rr_begin_depth, , );
rtDeclareVariable(int, cameraMode, , );
rtDeclareVariable(unsigned int, initSeed, , ); 

rtDeclareVariable(unsigned int, envWidth, , );
rtDeclareVariable(unsigned int, envHeight, , );


RT_PROGRAM void pinhole_camera()
{
    size_t2 screen = make_size_t2(160, 120);
    
    float2 inv_screen = 1.0f/make_float2(screen) * 2.f;
    float2 pixel = (make_float2(launch_index)) * inv_screen - 1.f;

    float2 jitter_scale = inv_screen / sqrt_num_samples;
    unsigned int samples_per_pixel = sqrt_num_samples*sqrt_num_samples;

    unsigned int seed = tea<32>( 
        ( (initSeed)*(screen.x*launch_index.y+launch_index.x) + initSeed ), 
        ( (screen.y * launch_index.x + launch_index.y) * initSeed ) ); 
    
    // Presampling to get the normal and the origins
    float3 normalAccu = make_float3(0.0);
    unsigned int sqrt_pre_samples = 2;
    unsigned int pre_samples_num = sqrt_pre_samples * sqrt_pre_samples;
    do{
        float3 ray_origin = eye;
        unsigned int x = pre_samples_num%sqrt_pre_samples;
        unsigned int y = pre_samples_num/sqrt_pre_samples;
        float2 jitter = make_float2(x-rnd(seed), y-rnd(seed) );
        float2 d = pixel + jitter* (inv_screen / sqrt_pre_samples );

        float3 ray_direction;
        ray_direction = normalize(d.x*cameraU + d.y*cameraV + cameraW);

        // Initialze per-ray data
        PerRayData_radiance prd;
        prd.attenuation = make_float3(1.f);
        prd.radiance = make_float3(0.f);
        prd.done = false;
        prd.seed = seed;
        prd.depth = 0;
        prd.direction = ray_direction;
        prd.origin = ray_origin; 
        prd.normal = make_float3(0.0);
        
        prd.brdfDirection = make_float3(1.0); 
        prd.isHitArea = false;
        prd.areaRadiance = make_float3(0.0);
        prd.areaDirection = make_float3(1.0); 
        prd.isHitEnv = false;
        prd.envRadiance = make_float3(0.0);
        prd.envDirection = make_float3(1.0);  

        Ray ray(ray_origin, ray_direction, 0, scene_epsilon);
        rtTrace(top_object, ray, prd); 
        normalAccu = normalAccu + prd.normal;
    } while(--pre_samples_num );

    // Sample pixel using jittering 
    if(normalAccu.x != 0 || normalAccu.y != 0 || normalAccu.z != 0){
        float3 upAxis = normalize(cameraV );
        float3 zAxis = normalize(normalAccu ); 
        float3 yAxis = upAxis - dot(upAxis, zAxis) * zAxis; 
        yAxis = normalize(yAxis );
        float3 xAxis = cross(zAxis, yAxis );  

        do{
            float3 ray_origin = eye;
            unsigned int x = samples_per_pixel%sqrt_num_samples;
            unsigned int y = samples_per_pixel/sqrt_num_samples;
            float2 jitter = make_float2(x-rnd(seed), y-rnd(seed) );
            float2 d = pixel + jitter*jitter_scale;
    
            float3 ray_direction;
            ray_direction = normalize(d.x*cameraU + d.y*cameraV + cameraW);
    
            // Initialze per-ray data
            PerRayData_radiance prd;
            prd.attenuation = make_float3(1.f);
            prd.radiance = make_float3(0.f);
            prd.done = false;
            prd.seed = seed;
            prd.depth = 0;
            prd.direction = ray_direction;
            prd.origin = ray_origin; 
            prd.normal = make_float3(0.0); 
            
            prd.brdfDirection = make_float3(1.0); 
            prd.isHitArea = false;
            prd.areaRadiance = make_float3(0.0);
            prd.areaDirection = make_float3(1.0); 
            prd.isHitEnv = false;
            prd.envRadiance = make_float3(0.0);
            prd.envDirection = make_float3(1.0);  
    
            // Each iteration is a segment of the ray path.  The closest hit will
            // return new segments to be traced here.
            for(;;) {
                Ray ray(ray_origin, ray_direction, 0, scene_epsilon);
                rtTrace(top_object, ray, prd);
     
                prd.depth++; 
            
                // Hit the light source or exceed the max depth
                if(prd.done || prd.depth >= max_depth) 
                    break; 
                
                // Update ray data for the next path segment
                ray_origin = prd.origin; 
                ray_direction = prd.direction;
                ray_origin = ray_origin + 0.1 * scene_epsilon * ray_direction;
            } 
    
            if(prd.depth >= 2 ){   
                if(true ){
                    float theta = acos(dot(prd.brdfDirection, zAxis ) );
                    float phi = atan2(
                        dot(prd.brdfDirection, yAxis), 
                        dot(prd.brdfDirection, xAxis ) );
                
                    if(phi > M_PIf )
                        phi = phi - 2 * M_PIf; 
        
                    float thetaId = (theta / M_PIf * 2) * envHeight;
                    float phiId = ( (phi / M_PIf ) + 1) * 0.5 * envWidth; 
                        
                    if(thetaId <= envHeight + 2){
                        if(thetaId >= envHeight) thetaId = envHeight-1e-6;
                        if(thetaId < 0 ) thetaId = 0;
                        if(phiId >= envWidth ) phiId = envWidth - 1e-6; 
                        if(phiId < 0 ) phiId = 0;
            
                        unsigned x = launch_index.x * envWidth + floor(phiId );
                        unsigned y = launch_index.y * envHeight + floor(thetaId );

                        intensity_buffer[make_uint2(x, y) ] += prd.radiance;
                    }
                }
                
                if(prd.isHitEnv ){
                    float theta = acos(dot(prd.envDirection, zAxis ) );
                    float phi = atan2(
                        dot(prd.envDirection, yAxis), 
                        dot(prd.envDirection, xAxis ) );
                    if(phi > M_PIf )
                        phi = phi - 2 * M_PIf; 
        
                    float thetaId = ( theta / M_PIf * 2) * envHeight;
                    float phiId = ( (phi / M_PIf ) + 1) * 0.5 * envWidth; 

                    if(thetaId <= envHeight + 2){
                        if(thetaId >= envHeight) thetaId = envHeight-1e-6;
                        if(thetaId < 0 ) thetaId = 0;
                        if(phiId >= envWidth ) phiId = envWidth - 1e-6; 
                        if(phiId < 0 ) phiId = 0;
            
                        unsigned x = launch_index.x * envWidth + floor(phiId );
                        unsigned y = launch_index.y * envHeight + floor(thetaId );

                        intensity_buffer[make_uint2(x, y) ] += prd.envRadiance; 
                    }
                }
    
                if(prd.isHitArea ){
                    float theta = acos(dot(prd.areaDirection, zAxis ) );
                    float phi = atan2(
                        dot(prd.areaDirection, yAxis), 
                        dot(prd.areaDirection, xAxis ) );
                    if(phi > M_PIf )
                        phi = phi - 2 * M_PIf; 
        
                    float thetaId = (theta / M_PIf * 2) * envHeight;
                    float phiId = ( (phi / M_PIf ) + 1) * 0.5 * envWidth; 

                    if(thetaId <= envHeight + 2){
                        if(thetaId >= envHeight) thetaId = envHeight-1e-6;
                        if(thetaId < 0 ) thetaId = 0;
                        if(phiId >= envWidth ) phiId = envWidth - 1e-6; 
                        if(phiId < 0 ) phiId = 0;
            
                        unsigned x = launch_index.x * envWidth + floor(phiId );
                        unsigned y = launch_index.y * envHeight + floor(thetaId );

                        intensity_buffer[make_uint2(x, y) ] += prd.areaRadiance;
                    }
                }
            }
        } while (--samples_per_pixel); 
    } 
}

RT_PROGRAM void exception()
{
  const unsigned int code = rtGetExceptionCode();
  rtPrintf( "Caught exception 0x%X at launch index (%d,%d)\n", code, launch_index.x, launch_index.y );
}
