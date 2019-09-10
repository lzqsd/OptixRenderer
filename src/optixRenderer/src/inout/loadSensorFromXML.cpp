#include "inout/readXML.h"

bool loadSensorFromXML(CameraInput& Camera, TiXmlNode* module)
{ 
    // Load the camera parameters  
    TiXmlElement* seEle = module -> ToElement();
    for(TiXmlAttribute* seAttri = seEle -> FirstAttribute(); seAttri != 0; seAttri = seAttri->Next() ){
        if(seAttri -> Name() == std::string("type") ){
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
                std::cout<<"Wrong: unrecognizable type of sensor "<<seAttri -> Value()<<" !"<<std::endl;
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
                    std::string integerName;
                    int integerValue;
                    for(TiXmlAttribute* saSubAttri = saSubEle -> FirstAttribute(); saSubAttri != 0; saSubAttri = saSubAttri-> Next() ){
                        if(saSubAttri -> Name() == std::string("value") ){
                            std::vector<float> intValues = parseFloatStr(saSubAttri -> Value() );
                            integerValue = int(intValues[0] );
                        }
                        else if(saSubAttri -> Name() == std::string("name") ){
                            integerName = saSubAttri -> Value();
                        }
                    }
                    if(integerName == std::string("sampleCount") ){
                        Camera.sampleNum = integerValue;
                    }
                    else if(integerName == std::string("maxIteration") ){
                        if(Camera.sampleType == std::string("independent") ){
                            std::cout<<"Wrong: independent sampler does not have maxIteration variable!"<<std::endl;
                            return false;
                        }
                        Camera.adaptiveSampler.maxIteration = integerValue;
                    }
                    else{
                        std::cout<<"Wrong: unrecognizable name of integer of sensor!"<<std::endl;
                        return false;
                    }
                }
                else if(saSubModule -> Value() == std::string("float") ){
                    TiXmlElement* saSubEle = saSubModule -> ToElement() ;
                    for(TiXmlAttribute* saSubAttri = saSubEle -> FirstAttribute(); saSubAttri != 0; saSubAttri = saSubAttri -> Next() ){

                        if(saSubAttri -> Name() == std::string("value") ){
                            std::vector<float> noiseThreshold = parseFloatStr(saSubAttri -> Value() );
                            Camera.adaptiveSampler.noiseThreshold = noiseThreshold[0];
                        }
                        else if(saSubAttri -> Name() == std::string("name") ){
                            if(saSubAttri -> Value() != std::string("noiseThreshold") ){
                                std::cout<<"Wrong: unrecognizable name of float of sensor "<<saSubAttri -> Value()<<" !"<<std::endl;
                                return false;
                            }
                            if(Camera.sampleType == std::string("independent") ){
                                std::cout<<"Wrong: independent sampler does not have noiseThreshold variable!"<<std::endl;
                                return false;
                            }
                        }
                    }
                }
            }
        }
        else{
            std::cout<<"Wrong: unrecognizable module "<<seSubModule -> Value()<<" of sensor!"<<std::endl;
            return false;
        }
    }
    return true;
}
