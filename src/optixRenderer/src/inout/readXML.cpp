#include "inout/readXML.h"


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


bool doObjTransform(shape_t& shape, std::vector<objTransform>& TArr)
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
            int vertexNum = int(shape.mesh.positions.size() / 3 );
            for(int i = 0; i < vertexNum; i++){
                for(int r = 0; r < 3; r++){
                    shape.mesh.positions[3*i + r] *= T.value[r];    
                }
            } 

            int normalNum = int(shape.mesh.normals.size() / 3);
            for(int i = 0; i < normalNum; i++){
                float norm = 0;
                for(int r = 0; r < 3; r++){
                    shape.mesh.normals[3*i + r] *= T.value[r];
                    norm += shape.mesh.normals[3*i + r] * 
                        shape.mesh.normals[3*i + r];
                }
                norm = sqrt(norm );
                for(int r = 0; r < 3; r++){
                    shape.mesh.normals[3*i+r ] /= norm;
                }
            }
        }
        else if(T.name == std::string("rotate") ){
            // When looking at the axis, it will rotate the object counter clockwise 
            double theta =  T.value[3] / 180.0 * PI;
            float x = T.value[0];
            float y = T.value[1];
            float z = T.value[2];
            double axis[3];
            axis[0] = x; axis[1] = y; axis[2] = z;
            double axisNorm = 0;
            for(int i = 0; i < 3; i++){
                axisNorm += axis[i] * axis[i];
            }
            axisNorm = sqrt(axisNorm );
            for(int i = 0; i < 3; i++){
                axis[i] /= std::max(axisNorm, 1e-6);
            }
                
            double rotMat[3][3];
            double rotMat_0[3][3], rotMat_1[3][3];
            for(int r = 0; r < 3; r++){
                for(int c = 0; c < 3; c++){
                    rotMat[r][c] = 0.0;
                    rotMat_0[r][c] = 0.0;
                    rotMat_1[r][c] = 0.0;
                }
            }

            rotMat_0[0][0] = rotMat_0[1][1] = rotMat_0[2][2] = 1.0;
            rotMat_1[0][1] = -z; rotMat_1[0][2] = y;
            rotMat_1[1][0] = z; rotMat_1[1][2] = -x;
            rotMat_1[2][0] = -y; rotMat_1[2][1] = x;
            for(int r = 0; r < 3; r++){
                for(int c = 0; c < 3; c++){
                    rotMat[r][c] = cos(theta) * rotMat_0[r][c] + 
                        sin(theta) * rotMat_1[r][c] + (1 - cos(theta) ) * axis[r] * axis[c];
                }
            } 

            // Rotate 3D points 
            int vertexNum = shape.mesh.positions.size() / 3;
            for(int i = 0; i < vertexNum; i++ ){
                float newPosition[3];
                for(int r = 0; r < 3; r++){
                    newPosition[r] = 0;
                    for(int c = 0; c < 3; c++){
                        newPosition[r] += rotMat[r][c] * shape.mesh.positions[3*i + c];
                    }
                }
                for(int r = 0; r < 3; r++){
                    shape.mesh.positions[3*i + r] = newPosition[r];
                }
            }
            
            // Rotate Normals 
            int normalNum = shape.mesh.normals.size() / 3;
            for(int i = 0; i < normalNum; i++ ){
                float newNormal[3];
                for(int r = 0; r < 3; r++){
                    newNormal[r] = 0;
                    for(int c = 0; c < 3; c++){
                        newNormal[r] += rotMat[r][c] * shape.mesh.normals[3*i + c];
                    }
                }
                for(int r=0; r < 3; r++){
                    shape.mesh.normals[3*i + r] = newNormal[r];
                }
            }
        }
        else{
            std::cout<<"Wrong: unrecognizable transform operatioin!"<<std::endl;
            return false;
        }
    }
    return true;
}



bool readXML(std::string fileName, 
        std::vector<shape_t>& shapes, 
        std::vector<material_t>& materials, 
        CameraInput& Camera, 
        std::vector<Envmap>& envmaps, 
        std::vector<Point>& points)
{
    shapes.erase(shapes.begin(), shapes.end() );
    materials.erase(materials.begin(), materials.end() );

    std::vector<shape_t> shapesAll;

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
                    bool isLoadSensor = loadSensorFromXML(Camera, module );
                    if(! isLoadSensor ) return false;
                }
                else if(module -> Value() == std::string("bsdf") ){
                    bool isLoadBsdf = loadBsdfFromXML(materials, module, fileName );
                    if(!isLoadBsdf) return false;
                }
                else if(module -> Value() == std::string("shape") ){
                    bool isLoadShape = loadShapeFromXML(shapes, materials, module, fileName );
                    if(!isLoadShape ) return false;
                }
                else if(module -> Value() == std::string("emitter") ){
                    bool isLoadLight = loadLightFromXML(envmaps, points, module, fileName );
                    if(!isLoadLight ) return false; 
                }
            }
        }
    }
    return true;
}
