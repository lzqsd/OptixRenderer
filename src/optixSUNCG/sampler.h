#ifndef SAMPLER_HEADER
#define SAMPLER_HEADER

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_aabb_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

using namespace optix;

static Buffer getOutputBuffer(Context& context)
{
    return context[ "output_buffer" ]->getBuffer();
}

void independentSampling(
        Context& context, 
        int width, int height, 
        float* imgData, 
        int sampleNum )
{
    unsigned sqrt_num_samples = (unsigned )(sqrt(float(sampleNum ) ) + 1.0);
    if(sqrt_num_samples == 0)
        sqrt_num_samples = 1;
    context["sqrt_num_samples"] -> setUint(sqrt_num_samples );
    
    srand(time(NULL) );
    context["initSeed"] -> setUint( rand() );

    context -> launch(0, width, height);
    Buffer imgBuffer = getOutputBuffer(context ); 
    float* imgDataBuffer = reinterpret_cast<float*>(imgBuffer -> map() );
    for(int i = 0; i < width * height * 3; i++){
        imgData[i] = imgDataBuffer[i];
    }
    imgBuffer -> unmap();
}

float RMSEAfterScaling(const float* im1, const float* im2, int length, float scale){
    float errorSum = 0;
    for(int i = 0; i < length; i++){
        errorSum += (im1[i] - im2[i]) * (im1[i] - im2[i] );
    } 
    return scale * sqrt(errorSum / length);
}

bool adaptiveSampling(
        Context& context,
        int width, int height, int& sampleNum, 
        float* imgData, 
        float noiseLimit = 0.11,
        bool noiseLimitEnabled = false,
        int maxExpo = 5)
{
    unsigned sqrt_num_samples = (unsigned )(sqrt(float(sampleNum ) ) + 1.0);
    if(sqrt_num_samples == 0)
        sqrt_num_samples = 1;
    context["sqrt_num_samples"] -> setUint(sqrt_num_samples );

    int pixelNum = width * height * 3;
    float* tempBuffer = new float[pixelNum ];
    
    // Render the first image 
    srand(time(NULL) );
    context["initSeed"] -> setUint( unsigned(0.5 * rand() ) );
    context -> launch(0, width, height);
    Buffer imgBuffer = getOutputBuffer(context );
    float* imgDataBuffer = reinterpret_cast<float*>(imgBuffer -> map() );

    // Compute the scale so that the mean of image will be 0.5
    float sum = 0;
    for(int i = 0; i < pixelNum; i++){
        tempBuffer[i] = imgDataBuffer[i];
        sum += tempBuffer[i];
    }
    float scale = 0.5 * pixelNum / fmaxf(sum, 1e-6); 
    std::cout<<"Scale: "<<scale<<std::endl;
    imgBuffer -> unmap();

    if(scale > 10){
        std::cout<<"Warning: the rendered image is too dark, will stop!"<<std::endl;
        return true;
    }
    else{ 
        // Render the second image
        srand(time(NULL) );
        context["initSeed"] -> setUint(rand() );
        context -> launch(0, width, height);
        imgBuffer = getOutputBuffer(context );
        imgDataBuffer = reinterpret_cast<float*>(imgBuffer -> map() );
        // Repeated render the image until the noise below the threshold
        int expo = 1;
        float threshold = 0.025;
        while(true ){
            float error = RMSEAfterScaling(tempBuffer, imgDataBuffer, pixelNum, scale);
            for(int i = 0; i < pixelNum; i++)
                tempBuffer[i] = 0.5 * (tempBuffer[i] + imgDataBuffer[i]);
            std::cout<<"Samle Num: "<<sampleNum * 2<<std::endl;
            std::cout<<"Error: "<<error<<std::endl;
            imgBuffer -> unmap();

            if(expo == 1 && noiseLimitEnabled == true && error > noiseLimit ){
                std::cout<<"Warning: the rendered image is too noisy, will stop!"<<std::endl;
                return true;
            }
 
            if(error < threshold || expo >= maxExpo){
                break;
            }
            expo += 1;
 
            // Update sampling parameters
            sampleNum = sampleNum * 2;
            sqrt_num_samples = (unsigned )(sqrt(float(sampleNum ) ) + 1.0);
            srand(time(NULL) * expo );
            context["initSeed"] -> setUint(rand() );
            context["sqrt_num_samples"] -> setUint(sqrt_num_samples );
 
            // Rendering
            context -> launch(0, width, height);
            imgBuffer = getOutputBuffer(context );
            imgDataBuffer = reinterpret_cast<float*>(imgBuffer -> map() );
        }
    } 
    for(int i = 0; i < pixelNum; i++)
        imgData[i] = tempBuffer[i];
    delete [] tempBuffer;
    sampleNum = sampleNum * 2;

    return false;
}

#endif
