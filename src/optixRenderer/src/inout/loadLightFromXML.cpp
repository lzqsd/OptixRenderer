#include "inout/readXML.h"

bool loadLightFromXML(std::vector<Envmap>& envmaps, 
        std::vector<Point>& points, TiXmlNode* module, std::string fileName )
{
    
    TiXmlElement* emEle = module -> ToElement();
    
    std::string emitterStr = "";
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
                        std::string envName = relativePath(fileName, emSubAttri->Value() );
                        env.fileName = envName;
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
    return true;
}
