#ifndef READXML_HEADER
#define READXML_HEADER

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_aabb_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>
#include <vector>
#include <string>
#include "sutil.h"
#include "tinyxml.h"
#include "sutil/tinyobjloader/objLoader.h"
#include "CameraInput.h"
#include "areaLight.h"
#include "envmap.h"
#include "point.h"
#include <iostream>
#include <set>
#include <stdio.h>
#include <stdlib.h>


std::vector<float> parseFloatStr(const std::string& str){
    const char* S = str.c_str();
    int length = str.size();
    int start = 0;
    std::vector<float> arr;
    char buffer[10];

    while(start < length){

        int end = start;
        memset(buffer, 0, 10);

        for(; end < length; end++){
            if(S[end] == ',' || S[end] == ' '){
                break;
            }
        }

        int cnt = 0;
        for(int i =  start; i < end; i++)
            buffer[cnt++] = S[i];
        
        arr.push_back(atof(buffer) );
        for(start = end; start < length; start++){
            if(S[start] == ' ' || S[start] == ',')
                continue;
            else
                break;
        }
    }

    return arr;
}

struct objTransform{
    std::string name;
    float value[3];
};

bool doObjTransform(objLoader::shape_t& shape, std::vector<objTransform>& TArr)
{
    for(int i = 0; i < TArr.size(); i++){
        objTransform T = TArr[i];
        if(T.name == std::string("translate") ){
            int vertexNum = shape.mesh.positions.size() / 3;
            for(int i = 0; i < vertexNum; i++){
                shape.mesh.positions[3*i] += T.value[0];
                shape.mesh.positions[3*i + 1] += T.value[1];
                shape.mesh.positions[3*i + 2] += T.value[2];
            }
        }
        else if(T.name == std::string("scale") ){
            for(int i = 0; i < shape.mesh.positions.size(); i++){
                shape.mesh.positions[i] *= T.value[0];
            }
        }
        else{
            std::cout<<"Wrong: unrecognizable transform operatioin!"<<std::endl;
            return false;
        }
    }
    return true;
}


