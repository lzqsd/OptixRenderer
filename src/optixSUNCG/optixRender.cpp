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
#include "sutil/HDRLoader.h"
#include <limits.h>

#include <sutil.h>
#include "commonStructs.h"
#include <Camera.h>
#include <SunSky.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw_gl2.h>
#include <opencv2/opencv.hpp>

#include <time.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <stdint.h>
#include "tinyxml.h"
#include "readXML.h"
#include "CameraInput.h"
#include "areaLight.h"
#include "envmap.h"
#include "rgbe.h"
#include "relativePath.h"
#include <cmath>
#include <map>

using namespace optix;

const char* const SAMPLE_NAME = "optixSUNCG";
const float PI = 3.1415926f;
const int faceLimit = 40000;

//------------------------------------------------------------------------------
//
// Globals
//
//------------------------------------------------------------------------------

Context      context = 0;

//------------------------------------------------------------------------------
//
//  Helper functions
//
//------------------------------------------------------------------------------
//
//
//



static std::string ptxPath( const std::string& cuda_file )
{
    return
        std::string(sutil::samplesPTXDir()) +
        "/" + std::string(SAMPLE_NAME) + "_generated_" +
        cuda_file +
        ".ptx";
}


static Buffer getOutputBuffer(Context& context)
{
    return context[ "output_buffer" ]->getBuffer();
}


void destroyContext()
{
    if( context ){
        context->destroy();
        context = 0;
    }
}


void createContext( bool use_pbo, 
        std::string cameraType,
        unsigned width, unsigned height, 
        unsigned int mode = 0,
        unsigned int sampleNum = 400, 
        unsigned maxDepth = 5, 
        unsigned rr_begin_depth = 3)
{
    unsigned sqrt_num_samples = (unsigned )(sqrt(float(sampleNum ) ) + 1.0);
    if(sqrt_num_samples == 0)
        sqrt_num_samples = 1;

    // Set up context
    context = Context::create();
    context->setRayTypeCount( 2 );
    context->setEntryPointCount( 1 );
    context->setStackSize( 600 );

    if(mode > 0){
        maxDepth = 1;
        sqrt_num_samples = 4;
    }

    context["max_depth"]->setInt(maxDepth);
    context["sqrt_num_samples"] -> setUint(sqrt_num_samples);
    context["rr_begin_depth"] -> setUint(rr_begin_depth);
    
    Buffer outputBuffer = context -> createBuffer(RT_BUFFER_OUTPUT, RT_FORMAT_FLOAT3, width, height);
    context["output_buffer"]->set( outputBuffer );

    // Ray generation program 
    std::string ptx_path( ptxPath( "path_trace_camera.cu" ) );
    Program ray_gen_program = context->createProgramFromPTXFile( ptx_path, "pinhole_camera" );
    context->setRayGenerationProgram( 0, ray_gen_program );
    if(cameraType == std::string("perspective") ){
        context["cameraMode"] -> setInt(0);
    }
    else if(cameraType == std::string("envmap") ){
        context["cameraMode"] -> setInt(1);
    }
    else if(cameraType == std::string("hemisphere") ){
        context["cameraMode"] -> setInt(2);
    }
    else{ 
        std::cout<<"Wrong: unrecognizable camera type!"<<std::endl;
        exit(1);
    }


    // Exception program
    Program exception_program = context->createProgramFromPTXFile( ptx_path, "exception" );
    context->setExceptionProgram( 0, exception_program );
    context["bad_color"]->setFloat( 0.0f, 0.0f, 0.0f );

    // Missing Program 
    std::string miss_path( ptxPath("envmap.cu") );
    if(mode == 0){
        Program miss_program = context->createProgramFromPTXFile(miss_path, "envmap_miss");
        context->setMissProgram(0, miss_program);
    }
    else{
        Program miss_program = context->createProgramFromPTXFile(miss_path, "miss");
        context->setMissProgram(0, miss_program);
    }
}

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


void createEnvmapBuffer(Context context, 
        unsigned width = 1024, unsigned height = 512, 
        unsigned gridWidth = 128, unsigned gridHeight = 64, 
        float* envmap = NULL)
{
    // Create tex sampler and populate with default values
    optix::TextureSampler sampler = context->createTextureSampler();
    sampler->setWrapMode(0, RT_WRAP_REPEAT );
    sampler->setWrapMode(1, RT_WRAP_REPEAT );
    sampler->setWrapMode(2, RT_WRAP_REPEAT );
    sampler->setIndexingMode(RT_TEXTURE_INDEX_NORMALIZED_COORDINATES );
    sampler->setReadMode(RT_TEXTURE_READ_NORMALIZED_FLOAT );
    sampler->setMaxAnisotropy(1.0f );
    sampler->setMipLevelCount(1u );
    sampler->setArraySize(1u );
    
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
    sampler -> setFilteringModes(RT_FILTER_LINEAR, RT_FILTER_LINEAR, RT_FILTER_NONE);
    context["envmap"] -> setTextureSampler(sampler);
  
    Buffer envcdfV = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT, 1, gridHeight);
    Buffer envcdfH = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT, gridWidth, gridHeight);
    Buffer envpdf = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT, gridWidth, gridHeight);
    context["envcdfV"] -> set(envcdfV);
    context["envcdfH"] -> set(envcdfH);
    context["envpdf"] -> set(envpdf);
}


