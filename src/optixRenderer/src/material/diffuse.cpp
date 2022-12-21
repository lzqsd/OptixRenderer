#include "creator/createMaterial.h"

Material createDiffuseMaterial(Context& context, material_t mat)
{
    const std::string ptx_path = ptxPath( "diffuse.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
    
    material["uvScale"] -> setFloat(mat.uvScale );
    
    // Texture Sampler 
    TextureSampler albedoSampler = createTextureSampler(context);

    if(mat.albedo_texname != std::string("") ){

        material["isAlbedoTexture"] -> setInt(1);
        cv::Mat albedoTexture = cv::imread(mat.albedo_texname, cv::IMREAD_COLOR);  
        cv::Mat albedoTextureFloat(
                albedoTexture.rows, 
                albedoTexture.cols, 
                CV_32FC3 );  

        if(albedoTexture.empty() ){
            std::cout<<"Wrong: unable to load the texture map: "<<mat.albedo_texname<<"!"<<std::endl;
            exit(1);
        } 
        for(int i = 0; i < albedoTexture.rows; i++){
            for(int j = 0; j < albedoTexture.cols; j++){
                float b = albedoTexture.at<cv::Vec3b>(i, j)[0];
                float g = albedoTexture.at<cv::Vec3b>(i, j)[1];
                float r = albedoTexture.at<cv::Vec3b>(i, j)[2]; 

                b = srgb2rgb(b / 255.0 ) * mat.albedoScale[2];
                g = srgb2rgb(g / 255.0 ) * mat.albedoScale[1];
                r = srgb2rgb(r / 255.0 ) * mat.albedoScale[0];
                
                b = std::min(b, 1.0f);
                g = std::min(g, 1.0f); 
                r = std::min(r, 1.0f);

                albedoTextureFloat.at<cv::Vec3f>(i, j)[0] = b;
                albedoTextureFloat.at<cv::Vec3f>(i, j)[1] = g;
                albedoTextureFloat.at<cv::Vec3f>(i, j)[2] = r;
            }
        }
        loadImageToTextureSampler(context, albedoSampler, albedoTextureFloat );
        material["albedo"] -> setFloat(make_float3(1.0) );
    } 
    else{
        material["isAlbedoTexture"] -> setInt(0);
        
        // albedo buffer
        loadEmptyToTextureSampler(context, albedoSampler);
        material["albedo"] -> setFloat(
                std::min(mat.albedo[0] * mat.albedoScale[0], 1.0f),
                std::min(mat.albedo[1] * mat.albedoScale[1], 1.0f),  
                std::min(mat.albedo[2] * mat.albedoScale[2], 1.0f) );
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
