#ifndef CREATETEXTURESAMPLER_HEADER
#define CREATETEXTURESAMPLER_HEADER

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_aabb_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

using namespace optix;

TextureSampler createTextureSampler(Context& context){
    TextureSampler Sampler = context->createTextureSampler();
    Sampler->setWrapMode(0, RT_WRAP_REPEAT );
    Sampler->setWrapMode(1, RT_WRAP_REPEAT );
    Sampler->setWrapMode(2, RT_WRAP_REPEAT );
    Sampler->setIndexingMode(RT_TEXTURE_INDEX_NORMALIZED_COORDINATES );
    Sampler->setReadMode(RT_TEXTURE_READ_NORMALIZED_FLOAT );
    Sampler->setMaxAnisotropy(1.0f );
    Sampler->setMipLevelCount(1u );
    Sampler->setArraySize(1u );
    Sampler -> setFilteringModes(RT_FILTER_LINEAR, RT_FILTER_LINEAR, RT_FILTER_NONE);
    return Sampler;
}

#endif
