#ifndef ENVMAP_HEADER
#define ENVMAP_HEADER

#include <string>

class Envmap
{
public:
    std::string fileName;
    float scale;
    Envmap(){
        scale = 1;
    }
    Envmap(std::string fn):fileName(fn){
        scale = 1;
    }
};

#endif
