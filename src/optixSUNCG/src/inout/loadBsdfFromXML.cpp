#include "inout/readXML.h"

bool loadBsdfFromXML(std::vector<material_t>& materials, TiXmlNode* module, std::string fileName, int offset )
{
    // Load materials
    material_t mat;

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
                                    mat.albedo_texname = relativePath(fileName, std::string(strAttri -> Value()) );
                                else if(texName == std::string("normal") ){
                                    mat.normal_texname = relativePath(fileName, std::string(strAttri -> Value() ) );
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
                                    mat.albedo_texname = relativePath(fileName, std::string(strAttri -> Value()) );
                                }
                                else if(texName == std::string("specularReflectance") ){
                                    mat.specular_texname = relativePath(fileName, std::string(strAttri -> Value()) );
                                }
                                else if(texName == std::string("alpha") ){
                                    mat.glossiness_texname = relativePath(fileName, std::string(strAttri -> Value() ) );
                                }
                                else if(texName == std::string("normal") ){
                                    mat.normal_texname = relativePath(fileName, std::string(strAttri -> Value() ) );
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
            else if(matAttri -> Value() == std::string("dielectric") ){
                mat.cls = std::string("dielectric");
                
                // Read the variable for dielectric BRDF
                // Only support rgb radiance and texture radiance
                for(TiXmlNode* matSubModule = module -> FirstChild(); matSubModule != 0; matSubModule = matSubModule -> NextSibling() ){
                    if(matSubModule -> Value() == std::string("rgb") ){

                        TiXmlElement* matSubEle = matSubModule -> ToElement();
                        std::string rgbName;
                        for(TiXmlAttribute* matSubAttri = matSubEle -> FirstAttribute(); matSubAttri != 0; matSubAttri = matSubAttri -> Next() ){
                            if(matSubAttri -> Name() == std::string("name") ){
                                if(matSubAttri -> Value() != std::string("specularReflectance") ){
                                    rgbName = std::string("specularReflectance" );
                                }
                                else if(matSubAttri -> Value() != std::string("specularTransmittance") ){
                                    rgbName = std::string("specularTransmittance");
                                }
                                else{
                                    std::cout<<"Wrong: unrecognizable name of rgb of dielectric BRDF!"<< matSubAttri->Value() <<std::endl;
                                    std::cout<<matSubAttri -> Value() <<std::endl;
                                    return false;
                                }
                            }
                        }
                        for(TiXmlAttribute* matSubAttri = matSubEle -> FirstAttribute(); matSubAttri != 0; matSubAttri = matSubAttri -> Next() ){
                            if(matSubAttri -> Name() == std::string("value") ){
                                if(rgbName == std::string("specularReflectance") ){
                                    std::vector<float> reflectanceArr = parseFloatStr(matSubAttri -> Value() );
                                    mat.specular[0] = reflectanceArr[0];
                                    mat.specular[1] = reflectanceArr[1];
                                    mat.specular[2] = reflectanceArr[2];
                                }
                                else if(rgbName == std::string("specularTransmittance") ){
                                    std::vector<float> transmittanceArr = parseFloatStr(matSubAttri -> Value() );
                                    mat.transmittance[0] = transmittanceArr[0];
                                    mat.transmittance[1] = transmittanceArr[1];
                                    mat.transmittance[2] = transmittanceArr[2];
                                }
                            }
                        }
                    }
                    else if(matSubModule -> Value() == std::string("float") ){
                        
                        TiXmlElement* matSubEle = matSubModule -> ToElement();
                        std::string floatName;
                        for(TiXmlAttribute* matSubAttri = matSubEle -> FirstAttribute(); matSubAttri != 0; matSubAttri = matSubAttri -> Next() ){
                            if(matSubAttri -> Name() == std::string("name") ){
                                if(matSubAttri -> Value() == std::string("intIOR") ){
                                    floatName = std::string("intIOR" );
                                }
                                else if(matSubAttri -> Value() == std::string("extIOR") ){
                                    floatName = std::string("extIOR");
                                }
                                else{
                                    std::cout<<"Wrong: unrecognizable name of float of dielectric BRDF!"<< matSubAttri->Value() <<std::endl;
                                    std::cout<<matSubAttri -> Value() <<std::endl;
                                    return false;
                                }
                            }
                        }
                        for(TiXmlAttribute* matSubAttri = matSubEle -> FirstAttribute(); matSubAttri != 0; matSubAttri = matSubAttri -> Next() ){
                            if(matSubAttri -> Name() == std::string("value") ){
                                if(floatName == std::string("intIOR") ){
                                    std::vector<float> intIORArr = parseFloatStr(matSubAttri -> Value() );
                                    mat.intIOR = intIORArr[0];
                                }
                                else if(floatName == std::string("extIOR") ){
                                    std::vector<float> extIORArr = parseFloatStr(matSubAttri -> Value() );
                                    mat.extIOR = extIORArr[0];
                                }
                            }
                        }
                        
                    }
                    else if(matSubModule -> Value() == std::string("texture") ){

                        std::string texName;
                        TiXmlElement* matSubEle = matSubModule -> ToElement();
                        for(TiXmlAttribute* matSubAttri = matSubEle -> FirstAttribute(); matSubAttri != 0; matSubAttri = matSubAttri -> Next() ){
                            if(matSubAttri -> Name() == std::string("name") ) {
                                if(matSubAttri -> Value() == std::string("normal") ){
                                    texName = matSubAttri -> Value();
                                }
                                else{
                                    std::cout<<"Wrong: unrecognizable name "<<matSubAttri -> Value() <<" of texture of dielectric BRDF!"<<std::endl;
                                    return false;
                                }
                            }
                        }
                    
                        TiXmlNode* strModule = matSubModule -> FirstChild();
                        TiXmlElement* strEle = strModule -> ToElement();
                        for(TiXmlAttribute* strAttri = strEle->FirstAttribute(); strAttri != 0; strAttri = strAttri->Next() ){
                            
                            if(strAttri -> Name() == std::string("value") ){
                                if(texName == std::string("normal") ){
                                    mat.normal_texname = relativePath(fileName, std::string(strAttri -> Value() ) );
                                }
                            }
                            else if(strAttri -> Name() == std::string("name") ){
                                if(strAttri -> Value() != std::string("filename") ){
                                    std::cout<<"Wrong: unrecognizable name of texture of dielectric BRDF!"<<std::endl;
                                    return false;
                                }
                            }
                        }
                    }
                    else{
                        std::cout<<"Wrong: unrecognizable module of dielectric BRDF!"<<std::endl;
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
                                    mat.albedo_texname = relativePath(fileName, std::string(strAttri -> Value()) );
                                }
                                else if(texName == std::string("normal") ){
                                    mat.normal_texname = relativePath(fileName, std::string(strAttri -> Value() ) );
                                }
                                else if(texName == std::string("roughness") ){
                                    mat.roughness_texname = relativePath(fileName, std::string(strAttri -> Value() ) );
                                }
                                else if(texName == std::string("metallic") ){
                                    mat.metallic_texname = relativePath(fileName, std::string(strAttri -> Value() ) );
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
