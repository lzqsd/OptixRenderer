#ifndef SAMPLER_HEADER
#define SAMPLER_HEADER

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_aabb_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>
#include "filter.h"

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
    //medianFilter(imgData, width, height, 1); 
}

float RMSEAfterScaling(const float* im1, const float* im2, int width, int height, float scale){
    
    int length = width * height * 3;
    float* temp1 = new float[length ];
    float* temp2 = new float[length ];
    for(int i = 0; i < length; i++){
        temp1[i] = im1[i];
        temp2[i] = im2[i];
    }
    /*
    medianFilter(temp1, width, height, 1);
    medianFilter(temp2, width, height, 1);
    */

    float errorSum = 0;
    for(int i = 0; i < length; i++){
        errorSum += (temp1[i] - temp2[i]) * (temp1[i] - temp2[i] );
    } 

    delete [] temp1;
    delete [] temp2;

    return scale * sqrt(errorSum / length);
}

bool adaptiveSampling(
        Context& context,
        int width, int height, int& sampleNum, 
        float* imgData, 
        float noiseLimit = 0.11,
        bool noiseLimitEnabled = false,
        int maxExpo = 4)
{
    unsigned sqrt_num_samples = (unsigned )(sqrt(float(sampleNum ) ) + 1.0);
    if(sqrt_num_samples == 0)
        sqrt_num_samples = 1;
    context["sqrt_num_samples"] -> setUint(sqrt_num_samples );

    int pixelNum = width * height * 3;

    float* tempBuffer = new float[pixelNum ];
    float* tempBuffer2 = new float[pixelNum ];
    float* imgDataBuffer = NULL;
    
    // Render the first image 
    srand(time(NULL) );
    context["initSeed"] -> setUint( unsigned(0.5 * rand() ) );
    context -> launch(0, width, height);
    Buffer imgBuffer = getOutputBuffer(context );
    imgDataBuffer = reinterpret_cast<float*>(imgBuffer -> map() );

    // Compute the scale so that the mean of image will be 0.5
    float sum = 0;
    for(int i = 0; i < pixelNum; i++){
        tempBuffer[i] = imgDataBuffer[i];
        sum += tempBuffer[i];
    }

    float scale = 0.5 * pixelNum / fmaxf(sum, 1e-6); 
    std::cout<<"Scale: "<<scale<<std::endl;
    imgBuffer -> unmap();

    // Render the second image
    srand(time(NULL) );
    context["initSeed"] -> setUint(rand() );
    context -> launch(0, width, height);
    imgBuffer = getOutputBuffer(context );
    imgDataBuffer = reinterpret_cast<float*>(imgBuffer -> map() );
    for(int i = 0; i < pixelNum; i++){
        tempBuffer2[i] = imgDataBuffer[i];
    }
    imgBuffer -> unmap();

    // Repeated render the image until the noise below the threshold
    int expo = 1;
    float threshold = 0.03;
    while(true ){
        float error = RMSEAfterScaling(tempBuffer, tempBuffer2, width, height, scale);
        for(int i = 0; i < pixelNum; i++)
            tempBuffer[i] = 0.5 * (tempBuffer[i] + tempBuffer2[i]);
        std::cout<<"Samle Num: "<<sampleNum * 2<<std::endl;
        std::cout<<"Error: "<<error<<std::endl;

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
        for(int i = 0; i < pixelNum; i++){
            tempBuffer2[i] = imgDataBuffer[i];
        }
        imgBuffer -> unmap();
    }
    for(int i = 0; i < pixelNum; i++)
        imgData[i] = tempBuffer[i];
    //medianFilter(imgData, width, height, 1); 

    delete [] tempBuffer;
    delete [] tempBuffer2;
    sampleNum = sampleNum * 2;

    return false;
}

#endif
