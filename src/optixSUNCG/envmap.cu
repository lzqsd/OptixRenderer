#include <optix.h>
#include <optixu/optixu_math_namespace.h>
#include "helpers.h"
#include "prd.h"
#include "random.h"
#include "commonStructs.h"
#include "areaLight.h"
#include <vector>

using namespace optix;


rtDeclareVariable(optix::Ray, ray,   rtCurrentRay, );
rtDeclareVariable(PerRayData_radiance, prd_radiance, rtPayload, );

// Environmental Lighting 
rtDeclareVariable(int, isEnvmap, , );
rtTextureSampler<float4, 2> envmap;
rtTextureSampler<float4, 2> envmapDirec;
rtBuffer<float, 2> envcdfV;
rtBuffer<float, 2> envcdfH;
rtBuffer<float, 2> envpdf;
rtDeclareVariable(float, infiniteFar, , );


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
    size_t2 pdfSize = envpdf.size();
    float u = uv.x, v = uv.y;
    int rowId = int(v * (pdfSize.y - 1) );
    int colId = int(u * (pdfSize.x - 1) );
    return envpdf[make_uint2(colId, rowId ) ];
}
RT_PROGRAM void envmap_miss(){
    if(isEnvmap == 0){
        prd_radiance.attenuation = make_float3(0.0);
    }
    else if(isEnvmap == 1){    
        float2 uv = EnvDirecToUV(prd_radiance.direction);

        if(prd_radiance.depth == 0){
            prd_radiance.radiance = make_float3(tex2D(envmapDirec, uv.x, uv.y) ); 
        }
        else{
            float3 radiance = make_float3(tex2D(envmap, uv.x, uv.y) );
            // Multiple Importance Sampling 
            float pdfSolidEnv = EnvDirecToPdf(prd_radiance.direction);
            float pdfSolidBRDF = prd_radiance.pdf;
            float pdfSolidEnv2 = pdfSolidEnv * pdfSolidEnv;
            float pdfSolidBRDF2 = pdfSolidBRDF * pdfSolidBRDF;

            prd_radiance.radiance += radiance  * pdfSolidBRDF2 / fmaxf(pdfSolidBRDF2 + pdfSolidEnv2, 1e-6)* prd_radiance.attenuation;
        }
    }
    prd_radiance.done = true;
}

RT_PROGRAM void miss(){
    prd_radiance.radiance = make_float3(0.0);
    prd_radiance.done = true;
}