void createEnvmap(
        Context context,
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



int createAreaLightsBuffer(
        Context& context, 
        const std::vector<objLoader::shape_t> shapes
    )
{
    std::vector<float> cdfArr;
    std::vector<float> pdfArr;
    std::vector<areaLight> lights;
    float sum = 0;

    // Assign all the area lights
    for(int i = 0; i < shapes.size(); i++){
        if(shapes[i].isLight ){
            
            objLoader::shape_t shape = shapes[i];
            int faceNum = shape.mesh.indicesP.size() /3;
            float3 radiance = make_float3(shape.radiance[0], shape.radiance[1], shape.radiance[2]);
            std::vector<int>& faces = shape.mesh.indicesP;
            std::vector<float>& vertices = shape.mesh.positions;

            for(int i = 0; i < faceNum; i++){
                int vId1 = faces[3*i], vId2 = faces[3*i+1], vId3 = faces[3*i+2];
                float3 v1 = make_float3(vertices[3*vId1], vertices[3*vId1+1], vertices[3*vId1+2]);
                float3 v2 = make_float3(vertices[3*vId2], vertices[3*vId2+1], vertices[3*vId2+2]);
                float3 v3 = make_float3(vertices[3*vId3], vertices[3*vId3+1], vertices[3*vId3+2]);

                float3 cproduct = cross(v2 - v1, v3 - v1);
                float area = 0.5 * sqrt(dot(cproduct, cproduct) );

                sum += area * length(radiance );
                cdfArr.push_back(sum);
                pdfArr.push_back(area * length(radiance) );
                
                areaLight al;
                al.vertices[0] = v1;
                al.vertices[1] = v2;
                al.vertices[2] = v3;
                al.radiance = radiance;
                lights.push_back(al);
            }
        }
    }
    
    // Computs the pdf and the cdf
    for(int i = 0; i < cdfArr.size(); i++){
        cdfArr[i] = cdfArr[i] / sum;
        pdfArr[i] = pdfArr[i] / sum;
    }

    Buffer lightBuffer = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_USER, lights.size() );
    lightBuffer->setElementSize( sizeof( areaLight) );
    memcpy( lightBuffer->map(), (char*)&lights[0], sizeof(areaLight) * lights.size() );
    lightBuffer->unmap(); 
    context["areaLights"]->set(lightBuffer );
    
    context["areaTriangleNum"] -> setInt(lights.size() );

    Buffer cdfBuffer = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT, cdfArr.size() );
    memcpy(cdfBuffer -> map(), &cdfArr[0], sizeof(float) * cdfArr.size() );
    cdfBuffer -> unmap();
    
    Buffer pdfBuffer = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT, pdfArr.size() );
    memcpy(pdfBuffer -> map(), &pdfArr[0], sizeof(float) * pdfArr.size() );
    pdfBuffer -> unmap();
    
    context["areaLightCDF"] -> set(cdfBuffer );
    context["areaLightPDF"] -> set(pdfBuffer );
}

Material createAreaLight(Context& context, objLoader::shape_t shape){
    const std::string ptx_path = ptxPath( "areaLight.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );
    
    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
    
    material["radiance"] -> setFloat(make_float3(shape.radiance[0], 
                shape.radiance[1], shape.radiance[2]) );
    return material;
}

void createPointLight(Context& context, std::vector<Point>& points){ 
    if(points.size() == 0){
        context["isPointLight"] -> setInt(0);
    }
    else{
        context["isPointLight"] -> setInt(1);
    }
    Buffer lightBuffer = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_USER, points.size() );
    lightBuffer -> setElementSize( sizeof(Point) );
    memcpy( lightBuffer -> map(), (char*)&points[0], sizeof(Point) * points.size() );
    lightBuffer -> unmap();
    context["pointLights"] -> set(lightBuffer );
    context["pointLightNum"] -> setInt(points.size() ); 
}

void updatePointLight(Context& context, std::vector<Point>& points){
    if(points.size() != 0){
        Buffer lightBuffer = context["pointLights"] -> getBuffer();
        memcpy(lightBuffer -> map(), (char*)&points[0], sizeof(Point)*points.size()  );
        lightBuffer -> unmap();
    }
}

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

