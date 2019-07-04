#ifndef CREATEGEOMETRY_HEADER
#define CREATEGEOMETRY_HEADER

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_aabb_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>
#include <vector>
#include <algorithm>
#include <map>
#include <stdio.h>
#include "creator/createMaterial.h"
#include "creator/createAreaLight.h"
#include "shapeStructs.h"

const int faceLimit = 100000;

void splitShapes(shape_t& shapeLarge, std::vector<shape_t >& shapeArr);

void createGeometry(
        Context& context,
        const std::vector<shape_t>& shapes,
        const std::vector<material_t>& materials,
        int mode );

#endif
