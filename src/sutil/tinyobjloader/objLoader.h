#ifndef OBJLOADER_HEADER
#define OBJLOADER_HEADER 

#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cctype>
#include <fstream>
#include <sstream>
#include <iostream>
#include "shapeStructs.h"

namespace objLoader {

bool LoadObj(shape_t& shape, const std::string& filename);

}


#endif

