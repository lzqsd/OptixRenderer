#include "creator/createPointFlashLight.h"


void createPointLight(Context& context, std::vector<Point>& points){ 
    if(points.size() == 0){
        context["isPointLight"] -> setInt(0);
    }
    else{
        context["isPointLight"] -> setInt(1);
    }
    Buffer lightBuffer = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_USER, points.size() );
    lightBuffer -> setElementSize( sizeof(Point) );
    memcpy( lightBuffer -> map(), (char*)&points[0], sizeof(Point) * points.size() );
    lightBuffer -> unmap();
    context["pointLights"] -> set(lightBuffer );
    context["pointLightNum"] -> setInt(points.size() ); 
}

void updatePointLight(Context& context, std::vector<Point>& points){
    if(points.size() != 0){
        Buffer lightBuffer = context["pointLights"] -> getBuffer();
        memcpy(lightBuffer -> map(), (char*)&points[0], sizeof(Point)*points.size()  );
        lightBuffer -> unmap();
    }
}
