#ifndef MATERIAL_HEADER
#define MATERIAL_HEADER

#include <vector>
#include <optixu/optixu_math_namespace.h>

using namespace optix;

class BRDF{
public:

};

class Diffuse: public BRDF{
public:
    float3 reflectance;
    
};

class Phong: public BRDF{
public:
    float3 Kd;
    float3 Ks;
    float3 Ka;
    float a;
};


#endif
