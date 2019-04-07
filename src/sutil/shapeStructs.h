#ifndef SHAPESTRUCTS_HEADER
#define SHAPESTRUCTS_HEADER

#include <string>
#include <vector>

struct material_t {
    std::string name;
    std::string cls;

    float albedo[3];
    float specular[3];
    float fresnel;
    float roughness;
    float glossiness;
    float metallic;


    std::string albedo_texname;             // map_Kd
    std::string specular_texname;           // map_Ks
    std::string normal_texname;             // normal map 
    std::string roughness_texname;          // shiness map 
    std::string glossiness_texname;         // map_Ns
    std::string metallic_texname;           // map metallic

    material_t(){
        albedo[0] = albedo[1] = albedo[2] = 0;
        specular[0] = specular[1] = specular[2] = 0;
        roughness = 1.0;
        glossiness = 0.0;
        fresnel = 0.05;
        metallic = 0.0;
        
        name = "";
        cls = "";
        albedo_texname = "";
        specular_texname = "";
        normal_texname = "";
        roughness_texname = "";
        glossiness_texname = "";
        metallic_texname = "";
    }
};


typedef struct {
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> texcoords;
    std::vector<int> indicesP;
    std::vector<int> indicesN;
    std::vector<int> indicesT;
    std::vector<int> materialIds; // per-mesh material ID
    std::vector<std::string > materialNames; 
    std::vector<int > materialNameIds;
}mesh_t;


struct shape_t {
    std::string name;
    mesh_t mesh; 
    bool isLight;
    float radiance[3];

    shape_t(){
        name = "";
        isLight = false;
        radiance[0] = radiance[1] = radiance[2] = 0;
    }
}; 



#endif 
