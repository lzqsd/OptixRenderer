#ifndef CREATETEXTURESAMPLER_HEADER
#define CREATETEXTURESAMPLER_HEADER

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_aabb_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>
#include <opencv2/opencv.hpp>

using namespace optix;

TextureSampler createTextureSampler(Context& context);

void loadImageToTextureSampler(Context& context, TextureSampler& Sampler, cv::Mat& texMat);

void updateImageToTextureSampler(TextureSampler& Sampler, cv::Mat& texMat);

#endif
