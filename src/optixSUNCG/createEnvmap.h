#ifndef CREATEENVMAP_HEADER
#define CREATEENVMAP_HEADER

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_aabb_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>
#include <string>
#include <opencv2/opencv.hpp>
#include "envmap.h"
#include "createTextureSampler.h"

using namespace optix;

const float PI = 3.1415926f;

cv::Mat loadEnvmap(std::string fileName, unsigned width = 1024, unsigned height = 512){
    HDRLoader hdr(fileName);
    if( hdr.failed() ){
        std::cout<<"Wrong: fail to load hdr image."<<std::endl;
        exit(0);
    }
    else{
        int envWidth = hdr.width();
        int envHeight = hdr.height();

        // Resize the  image
        cv::Mat envMat(envHeight, envWidth, CV_32FC3);
        for(int r = 0; r < envHeight; r++){
            for(int c = 0; c < envWidth; c++){
                int hdrInd = 4 *( r*envWidth + c );
                for(int ch = 0; ch < 3; ch++){
                    float color = hdr.raster()[hdrInd + ch];
                    envMat.at<cv::Vec3f>(r, envWidth -1 - c)[ch] = color;
                } 
            }
        }
        cv::Mat envMatNew(height, width, CV_32FC3);
        cv::resize(envMat, envMatNew, cv::Size(width, height), cv::INTER_AREA); 
        return envMatNew;
    }
}


void createEnvmapBuffer(Context& context, 
        unsigned width = 1024, unsigned height = 512, 
        unsigned gridWidth = 128, unsigned gridHeight = 64, 
        float* envmap = NULL)
{
    // Create tex sampler and populate with default values
    TextureSampler sampler = createTextureSampler(context );
    
    optix::Buffer  envBuffer = context->createBuffer(RT_BUFFER_INPUT, 
            RT_FORMAT_FLOAT4, width, height);
    if(envmap != NULL){
        float* envPt = reinterpret_cast<float*>(envBuffer->map() );
        for(int r = 0; r < height; r++){
            for(int c = 0; c < width; c++){
                int ind =  (r*width + c);
                for(int ch = 0; ch < 3; ch++){
                    envPt[4 * ind + ch] = envmap[3*ind + ch];
                }
                envPt[4*ind + 3] = 1.0;
            }
        }
        envBuffer -> unmap(); 
    }
    sampler -> setBuffer(0u, 0u, envBuffer);
    context["envmap"] -> setTextureSampler(sampler);
  
    Buffer envcdfV = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT, 1, gridHeight);
    Buffer envcdfH = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT, gridWidth, gridHeight);
    Buffer envpdf = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT, gridWidth, gridHeight);
    context["envcdfV"] -> set(envcdfV);
    context["envcdfH"] -> set(envcdfH);
    context["envpdf"] -> set(envpdf);
}


void createEnvmap(
        Context& context,
        std::vector<Envmap>& envmaps, 
        unsigned width = 1024, unsigned height = 512, 
        unsigned gridWidth=128, unsigned gridHeight = 64
        )
{
    if(envmaps.size() == 0){
        context["isEnvmap"] -> setInt(0);
        // Create the texture sampler 
        createEnvmapBuffer(context, 1, 1, 1, 1, NULL);
    }
    else{
        // Load the environmental map
        Envmap env = envmaps[0];
        cv::Mat envMat = loadEnvmap(env.fileName, width, height);
        context["isEnvmap"] -> setInt(1); 
        
        // Create the texture sampler
        float* envPt = new float[width * height * 3];
        for(int r = 0; r < height; r++){
            for(int c = 0; c < width; c++){
                for(int ch = 0; ch < 3; ch++){
                    int ind = r*width + c;
                    envPt[3*ind + ch] = envMat.at<cv::Vec3f>(height -1 -r, c)[ch] * env.scale;
                }
            }
        }
        createEnvmapBuffer(context, width, height, gridWidth, gridHeight, envPt);
        delete [] envPt;
        
        
        // Compute the weight map
        float* envWeight = new float[gridWidth * gridHeight];
        int winH = int(height / gridHeight);
        int winW = int(width  / gridWidth);
        for(int hid = 0; hid < gridHeight; hid++){
            float hidf = float(hid + 0.5) / float(gridHeight);
            float theta = hidf * PI;
            for(int wid = 0; wid < gridWidth; wid++){
                int hs = hid * winH;
                int he = (hid == gridHeight-1) ? height : (hid + 1) * winH;
                int ws = wid * winW;
                int we = (wid == gridWidth-1) ? width : (wid + 1) * winW;
                
                float N = (he - hs) * (we - ws);
                float W = 0;
                for(int i = hs; i < he; i++){
                    for(int j = ws; j < we; j++){
                        for(int ch = 0; ch < 3; ch++)
                            W += envMat.at<cv::Vec3f>(height - 1 - i, j)[0] / 3.0;
                    }
                }
                envWeight[hid*gridWidth + wid] = W / N * sinf(theta);
            }
        }


        // Compute the cdf of envmap 
        optix::Buffer envcdfV = context["envcdfV"] -> getBuffer();
        float* envcdfV_p = reinterpret_cast<float*>(envcdfV -> map() );
        optix::Buffer envcdfH = context["envcdfH"] -> getBuffer();
        float* envcdfH_p = reinterpret_cast<float*>(envcdfH -> map() );  
        for(int vId = 0; vId < gridHeight; vId++){
            int offset = vId * gridWidth;
            envcdfH_p[offset] = envWeight[offset];
            for(int uId = 1; uId < gridWidth; uId++){
                envcdfH_p[offset + uId] = 
                envcdfH_p[offset + uId-1] + envWeight[offset + uId];
            }
            float rowSum = envcdfH_p[offset + gridWidth-1];
            if(vId == 0)
                envcdfV_p[vId] = rowSum;
            else
                envcdfV_p[vId] = envcdfV_p[vId-1] + rowSum;
            for(int uId = 0; uId < gridWidth; uId++){
                float S = (rowSum > 1e-6) ? rowSum : 1e-6;
                envcdfH_p[offset + uId] /= S;
            }
        }
        float colSum = envcdfV_p[gridHeight-1];
        for(int vId = 0; vId < gridHeight; vId++){
            float S = (colSum > 1e-6) ? colSum : 1e-6;
            envcdfV_p[vId] /= colSum;
        }

        // Compute the pdf for sampling
        int gridSize = gridHeight * gridWidth;
        for(int vId = 0; vId < gridHeight; vId++){
            float vIdf = float(vId + 0.5) / float(gridHeight);
            float sinTheta = sinf(vIdf * PI);
            for(int uId = 0; uId < gridWidth; uId++){
                envWeight[vId * gridWidth+ uId] /= 
                    ((colSum * 2 * PI * PI * sinTheta) / gridSize );
            }
        } 

        optix::Buffer envpdf = context["envpdf"] -> getBuffer(); 
        float* envpdf_p = static_cast<float*> (envpdf -> map() );
        for(unsigned int i = 0; i < gridHeight; i++){
            for(unsigned int j = 0; j < gridWidth; j++){            
                int envId = i * gridWidth + j;
                envpdf_p[envId] = envWeight[envId];
            }
        }
 
        envpdf -> unmap();
        envcdfV -> unmap();
        envcdfH -> unmap();

        delete [] envWeight;
    }
}

#endif
