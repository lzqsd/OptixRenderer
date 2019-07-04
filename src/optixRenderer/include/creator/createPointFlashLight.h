#ifndef CREATEPOINTFLASHLIGHT_HEADER
#define CREATEPOINTFLASHLIGHT_HEADER

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_aabb_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>
#include <vector>
#include "lightStructs.h"
#include <cstdint>
#include <cstring>

using namespace optix;

void createPointLight(Context& context, std::vector<Point>& points);

void updatePointLight(Context& context, std::vector<Point>& points);

#endif