bool loadBsdf(std::vector<objLoader::material_t>& materials, TiXmlNode* module, int offset = 0)
{
    // Load materials
    objLoader::material_t mat;

    TiXmlElement* matEle = module -> ToElement();
    for(TiXmlAttribute* matAttri = matEle -> FirstAttribute();  matAttri != 0; matAttri = matAttri -> Next() ){
        if(matAttri -> Name() == std::string("type") ){
            if(matAttri -> Value() == std::string("diffuse") ){
                mat.cls = std::string("diffuse");
                
                // Read the variable for diffuse BRDF
                // Only support rgb radiance and texture radiance
                for(TiXmlNode* matSubModule = module -> FirstChild(); matSubModule != 0; matSubModule = matSubModule -> NextSibling() ){
                    if(matSubModule -> Value() == std::string("rgb") ){

                        TiXmlElement* matSubEle = matSubModule -> ToElement();
                        for(TiXmlAttribute* matSubAttri = matSubEle -> FirstAttribute(); matSubAttri != 0; matSubAttri = matSubAttri -> Next() ){
                            if(matSubAttri -> Name() == std::string("name") ){
                                if(matSubAttri -> Value() != std::string("reflectance") ){
                                    std::cout<<"Wrong: unrecognizable name of rgb of diffuse BRDF!"<< matSubAttri->Value() <<std::endl;
                                    std::cout<<matSubAttri -> Value() <<std::endl;
                                    return false;
                                }
                            }
                            else if(matSubAttri -> Name() == std::string("value") ){
                                std::vector<float> diffuseArr = parseFloatStr(matSubAttri -> Value() );
                                mat.albedo[0] = diffuseArr[0];
                                mat.albedo[1] = diffuseArr[1];
                                mat.albedo[2] = diffuseArr[2];
                            }
                            else{
                                std::cout<<"Wrong: unrecognizable attribute of rgb of diffuse BRDF"<<std::endl;
                                return false;
                            }
                        }
                    }
                    else if(matSubModule -> Value() == std::string("texture") ){

                        std::string texName;
                        TiXmlElement* matSubEle = matSubModule -> ToElement();
                        for(TiXmlAttribute* matSubAttri = matSubEle -> FirstAttribute(); matSubAttri != 0; matSubAttri = matSubAttri -> Next() ){
                            if(matSubAttri -> Name() == std::string("name") ) {
                                if(matSubAttri -> Value() == std::string("reflectance" ) ){
                                    texName = matSubAttri -> Value();
                                }
                                else if(matSubAttri -> Value() == std::string("normal") ){
                                    texName = matSubAttri -> Value();
                                }
                                else{
                                    std::cout<<"Wrong: unrecognizable name "<<matSubAttri -> Value() <<" of texture of diffuse BRDF!"<<std::endl;
                                    return false;
                                }
                            }
                        }
                    
                        TiXmlNode* strModule = matSubModule -> FirstChild();
                        TiXmlElement* strEle = strModule -> ToElement();
                        for(TiXmlAttribute* strAttri = strEle->FirstAttribute(); strAttri != 0; strAttri = strAttri->Next() ){
                            
                            if(strAttri -> Name() == std::string("value") ){
                                if(texName == std::string("reflectance") )
                                    mat.albedo_texname = std::string(strAttri -> Value());
                                else if(texName == std::string("normal") ){
                                    mat.normal_texname = std::string(strAttri -> Value() );
                                }
                            }
                            else if(strAttri -> Name() == std::string("name") ){
                                if(strAttri -> Value() != std::string("filename") ){
                                    std::cout<<"Wrong: unrecognizable name of texture of diffuse BRDF!"<<std::endl;
                                    return false;
                                }
                            }
                        }
                    }
                    else{
                        std::cout<<"Wrong: unrecognizable module of diffuse BRDF!"<<std::endl;
                        return false;
                    }
                }
            }
            else if(matAttri -> Value() == std::string("phong") ){
                mat.cls = std::string("phong");
                
                // Read the variable for diffuse BRDF
                // Only support rgb radiance and texture radiance
                for(TiXmlNode* matSubModule = module -> FirstChild(); matSubModule != 0; matSubModule = matSubModule -> NextSibling() ){
                    if(matSubModule -> Value() == std::string("rgb") ){

                        TiXmlElement* matSubEle = matSubModule -> ToElement();
                        std::string rgbName = "";
                        for(TiXmlAttribute* matSubAttri = matSubEle -> FirstAttribute(); matSubAttri != 0; matSubAttri = matSubAttri -> Next() ){
                            if(matSubAttri -> Name() == std::string("name") ){
                                if(matSubAttri -> Value() == std::string("diffuseReflectance") ){
                                    rgbName = std::string("diffuseReflectance");
                                }
                                else if(matSubAttri -> Value() == std::string("specularReflectance") ){
                                    rgbName = std::string("specularReflectance");
                                }
                                else{
                                    std::cout<<"Wrong: unrecognizable name of rgb of phong BRDF!"<<std::endl;
                                    return false; 
                                }
                            }
                        }

                        for(TiXmlAttribute* matSubAttri = matSubEle -> FirstAttribute(); matSubAttri != 0; matSubAttri = matSubAttri -> Next() ){
                            if(matSubAttri -> Name() == std::string("value") ){
                                std::vector<float> paraArr = parseFloatStr(matSubAttri -> Value() );
                                if(rgbName == std::string("diffuseReflectance") ){
                                    mat.albedo[0] = paraArr[0];
                                    mat.albedo[1] = paraArr[1];
                                    mat.albedo[2] = paraArr[2];
                                }
                                else if(rgbName == std::string("specularReflectance") ){
                                    mat.specular[0] = paraArr[0];
                                    mat.specular[1] = paraArr[1];
                                    mat.specular[2] = paraArr[2];
                                }
                            }
                        }
                    }
                    else if(matSubModule -> Value() == std::string("texture") ){

                        std::string texName;
                        TiXmlElement* matSubEle = matSubModule -> ToElement();
                        for(TiXmlAttribute* matSubAttri = matSubEle -> FirstAttribute(); matSubAttri != 0; matSubAttri = matSubAttri -> Next() ){
                            if(matSubAttri -> Name() == std::string("name") ) {
                                if(matSubAttri -> Value() == std::string("diffuseReflectance" ) ){
                                    texName = matSubAttri -> Value();
                                }
                                else if(matSubAttri -> Value() == std::string("specularReflectance" ) ){
                                    texName = matSubAttri -> Value();
                                }
                                else if(matSubAttri -> Value() == std::string("alpha") ){
                                    texName = matSubAttri -> Value();
                                }
                                else if(matSubAttri -> Value() == std::string("normal") ){
                                    texName = matSubAttri -> Value();
                                }
                                else{
                                    std::cout<<"Wrong: unrecognizable name "<<matSubAttri -> Value() <<" of texture of phong BRDF!"<<std::endl;
                                    return false;
                                }
                            }
                        }
                    
                        TiXmlNode* strModule = matSubModule -> FirstChild();
                        TiXmlElement* strEle = strModule -> ToElement();
                        for(TiXmlAttribute* strAttri = strEle->FirstAttribute(); strAttri != 0; strAttri = strAttri->Next() ){
                            
                            if(strAttri -> Name() == std::string("value") ){
                                if(texName == std::string("diffuseReflectance") ){
                                    mat.albedo_texname = std::string(strAttri -> Value());
                                }
                                else if(texName == std::string("specularReflectance") ){
                                    mat.specular_texname = std::string(strAttri -> Value());
                                }
                                else if(texName == std::string("alpha") ){
                                    mat.glossiness_texname = std::string(strAttri -> Value() );
                                }
                                else if(texName == std::string("normal") ){
                                    mat.normal_texname = std::string(strAttri -> Value() );
                                }
                            }
                            else if(strAttri -> Name() == std::string("name") ){
                                if(strAttri -> Value() != std::string("filename") ){
                                    std::cout<<"Wrong: unrecognizable name of string of texture of phong BRDF!"<<std::endl;
                                    return false;
                                }
                            }
                        }
                    }
                    else if(matSubModule -> Value() == std::string("float") ){
                        
                        TiXmlElement* matSubEle = matSubModule -> ToElement();
                        for(TiXmlAttribute* matSubAttri = matSubEle -> FirstAttribute(); matSubAttri != 0; matSubAttri = matSubAttri -> Next() ){
                            if(matSubAttri -> Name() == std::string("name") ){
                                if(matSubAttri -> Value() != std::string("alpha") ){
                                    std::cout<<"Wrong: unrecognizable name of float of phong BRDF!"<<std::endl;
                                    return false;
                                }
                            }
                            else if(matSubAttri -> Name() == std::string("value") ){
                                std::vector<float> glossinessArr = parseFloatStr(matSubAttri -> Value() );
                                mat.glossiness = glossinessArr[0];
                            }
                            else{
                                std::cout<<"Wrong: unrecognizable attribute of float of phong BRDF"<<std::endl;
                                return false;
                            }
                        }
                    }
                    else{
                        std::cout<<"Wrong: unrecognizable module of phong BRDF!"<<std::endl;
                        return false;
                    }
                }
            }
            else if(matAttri -> Value() == std::string("microfacet") ){
                mat.cls = std::string("microfacet");

                for(TiXmlNode* matSubModule = module -> FirstChild(); matSubModule != 0; matSubModule = matSubModule -> NextSibling() ){
                    if(matSubModule -> Value() == std::string("rgb") ){
                        
                        TiXmlElement* matSubEle = matSubModule -> ToElement();
                        for(TiXmlAttribute* matSubAttri = matSubEle -> FirstAttribute(); matSubAttri != 0; matSubAttri = matSubAttri -> Next() ){
                            if(matSubAttri -> Name() == std::string("name") ){
                                if(matSubAttri -> Value() != std::string("albedo") ){
                                    std::cout<<"Wrong: unrecognizable name of rgb of microfacet BRDF!"<<std::endl;
                                    return false;
                                }
                            }
                            else if(matSubAttri -> Name() == std::string("value") ){
                                std::vector<float> diffuseArr = parseFloatStr(matSubAttri -> Value() );
                                mat.albedo[0] = diffuseArr[0];
                                mat.albedo[1] = diffuseArr[1];
                                mat.albedo[2] = diffuseArr[2];
                            }
                            else{
                                std::cout<<"Wrong: unrecognizable attribute of rgb of microfacet BRDF!"<<std::endl;
                                return false;
                            }
                        }    
                    }
                    else if(matSubModule -> Value() == std::string("texture") ){
                        
                        std::string texName;
                        TiXmlElement* matSubEle = matSubModule -> ToElement();
                        for(TiXmlAttribute* matSubAttri = matSubEle -> FirstAttribute(); matSubAttri != 0; matSubAttri = matSubAttri -> Next() ){
                            if(matSubAttri -> Name() == std::string("name") ) {
                                if(matSubAttri -> Value() == std::string("albedo" ) )
                                    texName = matSubAttri -> Value();
                                else if(matSubAttri -> Value() == std::string("normal") )
                                    texName = matSubAttri -> Value();
                                else if(matSubAttri -> Value() == std::string("roughness") )
                                    texName = matSubAttri -> Value();
                                else if(matSubAttri -> Value() == std::string("metallic") ){
                                    texName = matSubAttri -> Value();
                                }
                                else{
                                    std::cout<<"Wrong: unrecognizable type of texture."<<std::endl;
                                    return false;
                                }
                            }
                        }
                    
                        TiXmlNode* strModule = matSubModule -> FirstChild();
                        TiXmlElement* strEle = strModule -> ToElement();
                        for(TiXmlAttribute* strAttri = strEle->FirstAttribute(); strAttri != 0; strAttri = strAttri->Next()){

                            if(strAttri -> Name() == std::string("value") ){
                                if(texName == std::string("albedo") ){
                                    mat.albedo_texname = std::string(strAttri -> Value());
                                }
                                else if(texName == std::string("normal") ){
                                    mat.normal_texname = std::string(strAttri -> Value() );
                                }
                                else if(texName == std::string("roughness") ){
                                    mat.roughness_texname = std::string(strAttri -> Value() );
                                }
                                else if(texName == std::string("metallic") ){
                                    mat.metallic_texname = std::string(strAttri -> Value() );
                                }
                            }
                            else if(strAttri -> Name() == std::string("name") ){
                                if(strAttri -> Value() != std::string("filename") ){
                                    std::cout<<"Wrong: unrecognizable name of texture!"<<std::endl;
                                    return false;
                                }
                            }
                        }
                    }
                    else if(matSubModule -> Value() == std::string("float") ){
                        TiXmlElement* matSubEle = matSubModule -> ToElement();
                        std::string floatName;
                        for(TiXmlAttribute* matSubAttri = matSubEle -> FirstAttribute(); matSubAttri != 0; matSubAttri = matSubAttri -> Next() ){
                            if(matSubAttri -> Name() == std::string("name") ){
                                if(matSubAttri -> Value() == std::string("roughness") ){
                                    floatName = std::string("roughness" );
                                }
                                else if(matSubAttri -> Value() == std::string("fresnel") ){
                                    floatName = std::string("fresnel");
                                }
                                else if(matSubAttri -> Value() == std::string("metallic") ){
                                    floatName = std::string("metallic");
                                }
                                else{
                                    std::cout<<"Wrong: unrecognizable name of float of microfacet BRDF!"<<std::endl;
                                    return false;
                                }
                            }
                        }
                        for(TiXmlAttribute* matSubAttri = matSubEle -> FirstAttribute(); matSubAttri != 0; matSubAttri = matSubAttri -> Next() ){
                            if(matSubAttri -> Name() == std::string("value") ){
                                std::vector<float> value = parseFloatStr(matSubAttri -> Value() );
                                if(floatName == std::string("fresnel")  ){
                                    mat.fresnel = value[0];
                                }
                                else if(floatName == std::string("roughness") ){
                                    mat.roughness = value[0];
                                }
                                else if(floatName == std::string("metallic") ){
                                    mat.metallic = value[0];
                                }
                            }
                            else if(matSubAttri -> Name() != std::string("name") ){
                                std::cout<<"Wrong: unrecognizable attribute of float of microfacet BRDF!"<<std::endl;
                                return false;
                            }
                        }     
                    }
                } 
            }
        }
        if(matAttri -> Name() == std::string("id") ){
            mat.name = matAttri -> Value();
        }
    }
    if(mat.name == std::string("") ){
        char buffer[50];
        std::sprintf(buffer, "OPTIXRENDER_MATERIALID__%d", int(materials.size() + 1 + offset) ); 
        mat.name = std::string(buffer);
    }
    for(int i = 0; i < materials.size(); i++){
        if(mat.name == materials[i].name){
            std::cout<<"Wrong: repeated material Id."<<std::endl;
            std::cout<<mat.name<<std::endl;
            return false;
        }
    }
    materials.push_back(mat);
    return true;
}


