#ifndef LIGHTSTRUCTS_HEADER
#define LIGHTSTRUCTS_HEADER

#include <vector>
#include <optixu/optixu_vector_types.h>
#include <optixu/optixu_math_namespace.h>
#include <string>

using namespace optix;

class areaLight{
public:
    float3 vertices[3];
    float3 radiance;
};


class Point
{
public:
    float3 intensity;
    float3 position;
    bool isFlash;
    Point(){
        position.x = position.y = position.z = 0.0;
        intensity.x = intensity.y = intensity.z = 0.0;
        isFlash = false;
    }
};


class Envmap
{
public:
    std::string fileName;
    float scale;
    Envmap(){
        scale = 1;
    }
    Envmap(std::string fn):fileName(fn){
        scale = 1;
    }
};


#endif
