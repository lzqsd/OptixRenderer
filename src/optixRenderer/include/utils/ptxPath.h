#ifndef PTXPATH_HEADER
#define PTXPATH_HEADER


#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_aabb_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>
#include <string>
#include "sutil.h"

using namespace optix;

const char* const SAMPLE_NAME = "optixRenderer";

std::string ptxPath( const std::string& cuda_file );

#endif
