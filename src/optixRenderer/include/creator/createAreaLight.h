#ifndef CREATEAREALIGHT_HEADER
#define CREATEAREALIGHT_HEADER

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_aabb_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>
#include "shapeStructs.h"
#include "utils/ptxPath.h"
#include "lightStructs.h"
#include <cstdint>
#include <cstring>

using namespace optix;


int createAreaLightsBuffer( Context& context, const std::vector<shape_t> shapes );

Material createAreaLight(Context& context, shape_t shape);

#endif
