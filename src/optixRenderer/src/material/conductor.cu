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
#include "commonStructs.h"
#include "lightStructs.h"
#include <vector> 

using namespace optix; 

rtDeclareVariable(float3, shading_normal, attribute shading_normal, ); 
rtDeclareVariable(float3, geometric_normal, attribute geometric_normal, );
rtDeclareVariable(float3, tangent_direction, attribute tangent_direction, );
rtDeclareVariable(float3, bitangent_direction, attribute bitangent_direction, );

rtDeclareVariable( float3, texcoord, attribute texcoord, );
rtDeclareVariable( float, t_hit, rtIntersectionDistance, );

rtDeclareVariable(optix::Ray, ray,   rtCurrentRay, );
rtDeclareVariable(PerRayData_radiance, prd_radiance, rtPayload, );
rtDeclareVariable(PerRayData_shadow,   prd_shadow, rtPayload, );
rtDeclareVariable(float, scene_epsilon, , );

// Normal 
rtTextureSampler<float4, 2> normalMap;
rtDeclareVariable( int, isNormalTexture, , );

// Material 
rtDeclareVariable(float3, specular, , );

// Geometry Group
rtDeclareVariable( rtObject, top_object, , );

// Computing the pdfSolidAngle of BRDF giving a direction 
RT_CALLABLE_PROGRAM float pdf(const float3& L, const float3& V, const float3& N)
{
    return 0.0f;
}

RT_CALLABLE_PROGRAM float3 evaluate(const float3& N, const float3& V, const float3& L, const float3& radiance)
{   
    return make_float3(0.0f);
}
 
RT_CALLABLE_PROGRAM void sample(unsigned& seed, 
        float3 N, const float3& V, 
        float3& attenuation, float3& direction, float& pdfSolid)
{
    pdfSolid = -1;
    direction = reflect(-V, N);   
    attenuation = attenuation * specular;
}


RT_PROGRAM void closest_hit_radiance()
{
    const float3 world_shading_normal   = normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, shading_normal ) );
    const float3 world_geometric_normal = normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, geometric_normal ) );

    float3 V = normalize(-ray.direction );
    
    float3 N;
    if( isNormalTexture == 0){
        N = world_shading_normal;
    }
    else{
        N = make_float3(tex2D(normalMap, texcoord.x, texcoord.y) );
        N = normalize(2 * N - 1);
        N = N.x * tangent_direction 
            + N.y * bitangent_direction 
            + N.z * world_shading_normal;
    }
    N = normalize(N );
    
    float3 hitPoint = ray.origin + t_hit * ray.direction;
    prd_radiance.origin = hitPoint;


    // Finish updating the ray
    sample(prd_radiance.seed,
            N, V,
            prd_radiance.attenuation, prd_radiance.direction, prd_radiance.pdf 
            );
}

// any_hit_shadow program for every material include the lighting should be the same
RT_PROGRAM void any_hit_shadow()
{
    prd_shadow.inShadow = true;
    rtTerminateRay();
}

