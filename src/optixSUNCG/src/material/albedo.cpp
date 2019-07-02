#include "creator/createMaterial.h"

Material createAlbedoMaterial(Context& context, material_t mat){
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