void loadImageToTextureSampler(Context& context, TextureSampler& Sampler, std::string& fileName){
    cv::Mat texMat = cv::imread(fileName, cv::IMREAD_COLOR);
    if(texMat.empty() ){
        std::cout<<"Wrong: unable to load the texture map: "<<fileName<<"!"<<std::endl;
        exit(1);
    }

    int width = texMat.cols;
    int height = texMat.rows;

    // Set up the texture sampler
    Buffer texBuffer = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT4, width, height);
    float* texPt = reinterpret_cast<float*> (texBuffer -> map() );
    for(int r = 0; r < height; r++){
        for(int c = 0; c < width; c++){
            cv::Vec3b color = texMat.at<cv::Vec3b>(height - 1 - r, c);
            int index = 4 * (r*width + c);
            texPt[index + 0] = float(color[2] ) / 255.0;
            texPt[index + 1] = float(color[1] ) / 255.0;
            texPt[index + 2] = float(color[0] ) / 255.0;
            texPt[index + 3] = 1.0;
        }   
    } 
    texBuffer -> unmap();
    Sampler -> setBuffer(0u, 0u, texBuffer);
}

void loadEmptyToTextureSampler(Context& context,  TextureSampler& Sampler){
    Buffer texBuffer = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_FLOAT4, 1u, 1u );
    float* texPt = static_cast<float*>( texBuffer->map() );
    texPt[0] = 1.0;
    texPt[1] = 1.0;
    texPt[2] = 1.0;
    texPt[3] = 1.0f;
    texBuffer->unmap(); 
    Sampler->setBuffer( 0u, 0u, texBuffer );
}

