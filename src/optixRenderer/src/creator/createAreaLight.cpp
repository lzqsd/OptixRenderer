#include "creator/createAreaLight.h"

int createAreaLightsBuffer(
        Context& context, 
        const std::vector<shape_t> shapes
    )
{
    std::vector<float> cdfArr;
    std::vector<float> pdfArr;
    std::vector<areaLight> lights;
    float sum = 0;

    // Assign all the area lights
    for(int i = 0; i < shapes.size(); i++){
        if(shapes[i].isLight ){
            
            shape_t shape = shapes[i];
            int faceNum = shape.mesh.indicesP.size() /3;
            float3 radiance = make_float3(shape.radiance[0], shape.radiance[1], shape.radiance[2]);

            std::vector<int>& faces = shape.mesh.indicesP;
            std::vector<float>& vertices = shape.mesh.positions;

            for(int i = 0; i < faceNum; i++){
                int vId1 = faces[3*i], vId2 = faces[3*i+1], vId3 = faces[3*i+2];
                float3 v1 = make_float3(vertices[3*vId1], vertices[3*vId1+1], vertices[3*vId1+2]);
                float3 v2 = make_float3(vertices[3*vId2], vertices[3*vId2+1], vertices[3*vId2+2]);
                float3 v3 = make_float3(vertices[3*vId3], vertices[3*vId3+1], vertices[3*vId3+2]);

                float3 cproduct = cross(v2 - v1, v3 - v1);
                float area = 0.5 * sqrt(dot(cproduct, cproduct) );

                sum += area * length(radiance );
                cdfArr.push_back(sum);
                pdfArr.push_back(area * length(radiance) );
                
                areaLight al;
                al.vertices[0] = v1;
                al.vertices[1] = v2;
                al.vertices[2] = v3;
                al.radiance = radiance;
                lights.push_back(al);
            }
        }
    }
    
    // Computs the pdf and the cdf
    for(int i = 0; i < cdfArr.size(); i++){
        cdfArr[i] = cdfArr[i] / sum;
        pdfArr[i] = pdfArr[i] / sum;
    }

    context["areaSum"] -> setFloat(sum );
    Buffer lightBuffer = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_USER, lights.size() );
    lightBuffer->setElementSize( sizeof( areaLight) );
    memcpy( lightBuffer->map(), (char*)&lights[0], sizeof(areaLight) * lights.size() );
    lightBuffer->unmap(); 
    context["areaLights"]->set(lightBuffer );
    
    context["areaTriangleNum"] -> setInt(lights.size() );

    Buffer cdfBuffer = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT, cdfArr.size() );
    memcpy(cdfBuffer -> map(), &cdfArr[0], sizeof(float) * cdfArr.size() );
    cdfBuffer -> unmap();
    
    Buffer pdfBuffer = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT, pdfArr.size() );
    memcpy(pdfBuffer -> map(), &pdfArr[0], sizeof(float) * pdfArr.size() );
    pdfBuffer -> unmap();
    
    context["areaLightCDF"] -> set(cdfBuffer );
    context["areaLightPDF"] -> set(pdfBuffer );
}

Material createAreaLight(Context& context, shape_t shape){
    const std::string ptx_path = ptxPath( "areaLight.cu" );
    Program ch_program = context->createProgramFromPTXFile( ptx_path, "closest_hit_radiance" );
    Program ah_program = context->createProgramFromPTXFile( ptx_path, "any_hit_shadow" );
    
    Material material = context->createMaterial();
    material->setClosestHitProgram( 0, ch_program );
    material->setAnyHitProgram( 1, ah_program );
    
    material["radiance"] -> setFloat(make_float3(shape.radiance[0], 
                shape.radiance[1], shape.radiance[2]) );
    return material;
}
