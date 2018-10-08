#ifndef SHAPE_HEADER
#define SHAPE_HEADER

#include <vector>
#include <algorithm>
#include <optixu/optixu_math_namespace.h>

using namespace optix;

class Shape{

public:
    Shape(){
        isLight = false;
        matId = -1;
    }

    void computeArea();
    void sampleArea();
    void sampleDirec();
    void pdf();

public:
    std::vector<float3> vertices;
    std::vector<float3> normals;
    std::vector<float2> uvs;
    std::vector<int3> faceVertices;
    std::vector<int3> faceNormals;
    std::vector<int3> faceUvs;
    std::vector<float> areas;
    bool isLight;
    int matId;
};


#endif
