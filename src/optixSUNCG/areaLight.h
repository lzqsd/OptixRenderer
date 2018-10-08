#ifndef AREALIGHT_HEADER
#define AREALIGHT_HEADER

#include <vector>
#include <optixu/optixu_vector_types.h>
#include <optixu/optixu_math_namespace.h>
#include <iostream>
#include "random.h"

using namespace optix;

class areaLight{
public:
    float3 vertices[3];
    float3 radiance;
};


#endif
