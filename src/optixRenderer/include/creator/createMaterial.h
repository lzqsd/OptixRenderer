#ifndef CREATEMATERIAL_HEADER
#define CREATEMATERIAL_HEADER

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_aabb_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>
#include <string>
#include <opencv2/opencv.hpp>
#include "creator/createTextureSampler.h"
#include "utils/ptxPath.h"
#include "shapeStructs.h"

using namespace optix;


void loadEmptyToTextureSampler(Context& context,  TextureSampler& Sampler);

Material createDefaultMaterial(Context& context );

Material createDiffuseMaterial(Context& context, material_t mat);

Material createPhongMaterial(Context& context, material_t mat);

Material createMicrofacetMaterial(Context& context, material_t mat);

Material createDielectricMaterial(Context& context, material_t mat);

Material createConductorMaterial(Context& context, material_t mat);

Material createWhiteMaterial(Context& context );

Material createMaskMaterial(Context& context, bool isAreaLight );

Material createBlackMaterial(Context& context );

Material createAlbedoMaterial(Context& context, material_t mat);

Material createNormalMaterial(Context& context, material_t mat);

Material createRoughnessMaterial(Context& context, material_t mat);

Material createMetallicMaterial(Context& context, material_t mat);

Material createDepthMaterial(Context& context );


#endif
