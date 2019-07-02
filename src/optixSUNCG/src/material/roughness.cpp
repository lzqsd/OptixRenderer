#include "creator/createMaterial.h"

Material createRoughnessMaterial(Context& context, material_t mat){
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
