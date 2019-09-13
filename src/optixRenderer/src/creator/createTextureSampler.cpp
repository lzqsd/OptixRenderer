#include "creator/createTextureSampler.h" 

TextureSampler createTextureSampler(Context& context){
    TextureSampler Sampler = context->createTextureSampler();
    Sampler->setWrapMode(0, RT_WRAP_REPEAT );
    Sampler->setWrapMode(1, RT_WRAP_REPEAT );
    Sampler->setWrapMode(2, RT_WRAP_REPEAT );
    Sampler->setIndexingMode(RT_TEXTURE_INDEX_NORMALIZED_COORDINATES );
    Sampler->setReadMode(RT_TEXTURE_READ_NORMALIZED_FLOAT );
    Sampler->setMaxAnisotropy(1.0f );
    Sampler->setMipLevelCount(1u );
    Sampler->setArraySize(1u );
    Sampler -> setFilteringModes(RT_FILTER_LINEAR, RT_FILTER_LINEAR, RT_FILTER_NONE);
    return Sampler;
}

void loadImageToTextureSampler(Context& context, TextureSampler& Sampler, cv::Mat& texMat){

    int width = texMat.cols;
    int height = texMat.rows;

    // Set up the texture sampler
    Buffer texBuffer = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT4, width, height);
    float* texPt = reinterpret_cast<float*> (texBuffer -> map() );
    for(int r = 0; r < height; r++){
        for(int c = 0; c < width; c++){

            int index = 4 * (r*width + c);
            if(texMat.type() == CV_8UC3){
                cv::Vec3b color = texMat.at<cv::Vec3b>(height - 1 - r, c);
                texPt[index + 0] = float(color[2] ) / 255.0;
                texPt[index + 1] = float(color[1] ) / 255.0;
                texPt[index + 2] = float(color[0] ) / 255.0;
            }
            else if(texMat.type() == CV_32FC3){
                cv::Vec3f color = texMat.at<cv::Vec3f>(height -1 -r, c);
                texPt[index + 0] = color[2];
                texPt[index + 1] = color[1];
                texPt[index + 2] = color[0];
            }
            else{
                std::cout<<"Wrong: unrecognizable Mat type when load image to TextureSampler."<<std::endl;
                exit(1);
            }
            texPt[index + 3] = 1.0;
        }   
    } 
    texBuffer -> unmap();
    Sampler -> setBuffer(0u, 0u, texBuffer);
}

void updateImageToTextureSampler(TextureSampler& Sampler, cv::Mat& texMat){

    int width = texMat.cols;
    int height = texMat.rows;

    // Set up the texture sampler
    Buffer texBuffer = Sampler->getBuffer(0u, 0u);
    float* texPt = reinterpret_cast<float*> (texBuffer -> map() );
    for(int r = 0; r < height; r++){
        for(int c = 0; c < width; c++){
            int index = 4 * (r*width + c);
            if(texMat.type() == CV_8UC3){
                cv::Vec3b color = texMat.at<cv::Vec3b>(height - 1 - r, c);
                texPt[index + 0] = float(color[2] ) / 255.0;
                texPt[index + 1] = float(color[1] ) / 255.0;
                texPt[index + 2] = float(color[0] ) / 255.0;
            }
            else if(texMat.type() == CV_32FC3){
                cv::Vec3f color = texMat.at<cv::Vec3f>(height -1 -r, c);
                texPt[index + 0] = color[2];
                texPt[index + 1] = color[1];
                texPt[index + 2] = color[0];
            }
            else{
                std::cout<<"Wrong: unrecognizable Mat type when load image to TextureSampler."<<std::endl;
                exit(1);
            }
            texPt[index + 3] = 1.0;
        }   
    } 
    texBuffer -> unmap();
}
