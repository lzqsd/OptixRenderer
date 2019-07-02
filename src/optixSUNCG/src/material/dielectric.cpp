#include "creator/createMaterial.h"

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

    
    material["specular"] -> setFloat(mat.specular[0], mat.specular[1], mat.specular[2] );
    material["transmittance"] -> setFloat(mat.transmittance[0], 
            mat.transmittance[1], mat.transmittance[2] ); 
    material["intIOR"] -> setFloat(mat.intIOR );
    material["extIOR"] -> setFloat(mat.extIOR );

    return material;
}