bool readXML(std::string fileName, 
        std::vector<objLoader::shape_t>& shapes, 
        std::vector<objLoader::material_t>& materials, 
        CameraInput& Camera, 
        std::vector<Envmap>& envmaps, 
        std::vector<Point>& points)
{
    shapes.erase(shapes.begin(), shapes.end() );
    materials.erase(materials.begin(), materials.end() );

    std::vector<objLoader::shape_t> shapesAll;

    // Load the xml file
    TiXmlDocument doc(fileName.c_str() );
    bool isLoad = doc.LoadFile();
    if(isLoad == false){
        std::cout<<"Wrong: failed to open the xml file!"<<fileName<<std::endl;
        return false;
    }


    // Parse the xml file
    TiXmlNode* docNode = &doc;


    for(TiXmlNode* node = docNode -> FirstChild(); node != 0; node = node -> NextSibling() ){
        if(node -> Value() != std::string("scene") ){
            continue;
        }
        else{
            for(TiXmlNode* module = node -> FirstChild(); module != 0; module = module -> NextSibling() ){
                if(module -> Value() == std::string("sensor") ){
                    // Load the camera parameters  
                    TiXmlElement* seEle = module -> ToElement();
                    for(TiXmlAttribute* seAttri = seEle -> FirstAttribute(); seAttri != 0; seAttri = seAttri->Next() ){
                        if(seAttri -> Name() != std::string("type") ){
                            if(seAttri -> Value() == std::string("perspective") ){
                                Camera.cameraType = std::string("perspective");
                            }
                            else if(seAttri -> Value() == std::string("envmap") ){
                                Camera.cameraType = std::string("envmap");
                            }
                            else if(seAttri -> Value() == std::string("hemisphere") ){
                                Camera.cameraType = std::string("hemisphere");
                            }
                            else{
                                std::cout<<"Wrong: unrecognizable type of sensor!"<<std::endl;
                                return false;
                            }
                        }
                    }
                    for(TiXmlNode* seSubModule = module->FirstChild(); seSubModule != 0; seSubModule = seSubModule -> NextSibling() ){
                        if(seSubModule -> Value() == std::string("float") ){
                            TiXmlElement* seSubEle = seSubModule -> ToElement();
                            for(TiXmlAttribute* seSubAttri = seSubEle -> FirstAttribute(); seSubAttri != 0; seSubAttri = seSubAttri -> Next() ){
                                if(seSubAttri -> Name() == std::string("value") ){
                                    std::vector<float> fovArr = parseFloatStr(seSubAttri -> Value() );
                                    if(fovArr.size() > 1){
                                        std::cout<<"Wrong: wrong format of fov of sensor!"<<std::endl;
                                        return false;
                                    }
                                    Camera.fov = fovArr[0];
                                }
                                else if(seSubAttri -> Name() == std::string("name") ){
                                    if(seSubAttri -> Value() != std::string("fov") ){
                                        std::cout<<"Wrong: unrecognizable name of float of sensor!"<<std::endl;
                                        return false;
                                    }
                                }
                                else{
                                    std::cout<<"Wrong: unrecognizable attribute of float of sensor!"<<std::endl;
                                    return false;
                                }
                            }
                        }
                        else if(seSubModule ->Value() == std::string("string") ){
                            TiXmlElement* seSubEle = seSubModule -> ToElement();
                            for(TiXmlAttribute* seSubAttri = seSubEle -> FirstAttribute(); seSubAttri != 0; seSubAttri = seSubAttri -> Next() ){
                                if(seSubAttri -> Name() == std::string("value") ){
                                    if(seSubAttri -> Value() == std::string("x") ){
                                        Camera.isAxisX = true;
                                    }
                                    else if(seSubAttri -> Value() == std::string("y") ){
                                        Camera.isAxisX = false;
                                    }
                                    else{
                                        std::cout<<"Wrong: unrecognizable value of fovAxis of sensor"<<std::endl;
                                        return false;
                                    }
                                }
                                else if(seSubAttri -> Name() == std::string("name") ){
                                    if(seSubAttri -> Value() != std::string("fovAxis") ){
                                        std::cout<<"Wrong: unrecognizable name of string of sensor"<<std::endl;
                                        return false;
                                    }
                                }
                                else{
                                    std::cout<<"Wrong: unrecognizable attribute of string of sensor!"<<std::endl;
                                    return false;
                                }
                            }
                        }
                        else if(seSubModule -> Value() == std::string("transform") ){
                            TiXmlElement* seSubEle = seSubModule -> ToElement();
                            for(TiXmlAttribute* seSubAttri = seSubEle -> FirstAttribute(); seSubAttri != 0; seSubAttri = seSubAttri -> Next() ){
                                if(seSubAttri -> Name() == std::string("name") ){
                                    if(seSubAttri -> Value() != std::string("toWorld") ){
                                        std::cout<<"Wrong: unrecognizable name of transform of sensor!"<<std::endl;
                                        return false;
                                    }
                                }
                            }
                            
                            for(TiXmlNode* trSubModule = seSubModule -> FirstChild(); trSubModule != 0; trSubModule = trSubModule -> NextSibling() ){
                                if(trSubModule -> Value() == std::string("lookAt") ){
                                    TiXmlElement* trSubEle = trSubModule -> ToElement();
                                    for(TiXmlAttribute* trSubAttri = trSubEle -> FirstAttribute(); trSubAttri != 0; trSubAttri = trSubAttri -> Next() ){
                                        if(trSubAttri -> Name() == std::string("target") ){
                                            std::vector<float> targetArr = parseFloatStr(trSubAttri->Value() );
                                            Camera.target[0] = targetArr[0];
                                            Camera.target[1] = targetArr[1];
                                            Camera.target[2] = targetArr[2];
                                        }
                                        else if(trSubAttri -> Name() == std::string("origin") ){
                                            std::vector<float> originArr = parseFloatStr(trSubAttri -> Value() );
                                            Camera.origin[0] = originArr[0];
                                            Camera.origin[1] = originArr[1];
                                            Camera.origin[2] = originArr[2];
                                        }
                                        else if(trSubAttri -> Name() == std::string("up") ){
                                            std::vector<float> upArr = parseFloatStr(trSubAttri -> Value() );
                                            Camera.up[0] = upArr[0];
                                            Camera.up[1] = upArr[1];
                                            Camera.up[2] = upArr[2];
                                        }
                                        else{
                                            std::cout<<"Wrong: unrecognizable attribute of look at of sensor!"<<std::endl;
                                            return false;
                                        }
                                    }
                                }
                                else{
                                    std::cout<<"Wrong: unrecognizable module of transform of sensor!"<<std::endl;
                                    return false;
                                }
                            }
                        }
                        else if(seSubModule -> Value() == std::string("film") ) {
                            TiXmlElement* seSubEle = seSubModule -> ToElement();
                            for(TiXmlAttribute* seSubAttri = seSubEle -> FirstAttribute(); seSubAttri != 0; seSubAttri = seSubAttri -> Next() ){
                                if(seSubAttri -> Name() == std::string("type") ){
                                    if(seSubAttri -> Value() == std::string("hdrfilm") ){
                                        Camera.isHdr = true;
                                    }
                                    else if(seSubAttri -> Value() == std::string("ldrfilm") ){
                                        Camera.isHdr = false;
                                    }    
                                    else{
                                        std::cout<<"Wrong: unrecognizable type of film of sensor"<<std::endl;
                                        return false;
                                    }
                                }
                            }
                            
                            // Read image length 
                            for(TiXmlNode* fiSubModule = seSubModule -> FirstChild(); fiSubModule != 0; fiSubModule = fiSubModule -> NextSibling() ){
                                if(fiSubModule -> Value() == std::string("integer") ){
                                    TiXmlElement* fiSubEle = fiSubModule -> ToElement();
                                    bool isWidth = false;
                                    int length = 0;
                                    for(TiXmlAttribute* fiSubAttri = fiSubEle -> FirstAttribute(); fiSubAttri != 0; fiSubAttri = fiSubAttri -> Next() ){
                                        if(fiSubAttri -> Name() == std::string("name") ){
                                            if(fiSubAttri -> Value() == std::string("width") )
                                                isWidth = true;
                                            else if(fiSubAttri -> Value() == std::string("height") )
                                                isWidth = false;
                                            else{
                                                std::cout<<"Wrong: unrecognizable name of integer of film of sensor!"<<std::endl;
                                                return false;
                                            }
                                        }
                                        else if(fiSubAttri -> Name() == std::string("value") ){ 
                                            std::vector<float>  lengthArr = parseFloatStr(fiSubAttri -> Value() );
                                            length = lengthArr[0];
                                        }
                                        else{
                                            std::cout<<"Wrong: unrecognizable attribute of integer of film of sensor!"<<std::endl;
                                            return false;
                                        }
                                    }
                                    if(isWidth ){
                                        Camera.width = length;
                                    }
                                    else{
                                        Camera.height = length;
                                    }
                                }
                            }
                        }
                        else if(seSubModule -> Value() == std::string("sampler") ){
                            TiXmlElement* seSubEle = seSubModule -> ToElement();
                            for(TiXmlAttribute* seSubAttri = seSubEle -> FirstAttribute(); seSubAttri != 0; seSubAttri = seSubAttri -> Next() ){
                                if(seSubAttri -> Name() == std::string("type") ){
                                    if(seSubAttri -> Value() == std::string("independent") ){
                                        Camera.sampleType = std::string("independent");
                                    }
                                    else if(seSubAttri -> Value() == std::string("adaptive") ){
                                        Camera.sampleType = std::string("adaptive");
                                    }
                                    else{
                                        std::cout<<"Wrong: unrecognizable type "<<seSubAttri->Name()<<" of sampler of sensor!"<<std::endl;
                                        return false;
                                    }
                                }
                            }

                            for(TiXmlNode* saSubModule = seSubModule -> FirstChild(); saSubModule != 0; saSubModule = saSubModule -> NextSibling() ){
                                if(saSubModule -> Value() == std::string("integer") ){
                                    TiXmlElement* saSubEle = saSubModule -> ToElement() ;
                                    for(TiXmlAttribute* saSubAttri = saSubEle -> FirstAttribute(); saSubAttri != 0; saSubAttri = saSubAttri-> Next() ){
                                        if(saSubAttri -> Name() == std::string("value") ){
                                            std::vector<float> sampleCount = parseFloatStr(saSubAttri -> Value() );
                                            Camera.sampleNum = sampleCount[0];
                                        }
                                        else if(saSubAttri -> Name() == std::string("name") ){
                                            if(saSubAttri -> Value() != std::string("sampleCount") ){
                                                std::cout<<"Wrong: unrecognizable name of sampler of sensor!"<<std::endl;
                                                return false;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else{
                            std::cout<<"Wrong: unrecognizable module of sensor!"<<std::endl;
                            return false;
                        }
                    }
                }
                else if(module -> Value() == std::string("bsdf") ){
                    bool isLoadBsdf = loadBsdf(materials, module);
                    if(!isLoadBsdf) return false;
                }
                else if(module -> Value() == std::string("shape") ){
                    objLoader::shape_t  shape;
                    std::vector<std::string> matRefNames;
                    std::vector<int> matRefIds;
                    std::vector<objLoader::material_t> materialsShape;
                    std::vector<objTransform> TArr;

                    //Read the shape, currently only support obj files
                    TiXmlElement* shEle = module -> ToElement();
                    for(TiXmlAttribute* shAttri = shEle -> FirstAttribute(); shAttri != 0; shAttri = shAttri -> Next() ){
                        if(shAttri -> Name() == std::string("type") ){
                            if(shAttri -> Value() == std::string("obj") ){
                            }
                            else{
                                std::cout<<"Wrong: unrecognizable type of shape!"<<std::endl;
                                return false;
                            }
                        }
                        else if(shAttri -> Name() == std::string("id") ){
                            // Keep only the shape with same id
                            shape.name = shAttri -> Value();
                        }
                    }
                    // Read the content from the files
                    for(TiXmlNode* shSubModule = module->FirstChild(); shSubModule != 0; shSubModule = shSubModule->NextSibling()){
                        
                        if(shSubModule -> Value() == std::string("string") ){
                            TiXmlElement* shSubEle = shSubModule -> ToElement();
                            for(TiXmlAttribute* shSubAttri = shSubEle -> FirstAttribute(); shSubAttri != 0; shSubAttri = shSubAttri -> Next() ){

                                if(shSubAttri -> Name() == std::string("value") ){
                                    bool isLoad = objLoader::LoadObj(shape, shSubAttri -> Value() );
                                    if(isLoad == false){
                                        std::cout<<"Wrong: fail to load obj file!"<<std::endl;
                                        return false;
                                    }
                                }
                                else if( shSubAttri -> Name() == std::string("name") ){
                                    if(shSubAttri -> Value() != std::string("filename") ){
                                        std::cout<<"Wrong: unrecognizable name of string of shape!"<<std::endl;
                                        return false;
                                    }
                                }
                            }
                        }
                        else if(shSubModule -> Value() == std::string("ref") ){
                            TiXmlElement* shSubEle = shSubModule -> ToElement();
                            for(TiXmlAttribute* shSubAttri = shSubEle -> FirstAttribute(); shSubAttri != 0; shSubAttri = shSubAttri -> Next() ){
                                if(shSubAttri -> Name() == std::string("name") ){
                                    if(shSubAttri -> Value() != "bsdf"){ 
                                        std::cout<<"Wrong: unrecognizable name of ref of shape!"<<std::endl;
                                    }
                                }
                                else if(shSubAttri -> Name() == std::string("id") ){
                                    // Here i require that the bsdf be load ahead of shape_t
                                    // That's kind of hack may need to be changed some time later
                                    bool find = false;
                                    for(int i = 0; i < materials.size(); i++){
                                        if(materials[i].name == shSubAttri -> Value() ){
                                            find = true;
                                            matRefNames.push_back(shSubAttri -> Value() );
                                            matRefIds.push_back(i);
                                            break;
                                        }
                                    }
                                    if(find == false){
                                        std::cout<<"Wrong: missing material, ref id does not exist!"<<std::endl;
                                        return false;
                                    }
                                }
                            }   
                        }
                        else if(shSubModule -> Value() == std::string("bsdf") ){
                            bool isLoadBsdf = loadBsdf(materialsShape, shSubModule, materials.size() );
                            if(!isLoadBsdf ) return false;
                        }
                        else if(shSubModule -> Value() == std::string("emitter") ){
                            shape.isLight = true;
                            TiXmlElement* shSubEle = shSubModule -> ToElement();
                            for(TiXmlAttribute* shSubAttri = shSubEle -> FirstAttribute(); shSubAttri != 0; shSubAttri = shSubAttri -> Next() ){
                                if(shSubAttri -> Name() == std::string("type") ){
                                    if(shSubAttri -> Value() != std::string("area") ){
                                        std::cout<<"Wrong: unrecognizable type of emitter of shape!"<<std::endl;
                                        return false;
                                    }
                                }
                            }
                            
                            for(TiXmlNode* emSubModule = shSubModule -> FirstChild(); emSubModule != 0; emSubModule = emSubModule -> NextSibling()){
                                if(emSubModule -> Value() == std::string("rgb") ){
                                    TiXmlElement* emEle = emSubModule -> ToElement();
                                    for(TiXmlAttribute* emAttri = emEle->FirstAttribute(); emAttri != 0; emAttri = emAttri -> Next() ){
                                        if(emAttri -> Name() == std::string("name" ) ){
                                            if(emAttri -> Value() != std::string("radiance") ){
                                                std::cout<<"Wrong: unrecognizable name of rgb of emitter of shape!"<<std::endl;
                                                return false;
                                            }
                                        }
                                        else if(emAttri -> Name() == std::string("value") ){
                                            std::vector<float> radianceArr = parseFloatStr(emAttri -> Value() );
                                            shape.radiance[0] = radianceArr[0];
                                            shape.radiance[1] = radianceArr[1];
                                            shape.radiance[2] = radianceArr[2];               
                                        }
                                    }
                                }
                            }
                        }
                        else if(shSubModule -> Value() == std::string("transform") ){
                            TiXmlElement* shSubEle = shSubModule -> ToElement();
                            for(TiXmlAttribute* shSubAttri = shSubEle -> FirstAttribute(); shSubAttri != 0; shSubAttri = shSubAttri -> Next() ){
                                if(shSubAttri -> Name() == std::string("name") ){
                                    if(shSubAttri -> Value() != std::string("toWorld") ){
                                        std::cout<<"Wrong: unrecognizable name of transformation of shape"<<std::endl;
                                        return false;
                                    }
                                }
                            }
                            
                            for(TiXmlNode* trSubModule = shSubModule -> FirstChild(); trSubModule != 0; trSubModule = trSubModule -> NextSibling() ){
                                if(trSubModule -> Value() == std::string("scale") ){
                                    TiXmlElement* trSubEle = trSubModule -> ToElement();
                                    objTransform T;
                                    T.name = std::string("scale");
                                    T.value[0] = T.value[1] = T.value[2] = 1.0;
                                    for(TiXmlAttribute* trSubAttri = trSubEle -> FirstAttribute(); trSubAttri != 0; trSubAttri = trSubAttri -> Next() ){
                                        if(trSubAttri -> Name() == std::string("value") ){
                                            std::vector<float> scaleArr = parseFloatStr(trSubAttri -> Value() );
                                            T.value[0] = scaleArr[0];
                                        }
                                        else{
                                            std::cout<<"Wrong: unrecognizable attribute of scale of transform of shape!"<<std::endl;
                                            return false;
                                        }
                                    }
                                    TArr.push_back(T);
                                }
                                else if(trSubModule -> Value() == std::string("translate") ){
                                    TiXmlElement* trSubEle = trSubModule -> ToElement();
                                    objTransform T;
                                    T.value[0] = T.value[1] = T.value[2] = 0.0;
                                    T.name = std::string("translate");
                                    for(TiXmlAttribute* trSubAttri = trSubEle -> FirstAttribute(); trSubAttri != 0; trSubAttri = trSubAttri -> Next() ){
                                        if(trSubAttri -> Name() == std::string("x") ){
                                            std::vector<float> translateArr = parseFloatStr(trSubAttri -> Value() );
                                            T.value[0] = translateArr[0];
                                        }
                                        else if(trSubAttri -> Name() == std::string("y") ){
                                            std::vector<float> translateArr = parseFloatStr(trSubAttri -> Value() );
                                            T.value[1] = translateArr[0]; 
                                        }
                                        else if(trSubAttri -> Name() == std::string("z") ){
                                            std::vector<float> translateArr = parseFloatStr(trSubAttri -> Value() );
                                            T.value[2] = translateArr[0];  
                                        }
                                        else{
                                            std::cout<<"Wrong: unrecognizable attribute of translation of transform of shape!"<<std::endl;
                                            return false;
                                        }
                                    }
                                    TArr.push_back(T);
                                }
                            }
                        }
                    }
                    // When no materials are defined in obj 
                    if(!shape.isLight ) {
                        if(shape.mesh.materialNames.size() == 0){
                            if(materialsShape.size() + matRefNames.size() > 1){
                                std::cout<<"Wrong: more than one materials corresponds to a single shape without assigned material!"<<std::endl;
                                return false;
                            }
                            else if(materialsShape.size() + matRefNames.size() == 0){
                                std::cout<<"Warning: no materials assigned to this shapes."<<std::endl;
                            }
                            else if(materialsShape.size() == 1){
                                shape.mesh.materialNames.push_back(materialsShape[0].name );
                                shape.mesh.materialNameIds.push_back(materials.size() );
                            }
                            else{
                                shape.mesh.materialNames.push_back(matRefNames[0] );
                                shape.mesh.materialNameIds.push_back(matRefIds[0] );
                            }
                            for(int i = 0; i < shape.mesh.materialIds.size(); i++){
                                shape.mesh.materialIds[i] = 0;
                            }
                        }
                        else{
                            // Check whether all materials are covered
                            for(int i = 0; i < shape.mesh.materialNames.size(); i++){
                                std::string matName = shape.mesh.materialNames[i];
                                bool isFind = false;
                                for(int j = 0; j < materialsShape.size(); j++){
                                    if(matName == materialsShape[j].name ){
                                        isFind = true;
                                        shape.mesh.materialNameIds[i] = j + materials.size();
                                        break;
                                    }
                                }
                                if(isFind == true)
                                    continue;

                                for(int j = 0; j < matRefNames.size(); j++){
                                    if(matName == matRefNames[j] ){
                                        isFind = true;
                                        shape.mesh.materialNameIds[i] = matRefIds[j];
                                        break;
                                    }
                                }
                                if(!isFind){
                                    std::cout<<"Wrong: unrecognizable materials "<<shape.mesh.materialNames[i] <<" in .obj file."<<std::endl;
                                    return false;
                                }
                            }
                            for(int i = 0; i < shape.mesh.materialIds.size(); i++){
                                if(shape.mesh.materialIds[i] == -1)
                                    shape.mesh.materialIds[i] = shape.mesh.materialNames.size();
                            }
                        }
                    }
                    else{
                        for(int i = 0; i < shape.mesh.materialIds.size(); i++){
                           shape.mesh.materialIds[i] = 0;
                        }
                    }
                    for(int i = 0; i < materialsShape.size(); i++){
                        for(int j = 0; j < materials.size(); j++){
                            if(materials[j].name == materialsShape[i].name ){
                                std::cout<<"Wrong: repeated material Id!"<<std::endl;
                                std::cout<<materialsShape[i].name<<std::endl;
                                return false;
                            }
                        }
                        materials.push_back(materialsShape[i] );
                    }
                    doObjTransform(shape, TArr);
                    shapes.push_back(shape);
                }
                else if(module -> Value() == std::string("emitter") ){
                    TiXmlElement* emEle = module -> ToElement();
                    
                    std::string emitterStr = "";
                    // Emitter, currently only support environmental map
                    for(TiXmlAttribute* emAttri = emEle -> FirstAttribute(); emAttri != 0; emAttri = emAttri -> Next() ){
                        if(emAttri -> Name() == std::string("type") ){
                            if(emAttri -> Value() == std::string("envmap") ){
                                emitterStr = std::string("envmap");
                            }
                            else if(emAttri -> Value() == std::string("point") ){
                                emitterStr = std::string("point");
                            }
                            else if(emAttri -> Value() == std::string("flash") ){
                                emitterStr = std::string("flash");
                            }
                            else{
                                std::cout<<"Wrong: unrecognizable type of emitter "<<emAttri->Value() <<"!"<<std::endl;
                                return false; 
                            }
                        }
                        else{
                            std::cout<<"Wrong: unrecognizable attribute of emitter!"<<std::endl;
                            return false;
                        }
                    }                        
                    
                    if(emitterStr == std::string("envmap") ){
                        Envmap env;
                        for(TiXmlNode* emSubModule = module -> FirstChild(); emSubModule != 0; emSubModule = emSubModule -> NextSibling() ){
                            if(emSubModule -> Value() == std::string("string") ){
                                TiXmlElement* emSubEle = emSubModule -> ToElement();
                                for(TiXmlAttribute* emSubAttri = emSubEle -> FirstAttribute(); emSubAttri != 0; emSubAttri = emSubAttri -> Next() ){
                                    if(emSubAttri -> Name() == std::string("name") ){
                                        if(emSubAttri -> Value() != std::string("filename") ){
                                            std::cout<<"Wrong: unrecognizable name of string of emitter!"<<std::endl;
                                            return false;
                                        }
                                    }
                                    else if(emSubAttri -> Name() == std::string("value") ){
                                        std::string fileName = emSubAttri->Value();
                                        env.fileName = fileName;
                                    }
                                    else{
                                        std::cout<<"Wrong: unrecognizable attribute of string of emitter!"<<std::endl;
                                        return false;
                                    }
                                }
                            } 
                            else if(emSubModule -> Value() == std::string("float") ) {
                                TiXmlElement* emSubEle = emSubModule -> ToElement();
                                for(TiXmlAttribute* emSubAttri = emSubEle -> FirstAttribute(); emSubAttri != 0; emSubAttri = emSubAttri -> Next() ){
                                    if(emSubAttri -> Name() == std::string("name") ){
                                        if(emSubAttri -> Value() != std::string("scale") ){
                                            std::cout<<"Wrong: unrecognizable name of float of emitter!"<<std::endl;
                                            return false;
                                        }
                                    }
                                    else if(emSubAttri -> Name() == std::string("value") ){
                                        std::vector<float> scaleArr = parseFloatStr(emSubAttri -> Value() );
                                        env.scale = scaleArr[0];
                                    }
                                }
                            }
                        }
                        envmaps.push_back(env);
                    }
                    else{
                        Point p;
                        if(emitterStr == std::string("flash") ){
                            p.isFlash = true;
                        }

                        for(TiXmlNode* emSubModule = module -> FirstChild(); emSubModule != 0; emSubModule = emSubModule -> NextSibling() ){
                            if(emSubModule -> Value() == std::string("rgb") ){
                                TiXmlElement* emSubEle = emSubModule -> ToElement();
                                for(TiXmlAttribute* emSubAttri = emSubEle -> FirstAttribute(); emSubAttri != 0; emSubAttri = emSubAttri -> Next() ){
                                    if(emSubAttri -> Name() == std::string("name") ){
                                        if(emSubAttri -> Value() != std::string("intensity") ){
                                            std::cout<<"Wrong: unrecognizable name of rgb of emitter!"<<std::endl;
                                            return false;
                                        }
                                    }
                                    else if(emSubAttri -> Name() == std::string("value") ){
                                        std::vector<float> intensityArr = parseFloatStr(emSubAttri -> Value() );
                                        p.intensity.x = intensityArr[0];
                                        p.intensity.y = intensityArr[1];
                                        p.intensity.z = intensityArr[2];
                                    }
                                    else{
                                        std::cout<<"Wrong: unrecognizable attribute of  rgb of emitter!"<<std::endl;
                                        return false;
                                    }
                                }
                            }
                            else if(emSubModule -> Value() == std::string("point") ){
                                TiXmlElement* emSubEle = emSubModule -> ToElement();
                                for(TiXmlAttribute* emSubAttri = emSubEle -> FirstAttribute(); emSubAttri != 0; emSubAttri = emSubAttri -> Next() ){
                                    if(emSubAttri -> Name() == std::string("name") ){
                                        if(emSubAttri -> Value() != std::string("position") ){
                                            std::cout<<"Wrong: unrecognizable name of point of emitter"<<std::endl;
                                            return false;
                                        }
                                    }
                                    else if(emSubAttri -> Name() == std::string("value") ){
                                        std::vector<float> positionArr = parseFloatStr(emSubAttri -> Value() );
                                        p.position.x = positionArr[0];
                                        p.position.y = positionArr[1];
                                        p.position.z = positionArr[2];
                                    }
                                    else{
                                        std::cout<<"Wrong: unrecognizable attribute of position of emitter!"<<std::endl;
                                        return false;
                                    }
                                }
                            }
                            else{
                                std::cout<<"Wrong: unrecognizable attribute of emitter!"<<std::endl;
                                return false;
                            }
                        }
                        points.push_back(p);
                    }
                }
            }
        }
    }
    return true;
}

#endif
