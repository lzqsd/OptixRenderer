
#include <optix.h>
#include <optixu/optixu_math_namespace.h>
#include "helpers.h"
#include "prd.h"
#include "random.h"
#include "commonStructs.h"

using namespace optix;

rtDeclareVariable( float3, shading_normal, attribute shading_normal, ); 
rtDeclareVariable( float3, geometric_normal, attribute geometric_normal, );
rtDeclareVariable( float, t_hit, rtIntersectionDistance, );

rtDeclareVariable(optix::Ray, ray,   rtCurrentRay, );
rtDeclareVariable(PerRayData_radiance, prd_radiance, rtPayload, );
rtDeclareVariable(PerRayData_shadow,   prd_shadow, rtPayload, );

rtDeclareVariable(float3, radiance, , );
rtDeclareVariable(float, areaSum, , );


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
        // Use MIS to compute the radiance
        float3 hitPoint = ray.origin + t_hit * ray.direction;
        float Dist = length(hitPoint - prd_radiance.origin);
        float3 L = normalize(hitPoint - prd_radiance.origin);
        float cosPhi = dot(L, ffnormal);
        if (cosPhi < 0) cosPhi = -cosPhi;
        if (cosPhi < 1e-6) cosPhi = 0;
        
        float pdfAreaBRDF = prd_radiance.pdf * cosPhi / Dist / Dist;
        float pdfAreaLight = 1 / areaSum;

        float pdfAreaBRDF2 = pdfAreaBRDF * pdfAreaBRDF;
        float pdfAreaLight2 = pdfAreaLight * pdfAreaLight;
       
        prd_radiance.radiance += radiance * pdfAreaBRDF2 / (pdfAreaBRDF2 + pdfAreaLight2) * prd_radiance.attenuation;
    }
    prd_radiance.done = true;
}


RT_PROGRAM void any_hit_shadow()
{
    prd_shadow.inShadow = true;
    rtTerminateRay();
}
