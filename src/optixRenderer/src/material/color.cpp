#include "creator/createMaterial.h"

float srgb2rgb(float c){
    if(c <= 0.0405){
        return c / 12.92;
    }
    else{
        return pow(float((c + 0.055) / 1.055 ), 2.4f);
    }
}

float rgb2srgb(float c){
    if(c <= 0.0031308){
        return 12.92 * c;
    }
    else{
        return 1.055 * pow(c, 1.0f/2.4f) - 0.055;
    }
}
