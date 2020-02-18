#include "sampler/sampler.h"
#include <cmath>
#include <iostream>

void getLightOutputBuffer(Context& context, 
        float* imgData,
        int width, int height, 
        int envWidth, int envHeight, 
        unsigned sampleNum )
{
    Buffer imgBuffer;
    float* imgDataBuffer = NULL;

    imgBuffer = context[ "intensity_buffer" ]->getBuffer();
    imgDataBuffer = reinterpret_cast<float*>(imgBuffer -> map() ); 

    double pi = 3.1415926535;
    for(int r = 0; r < height; r++){
        for(int c = 0; c < width; c++){ 
            for(int er = 0; er < envHeight; er++){
                float sin_theta = sin( pi / 2.0 * (er + 0.5) / envHeight);
                float weight = sin_theta * pi * pi / envHeight / envWidth;

                for(int ec = 0; ec < envWidth; ec++){
                    int br = (height-1-r) * envHeight + er;
                    int bc = c * envWidth + ec; 

                    int bInd = 3 * (br * envWidth * width + bc );

                    int ind = (r * width + c) * 3 * envWidth * envHeight + 
                        3 * (er * envWidth + ec );
                    
                    for(int ch = 0; ch < 3; ch++){  
                        imgData[ind + ch] = imgDataBuffer[bInd + ch] / sampleNum / weight; 
                        if(isnan(imgData[ind + ch] ) )
                            std::cout<<"Wrong: nan in the image"<<std::endl;
                        if(isnan(imgDataBuffer[bInd + ch] ) )
                            std::cout<<"Wrong: nan in the buffer"<<std::endl;
                        imgDataBuffer[bInd + ch ] = 0;
                    } 
                }
            }
        }
    }
    imgBuffer -> unmap();
}

void independentSampling(
        Context& context, 
        int width, int height,  
        int envWidth, int envHeight, 
        float* imgData,
        int sampleNum )
{
    srand(time(NULL) );
    context["initSeed"] -> setUint( rand() ); 
    context -> launch(0, width, height);
    getLightOutputBuffer(context, imgData, width, height, envWidth, envHeight, sampleNum ); 
}
