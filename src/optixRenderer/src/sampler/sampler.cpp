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

void getLightOutputBuffer(Context& context, float* imgData, int width, int height, unsigned sizeScale)
{
    Buffer imgBuffer1, imgBuffer2, imgBuffer3; 
    float *imgDataBuffer1 = NULL, *imgDataBuffer2 = NULL;
    float *imgDataBuffer3 = NULL;
    int offset1 = 0, offset2 = 3, offset3 = 6; 

    imgBuffer1 = context[ "position_buffer" ]->getBuffer();
    imgDataBuffer1 = reinterpret_cast<float*>(imgBuffer1 -> map() );
    imgBuffer2 = context[ "direction_buffer" ]->getBuffer();
    imgDataBuffer2 = reinterpret_cast<float*>(imgBuffer2 -> map() );
    imgBuffer3 = context[ "intensity_buffer" ]->getBuffer();
    imgDataBuffer3 = reinterpret_cast<float*>(imgBuffer3 -> map() );

    float *sum1 = NULL, *sum2 = NULL, *sum3 = NULL;
    float *d1 = NULL, *d2 = NULL, *d3 = NULL;
    sum1 = new float[3];
    sum2 = new float[3];
    sum3 = new float[3];
    d1 = new float[3];


    for(int r = 0; r < height; r++){
        for(int c = 0; c < width; c++){

            int count1 = 0;
            int N = sizeScale * sizeScale;
            for(int ch = 0; ch < 3; ch++ ){
                sum1[ch] = sum2[ch] = sum3[ch] = 0;
            }

            for(int sr = 0; sr < sizeScale; sr++){
                for(int sc = 0; sc < sizeScale; sc++){
                    int C = c * sizeScale + sc;
                    int R = r * sizeScale + sr;
                    int Index = 3 * (R * width * sizeScale + C);
                    for(int ch = 0; ch < 3; ch++){
                        d1[ch] = imgDataBuffer1[Index + ch];
                        sum2[ch] += imgDataBuffer2[Index + ch];
                        sum3[ch] += imgDataBuffer3[Index + ch];
                    }
                    if(fabs(d1[0]) > 0 || fabs(d1[1]) > 0 || fabs(d1[2]) > 0){
                        count1 += 0;
                        for(int ch = 0; ch < 3; ch++){
                            sum1[ch] += d1[ch];
                        }
                    }
                }
            }

            for(int ch = 0; ch < 3; ch++){
                imgData[9*(r*width + c) + ch + offset1] = sum1[ch] / count1;
                imgData[9*(r*width + c) + ch + offset2] = sum2[ch] / N;
                imgData[9*(r*width + c) + ch + offset3] = sum3[ch] / N;
            }
        }
    }
    imgBuffer1 -> unmap();
    imgBuffer2 -> unmap();
    imgBuffer3 -> unmap();

    delete [] sum1;
    delete [] sum2;
    delete [] sum3;
    delete [] d1;
}

void independentSampling(
        Context& context, 
        int width, int height, 
        float* imgData, 
        int sampleNum, 
        unsigned sizeScale, 
        int mode )
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
        getLightOutputBuffer(context, imgData, width, height, sizeScale ); 
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
