#ifndef SAMPLER_HEADER
#define SAMPLER_HEADER

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_aabb_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>
#include "postprocessing/filter.h"

using namespace optix;

void getOutputBuffer(Context& context, float* imgData, int width, int height, unsigned sizeScale = 1);

void independentSampling(
        Context& context, 
        int width, int height, 
        float* imgData, 
        int sampleNum, 
        unsigned sizeScale = 1);

float RMSEAfterScaling(const float* im1, const float* im2, int width, int height, float scale);

bool adaptiveSampling(
        Context& context,
        int width, int height, int& sampleNum, 
        float* imgData, 
        float noiseLimit = 0.11,
        bool noiseLimitEnabled = false,
        int maxIteration = 4, 
        float noiseThreshold = 0.03,
        unsigned sizeScale = 1
        );

#endif
