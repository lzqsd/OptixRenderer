#include "creator/createMaterial.h"
#include <algorithm>

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
        if(mat.roughnessScale != 1){
            for(int i = 0; i < roughnessTexture.rows; i++){
                for(int j = 0; j < roughnessTexture.cols; j++){
                    float b = roughnessTexture.at<cv::Vec3b>(i, j)[0];
                    float g = roughnessTexture.at<cv::Vec3b>(i, j)[1];
                    float r = roughnessTexture.at<cv::Vec3b>(i, j)[2];

                    b = b * mat.roughnessScale;
                    g = g * mat.roughnessScale; 
                    r = r * mat.roughnessScale; 

                    b = std::min(b, 255.0f);
                    g = std::min(g, 255.0f); 
                    r = std::min(r, 255.0f);

                    roughnessTexture.at<cv::Vec3b>(i, j)[0] = (unsigned char)b;
                    roughnessTexture.at<cv::Vec3b>(i, j)[1] = (unsigned char)g;
                    roughnessTexture.at<cv::Vec3b>(i, j)[2] = (unsigned char)r;
                }
            }
        }
        loadImageToTextureSampler(context, roughSampler, roughnessTexture); 
        material["rough"] -> setFloat(1.0 );
    }
    else{
        material["isRoughTexture"] -> setInt(0 );
        loadEmptyToTextureSampler(context, roughSampler );
        material["rough"] -> setFloat(std::min(mat.roughness * mat.roughnessScale, 1.0f ) );
    }
    material["roughMap"] -> setTextureSampler(roughSampler );

    return material; 
}
