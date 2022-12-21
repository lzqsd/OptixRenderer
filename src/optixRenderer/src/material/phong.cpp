#include "creator/createMaterial.h"

Material createPhongMaterial(Context& context, material_t mat)
{
    const std::string ptx_path = ptxPath( "phong.cu" );
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
        loadEmptyToTextureSampler(context, albedoSampler);
        material["albedo"] -> setFloat( 
                std::min(mat.albedo[0] * mat.albedoScale[0], 1.0f ),
                std::min(mat.albedo[1] * mat.albedoScale[1], 1.0f ), 
                std::min(mat.albedo[2] * mat.albedoScale[2], 1.0f ) );
    }
    material["albedoMap"] -> setTextureSampler(albedoSampler);
    
    TextureSampler specularSampler = createTextureSampler(context);
    
    if(mat.specular_texname != std::string("") ){
        material["isSpecularTexture"] -> setInt(1); 
        cv::Mat specularTexture = cv::imread(mat.specular_texname, cv::IMREAD_COLOR); 
        cv::Mat specularTextureFloat(
                specularTexture.rows, 
                specularTexture.cols, 
                CV_32FC3 );
        if(specularTexture.empty() ){
            std::cout<<"Wrong: unable to load the texture map: "<<mat.specular_texname<<"!"<<std::endl;
            exit(1);
        }
        for(int i = 0; i < specularTexture.rows; i++){
            for(int j = 0; j < specularTexture.cols; j++){
                float b = specularTexture.at<cv::Vec3b>(i, j)[0];
                float g = specularTexture.at<cv::Vec3b>(i, j)[1];
                float r = specularTexture.at<cv::Vec3b>(i, j)[2]; 

                b = srgb2rgb(b / 255.0 ) * mat.specularScale[2];
                g = srgb2rgb(g / 255.0 ) * mat.specularScale[1];
                r = srgb2rgb(r / 255.0 ) * mat.specularScale[0];  

                b = std::min(b, 1.0f);
                g = std::min(g, 1.0f);
                r = std::min(r, 1.0f);

                specularTextureFloat.at<cv::Vec3f>(i, j)[0] = b;
                specularTextureFloat.at<cv::Vec3f>(i, j)[1] = g;
                specularTextureFloat.at<cv::Vec3f>(i, j)[2] = r;
            }
        }
        loadImageToTextureSampler(context, specularSampler, specularTextureFloat );
        material["specular"] -> setFloat(1.0, 1.0, 1.0);
    } 
    else{
        material["isSpecularTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, specularSampler);
        material["specular"] -> setFloat( 
                std::min(mat.specular[0] * mat.specularScale[0], 1.0f ),
                std::min(mat.specular[1] * mat.specularScale[1], 1.0f ), 
                std::min(mat.specular[2] * mat.specularScale[2], 1.0f ) );
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
        if(mat.glossinessScale != 1){
            for(int i = 0; i < glossyTexture.rows; i++){
                for(int j = 0; j < glossyTexture.cols; j++){
                    float b = glossyTexture.at<cv::Vec3b>(i, j)[0];
                    float g = glossyTexture.at<cv::Vec3b>(i, j)[1];
                    float r = glossyTexture.at<cv::Vec3b>(i, j)[2];

                    b = b * mat.glossinessScale;
                    g = g * mat.glossinessScale; 
                    r = r * mat.glossinessScale; 

                    b = std::min(b, 255.0f);
                    g = std::min(g, 255.0f); 
                    r = std::min(r, 255.0f);

                    glossyTexture.at<cv::Vec3b>(i, j)[0] = (unsigned char)b;
                    glossyTexture.at<cv::Vec3b>(i, j)[1] = (unsigned char)g;
                    glossyTexture.at<cv::Vec3b>(i, j)[2] = (unsigned char)r;
                }
            }
        }
        loadImageToTextureSampler(context, glossySampler, glossyTexture); 
        material["glossy"] -> setFloat(0.0);
    }
    else{
        material["isGlossyTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, glossySampler);
        material["glossy"] -> setFloat(std::min(mat.glossiness * mat.glossinessScale, 1.0f ) );
    }
    material["glossyMap"] -> setTextureSampler(glossySampler );
    return material;
}
