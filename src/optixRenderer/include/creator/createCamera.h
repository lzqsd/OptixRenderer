#ifndef CREATECAMERA_HEADER
#define CREATECAMERA_HEADER

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_aabb_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

#include "structs/cameraInput.h"
#include "Camera.h"

using namespace optix;

void createCamera(Context& context, CameraInput& camInput);

#endif
