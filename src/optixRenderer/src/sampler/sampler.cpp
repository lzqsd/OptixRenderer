#include "sampler/sampler.h"


void getOutputBuffer(Context& context, float* imgData, int width, int height, unsigned sizeScale)
{
    Buffer imgBuffer = context[ "output_buffer" ]->getBuffer();
    float* imgDataBuffer = reinterpret_cast<float*>(imgBuffer -> map() );
    for(int r = 0; r < height; r++){
        for(int c = 0; c < width; c++){
            int N = sizeScale * sizeScale;
            for(int ch = 0; ch < 3; ch++){
                
                float sum = 0;
                for(int sr = 0; sr < sizeScale; sr++){
                    for(int sc = 0; sc < sizeScale; sc++){
                        int C = c * sizeScale + sc;
                        int R = r * sizeScale + sr;
                        int Index = 3 * (R * width * sizeScale + C) + ch;
                        sum += imgDataBuffer[Index];
                    }
                }
                imgData[3*(r*width + c) + ch] = sum / N;
            }
        }
    }
    imgBuffer -> unmap();
}

void getTwoBounceOutputBuffer(Context& context, float* imgData, int width, int height, unsigned sizeScale)
{
    Buffer imgBuffer1, imgBuffer2, imgBuffer3, imgBuffer4; 
    float *imgDataBuffer1 = NULL, *imgDataBuffer2 = NULL;
    float *imgDataBuffer3 = NULL, *imgDataBuffer4 = NULL;
    int offset1 = 0, offset2 = 3, offset3 = 6, offset4 = 9; 

    imgBuffer1 = context[ "normal1_buffer" ]->getBuffer();
    imgDataBuffer1 = reinterpret_cast<float*>(imgBuffer1 -> map() );
    imgBuffer2 = context[ "depth1_buffer" ]->getBuffer();
    imgDataBuffer2 = reinterpret_cast<float*>(imgBuffer2 -> map() );
    imgBuffer3 = context[ "normal2_buffer" ]->getBuffer();
    imgDataBuffer3 = reinterpret_cast<float*>(imgBuffer3 -> map() );
    imgBuffer4 = context[ "depth2_buffer" ]->getBuffer();
    imgDataBuffer4 = reinterpret_cast<float*>(imgBuffer4 -> map() );

    for(int r = 0; r < height; r++){
        for(int c = 0; c < width; c++){
            int N = sizeScale * sizeScale;
            for(int ch = 0; ch < 3; ch++){ 
                float sum1=0, sum2=0, sum3=0, sum4=0;

                for(int sr = 0; sr < sizeScale; sr++){
                    for(int sc = 0; sc < sizeScale; sc++){
                        int C = c * sizeScale + sc;
                        int R = r * sizeScale + sr;
                        int Index = 3 * (R * width * sizeScale + C) + ch;

                        sum1 += imgDataBuffer1[Index];
                        sum2 += imgDataBuffer2[Index];
                        sum3 += imgDataBuffer3[Index];
                        sum4 += imgDataBuffer4[Index];
                    }
                }
                imgData[12*(r*width + c) + ch + offset1] = sum1 / N;
                imgData[12*(r*width + c) + ch + offset2] = sum2 / N;
                imgData[12*(r*width + c) + ch + offset3] = sum3 / N;
                imgData[12*(r*width + c) + ch + offset4] = sum4 / N;
            }
        }
    }
    imgBuffer1 -> unmap();
    imgBuffer2 -> unmap();
    imgBuffer3 -> unmap();
    imgBuffer4 -> unmap();
}

void independentSampling(
        Context& context, 
        int width, int height, 
        float* imgData, 
        int sampleNum, 
        unsigned sizeScale, 
        int mode)
{
    unsigned bWidth = sizeScale * width;
    unsigned bHeight = sizeScale * height;

    srand(time(NULL) );
    context["initSeed"] -> setUint( rand() );

    context -> launch(0, bWidth, bHeight);
    if(mode != 7){
        getOutputBuffer(context, imgData, width, height, sizeScale ); 
    }
    else{
        getTwoBounceOutputBuffer(context, imgData, width, height, sizeScale ); 
    }
}

float RMSEAfterScaling(const float* im1, const float* im2, int width, int height, float scale){
    
    int length = width * height * 3;
    float* temp1 = new float[length ];
    float* temp2 = new float[length ];
    for(int i = 0; i < length; i++){
        temp1[i] = im1[i];
        temp2[i] = im2[i];
    }

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
        float noiseLimit,
        bool noiseLimitEnabled,
        int maxIteration, 
        float noiseThreshold,
        unsigned sizeScale
        )
{
    unsigned bWidth = sizeScale * width;
    unsigned bHeight = sizeScale * height;

    unsigned sqrt_num_samples = (unsigned )(sqrt(float(sampleNum ) / float(sizeScale * sizeScale) ) + 1.0);
    if(sqrt_num_samples == 0)
        sqrt_num_samples = 1;
    context["sqrt_num_samples"] -> setUint(sqrt_num_samples );

    int pixelNum = width * height * 3;

    float* tempBuffer = new float[pixelNum ];
    float* tempBuffer2 = new float[pixelNum ];
    
    // Render the first image 
    srand(time(NULL) );
    context["initSeed"] -> setUint( unsigned(0.5 * rand() ) );
    context -> launch(0, bWidth, bHeight );
    getOutputBuffer(context, tempBuffer, width, height, sizeScale );

    // Compute the scale so that the mean of image will be 0.5
    float sum = 0;
    for(int i = 0; i < pixelNum; i++){
        sum += tempBuffer[i];
    }

    float scale = 0.5 * pixelNum / fmaxf(sum, 1e-6); 
    std::cout<<"Scale: "<<scale<<std::endl;

    // Render the second image
    srand(time(NULL) );
    context["initSeed"] -> setUint(rand() );
    context -> launch(0, bWidth, bHeight );
    getOutputBuffer(context, tempBuffer2, width, height, sizeScale );

    // Repeated render the image until the noise below the threshold
    int expo = 1;
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
 
        if(error < noiseThreshold || expo >= maxIteration){
            break;
        }
        expo += 1;
 
        // Update sampling parameters
        sampleNum = sampleNum * 2;
        sqrt_num_samples = (unsigned )(sqrt(float(sampleNum ) / float(sizeScale * sizeScale) ) + 1.0);
        srand(time(NULL) * expo );
        context["initSeed"] -> setUint(rand() );
        context["sqrt_num_samples"] -> setUint(sqrt_num_samples );
 
        // Rendering
        context -> launch(0, bWidth, bHeight);
        getOutputBuffer(context, tempBuffer2, width, height, sizeScale );
    }
    for(int i = 0; i < pixelNum; i++)
        imgData[i] = tempBuffer[i];

    delete [] tempBuffer;
    delete [] tempBuffer2;
    sampleNum = sampleNum * 2;

    return false;
}
