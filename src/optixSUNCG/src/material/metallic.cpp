#include "creator/createMaterial.h"

Material createMetallicMaterial(Context& context, material_t mat){
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
