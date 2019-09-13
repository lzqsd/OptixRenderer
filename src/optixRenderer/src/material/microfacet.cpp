#include "creator/createMaterial.h"
#include <algorithm>


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
        if(mat.albedoScale[0] != 1 || mat.albedoScale[1] != 1 || mat.albedoScale[2] != 1){
            for(int i = 0; i < albedoTexture.rows; i++){
                for(int j = 0; j < albedoTexture.cols; j++){
                    float b = albedoTexture.at<cv::Vec3b>(i, j)[0];
                    float g = albedoTexture.at<cv::Vec3b>(i, j)[1];
                    float r = albedoTexture.at<cv::Vec3b>(i, j)[2]; 

                    b = 255.0 * pow(pow(b / 255.0, 2.2f) * mat.albedoScale[2], 1.0f / 2.2f);
                    g = 255.0 * pow(pow(g / 255.0, 2.2f) * mat.albedoScale[1], 1.0f / 2.2f);
                    r = 255.0 * pow(pow(r / 255.0, 2.2f) * mat.albedoScale[0], 1.0f / 2.2f);  

                    b = std::min(b, 255.0f);
                    g = std::min(g, 255.0f);
                    r = std::min(r, 255.0f);

                    albedoTexture.at<cv::Vec3b>(i, j)[0] = (unsigned char)b;
                    albedoTexture.at<cv::Vec3b>(i, j)[1] = (unsigned char)g;
                    albedoTexture.at<cv::Vec3b>(i, j)[2] = (unsigned char)r;
                }
            }
        }
        loadImageToTextureSampler(context, albedoSampler, albedoTexture );
        material["albedo"] -> setFloat(make_float3(1.0) );
    } 
    else{
        material["isAlbedoTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, albedoSampler);
        material["albedo"] -> setFloat( 
                std::min(mat.albedo[0] * mat.albedoScale[0], 1.0f ),
                std::min(mat.albedo[1] * mat.albedoScale[1], 1.0f ), 
                std::min(mat.albedo[2] * mat.albedoScale[2], 1.0f ) );
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
        material["rough"] -> setFloat(1.0);
    }
    else{
        material["isRoughTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, roughSampler);
        material["rough"] -> setFloat(
                std::min(mat.roughness * mat.roughnessScale, 1.0f) );
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
