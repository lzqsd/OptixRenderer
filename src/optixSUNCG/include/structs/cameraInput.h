#ifndef CAMERAINPUT_HEADER
#define CAMERAINPUT_HEADER

#include <string>

class AdaptiveSampler{
public:
    int maxIteration;
    float noiseThreshold;
    AdaptiveSampler(){
        noiseThreshold = 0.025;
        maxIteration = 4;
    }
};

class CameraInput{
public:
    std::string cameraType;
    float fov;
    bool isAxisX;
    float target[3];
    float origin[3];
    float up[3];
    int width;
    int height;
    int sampleNum; 
    std::string sampleType;
    bool isHdr;
    AdaptiveSampler adaptiveSampler;   

    CameraInput(){
        cameraType = "perspective";

        fov = 60;
        isAxisX = true;
        isHdr = false;

        // Camera location
        origin[0] = 0;
        origin[1] = 0;
        origin[2] = 1;

        // Camera Up 
        up[0] = 0;
        up[1] = 0;
        up[2] = 1;

        // Camera target
        target[0] = 0;
        target[1] = 0;
        target[2] = 0;

        width = 512;
        height = 512;

        sampleNum = 400;
        sampleType = std::string("independent");
    }
};

#endif