Material createDefaultMaterial(Context& context ){
    const std::string ptx_path = ptxPath( "diffuse.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
        
    // Texture Sampler 
    TextureSampler albedoSampler = createTextureSampler(context);
    material["isAlbedoTexture"] -> setInt(0);   
    loadEmptyToTextureSampler(context, albedoSampler);
    material["albedo"] -> setFloat(0.5, 0.5, 0.5 );
    material["albedoMap"] -> setTextureSampler(albedoSampler);
   
    TextureSampler normalSampler = createTextureSampler(context);
    material["isNormalTexture"] -> setInt(0);
    loadEmptyToTextureSampler(context, normalSampler);
    material["normalMap"] -> setTextureSampler(normalSampler );
    return material;
}

Material createDiffuseMaterial(Context& context, objLoader::material_t mat)
{
    const std::string ptx_path = ptxPath( "diffuse.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
        
    // Texture Sampler 
    TextureSampler albedoSampler = createTextureSampler(context);

    if(mat.albedo_texname != std::string("") ){

        material["isAlbedoTexture"] -> setInt(1);
        loadImageToTextureSampler(context, albedoSampler, mat.albedo_texname );
        material["albedo"] -> setFloat(make_float3(1.0) );
    } 
    else{
        material["isAlbedoTexture"] -> setInt(0);
        
        // albedo buffer
        loadEmptyToTextureSampler(context, albedoSampler);
        material["albedo"] -> setFloat(mat.albedo[0], \
                mat.albedo[1], mat.albedo[2] );
    }
    material["albedoMap"] -> setTextureSampler(albedoSampler);
   
    TextureSampler normalSampler = createTextureSampler(context);
    
    if(mat.normal_texname != std::string("") ){
        material["isNormalTexture"] -> setInt(1);
        loadImageToTextureSampler(context, normalSampler, mat.normal_texname); 
    }
    else{
        material["isNormalTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, normalSampler);
    }
    material["normalMap"] -> setTextureSampler(normalSampler );
    return material;
}

Material createPhongMaterial(Context& context, objLoader::material_t mat)
{
    const std::string ptx_path = ptxPath( "phong.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
        
    // Texture Sampler 
    TextureSampler albedoSampler = createTextureSampler(context);
    
    if(mat.albedo_texname != std::string("") ){
        material["isAlbedoTexture"] -> setInt(1);
        loadImageToTextureSampler(context, albedoSampler, mat.albedo_texname);
        material["albedo"] -> setFloat(1.0, 1.0, 1.0);
    } 
    else{
        material["isAlbedoTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, albedoSampler);
        material["albedo"] -> setFloat(mat.albedo[0], \
                mat.albedo[1], mat.albedo[2] );
    }
    material["albedoMap"] -> setTextureSampler(albedoSampler);
    
    TextureSampler specularSampler = createTextureSampler(context);
    
    if(mat.specular_texname != std::string("") ){
        material["isSpecularTexture"] -> setInt(1);
        loadImageToTextureSampler(context, specularSampler, mat.specular_texname);
        material["specular"] -> setFloat(1.0, 1.0, 1.0);
    } 
    else{
        material["isSpecularTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, specularSampler);
        material["specular"] -> setFloat(mat.specular[0], \
                mat.specular[1], mat.specular[2] );
    }
    material["specularMap"] -> setTextureSampler(specularSampler);
   
    TextureSampler normalSampler = createTextureSampler(context );
    
    if(mat.normal_texname != std::string("") ){
        material["isNormalTexture"] -> setInt(1);
        loadImageToTextureSampler(context, normalSampler, mat.normal_texname);
    }
    else{
        material["isNormalTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, normalSampler);
    }
    material["normalMap"] -> setTextureSampler(normalSampler );

    TextureSampler glossySampler = createTextureSampler(context );
    
    if(mat.glossiness_texname != std::string("") ){
        material["isGlossyTexture"] -> setInt(1);
        loadImageToTextureSampler(context, glossySampler, mat.glossiness_texname);
        material["glossy"] -> setFloat(0.0);
    }
    else{
        material["isGlossyTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, glossySampler);
        material["glossy"] -> setFloat(mat.glossiness);
    }
    material["glossyMap"] -> setTextureSampler(glossySampler );



    return material;
}

Material createMicrofacetMaterial(Context& context, objLoader::material_t mat)
{
    const std::string ptx_path = ptxPath( "microfacet.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
        
    // Texture Sampler 
    TextureSampler albedoSampler = createTextureSampler(context);
    
    if(mat.albedo_texname != std::string("") ){
        material["isAlbedoTexture"] -> setInt(1);
        loadImageToTextureSampler(context, albedoSampler, mat.albedo_texname);
        material["albedo"] -> setFloat(make_float3(1.0) );
    } 
    else{
        material["isAlbedoTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, albedoSampler);
        material["albedo"] -> setFloat(mat.albedo[0], \
                mat.albedo[1], mat.albedo[2] );
    }
    material["albedoMap"] -> setTextureSampler(albedoSampler);
   
    TextureSampler normalSampler = createTextureSampler(context );
    
    if(mat.normal_texname != std::string("") ){
        material["isNormalTexture"] -> setInt(1);
        loadImageToTextureSampler(context, normalSampler, mat.normal_texname);
    }
    else{
        material["isNormalTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, normalSampler);
    }
    material["normalMap"] -> setTextureSampler(normalSampler );

    TextureSampler roughSampler = createTextureSampler(context );
    
    if(mat.roughness_texname != std::string("") ){
        material["isRoughTexture"] -> setInt(1);
        loadImageToTextureSampler(context, roughSampler, mat.roughness_texname);
        material["rough"] -> setFloat(1.0);
    }
    else{
        material["isRoughTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, roughSampler);
        material["rough"] -> setFloat(mat.roughness);
    }
    material["roughMap"] -> setTextureSampler(roughSampler );
    
    TextureSampler metallicSampler = createTextureSampler(context );
    
    if(mat.metallic_texname != std::string("") ){
        material["isMetallicTexture"] -> setInt(1);
        loadImageToTextureSampler(context, metallicSampler, mat.metallic_texname);
        material["metallic"] -> setFloat(0.0);
    }
    else{
        material["isMetallicTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, metallicSampler);
        material["metallic"] -> setFloat(mat.metallic );
    }
    material["metallicMap"] -> setTextureSampler(metallicSampler );

    material["F0"] -> setFloat(mat.fresnel);
    return material;
}

Material createWhiteMaterial(Context& context ){
    const std::string ptx_path = ptxPath( "albedo.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
        
    // Texture Sampler 
    TextureSampler albedoSampler = createTextureSampler(context);
    material["isAlbedoTexture"] -> setInt(0);   
    loadEmptyToTextureSampler(context, albedoSampler);
    material["albedo"] -> setFloat(1.0, 1.0, 1.0 );
    material["albedoMap"] -> setTextureSampler(albedoSampler);
   
    return material;
}

Material createBlackMaterial(Context& context ){
    const std::string ptx_path = ptxPath( "albedo.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
        
    // Texture Sampler 
    TextureSampler albedoSampler = createTextureSampler(context);
    material["isAlbedoTexture"] -> setInt(0);   
    loadEmptyToTextureSampler(context, albedoSampler);
    material["albedo"] -> setFloat(0.0, 0.0, 0.0 );
    material["albedoMap"] -> setTextureSampler(albedoSampler);
   
    return material;
}

Material createAlbedoMaterial(Context& context, objLoader::material_t mat){
    const std::string ptx_path = ptxPath( "albedo.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
        
    // Texture Sampler 
    TextureSampler albedoSampler = createTextureSampler(context);

    if(mat.albedo_texname != std::string("") ){
        material["isAlbedoTexture"] -> setInt(1);
        loadImageToTextureSampler(context, albedoSampler, mat.albedo_texname );
        material["albedo"] -> setFloat(make_float3(1.0) );
    } 
    else{
        material["isAlbedoTexture"] -> setInt(0);
        
        // albedo buffer
        loadEmptyToTextureSampler(context, albedoSampler);
        material["albedo"] -> setFloat(mat.albedo[0], \
                mat.albedo[1], mat.albedo[2] );
    }
    material["albedoMap"] -> setTextureSampler(albedoSampler); 
    return material;
}

Material createNormalMaterial(Context& context, objLoader::material_t mat){
    const std::string ptx_path = ptxPath( "normal.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
        
    TextureSampler normalSampler = createTextureSampler(context );
    
    if(mat.normal_texname != std::string("") ){
        material["isNormalTexture"] -> setInt(1);
        loadImageToTextureSampler(context, normalSampler, mat.normal_texname);
    }
    else{
        material["isNormalTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, normalSampler);
    }
    material["normalMap"] -> setTextureSampler(normalSampler );

    return material;
}

Material createRoughnessMaterial(Context& context, objLoader::material_t mat){
    const std::string ptx_path = ptxPath( "roughness.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
        
    TextureSampler roughSampler = createTextureSampler(context );
    
    if(mat.roughness_texname != std::string("") ){
        material["isRoughTexture"] -> setInt(1);
        loadImageToTextureSampler(context, roughSampler, mat.roughness_texname);
        material["rough"] -> setFloat(1.0);
    }
    else{
        material["isRoughTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, roughSampler);
        material["rough"] -> setFloat(mat.roughness);
    }
    material["roughMap"] -> setTextureSampler(roughSampler );

    return material; 
}

Material createMetallicMaterial(Context& context, objLoader::material_t mat){
    const std::string ptx_path = ptxPath( "metallic.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
        
    TextureSampler metallicSampler = createTextureSampler(context );
    
    if(mat.metallic_texname != std::string("") ){
        material["isMetallicTexture"] -> setInt(1);
        loadImageToTextureSampler(context, metallicSampler, mat.metallic_texname);
        material["metallic"] -> setFloat(1.0);
    }
    else{
        material["isMetallicTexture"] -> setInt(0);
        loadEmptyToTextureSampler(context, metallicSampler);
        material["metallic"] -> setFloat(mat.metallic);
    }
    material["metallicMap"] -> setTextureSampler(metallicSampler );

    return material; 
}

Material createDepthMaterial(Context& context ){
    const std::string ptx_path = ptxPath( "depth.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
        
    return material; 
}

inline int idivCeil( int x, int y )                                              
{                                                                                
    return (x + y-1)/y;                                                            
}


void createCamera(Context& context, CameraInput& camInput){

    const float3 camera_eye(make_float3(camInput.origin[0],
                camInput.origin[1], camInput.origin[2]) );
    const float3 camera_lookat(make_float3(camInput.target[0],
                camInput.target[1], camInput.target[2]) );
    const float3 camera_up( optix::make_float3(camInput.up[0],
                camInput.up[1], camInput.up[2]) );
    float vfov = 0;
    if(camInput.isAxisX == false)
        vfov = camInput.fov;
    else{
        float t = tan(camInput.fov /2 / 180.0 * 3.1415926f); 
        float new_t = t / float(camInput.width) * float(camInput.height);
        vfov = 2 * atan(new_t) * 180.0 / 3.1415926f;
    }
    sutil::Camera camera( camInput.width, camInput.height, 
            &camera_eye.x, &camera_lookat.x, &camera_up.x,
            context["eye"], context["cameraU"], context["cameraV"], context["cameraW"], 
            vfov);
}

void splitShapes(objLoader::shape_t& shapeLarge, std::vector<objLoader::shape_t >& shapeArr)
{
    int faceNum = shapeLarge.mesh.indicesP.size() / 3;
    if(faceNum <= faceLimit ){
        shapeArr.push_back(shapeLarge);
    }
    else{
        int shapeNum = ceil(float(faceNum) / float(faceLimit) );
        for(int i = 0; i < shapeNum; i++){
            int fs = i * faceLimit, fe = (i+1) * faceLimit;
            fe = (fe > faceNum ) ? faceNum : fe;

            objLoader::shape_t shape; 
            shape.isLight = shapeLarge.isLight;
            for(int j = 0; j < 3; j++){
                shape.radiance[j] = shapeLarge.radiance[j];
            }
            char tempNum[10];
            sprintf(tempNum, "_%d", i);
            shape.name = shapeLarge.name + std::string(tempNum );
            
            // Change the face and vertex 
            std::map<int, int> facePointMap;
            std::map<int, int> faceNormalMap;
            std::map<int, int> faceTexMap;
            
            facePointMap[-1] = -1;
            faceNormalMap[-1] = -1;
            faceTexMap[-1] = -1;
            std::map<int, int>::iterator it;

            for(int j = fs; j < fe; j++){
                for(int k = j * 3; k < j*3 + 3; k++){
                    // Process the vertex 
                    if(shapeLarge.mesh.indicesP[k] != -1){
                        int vId = shapeLarge.mesh.indicesP[k];
                        it = facePointMap.find(vId );
                        if(it == facePointMap.end() ){
                            for(int l = 0; l < 3; l++){
                                shape.mesh.positions.push_back(
                                        shapeLarge.mesh.positions[3*vId + l] );
                            }
                            facePointMap[vId] = shape.mesh.positions.size() / 3 - 1;
                        }
                    }
                    
                    // Process the normals 
                    if(shapeLarge.mesh.indicesN[k] != -1){
                        int vnId = shapeLarge.mesh.indicesN[k];
                        it = faceNormalMap.find(vnId );
                        if(it == faceNormalMap.end() ){
                            for(int l = 0; l < 3; l++){
                                shape.mesh.normals.push_back(
                                        shapeLarge.mesh.normals[3*vnId + l] );
                            }
                            faceNormalMap[vnId] = shape.mesh.normals.size() / 3 - 1;
                        }
                    }
                    
                    // Process the texture 
                    if(shapeLarge.mesh.indicesT[k] != -1){
                        int vtId = shapeLarge.mesh.indicesT[k];
                        it = faceTexMap.find(vtId );
                        if(it == faceTexMap.end() ){ 
                            for(int l = 0; l < 2; l++){
                                shape.mesh.texcoords.push_back(
                                        shapeLarge.mesh.texcoords[2*vtId + l] );
                            }
                        }
                        faceTexMap[vtId] = shape.mesh.texcoords.size() / 2 - 1;
                    }
                }
            }

            // Process the indexP indexT and indexN
            for(int j = fs; j < fe; j++){
                for(int k = j*3; k < j*3 + 3; k++){
                    shape.mesh.indicesP.push_back(
                            facePointMap[shapeLarge.mesh.indicesP[k] ] );
                    shape.mesh.indicesN.push_back(
                            faceNormalMap[shapeLarge.mesh.indicesN[k] ] );
                    shape.mesh.indicesT.push_back(
                            faceTexMap[shapeLarge.mesh.indicesT[k] ] );
                }
            }
            

            // Process the material  
            std::map<int, int> faceMatMap;
            for(int j = fs; j < fe; j++){
                int matId = shapeLarge.mesh.materialIds[j];
                it = faceMatMap.find(matId );
                if(it  == faceMatMap.end() ){
                    faceMatMap[matId] = shape.mesh.materialNames.size();   
                    if(shape.isLight == false){
                        shape.mesh.materialNames.push_back(shapeLarge.mesh.materialNames[matId] );
                        shape.mesh.materialNameIds.push_back(shapeLarge.mesh.materialNameIds[matId] );
                    }
                }
            }
            
            for(int j = fs; j < fe; j++){
                shape.mesh.materialIds.push_back(faceMatMap[shapeLarge.mesh.materialIds[j] ] );
            }
            shapeArr.push_back(shape );
        }
    }   
}

void createGeometry(
        Context& context,
        const std::vector<objLoader::shape_t>& shapes,
        const std::vector<objLoader::material_t>& materials,
        int mode
        )
{
    // Create geometry group;
    GeometryGroup geometry_group = context->createGeometryGroup();
    geometry_group->setAcceleration( context->createAcceleration( "Trbvh" ) );

    std::cout<<"Total number of materials: "<<materials.size()<<std::endl;

    // Area Light
    context["isAreaLight"] -> setInt(0);
    for(int i = 0; i < shapes.size(); i++) {
        
        const std::string path = ptxPath( "triangle_mesh.cu" );
        optix::Program bounds_program = context->createProgramFromPTXFile( path, "mesh_bounds" );
        optix::Program intersection_program = context->createProgramFromPTXFile( path, "mesh_intersect" );
        
        objLoader::shape_t  shapeLarge = shapes[i];
        std::vector<objLoader::shape_t > shapeArr;
        splitShapes(shapeLarge, shapeArr );

        if(shapeArr.size() != 1){
            std::cout<<"Warning: the mesh "<<shapeLarge.name<<" is too large and has been splited into "<<shapeArr.size()<<" parts"<<std::endl;
        }

        for(int j = 0; j < shapeArr.size(); j++){
            objLoader::shape_t shape = shapeArr[j];
            int vertexNum = shape.mesh.positions.size() / 3;
            int normalNum = shape.mesh.normals.size() / 3;
            int texNum = shape.mesh.texcoords.size() / 2;
            int faceNum = shape.mesh.indicesP.size() / 3;
         
            bool hasNormal = (normalNum != 0);
            bool hasTex = (texNum != 0);
         
            Buffer vertexBuffer = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, vertexNum );
            Buffer normalBuffer = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, normalNum );
            Buffer texBuffer = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT2, texNum );
            Buffer faceBuffer = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_INT3, faceNum );
            Buffer faceVtBuffer = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_INT3, faceNum );
            Buffer faceVnBuffer = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_INT3, faceNum );
            Buffer materialBuffer = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_INT, faceNum );
         
            float* vertexPt = reinterpret_cast<float*>(vertexBuffer -> map() );
            int* facePt = reinterpret_cast<int32_t*>(faceBuffer -> map() );
            int* faceVtPt = reinterpret_cast<int32_t*>( faceVtBuffer -> map() );
            int* faceVnPt = reinterpret_cast<int32_t*>( faceVnBuffer -> map() );
            float* texPt = reinterpret_cast<float*> (hasTex ? texBuffer -> map() : 0);
            float* normalPt = reinterpret_cast<float*>( hasNormal ? normalBuffer -> map() : 0);
            int* materialPt = reinterpret_cast<int32_t*>(materialBuffer -> map() );

            for(int i = 0; i < vertexNum * 3; i++){
                vertexPt[i] = shape.mesh.positions[i];
            }
            for(int i = 0; i < normalNum * 3; i++){
                normalPt[i] = shape.mesh.normals[i];
            }
            for(int i = 0; i < texNum * 2; i++){
                texPt[i] = shape.mesh.texcoords[i];
            }
            for(int i = 0; i < faceNum * 3; i++){
                facePt[i] = shape.mesh.indicesP[i];
                faceVtPt[i] = shape.mesh.indicesT[i];
                faceVnPt[i] = shape.mesh.indicesN[i];
            }
            for(int i = 0; i < faceNum; i++ ) {
                materialPt[i] = shape.mesh.materialIds[i];
            }
         
            vertexBuffer -> unmap();
            faceBuffer -> unmap();
            faceVnBuffer -> unmap();
            faceVtBuffer -> unmap();
            if(hasNormal){
                normalBuffer -> unmap();
            }
            if(hasTex){
                texBuffer -> unmap();
            }
            materialBuffer -> unmap();
         
            Geometry geometry = context -> createGeometry();
            geometry[ "vertex_buffer" ] -> setBuffer(vertexBuffer );
            geometry[ "normal_buffer" ] -> setBuffer(normalBuffer );
            geometry[ "texcoord_buffer" ] -> setBuffer(texBuffer );
            geometry[ "index_buffer" ] -> setBuffer(faceBuffer );
            geometry[ "index_normal_buffer" ] -> setBuffer(faceVnBuffer );
            geometry[ "index_tex_buffer" ] -> setBuffer(faceVtBuffer );
            geometry[ "material_buffer"] -> setBuffer(materialBuffer );
            geometry -> setPrimitiveCount( faceNum );
            geometry->setBoundingBoxProgram ( bounds_program );
            geometry->setIntersectionProgram( intersection_program );
         
            // Currently only support diffuse material and area light 
            std::vector<Material> optix_materials;
            if(!shape.isLight ){
                if(mode == 0){
                    for(int i = 0; i < shape.mesh.materialNames.size(); i++){
                        Material mat;
                        objLoader::material_t matInput = materials[shape.mesh.materialNameIds[i] ];
                        if(matInput.cls == std::string("diffuse") ){
                            mat = createDiffuseMaterial(context, matInput );
                        }
                        else if(matInput.cls == std::string("microfacet") ){
                            mat = createMicrofacetMaterial(context, matInput );
                        }
                        else if(matInput.cls == std::string("phong") ){
                            mat = createPhongMaterial(context, matInput );
                        }
                        optix_materials.push_back(mat);
                    }
                    optix_materials.push_back(createDefaultMaterial(context ) );
                }
                else if(mode == 1){
                    // Output the albedo value 
                    for(int i = 0; i < shape.mesh.materialNames.size(); i++){
                        objLoader::material_t matInput = materials[shape.mesh.materialNameIds[i] ]; 
                        Material mat = createAlbedoMaterial(context, matInput);
                        optix_materials.push_back(mat);
                    }
                    optix_materials.push_back(createDefaultMaterial(context ) );
                }
                else if(mode == 2){
                    // Output the normal value 
                    for(int i = 0; i < shape.mesh.materialNames.size(); i++){
                        objLoader::material_t matInput = materials[shape.mesh.materialNameIds[i] ]; 
                        Material mat = createNormalMaterial(context, matInput);
                        optix_materials.push_back(mat);
                    }
                    optix_materials.push_back(createDefaultMaterial(context ) ); 
                }
                else if(mode == 3){
                    // Output the roughness value 
                    for(int i = 0; i < shape.mesh.materialNames.size(); i++){
                        objLoader::material_t matInput = materials[shape.mesh.materialNameIds[i] ]; 
                        Material mat = createRoughnessMaterial(context, matInput);
                        optix_materials.push_back(mat);
                    }
                    optix_materials.push_back(createDefaultMaterial(context ) );  
                }
                else if(mode == 4){
                    // Output the mask
                    for(int i = 0; i < shape.mesh.materialNames.size(); i++){
                        Material mat = createWhiteMaterial(context );
                        optix_materials.push_back(mat );
                    }
                    optix_materials.push_back(createDefaultMaterial(context ) );
                }
                else if(mode == 5){
                    // Output the depth 
                    for(int i = 0; i < shape.mesh.materialNames.size(); i++){
                        Material mat = createDepthMaterial(context );
                        optix_materials.push_back(mat);
                    }
                    optix_materials.push_back(createDefaultMaterial(context) );
                }
                else if(mode == 6){
                    // Output the metallic 
                    for(int i = 0; i < shape.mesh.materialNames.size(); i++){
                        objLoader::material_t matInput = materials[shape.mesh.materialNameIds[i] ]; 
                        Material mat = createMetallicMaterial(context, matInput);
                        optix_materials.push_back(mat );
                    }
                    optix_materials.push_back(createDefaultMaterial(context) );
                }
                else{
                    std::cout<<"Wrong: Unrecognizable mode!"<<std::endl;
                    exit(1);
                }
            }
            else{
                context["isAreaLight"] -> setInt(1);
                if(mode == 0){
                    Material mat = createAreaLight(context, shape );
                    optix_materials.push_back(mat);
                }
                else{
                    Material mat = createBlackMaterial(context );
                    optix_materials.push_back(mat);
                }
            }
         
            GeometryInstance geom_instance = context->createGeometryInstance(
                    geometry,
                    optix_materials.begin(),
                    optix_materials.end()
                    );
            geometry_group -> addChild(geom_instance );
        }
    }
    context["top_object"] -> set( geometry_group);
}

void boundingBox(
        Context& context,
        const std::vector<objLoader::shape_t>& shapes
        )
{
    float3 vmin, vmax;
    for(int i = 0; i < shapes.size(); i++){
        objLoader::shape_t shape = shapes[i];
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
                if(mode == 0){
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

    if(camNum > 1){
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
    if(!camIn){
        std::cout<<"Wrong: can not load the camera file!"<<std::endl;
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
        int maxExpo = 4)
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



int main( int argc, char** argv )
{
    bool use_pbo  = false;

    std::string fileName;
    std::string outputFileName;
    std::string cameraFile("");
    int mode = 0;
    std::vector<int> gpuIds;

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
        else{
            std::cout<<"Unrecognizable input command"<<std::endl;
            exit(1);
        }
    }
    
    std::vector<objLoader::shape_t> shapes;
    std::vector<objLoader::material_t> materials;
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

    std::cout<<"Material num: "<<materials.size() << std::endl;
    std::cout<<"Shape num: "<<shapes.size() <<std::endl;
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

    createContext(use_pbo, cameraInput.cameraType, cameraInput.width, cameraInput.height, mode, cameraInput.sampleNum);
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
    for(int i = 0; i < camNum; i++){ 
        std::string outputFileNameNew = generateOutputFilename(outputFileName, mode,
                cameraInput.isHdr, i, camNum);

        std::ifstream f(outputFileNameNew.c_str() );
        if(f.good() ){
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
        std::cout<<"Start to render: "<<i+1<<"/"<<camNum<<std::endl;
        if(cameraInput.sampleType == std::string("independent") || mode != 0){
            int sampleNum = cameraInput.sampleNum;
            independentSampling(context, cameraInput.width, cameraInput.height, imgData, sampleNum);
        }
        else if(cameraInput.sampleType == std::string("adaptive") ) {
            int sampleNum = cameraInput.sampleNum;
            bool isTooDark = adaptiveSampling(context, cameraInput.width, cameraInput.height, sampleNum, imgData);
            std::cout<<"Sample Num: "<<sampleNum<<std::endl;
            if(isTooDark){
                std::cout<<"This image will not be output!"<<std::endl;
                continue;
            }
        }
        t = clock() - t;
        std::cout<<"Time: "<<float(t) / CLOCKS_PER_SEC<<'s'<<std::endl;

         
        bool isWrite = writeBufferToFile(outputFileNameNew.c_str(), 
                imgData,
                cameraInput.width, cameraInput.height, 
                cameraInput.isHdr,
                mode);
    }
    destroyContext();
    delete [] imgData;

    return 0;
}
