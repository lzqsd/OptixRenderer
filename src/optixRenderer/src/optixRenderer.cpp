/* 
 * Copyright (c) 2016, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//-----------------------------------------------------------------------------
//
// optixVox: a sample that renders a subset of the VOX file format from MagicaVoxel @ ephtracy.
// Demonstrates non-triangle geometry, and naive random path tracing.
//
//-----------------------------------------------------------------------------

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_aabb_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>
#include <time.h>
#include <limits.h>

#include <opencv2/opencv.hpp>

#include <time.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <stdint.h>
#include <cmath>
#include <map>


#include "inout/readXML.h"
#include "structs/cameraInput.h"
#include "lightStructs.h"
#include "inout/rgbe.h"
#include "inout/relativePath.h"
#include "Camera.h"
#include "sutil.h"
#include "shapeStructs.h"

#include "creator/createAreaLight.h"
#include "creator/createCamera.h"
#include "creator/createContext.h"
#include "creator/createEnvmap.h"
#include "creator/createGeometry.h"
#include "creator/createMaterial.h"
#include "creator/createPointFlashLight.h"
#include "sampler/sampler.h" 
#include "postprocessing/filter.h"
#include "stdio.h"

using namespace optix;

long unsigned vertexCount(const std::vector<shape_t>& shapes)
{
    long unsigned vertexSum = 0;
    for(int i = 0; i < shapes.size(); i++){
        shape_t shape = shapes[i];
        int vertexNum = shape.mesh.positions.size() / 3;
        vertexSum += vertexNum;
    }
    return vertexSum;
}

void boundingBox(
        Context& context,
        const std::vector<shape_t>& shapes
        )
{
    float3 vmin, vmax;
    for(int i = 0; i < shapes.size(); i++){
        shape_t shape = shapes[i];
        int vertexNum = shape.mesh.positions.size() / 3;
        for(int j = 0; j < vertexNum; j++){
            float vx = shape.mesh.positions[3*j];
            float vy = shape.mesh.positions[3*j+1];
            float vz = shape.mesh.positions[3*j+2];
            float3 v = make_float3(vx, vy, vz);
            if(j == 0 && i == 0){
                vmin = v;
                vmax = v;
            }
            else{
                vmin = fminf(vmin, v);
                vmax = fmaxf(vmax, v);
            }
        }
    }
    float infiniteFar = length(vmax - vmin) * 10;
    std::cout<<"The length of diagonal of bouncding box: "<<(infiniteFar / 10) <<std::endl;
    printf("Max X: %.3f Max Y: %.3f Max Z: %.3f \n", vmax.x, vmax.y, vmax.z);
    printf("Min X: %.3f Min Y: %.3f Min Z: %.3f \n", vmin.x, vmin.y, vmin.z);
    context["infiniteFar"] -> setFloat(infiniteFar);
    context["scene_epsilon"]->setFloat(infiniteFar / 1e6);
}

float clip(float a, float min, float max){
    a = (a < min) ? min : a;
    a = (a > max) ? max : a;
    return a;
}


bool writeBufferToFile(const char* fileName, float* imgData, int width, int height, bool isHdr = false, int mode = 0)
{   
    if(mode == 1 || mode == 2 || mode == 3 || mode == 4 || mode  == 6)
        isHdr = false;
    
    if(mode == 5){
        std::ofstream depthOut(fileName, std::ios::out|std::ios::binary);
        depthOut.write((char*)&height, sizeof(int) );
        depthOut.write((char*)&width, sizeof(int) );

        float* image = new float[width * height];
        for(int i = 0; i < height; i++){
            for(int j = 0; j < width; j++){
                image[i*width + j] = imgData[3 * ( (height-1-i) * width +j ) ];
            }
        }
        depthOut.write( (char*)image, sizeof(float) * width * height);
        depthOut.close();
        delete [] image;

        return true;
    }

    if(isHdr){
        FILE* imgOut = fopen(fileName, "w");
        if(imgOut == NULL){
            std::cout<<"Wrong: can not open the output file!"<<std::endl;
            return false;
        }
        float* image = new float[width * height * 3];
        for(int i = 0; i < height; i++){
            for(int j = 0; j < width; j++){
                for(int ch = 0; ch < 3; ch ++){
                    image[3*(i * width + j) + ch] = imgData[3*( (height-1-i) * width + j) + ch];
                }
            }
        }
        RGBE_WriteHeader(imgOut, width, height, NULL);
        RGBE_WritePixels(imgOut, image, width * height);
        fclose(imgOut);
        delete [] image;
    }
    else{
        cv::Mat image(height, width, CV_8UC3);
        for(int i = 0; i < height; i++){
            for(int j = 0; j < width; j++){
                int ind = 3 * ( (height - 1 -i) * width + j);
                float r = imgData[ind];
                float g = imgData[ind + 1]; 
                float b = imgData[ind + 2];
                if(mode == 0 || mode == 1){
                    r = pow(r, 1.0f/2.2f);
                    g = pow(g, 1.0f/2.2f);
                    b = pow(b, 1.0f/2.2f);
                }
                r = clip(r, 0.0f, 1.0f);
                g = clip(g, 0.0f, 1.0f);
                b = clip(b, 0.0f, 1.0f);
                image.at<cv::Vec3b>(i, j)[0] = (unsigned char)(b * 255);
                image.at<cv::Vec3b>(i, j)[1] = (unsigned char)(g * 255);
                image.at<cv::Vec3b>(i, j)[2] = (unsigned char)(r * 255);
            }
        } 
        cv::imwrite(fileName, image);
    }

    return true;
}


std::string generateOutputFilename(std::string fileName, int mode, bool isHdr, int i,  int camNum){
    std::string root;
    std::string suffix;
    std::size_t pos = fileName.find_last_of(".");
    if(pos == std::string::npos ){
        root = fileName;
        suffix = std::string("");
    }
    else{
        root = fileName.substr(0, pos);
        suffix = fileName.substr(pos+1, fileName.length() );
    }
    
    if(mode == 0){
        if(isHdr){
            if(suffix != std::string("rgbe") ){
                std::cout<<"Warning: we only support rgbe image for hdr"<<std::endl;
            }
            suffix = std::string("rgbe");
        }
        else{
            if(suffix != std::string("bmp") && suffix != std::string("png") 
                    && suffix != std::string("jpg") && suffix != std::string("jpeg") )
            {
                std::cout<<"Warning: only support jpg, bmp, png file format"<<std::endl;
                suffix = std::string("png");
            }
        }
    }
    else if(mode == 1 || mode == 2 || mode == 3 || mode == 4 || mode == 6){
        suffix = std::string("png");
    }
    else if(mode == 5){
        suffix = std::string("dat");
    }

    std::string outputFileName;
    std::string modeString = "";
    switch(mode){
        case 1: modeString = "baseColor"; break;
        case 2: modeString = "normal"; break;
        case 3: modeString = "roughness"; break;
        case 4: modeString = "mask"; break;
        case 5: modeString = "depth"; break;
        case 6: modeString = "metallic"; break;
    }

    if(camNum > 0){
        char camId[10];
        std::sprintf(camId, "%d", i+1 );
        outputFileName = root + modeString + "_" + camId + "." + suffix;
    }
    else{
        outputFileName = root + modeString + "." + suffix;
    }
    return outputFileName;
}

bool loadCamFile(std::string fileName, int& camNum, std::vector<float>& originArr, std::vector<float>& targetArr, std::vector<float>& upArr)
{    
    std::ifstream camIn(fileName.c_str(), std::ios::in);
    if(!camIn ){
        std::cout<<"Wrong: can not load the camera file "<<fileName<<" !"<<std::endl;
        return false;
    }
    camIn >> camNum; 
    for(int i = 0; i < camNum; i++){
        std::vector<float> origin(3, 0.0f);
        std::vector<float> target(3, 0.0f);
        std::vector<float> up(3, 0.0f);
        camIn>>origin[0]>>origin[1]>>origin[2];
        camIn>>target[0]>>target[1]>>target[2];
        camIn>>up[0]>>up[1]>>up[2];

        for(int j = 0; j < 3; j++){
            originArr.push_back(origin[j] );
            targetArr.push_back(target[j] );
            upArr.push_back(up[j] );
        }
    }
    return true;
}



int main( int argc, char** argv )
{
    bool use_pbo  = false;

    std::string fileName;
    std::string outputFileName;
    std::string cameraFile("");
    int mode = 0; 
    int maxIteration = -1;
    std::vector<int> gpuIds;
    float noiseLimit = 0.11;
    int vertexLimit = 150000;
    float intensityLimit = 0.05;
    bool noiseLimitEnabled = false;
    bool vertexLimitEnabled = false;
    bool intensityLimitEnabled = false;
    int maxPathLength = 7;
    int rrBeginLength = 5;
    
    bool isForceOutput = false;
    bool isMedianFilter = false;

    int camStart = 0;
    int camEnd = -1;

    Context context = 0;

    for(int i = 0; i < argc; i++){
        if(i == 0){
            continue;
        }
        else if(std::string(argv[i]) == std::string("-f") ){
            if(i == argc - 1){
                std::cout<<"Missing input variable"<<std::endl;
                exit(1);
            }
            fileName = std::string(argv[++i] ); 
        }
        else if(std::string(argv[i]) == std::string("-o") ){
            if(i == argc - 1){
                std::cout<<"Missing input variable"<<std::endl;
                exit(1);
            }
            outputFileName = std::string(argv[++i] );
        }
        else if(std::string(argv[i]) == std::string("-m") ) {
            if(i == argc - 1){
                std::cout<<"Missing  input valriable"<<std::endl;
                exit(1);
            }
            mode = atoi(argv[++i] );
        }
        else if(std::string(argv[i] ) == std::string("-c") ){
            if(i == argc - 1){
                std::cout<<"Missing inut variable"<<std::endl;
                exit(1);
            }
            cameraFile = std::string(argv[++i] );
        }
        else if(std::string(argv[i] ) == std::string("--gpuIds") ){
            if(i == argc - 1){
                std::cout<<"Missing input variable"<<std::endl;
                exit(1);
            }
            while(i + 1 <= argc-1 && argv[i+1][0] != '-'){
                gpuIds.push_back(atoi(argv[++i] ) );
            }
        }
        else if(std::string(argv[i] ) == std::string("--noiseLimit") ){
            if(i == argc - 1){
                std::cout<<"Missing  input variable"<<std::endl;
                exit(1);
            }
            std::vector<float> flArr = parseFloatStr(std::string(argv[++i] ) );
            noiseLimit = flArr[0];
            noiseLimitEnabled = true;
            std::cout<<"Warning: noiseLimit "<<noiseLimit<<"is enabled!"<<std::endl;
        }
        else if(std::string(argv[i] ) == std::string("--vertexLimit") ){
            if(i == argc - 1){
                std::cout<<"Missing input variable"<<std::endl;
                exit(1);
            }
            vertexLimit = atoi(argv[++i] );
            vertexLimitEnabled = true; 
            std::cout<<"Warning: vertexLimit "<<vertexLimit<<"is enabled!"<<std::endl;
        }
        else if(std::string(argv[i] ) == std::string("--intensityLimit") ){
            if(i == argc - 1){
                std::cout<<"Missing input variable"<<std::endl;
                exit(1);
            }
            std::vector<float> flArr = parseFloatStr(std::string(argv[++i] ) );
            intensityLimit = flArr[0];
            intensityLimitEnabled = true;
        }
        else if(std::string(argv[i] ) == std::string("--camStart") ){
            if(i == argc - 1){
                std::cout<<"Missing input variable"<<std::endl;
                exit(1);
            }
            camStart = atoi(argv[++i] );
        }
        else if(std::string(argv[i] ) == std::string("--camEnd") ){
            if(i == argc - 1){
                std::cout<<"Missing input variable"<<std::endl;
                exit(1);
            }
            camEnd = atoi(argv[++i] );
        }
        else if(std::string(argv[i] ) == std::string("--forceOutput") ){
            isForceOutput = true;   
        }
        else if(std::string(argv[i] ) == std::string("--rrBeginLength") ){
            if(i == argc - 1){
                std::cout<<"Missing input variable"<<std::endl;
                exit(1);
            }
            rrBeginLength = atoi(argv[++i] );
        }
        else if(std::string(argv[i] ) == std::string("--maxPathLength") ){
            if(i == argc-1){
                std::cout<<"Missing input variable"<<std::endl;
                exit(1);
            }
            maxPathLength = atoi(argv[++i] );
        } 
        else if(std::string(argv[i] ) == std::string("--maxIteration") ){
            if(i == argc-1){
                std::cout<<"Missing input variable"<<std::endl;
                exit(1);
            }
            maxIteration = atoi(argv[++i] );
        } 
        else if(std::string(argv[i] ) == std::string("--medianFilter") ){ 
            isMedianFilter = true;
        }
        else{
            std::cout<<"Unrecognizable input command"<<std::endl;
            exit(1);
        }
    }
    
    std::vector<shape_t> shapes;
    std::vector<material_t> materials;
    CameraInput cameraInput;
    std::vector<Envmap> envmaps;
    std::vector<Point> points;

    char fileNameNew[PATH_MAX+1 ];
    char* isRealPath = realpath(fileName.c_str(), fileNameNew );
    if(isRealPath == NULL){
        std::cout<<"Wrong: Fail to transform the realpath of XML to true path."<<std::endl;
        return false;
    }
    fileName = std::string(fileNameNew );
    
    outputFileName = relativePath(fileName, outputFileName );

    std::cout<<"Input file name: "<<fileName<<std::endl;
    std::cout<<"Output file name: "<<outputFileName<<std::endl;

    bool isXml = readXML(fileName, shapes, materials, cameraInput, envmaps, points);
    if(!isXml ) return false;

    long unsigned vertexNum = vertexCount(shapes );

    std::cout<<"Material num: "<<materials.size() << std::endl;
    std::cout<<"Shape num: "<<shapes.size() <<std::endl;
    std::cout<<"Vertex num: "<<vertexNum <<std::endl;
    if(vertexLimitEnabled && vertexNum > vertexLimit){
        std::cout<<"Warning: the model is too huge, will be skipped!"<<std::endl;
        return 0;
    }

    std::cout<<"Camera Parameters: "<<std::endl;
    std::cout<<"Camera Field of View: "<<cameraInput.fov<<std::endl;
    std::cout<<"Width: "<<cameraInput.width<<std::endl;
    std::cout<<"Height: "<<cameraInput.height<<std::endl;
    std::cout<<"FovAxis: ";
    if(cameraInput.isAxisX == true)
        std::cout<<"x"<<std::endl;
    else
        std::cout<<"y"<<std::endl;
    std::cout<<"Target: "<<cameraInput.target[0]<<' '<<cameraInput.target[1]<<' '<<cameraInput.target[2]<<std::endl;
    std::cout<<"Origin: "<<cameraInput.origin[0]<<' '<<cameraInput.origin[1]<<' '<<cameraInput.origin[2]<<std::endl;
    std::cout<<"Up: "<<cameraInput.up[0]<<' '<<cameraInput.up[1]<<' '<<cameraInput.up[2]<<std::endl; 
    std::cout<<"Sample Count: "<<cameraInput.sampleNum<<std::endl;
    
    std::cout<<"Sampler: "<<cameraInput.sampleType<<std::endl;
    std::cout<<"Sampler Count: "<<cameraInput.sampleNum <<std::endl;

    if(envmaps.size() == 0){
        std::cout<<"No Environmental maps."<<std::endl;
    }
    else{
        std::cout<<"Environmental map: "<<envmaps[0].fileName<<std::endl;
        std::cout<<"Environmental scale: "<<envmaps[0].scale<<std::endl;
    }

    if(points.size() == 0){
        std::cout<<"No point light source."<<std::endl;
    }
    else{
        std::cout<<"Point Light num: "<<points.size()<<std::endl;
        std::cout<<"Light intensity: ";
        std::cout<<points[0].intensity.x<<' '<<points[0].intensity.y<<' '<<points[0].intensity.z<<std::endl;
        std::cout<<"Light Position: ";
        std::cout<<points[0].position.x<<' '<<points[0].position.y<<' '<<points[0].position.z<<std::endl;
    }

    // Camera File
    // The begining of the file should be the number of camera we are going to HAVE
    // Then we just load the camera view one by one, in the order of origin, target and up
    int camNum = 1;
    std::vector<float> originArr;
    std::vector<float> targetArr;
    std::vector<float> upArr;
    if(cameraFile != std::string("") ){
        cameraFile = relativePath(fileName, cameraFile );
        bool isLoad = loadCamFile(cameraFile, camNum, originArr, targetArr, upArr);
        if (!isLoad ) return false;
    }
    else{
        camNum = 1;
        for(int i = 0; i < 3; i++){
            originArr.push_back(cameraInput.origin[i]);
            targetArr.push_back(cameraInput.target[i]);
            upArr.push_back(cameraInput.up[i]);
        }
    }

    unsigned scale = createContext(context, use_pbo, cameraInput.cameraType, 
            cameraInput.width, 
            cameraInput.height, 
            mode, 
            cameraInput.sampleNum, 
            maxPathLength, 
            rrBeginLength );

    if(gpuIds.size() != 0){
        std::cout<<"GPU Num: "<<gpuIds.size()<<std::endl;
        context -> setDevices(gpuIds.begin(), gpuIds.end() );
    }
    boundingBox(context, shapes);
    createGeometry(context, shapes, materials, mode);
    createAreaLightsBuffer(context, shapes);
    createEnvmap(context, envmaps); 
    createPointLight(context, points);
    
    std::cout<<std::endl;
    
    float* imgData = new float[cameraInput.width * cameraInput.height * 3];
    unsigned camSp, camEp;
    camSp = std::max(0, camStart);
    if(camEnd == -1){
        camEp = camNum;
    }
    else{
        camEp = std::max(std::min(camEnd, camNum), 0);
    }
    
    for(int i = camSp; i < camEp; i++){ 
        std::string outputFileNameNew = generateOutputFilename(outputFileName, mode,
                cameraInput.isHdr, i, camNum);

        std::ifstream f(outputFileNameNew.c_str() );
        if(f.good() && !isForceOutput ) {
            std::cout<<"Warning: "<<outputFileNameNew<<" already exists. Will be skipped."<<std::endl;
            continue;
        }
        else{
            std::cout<<"Output Image: "<<outputFileNameNew<<std::endl;
        }

        for(int j = 0; j < 3; j++){
            cameraInput.origin[j] = originArr[i * 3 + j];
            cameraInput.target[j] = targetArr[i * 3 + j];
            cameraInput.up[j] = upArr[i * 3 + j];
        }
        createCamera(context, cameraInput);
        bool isFindFlash = false;
        for(int j = 0; j < points.size(); j++){
            if(points[j].isFlash == true){
                isFindFlash = true;
                points[j].position.x = cameraInput.origin[0];
                points[j].position.y = cameraInput.origin[1];
                points[j].position.z = cameraInput.origin[2];
            }
        }
        if(isFindFlash ){
            updatePointLight(context, points);            
        }

        context->validate();
        clock_t t;
        t = clock();
        
        if(intensityLimitEnabled == true && mode == 0){
            independentSampling(context, cameraInput.width, cameraInput.height, imgData, 4);
            float meanIntensity = 0;
            int pixelNum = cameraInput.width * cameraInput.height * 3;
            for(int i = 0; i < pixelNum; i++){
                meanIntensity += imgData[i];
            }
            meanIntensity /= pixelNum;
            std::cout<<"Mean Intensity of image: "<<meanIntensity<<std::endl;
            if(meanIntensity < 0.05){
                std::cout<<"Warning: the image is too dark, will be skipped"<<std::endl;
                continue;
            }
        }

        std::cout<<"Start to render: "<<i+1<<"/"<<camEp<<std::endl;
        if(cameraInput.sampleType == std::string("independent") || mode != 0){
            int sampleNum = cameraInput.sampleNum;
            independentSampling(context, cameraInput.width, cameraInput.height, imgData, sampleNum, scale);
        }
        else if(cameraInput.sampleType == std::string("adaptive") ) {
            int sampleNum = cameraInput.sampleNum; 
            
            if(maxIteration >= 1){
                cameraInput.adaptiveSampler.maxIteration =  maxIteration; 
            }

            bool isTooNoisy = adaptiveSampling(context, cameraInput.width, cameraInput.height, sampleNum, imgData, 
                    noiseLimit, noiseLimitEnabled, 
                    cameraInput.adaptiveSampler.maxIteration, 
                    cameraInput.adaptiveSampler.noiseThreshold, 
                    scale);
            std::cout<<"Sample Num: "<<sampleNum<<std::endl;
            if(isTooNoisy){
                std::cout<<"This image will not be output!"<<std::endl;
                continue;
            }
        }
        t = clock() - t;
        std::cout<<"Time: "<<float(t) / CLOCKS_PER_SEC<<'s'<<std::endl;
        
        if(isMedianFilter && mode == 0 ){
            medianFilter(imgData, cameraInput.width, cameraInput.height, 1);
        }
         
        bool isWrite = writeBufferToFile(outputFileNameNew.c_str(), 
                imgData,
                cameraInput.width, cameraInput.height, 
                cameraInput.isHdr,
                mode);
    }
    destroyContext(context );
    delete [] imgData;

    return 0;
}
