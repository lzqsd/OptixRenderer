#include "creator/createCamera.h"

void createCamera(Context& context, CameraInput& camInput){

    const float3 camera_eye(make_float3(camInput.origin[0],
                camInput.origin[1], camInput.origin[2]) );
    const float3 camera_lookat(make_float3(camInput.target[0],
                camInput.target[1], camInput.target[2]) );
    const float3 camera_up( optix::make_float3(camInput.up[0],
                camInput.up[1], camInput.up[2]) );
    float vfov = 0;
    if(camInput.isAxisX == false)
        vfov = camInput.fov;
    else{
        float t = tan(camInput.fov /2 / 180.0 * 3.1415926f); 
        float new_t = t / float(camInput.width) * float(camInput.height);
        vfov = 2 * atan(new_t) * 180.0 / 3.1415926f;
    }
    sutil::Camera camera( camInput.width, camInput.height, 
            &camera_eye.x, &camera_lookat.x, &camera_up.x,
            context["eye"], context["cameraU"], context["cameraV"], context["cameraW"], 
            vfov);
}
