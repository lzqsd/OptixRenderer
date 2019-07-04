#include "creator/createMaterial.h"

Material createMaskMaterial(Context& context, bool isAreaLight ){
    const std::string ptx_path = ptxPath( "mask.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );

    Material material = context->createMaterial();
    material->setClosestHitProgram(0, ch_program );
    material->setAnyHitProgram(1, ah_program );
    
    if ( isAreaLight == true ) {
        material["isAreaLight"] -> setInt(1); 
    }
    else{
        material["isAreaLight"] -> setInt(0);
    }
    return material;
}

