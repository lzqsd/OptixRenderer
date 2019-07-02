#ifndef PLYLOADER_HEADER
#define PLYLOADER_HEADER


#define TINYPLY_IMPLEMENTATION 
#include "tinyply.h"
#include "shapeStructs.h"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <assert.h>
#include <cstring>

namespace plyLoader{

bool LoadPly(shape_t& shape, const std::string& filename);

}


#endif
