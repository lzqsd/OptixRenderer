
#include <optix.h>
#include <optixu/optixu_math_namespace.h>
#include "helpers.h"
#include "lightStructs.h"
#include "light/areaLight.h"
#include "structs/prd.h"
#include "random.h"
#include "commonStructs.h"

using namespace optix;

rtDeclareVariable( float3, shading_normal, attribute shading_normal, ); 
rtDeclareVariable( float3, geometric_normal, attribute geometric_normal, );
rtDeclareVariable( float, t_hit, rtIntersectionDistance, );
rtDeclareVariable(int, max_depth, , );

rtDeclareVariable(optix::Ray, ray,   rtCurrentRay, );
rtDeclareVariable(PerRayData_radiance, prd_radiance, rtPayload, );
rtDeclareVariable(PerRayData_shadow,   prd_shadow, rtPayload, );

rtDeclareVariable(float3, radiance, , );
rtDeclareVariable(float, areaSum, , );

rtDeclareVariable(int, areaTriangleNum, , );
rtBuffer<areaLight> areaLights;
rtBuffer<float> areaLightCDF;
rtBuffer<float> areaLightPDF;

RT_CALLABLE_PROGRAM void sampleAreaLight(unsigned int& seed, float3& radiance, float3& position, float3& normal, float& pdfAreaLight){
    float randf = rnd(seed);

    int left = 0, right = areaTriangleNum;
    int middle = int( (left + right) / 2);
    while(left < right){
        if(areaLightCDF[middle] <= randf)
            left = middle + 1;
        else if(areaLightCDF[middle] > randf)
            right = middle;
        middle = int( (left + right) / 2);
    }
    areaLight L = areaLights[left];
    
    float3 v1 = L.vertices[0];
    float3 v2 = L.vertices[1];
    float3 v3 = L.vertices[2];

    normal = cross(v2 - v1, v3 - v1);
    float area = 0.5 * length(normal);
    normal = normalize(normal);

    float ep1 = rnd(seed);
    float ep2 = rnd(seed);
    
    float u = 1 - sqrt(ep1);
    float v = ep2 * sqrt(ep1);

    position = v1 + (v2 - v1) * u + (v3 - v1) * v;

    radiance = L.radiance;
    pdfAreaLight = areaLightPDF[left] /  fmaxf(area, 1e-14);
}

RT_PROGRAM void closest_hit_radiance()
{
    const float3 world_shading_normal   = normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, shading_normal ) );
    const float3 world_geometric_normal = normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, geometric_normal ) );
    const float3 ffnormal = faceforward( world_shading_normal, -ray.direction, world_geometric_normal );

    if(prd_radiance.depth == 0){
        // Directly hit the light
        prd_radiance.radiance = radiance;
    }
    else{
        if(prd_radiance.pdf < 0){
            prd_radiance.radiance += radiance * prd_radiance.attenuation;
        }
        else{
            // Use MIS to compute the radiance
            if(prd_radiance.depth == (max_depth - 1) ){
                prd_radiance.radiance += radiance * prd_radiance.attenuation;
            }
            else{
                float3 hitPoint = ray.origin + t_hit * ray.direction;
                float Dist = length(hitPoint - prd_radiance.origin);
                float3 L = normalize(hitPoint - prd_radiance.origin);
                float cosPhi = dot(L, ffnormal);
                if (cosPhi < 0) cosPhi = -cosPhi;
                if (cosPhi < 1e-14) cosPhi = 0;
        
                float pdfAreaBRDF = prd_radiance.pdf * cosPhi / Dist / Dist;
                float pdfAreaLight = length(radiance) / areaSum;

                float pdfAreaBRDF2 = pdfAreaBRDF * pdfAreaBRDF;
                float pdfAreaLight2 = pdfAreaLight * pdfAreaLight;
       
                prd_radiance.radiance += radiance * pdfAreaBRDF2 / fmaxf(pdfAreaBRDF2 + pdfAreaLight2, 1e-14) * prd_radiance.attenuation;
            }
        }
    }
    prd_radiance.done = true;
}


RT_PROGRAM void any_hit_shadow()
{
    prd_shadow.inShadow = true;
    rtTerminateRay();
}
