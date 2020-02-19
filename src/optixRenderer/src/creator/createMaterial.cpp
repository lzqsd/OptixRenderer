#include "creator/createMaterial.h"


void loadEmptyToTextureSampler(Context& context,  TextureSampler& Sampler){
    Buffer texBuffer = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_FLOAT4, 1u, 1u );
    float* texPt = static_cast<float*>( texBuffer->map() );
    texPt[0] = 1.0f;
    texPt[1] = 1.0f;
    texPt[2] = 1.0f;
    texPt[3] = 1.0f;
    texBuffer->unmap(); 
    Sampler->setBuffer( 0u, 0u, texBuffer );
}

Material createDefaultMaterial(Context& context ){
    const std::string ptx_path = ptxPath( "diffuse.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
        
    // Texture Sampler 
    TextureSampler albedoSampler = createTextureSampler(context);
    material["isAlbedoTexture"] -> setInt(0);   
    loadEmptyToTextureSampler(context, albedoSampler);
    material["albedo"] -> setFloat(0.8, 0.8, 0.8 );
    material["albedoMap"] -> setTextureSampler(albedoSampler);
   
    TextureSampler normalSampler = createTextureSampler(context);
    material["isNormalTexture"] -> setInt(0);
    loadEmptyToTextureSampler(context, normalSampler);
    material["normalMap"] -> setTextureSampler(normalSampler );
    return material;
}

Material createWhiteMaterial(Context& context ){
    const std::string ptx_path = ptxPath( "albedo.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
        
    // Texture Sampler 
    TextureSampler albedoSampler = createTextureSampler(context);
    material["isAlbedoTexture"] -> setInt(0);   
    loadEmptyToTextureSampler(context, albedoSampler);
    material["albedo"] -> setFloat(1.0, 1.0, 1.0 ); 
    material["albedoMap"] -> setTextureSampler(albedoSampler);
   
    return material;
}

Material createBlackMaterial(Context& context ){
    const std::string ptx_path = ptxPath( "albedo.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
        
    // Texture Sampler 
    TextureSampler albedoSampler = createTextureSampler(context);
    material["isAlbedoTexture"] -> setInt(0);   
    loadEmptyToTextureSampler(context, albedoSampler);
    material["albedo"] -> setFloat(0.0, 0.0, 0.0 );
    material["albedoMap"] -> setTextureSampler(albedoSampler);
   
    return material;
}

