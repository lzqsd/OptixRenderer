#ifndef POINT_HEADER
#define POINT_HEADER

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

#endif
