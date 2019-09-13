#include "creator/createMaterial.h"
#include <algorithm>

Material createDielectricMaterial(Context& context, material_t mat)
{
    const std::string ptx_path = ptxPath( "dielectric.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
    
    // Texture Sampler 
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

    
    material["specular"] -> setFloat(
            std::min(mat.specular[0] * mat.specularScale[0], 1.0f), 
            std::min(mat.specular[1] * mat.specularScale[1], 1.0f), 
            std::min(mat.specular[2] * mat.specularScale[2], 1.0f) );
    material["transmittance"] -> setFloat(
            std::min(mat.transmittance[0] * mat.transmittanceScale[0], 1.0f), 
            std::min(mat.transmittance[1] * mat.transmittanceScale[1], 1.0f), 
            std::min(mat.transmittance[2] * mat.transmittanceScale[2], 1.0f) ); 
    material["intIOR"] -> setFloat(mat.intIOR );
    material["extIOR"] -> setFloat(mat.extIOR );

    return material;
}
