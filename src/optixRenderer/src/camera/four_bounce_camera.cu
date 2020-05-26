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

rtBuffer<float3, 2>              normal1_buffer;
rtBuffer<float3, 2>              depth1_buffer;
rtBuffer<float3, 2>              normal2_buffer;
rtBuffer<float3, 2>              depth2_buffer;
rtBuffer<float3, 2>              normal3_buffer;
rtBuffer<float3, 2>              depth3_buffer;
rtBuffer<float3, 2>              normal4_buffer;
rtBuffer<float3, 2>              depth4_buffer;

rtDeclareVariable(rtObject,      top_object, , );
rtDeclareVariable(uint2,         launch_index, rtLaunchIndex, );
rtDeclareVariable(unsigned int, sqrt_num_samples, , );
rtDeclareVariable(unsigned int, rr_begin_depth, , );
rtDeclareVariable(int, cameraMode, , );
rtDeclareVariable(unsigned int, initSeed, , );

RT_PROGRAM void pinhole_camera()
{
    size_t2 screen = normal1_buffer.size();
    
    float2 inv_screen = 1.0f/make_float2(screen) * 2.f;
    float2 pixel = (make_float2(launch_index)) * inv_screen - 1.f;

    float2 jitter_scale = inv_screen / sqrt_num_samples;
    unsigned int samples_per_pixel = sqrt_num_samples*sqrt_num_samples;

    unsigned int seed = tea<32>( 
        ( (initSeed)*(screen.x*launch_index.y+launch_index.x) + initSeed ), 
        ( (screen.y * launch_index.x + launch_index.y) * initSeed ) );
    
    float3 normal1 = make_float3(0.0f);
    float3 depth1 = make_float3(0.0f);
    float3 normal2 = make_float3(0.0f);
    float3 depth2 = make_float3(0.0f);
    float3 normal3 = make_float3(0.0f);
    float3 depth3 = make_float3(0.0f);
    float3 normal4 = make_float3(0.0f);
    float3 depth4 = make_float3(0.0f);

    do{
        // Sample pixel using jittering
        float3 ray_origin = eye;
        unsigned int x = samples_per_pixel%sqrt_num_samples;
        unsigned int y = samples_per_pixel/sqrt_num_samples;
        float2 jitter = make_float2(x-rnd(seed), y-rnd(seed) );
        float2 d = pixel + jitter*jitter_scale;

        
        float3 ray_direction;
        if(cameraMode == 0){
            ray_direction = normalize(d.x*cameraU + d.y*cameraV + cameraW);
        }
        else{
            float3 axisZ = normalize(cameraW );
            float3 axisX = normalize(cameraU );
            float3 axisY = normalize(cameraV );

            d.x = (d.x > 1.0f) ? 1.0f : d.x;
            d.y = (d.y > 1.0f) ? 1.0f : d.y;
            d.x = (d.x < -1.0f) ? -1.0f : d.x;
            d.y = (d.y < -1.0f) ? -1.0f : d.y;

            float phi = d.x * M_PIf;
            float theta;
            if(cameraMode == 1){
                theta = 0.5f * (-d.y + 1.0f) * M_PIf;
            }
            else if(cameraMode == 2){
                theta = 0.25f * (-d.y + 1.0f) * M_PIf;
            }
            ray_direction = normalize(
                    sinf(theta) * cosf(phi) * axisX 
                    + sinf(theta) * sinf(phi) * axisY 
                    + cosf(theta) * axisZ
                    );
        }

        // Initialze per-ray data
        FourBounce_data prd;
        prd.done = false;
        prd.seed = seed;
        prd.depth = 0;
        prd.direction = ray_direction;
        prd.normal1 = make_float3(0.0f);
        prd.depth1 = make_float3(0.0f);
        prd.normal2 = make_float3(0.0f);
        prd.depth2 = make_float3(0.0f);
        prd.normal3 = make_float3(0.0f);
        prd.depth3 = make_float3(0.0f);
        prd.normal4 = make_float3(0.0f);
        prd.depth4 = make_float3(0.0f);

        // Each iteration is a segment of the ray path.  The closest hit will
        // return new segments to be traced here.
        for(;;)
        {
            Ray ray(ray_origin, ray_direction, 0, scene_epsilon);
            rtTrace(top_object, ray, prd);
 
            prd.depth++; 
           
            // Hit the light source or exceed the max depth
            if(prd.done || prd.depth >= max_depth) 
                break; 
            
            // Update ray data for the next path segment
            ray_origin = prd.origin;
            ray_direction = prd.direction;
        } 
        normal1 += prd.normal1;
        depth1 += prd.depth1; 
        normal2 += prd.normal2; 
        depth2 += prd.depth2;
        normal3 += prd.normal3;
        depth3 += prd.depth3; 
        normal4 += prd.normal4; 
        depth4 += prd.depth4;

        seed = prd.seed;
    } while (--samples_per_pixel);

    // Update the output buffer
    unsigned sampleNum = sqrt_num_samples * sqrt_num_samples;
    normal1_buffer[launch_index ] = normal1 / sampleNum;
    depth1_buffer[launch_index ] = depth1 / sampleNum;
    normal2_buffer[launch_index ] = normal2 / sampleNum;
    depth2_buffer[launch_index ] = depth2 / sampleNum;
    normal3_buffer[launch_index ] = normal3 / sampleNum;
    depth3_buffer[launch_index ] = depth3 / sampleNum;
    normal4_buffer[launch_index ] = normal4 / sampleNum;
    depth4_buffer[launch_index ] = depth4 / sampleNum;
}

RT_PROGRAM void exception()
{
  const unsigned int code = rtGetExceptionCode();
  rtPrintf( "Caught exception 0x%X at launch index (%d,%d)\n", code, launch_index.x, launch_index.y );
  normal1_buffer[launch_index] = bad_color;
  depth1_buffer[launch_index] = bad_color;
  normal2_buffer[launch_index] = bad_color;
  depth2_buffer[launch_index] = bad_color;
  normal3_buffer[launch_index] = bad_color;
  depth3_buffer[launch_index] = bad_color;
  normal4_buffer[launch_index] = bad_color;
  depth4_buffer[launch_index] = bad_color;
}
