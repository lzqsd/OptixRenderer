#include "inout/readXML.h"


bool loadShapeFromXML(std::vector<shape_t>& shapes, std::vector<material_t>& materials, 
        TiXmlNode* module, std::string fileName )
{
    shape_t  shape;
    std::vector<std::string> matRefNames;
    std::vector<int> matRefIds;
    std::vector<material_t> materialsShape;
    std::vector<objTransform> TArr;

    //Read the shape, currently only support obj files
    TiXmlElement* shEle = module -> ToElement();
    std::string shapeType;
    for(TiXmlAttribute* shAttri = shEle -> FirstAttribute(); shAttri != 0; shAttri = shAttri -> Next() ){
        if(shAttri -> Name() == std::string("type") ){
            if(shAttri -> Value() == std::string("obj") ){
                shapeType = "obj";
            }
            else if(shAttri -> Value() == std::string("ply") ){
                shapeType = "ply";
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
                    bool isLoad = false;
                    if(shapeType == "obj")
                        isLoad = objLoader::LoadObj(shape, relativePath(fileName, shSubAttri -> Value() ) );
                    else if(shapeType == "ply")
                        isLoad = plyLoader::LoadPly(shape, relativePath(fileName, shSubAttri -> Value() ) );
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
                    if(shSubAttri -> Value() != std::string("bsdf") ){ 
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
            bool isLoadBsdf = loadBsdfFromXML(materialsShape, shSubModule, fileName, materials.size() );
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
                            if(scaleArr.size() == 1){
                                T.value[0] = scaleArr[0];
                                T.value[1] = scaleArr[0];
                                T.value[2] = scaleArr[0];
                            }
                            else if(scaleArr.size() == 3){
                                T.value[0] = scaleArr[0];
                                T.value[1] = scaleArr[1];
                                T.value[2] = scaleArr[2];
                            }
                        }
                        else if(trSubAttri -> Name() == std::string("x") ){
                            std::vector<float> scaleXArr = parseFloatStr(trSubAttri -> Value() );
                            T.value[0] = scaleXArr[0];
                        }
                        else if(trSubAttri -> Name() == std::string("y") ){
                            std::vector<float> scaleYArr = parseFloatStr(trSubAttri -> Value() );
                            T.value[1] = scaleYArr[0];
                        }
                        else if(trSubAttri -> Name() == std::string("z") ){
                            std::vector<float> scaleZArr = parseFloatStr(trSubAttri -> Value() );
                            T.value[2] = scaleZArr[0];
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
                            std::cout<<"Wrong: unrecognizable attribute of translation of transformation of shape!"<<std::endl;
                            return false;
                        }
                    }
                    TArr.push_back(T);
                }
                else if(trSubModule -> Value() == std::string("rotate") ){
                    TiXmlElement* trSubEle = trSubModule -> ToElement();
                    objTransform T;
                    T.value[0] = T.value[1] = T.value[2] = T.value[3] = 0.0;
                    T.name = std::string("rotate");
                    for(TiXmlAttribute* trSubAttri = trSubEle -> FirstAttribute(); trSubAttri != 0; trSubAttri = trSubAttri -> Next() ){
                        if(trSubAttri -> Name() == std::string("x") ){
                            std::vector<float> rotateArr = parseFloatStr(trSubAttri -> Value() );
                            T.value[0] = rotateArr[0];
                        }
                        else if(trSubAttri -> Name() == std::string("y") ){
                            std::vector<float> rotateArr = parseFloatStr(trSubAttri -> Value() );
                            T.value[1] = rotateArr[0]; 
                        }
                        else if(trSubAttri -> Name() == std::string("z") ){
                            std::vector<float> rotateArr = parseFloatStr(trSubAttri -> Value() );
                            T.value[2] = rotateArr[0];  
                        }
                        else if(trSubAttri -> Name() == std::string("angle") ){
                            std::vector<float> rotateArr = parseFloatStr(trSubAttri -> Value() );
                            T.value[3] = rotateArr[0];
                        }
                        else{
                            std::cout<<"Wrong: unrecognizable attribute of translation of transformation of shape!"<<std::endl;
                            return false;
                        }
                    }
                    TArr.push_back(T); 
                }
                else{
                    std::cout<<"Wrong: unrecognizable module of transformation of shape!"<<std::endl;
                    return false;
                }
            }
        }
    }
    // When no materials are defined in obj 
    if(!shape.isLight ) { 
        
        if(materialsShape.size() == 0 && matRefNames.size() == 0) {
            std::cout<<"Warning: no materials assigned to this shapes."<<std::endl;
            for(int i = 0; i < shape.mesh.materialIds.size(); i++){
                shape.mesh.materialIds[i] = 0;
            } 
            shape.mesh.materialNames.erase(shape.mesh.materialNames.begin(), 
                    shape.mesh.materialNames.end() );
            shape.mesh.materialNameIds.erase(shape.mesh.materialNameIds.begin(), 
                    shape.mesh.materialNameIds.end() );
        }
        else{
            if(shape.mesh.materialNames.size() == 0){
                if(materialsShape.size() + matRefNames.size() > 1){
                    std::cout<<"Wrong: more than one materials corresponds to a single shape without assigned material!"<<std::endl;
                    return false;
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
                        std::cout<<"Warning: unrecognizable materials "<<shape.mesh.materialNames[i] <<" in .obj file."<<std::endl;
                        return false;
                    }
                }
                for(int i = 0; i < shape.mesh.materialIds.size(); i++){
                    if(shape.mesh.materialIds[i] == -1)
                        shape.mesh.materialIds[i] = shape.mesh.materialNames.size();
                }
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

    return true;
}
