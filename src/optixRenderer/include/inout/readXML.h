#ifndef READXML_HEADER
#define READXML_HEADER

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_aabb_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>
#include <vector>
#include <string>
#include <iostream>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <cmath>
#include <algorithm>

#include "sutil.h"
#include "tinyxml/tinyxml.h"
#include "tinyobjloader/objLoader.h"
#include "tinyplyloader/plyLoader.h"
#include "lightStructs.h"

#include "structs/cameraInput.h"
#include "inout/relativePath.h"
#include "structs/constant.h"


std::vector<float> parseFloatStr(const std::string& str);

struct objTransform{
    std::string name;
    float value[4];
};

bool doObjTransform(shape_t& shape, std::vector<objTransform>& TArr);

bool loadSensorFromXML(CameraInput& Camera, TiXmlNode* module );

bool loadBsdfFromXML(std::vector<material_t>& materials, TiXmlNode* module, std::string fileName, int offset = 0);

bool loadLightFromXML(std::vector<Envmap>& envmaps, std::vector<Point>& points, 
        TiXmlNode* module, std::string fileName );

bool loadShapeFromXML(std::vector<shape_t>& shapes, std::vector<material_t>& materials, 
        TiXmlNode* module, std::string fileName );

bool readXML(std::string fileName, 
        std::vector<shape_t>& shapes, 
        std::vector<material_t>& materials, 
        CameraInput& Camera, 
        std::vector<Envmap>& envmaps, 
        std::vector<Point>& points);

#endif
