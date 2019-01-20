#ifndef CREATEMATERIAL_HEADER
#define CREATEMATERIAL_HEADER

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_aabb_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>
#include <string>
#include <opencv2/opencv.hpp>
#include "createTextureSampler.h"
#include "ptxPath.h"
#include "sutil/tinyobjloader/objLoader.h"

using namespace optix;


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
    material["albedo"] -> setFloat(0.5, 0.5, 0.5 );
    material["albedoMap"] -> setTextureSampler(albedoSampler);
   
    TextureSampler normalSampler = createTextureSampler(context);
    material["isNormalTexture"] -> setInt(0);
    loadEmptyToTextureSampler(context, normalSampler);
    material["normalMap"] -> setTextureSampler(normalSampler );
    return material;
}

Material createDiffuseMaterial(Context& context, objLoader::material_t mat)
{
    const std::string ptx_path = ptxPath( "diffuse.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
        
    // Texture Sampler 
    TextureSampler albedoSampler = createTextureSampler(context);

    if(mat.albedo_texname != std::string("") ){

        material["isAlbedoTexture"] -> setInt(1);
        cv::Mat albedoTexture = cv::imread(mat.albedo_texname, cv::IMREAD_COLOR);
        if(albedoTexture.empty() ){
            std::cout<<"Wrong: unable to load the texture map: "<<mat.albedo_texname<<"!"<<std::endl;
            exit(1);
        }
        loadImageToTextureSampler(context, albedoSampler, albedoTexture );
        material["albedo"] -> setFloat(make_float3(1.0) );
    } 
    else{
        material["isAlbedoTexture"] -> setInt(0);
        
        // albedo buffer
        loadEmptyToTextureSampler(context, albedoSampler);
        material["albedo"] -> setFloat(mat.albedo[0], \
                mat.albedo[1], mat.albedo[2] );
    }
    material["albedoMap"] -> setTextureSampler(albedoSampler);
   
    TextureSampler normalSampler = createTextureSampler(context);
    
    if(mat.normal_texname != std::string("") ){
        material["isNormalTexture"] -> setInt(1);
        cv::Mat normalTexture = cv::imread(mat.normal_texname, cv::IMREAD_COLOR);
        if(normalTexture.empty() ){
            std::cout<<"Wrong: unable to load the texture map: "<<mat.normal_texname<<"!"<<std::endl;
            exit(1);
        }
        loadImageToTextureSampler(context, normalSampler, normalTexture); 
    }
    else{
        material["isNormalTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, normalSampler);
    }
    material["normalMap"] -> setTextureSampler(normalSampler );
    return material;
}

Material createPhongMaterial(Context& context, objLoader::material_t mat)
{
    const std::string ptx_path = ptxPath( "phong.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
        
    // Texture Sampler 
    TextureSampler albedoSampler = createTextureSampler(context);
    
    if(mat.albedo_texname != std::string("") ){
        material["isAlbedoTexture"] -> setInt(1);
        cv::Mat albedoTexture = cv::imread(mat.albedo_texname, cv::IMREAD_COLOR);
        if(albedoTexture.empty() ){
            std::cout<<"Wrong: unable to load the texture map: "<<mat.albedo_texname<<"!"<<std::endl;
            exit(1);
        }
        loadImageToTextureSampler(context, albedoSampler, albedoTexture );
        material["albedo"] -> setFloat(1.0, 1.0, 1.0);
    } 
    else{
        material["isAlbedoTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, albedoSampler);
        material["albedo"] -> setFloat(mat.albedo[0], \
                mat.albedo[1], mat.albedo[2] );
    }
    material["albedoMap"] -> setTextureSampler(albedoSampler);
    
    TextureSampler specularSampler = createTextureSampler(context);
    
    if(mat.specular_texname != std::string("") ){
        material["isSpecularTexture"] -> setInt(1); 
        cv::Mat specularTexture = cv::imread(mat.specular_texname, cv::IMREAD_COLOR);
        if(specularTexture.empty() ){
            std::cout<<"Wrong: unable to load the texture map: "<<mat.specular_texname<<"!"<<std::endl;
            exit(1);
        }
        loadImageToTextureSampler(context, specularSampler, specularTexture );
        material["specular"] -> setFloat(1.0, 1.0, 1.0);
    } 
    else{
        material["isSpecularTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, specularSampler);
        material["specular"] -> setFloat(mat.specular[0], \
                mat.specular[1], mat.specular[2] );
    }
    material["specularMap"] -> setTextureSampler(specularSampler);
   
    TextureSampler normalSampler = createTextureSampler(context );
    
    if(mat.normal_texname != std::string("") ){
        material["isNormalTexture"] -> setInt(1);
        cv::Mat normalTexture = cv::imread(mat.normal_texname, cv::IMREAD_COLOR);
        if(normalTexture.empty() ){
            std::cout<<"Wrong: unable to load the texture map: "<<mat.normal_texname<<"!"<<std::endl;
            exit(1);
        }
        loadImageToTextureSampler(context, normalSampler, normalTexture); 
    }
    else{
        material["isNormalTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, normalSampler);
    }
    material["normalMap"] -> setTextureSampler(normalSampler );

    TextureSampler glossySampler = createTextureSampler(context );
    
    if(mat.glossiness_texname != std::string("") ){
        material["isGlossyTexture"] -> setInt(1);
        cv::Mat glossyTexture = cv::imread(mat.glossiness_texname, cv::IMREAD_COLOR);
        if(glossyTexture.empty() ){
            std::cout<<"Wrong: unable to load the texture map: "<<mat.glossiness_texname<<"!"<<std::endl;
            exit(1);
        }
        loadImageToTextureSampler(context, glossySampler, glossyTexture); 
        material["glossy"] -> setFloat(0.0);
    }
    else{
        material["isGlossyTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, glossySampler);
        material["glossy"] -> setFloat(mat.glossiness);
    }
    material["glossyMap"] -> setTextureSampler(glossySampler );



    return material;
}

Material createMicrofacetMaterial(Context& context, objLoader::material_t mat)
{
    const std::string ptx_path = ptxPath( "microfacet.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
        
    // Texture Sampler 
    TextureSampler albedoSampler = createTextureSampler(context);
    
    if(mat.albedo_texname != std::string("") ){
        material["isAlbedoTexture"] -> setInt(1);
        cv::Mat albedoTexture = cv::imread(mat.albedo_texname, cv::IMREAD_COLOR);
        if(albedoTexture.empty() ){
            std::cout<<"Wrong: unable to load the texture map: "<<mat.albedo_texname<<"!"<<std::endl;
            exit(1);
        }
        loadImageToTextureSampler(context, albedoSampler, albedoTexture );
        material["albedo"] -> setFloat(make_float3(1.0) );
    } 
    else{
        material["isAlbedoTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, albedoSampler);
        material["albedo"] -> setFloat(mat.albedo[0], \
                mat.albedo[1], mat.albedo[2] );
    }
    material["albedoMap"] -> setTextureSampler(albedoSampler);
   
    TextureSampler normalSampler = createTextureSampler(context );
    
    if(mat.normal_texname != std::string("") ){
        material["isNormalTexture"] -> setInt(1);
        cv::Mat normalTexture = cv::imread(mat.normal_texname, cv::IMREAD_COLOR);
        if(normalTexture.empty() ){
            std::cout<<"Wrong: unable to load the texture map: "<<mat.normal_texname<<"!"<<std::endl;
            exit(1);
        }
        loadImageToTextureSampler(context, normalSampler, normalTexture); 
    }
    else{
        material["isNormalTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, normalSampler);
    }
    material["normalMap"] -> setTextureSampler(normalSampler );

    TextureSampler roughSampler = createTextureSampler(context );
    
    if(mat.roughness_texname != std::string("") ){
        material["isRoughTexture"] -> setInt(1);
        cv::Mat roughnessTexture = cv::imread(mat.roughness_texname, cv::IMREAD_COLOR);
        if(roughnessTexture.empty() ){
            std::cout<<"Wrong: unable to load the texture map: "<<mat.roughness_texname<<"!"<<std::endl;
            exit(1);
        }
        loadImageToTextureSampler(context, roughSampler, roughnessTexture); 
        material["rough"] -> setFloat(1.0);
    }
    else{
        material["isRoughTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, roughSampler);
        material["rough"] -> setFloat(mat.roughness);
    }
    material["roughMap"] -> setTextureSampler(roughSampler );
    
    TextureSampler metallicSampler = createTextureSampler(context );
    
    if(mat.metallic_texname != std::string("") ){
        material["isMetallicTexture"] -> setInt(1);
        cv::Mat metallicTexture = cv::imread(mat.metallic_texname, cv::IMREAD_COLOR);
        if(metallicTexture.empty() ){
            std::cout<<"Wrong: unable to load the texture map: "<<mat.metallic_texname<<"!"<<std::endl;
            exit(1);
        }
        loadImageToTextureSampler(context, metallicSampler, metallicTexture); 
        material["metallic"] -> setFloat(0.0);
    }
    else{
        material["isMetallicTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, metallicSampler);
        material["metallic"] -> setFloat(mat.metallic );
    }
    material["metallicMap"] -> setTextureSampler(metallicSampler );

    material["F0"] -> setFloat(mat.fresnel);
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

Material createMaskMaterial(Context& context, bool isAreaLight ){
    const std::string ptx_path = ptxPath( "mask.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram(0, ch_program );
    material->setAnyHitProgram(1, ah_program );
    
    if ( isAreaLight == true ) {
        material["isAreaLight"] -> setInt(1); 
    }
    else{
        material["isAreaLight"] -> setInt(0);
    }
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

Material createAlbedoMaterial(Context& context, objLoader::material_t mat){
    const std::string ptx_path = ptxPath( "albedo.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
        
    // Texture Sampler 
    TextureSampler albedoSampler = createTextureSampler(context);

    if(mat.albedo_texname != std::string("") ){
        material["isAlbedoTexture"] -> setInt(1);
        cv::Mat albedoTexture = cv::imread(mat.albedo_texname, cv::IMREAD_COLOR);
        if(albedoTexture.empty() ){
            std::cout<<"Wrong: unable to load the texture map: "<<mat.albedo_texname<<"!"<<std::endl;
            exit(1);
        }
        loadImageToTextureSampler(context, albedoSampler, albedoTexture );
        material["albedo"] -> setFloat(make_float3(1.0) );
    } 
    else{
        material["isAlbedoTexture"] -> setInt(0);
        
        // albedo buffer
        loadEmptyToTextureSampler(context, albedoSampler);
        material["albedo"] -> setFloat(mat.albedo[0], \
                mat.albedo[1], mat.albedo[2] );
    }
    material["albedoMap"] -> setTextureSampler(albedoSampler); 
    return material;
}

Material createNormalMaterial(Context& context, objLoader::material_t mat){
    const std::string ptx_path = ptxPath( "normal.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
        
    TextureSampler normalSampler = createTextureSampler(context );
    
    if(mat.normal_texname != std::string("") ){
        material["isNormalTexture"] -> setInt(1);
        cv::Mat normalTexture = cv::imread(mat.normal_texname, cv::IMREAD_COLOR);
        if(normalTexture.empty() ){
            std::cout<<"Wrong: unable to load the texture map: "<<mat.normal_texname<<"!"<<std::endl;
            exit(1);
        }
        loadImageToTextureSampler(context, normalSampler, normalTexture); 
    }
    else{
        material["isNormalTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, normalSampler);
    }
    material["normalMap"] -> setTextureSampler(normalSampler );

    return material;
}

Material createRoughnessMaterial(Context& context, objLoader::material_t mat){
    const std::string ptx_path = ptxPath( "roughness.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
        
    TextureSampler roughSampler = createTextureSampler(context );
    
    if(mat.roughness_texname != std::string("") ){
        material["isRoughTexture"] -> setInt(1);
        cv::Mat roughnessTexture = cv::imread(mat.roughness_texname, cv::IMREAD_COLOR);
        if(roughnessTexture.empty() ){
            std::cout<<"Wrong: unable to load the texture map: "<<mat.roughness_texname<<"!"<<std::endl;
            exit(1);
        }
        loadImageToTextureSampler(context, roughSampler, roughnessTexture); 
        material["rough"] -> setFloat(1.0);
    }
    else{
        material["isRoughTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, roughSampler);
        material["rough"] -> setFloat(mat.roughness);
    }
    material["roughMap"] -> setTextureSampler(roughSampler );

    return material; 
}

Material createMetallicMaterial(Context& context, objLoader::material_t mat){
    const std::string ptx_path = ptxPath( "metallic.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
        
    TextureSampler metallicSampler = createTextureSampler(context );
    
    if(mat.metallic_texname != std::string("") ){
        material["isMetallicTexture"] -> setInt(1);
        cv::Mat metallicTexture = cv::imread(mat.metallic_texname, cv::IMREAD_COLOR);
        if(metallicTexture.empty() ){
            std::cout<<"Wrong: unable to load the texture map: "<<mat.metallic_texname<<"!"<<std::endl;
            exit(1);
        }
        loadImageToTextureSampler(context, metallicSampler, metallicTexture); 
        material["metallic"] -> setFloat(1.0);
    }
    else{
        material["isMetallicTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, metallicSampler);
        material["metallic"] -> setFloat(mat.metallic);
    }
    material["metallicMap"] -> setTextureSampler(metallicSampler );

    return material; 
}

Material createDepthMaterial(Context& context ){
    const std::string ptx_path = ptxPath( "depth.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
        
    return material; 
}


#endif
