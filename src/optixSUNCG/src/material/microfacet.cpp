#include "creator/createMaterial.h"


Material createMicrofacetMaterial(Context& context, material_t mat)
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
