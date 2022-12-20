#include "creator/createGeometry.h"

void createGeometry(
        Context& context,
        const std::vector<shape_t>& shapes,
        const std::vector<material_t>& materials,
        int mode
        )
{
    // Create geometry group;
    GeometryGroup geometry_group = context->createGeometryGroup();
    geometry_group->setAcceleration( context->createAcceleration( "Trbvh" ) );

    std::cout<<"Total number of materials: "<<materials.size()<<std::endl;

    // Area Light
    context["isAreaLight"] -> setInt(0);
    for(int i = 0; i < shapes.size(); i++) {
        
        const std::string path = ptxPath( "triangle_mesh.cu" );
        optix::Program bounds_program = context->createProgramFromPTXFile( path, "mesh_bounds" );
        optix::Program intersection_program = context->createProgramFromPTXFile( path, "mesh_intersect" );
        
        shape_t  shapeLarge = shapes[i];
        std::vector<shape_t > shapeArr;
        shapeArr.push_back(shapeLarge );

        if(shapeArr.size() != 1){
            std::cout<<"Warning: the mesh "<<shapeLarge.name<<" is too large and has been splited into "<<shapeArr.size()<<" parts"<<std::endl;
        }

        for(int j = 0; j < shapeArr.size(); j++){
            shape_t shape = shapeArr[j];
            int vertexNum = shape.mesh.positions.size() / 3;
            int normalNum = shape.mesh.normals.size() / 3;
            int texNum = shape.mesh.texcoords.size() / 2;
            int faceNum = shape.mesh.indicesP.size() / 3;
         
            bool hasNormal = (normalNum != 0);
            bool hasTex = (texNum != 0);
         
            Buffer vertexBuffer = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, vertexNum );
            Buffer normalBuffer = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, normalNum );
            Buffer texBuffer = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT2, texNum );
            Buffer faceBuffer = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_INT3, faceNum );
            Buffer faceVtBuffer = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_INT3, faceNum );
            Buffer faceVnBuffer = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_INT3, faceNum );
            Buffer materialBuffer = context -> createBuffer(RT_BUFFER_INPUT, RT_FORMAT_INT, faceNum );
         
            float* vertexPt = reinterpret_cast<float*>(vertexBuffer -> map() );
            int* facePt = reinterpret_cast<int32_t*>(faceBuffer -> map() );
            int* faceVtPt = reinterpret_cast<int32_t*>( faceVtBuffer -> map() );
            int* faceVnPt = reinterpret_cast<int32_t*>( faceVnBuffer -> map() );
            float* texPt = reinterpret_cast<float*> (hasTex ? texBuffer -> map() : 0);
            float* normalPt = reinterpret_cast<float*>( hasNormal ? normalBuffer -> map() : 0);
            int* materialPt = reinterpret_cast<int32_t*>(materialBuffer -> map() );

            for(int i = 0; i < vertexNum * 3; i++){
                vertexPt[i] = shape.mesh.positions[i];
            }
            for(int i = 0; i < normalNum * 3; i++){
                normalPt[i] = shape.mesh.normals[i];
            }
            for(int i = 0; i < texNum * 2; i++){
                texPt[i] = shape.mesh.texcoords[i];
            }
            for(int i = 0; i < faceNum * 3; i++){
                facePt[i] = shape.mesh.indicesP[i];
                faceVtPt[i] = shape.mesh.indicesT[i];
                faceVnPt[i] = shape.mesh.indicesN[i];
            }
            for(int i = 0; i < faceNum; i++ ) {
                materialPt[i] = shape.mesh.materialIds[i];
            }
         
            vertexBuffer -> unmap();
            faceBuffer -> unmap();
            faceVnBuffer -> unmap();
            faceVtBuffer -> unmap();
            if(hasNormal){
                normalBuffer -> unmap();
            }
            if(hasTex){
                texBuffer -> unmap();
            }
            materialBuffer -> unmap();
         
            Geometry geometry = context -> createGeometry();
            geometry[ "vertex_buffer" ] -> setBuffer(vertexBuffer );
            geometry[ "normal_buffer" ] -> setBuffer(normalBuffer );
            geometry[ "texcoord_buffer" ] -> setBuffer(texBuffer );
            geometry[ "index_buffer" ] -> setBuffer(faceBuffer );
            geometry[ "index_normal_buffer" ] -> setBuffer(faceVnBuffer );
            geometry[ "index_tex_buffer" ] -> setBuffer(faceVtBuffer );
            geometry[ "material_buffer"] -> setBuffer(materialBuffer );
            geometry -> setPrimitiveCount( faceNum );
            geometry->setBoundingBoxProgram ( bounds_program );
            geometry->setIntersectionProgram( intersection_program );
         
            // Currently only support diffuse material and area light 
            std::vector<Material> optix_materials;
            if(!shape.isLight ){
                if(mode == 0){
                    for(int i = 0; i < shape.mesh.materialNames.size(); i++){
                        Material mat; 

                        if(materials.size() == 0){
                            optix_materials.push_back(createDefaultMaterial(context) );
                            continue;
                        }
                        material_t matInput = materials[shape.mesh.materialNameIds[i] ];
                        if(matInput.cls == std::string("diffuse") ){
                            mat = createDiffuseMaterial(context, matInput );
                        }
                        else if(matInput.cls == std::string("microfacet") ){
                            mat = createMicrofacetMaterial(context, matInput );
                        }
                        else if(matInput.cls == std::string("phong") ){
                            mat = createPhongMaterial(context, matInput );
                        }
                        else if(matInput.cls == std::string("dielectric") ){
                            mat = createDielectricMaterial(context, matInput );
                        }
                        else if(matInput.cls == std::string("conductor") ){
                            mat = createConductorMaterial(context, matInput );
                        }
                        optix_materials.push_back(mat);
                    }
                    optix_materials.push_back(createDefaultMaterial(context ) );
                }
                else if(mode == 1){
                    // Output the albedo value 
                    material_t matEmpty;
                    for(int i = 0; i < shape.mesh.materialNames.size(); i++){
                        if(materials.size() == 0){
                            Material mat = createAlbedoMaterial(context, matEmpty );
                            optix_materials.push_back(mat);
                            continue;
                        }
                        material_t matInput = materials[shape.mesh.materialNameIds[i] ]; 
                        Material mat = createAlbedoMaterial(context, matInput);
                        optix_materials.push_back(mat);
                    }
                    optix_materials.push_back(createAlbedoMaterial(context, matEmpty ) );
                }
                else if(mode == 2){
                    // Output the normal value 
                    material_t matEmpty; 
                    for(int i = 0; i < shape.mesh.materialNames.size(); i++){
                        if(materials.size() == 0){
                            Material mat = createNormalMaterial(context, matEmpty );
                            optix_materials.push_back(mat);
                            continue;
                        }
                        material_t matInput = materials[shape.mesh.materialNameIds[i] ]; 
                        Material mat = createNormalMaterial(context, matInput);
                        optix_materials.push_back(mat);
                    }
                    optix_materials.push_back(createNormalMaterial(context, matEmpty ) ); 
                }
                else if(mode == 3){
                    // Output the roughness value 
                    material_t matEmpty; 
                    for(int i = 0; i < shape.mesh.materialNames.size(); i++){
                        if(materials.size() == 0){
                            Material mat = createRoughnessMaterial(context, matEmpty );
                            optix_materials.push_back(mat);
                            continue;
                        }
                        material_t matInput = materials[shape.mesh.materialNameIds[i] ]; 
                        Material mat = createRoughnessMaterial(context, matInput);
                        optix_materials.push_back(mat);
                    }
                    optix_materials.push_back(createRoughnessMaterial(context, matEmpty ) );  
                }
                else if(mode == 4){
                    // Output the mask
                    for(int i = 0; i < shape.mesh.materialNames.size(); i++){
                        Material mat = createMaskMaterial(context, false );
                        optix_materials.push_back(mat );
                    }
                    optix_materials.push_back(createMaskMaterial(context, false ) );
                }
                else if(mode == 5){
                    // Output the depth  
                    for(int i = 0; i < shape.mesh.materialNames.size(); i++){
                        Material mat = createDepthMaterial(context );
                        optix_materials.push_back(mat);
                    }
                    optix_materials.push_back(createDepthMaterial(context ) );
                }
                else if(mode == 6){
                    // Output the metallic  
                    material_t matEmpty; 
                    for(int i = 0; i < shape.mesh.materialNames.size(); i++){
                        if(materials.size() == 0){
                            Material mat = createMetallicMaterial(context, matEmpty );
                            optix_materials.push_back(mat );
                            continue;
                        }
                        material_t matInput = materials[shape.mesh.materialNameIds[i] ]; 
                        Material mat = createMetallicMaterial(context, matInput);
                        optix_materials.push_back(mat );
                    }
                    optix_materials.push_back(createMetallicMaterial(context, matEmpty ) );
                }
                else{
                    std::cout<<"Wrong: Unrecognizable mode!"<<std::endl;
                    exit(1);
                }
            }
            else{
                context["isAreaLight"] -> setInt(1);
                material_t matInput;
                if(mode == 0){
                    // Render image 
                    Material mat = createAreaLight(context, shape );
                    optix_materials.push_back(mat);
                } 
                else if(mode == 1){
                    optix_materials.push_back(createAlbedoMaterial(context, matInput ) );
                }
                else if(mode == 2){
                    // Render normal 
                    Material mat = createNormalMaterial(context, matInput );
                    optix_materials.push_back(mat );
                }
                else if(mode == 3){
                    optix_materials.push_back(createRoughnessMaterial(context, matInput ) );
                }
                else if(mode == 4){
                    // Render Mask 
                    Material mat = createMaskMaterial(context, true);
                    optix_materials.push_back(mat );
                }
                else if(mode == 5){
                    // Render Depth
                    Material mat = createDepthMaterial(context);
                    optix_materials.push_back(mat );
                }
                else{
                    Material mat = createBlackMaterial(context );
                    optix_materials.push_back(mat);
                }
            }
         
            GeometryInstance geom_instance = context->createGeometryInstance(
                    geometry,
                    optix_materials.begin(),
                    optix_materials.end()
                    );
            geometry_group -> addChild(geom_instance );
        }
    }
    context["top_object"] -> set( geometry_group);
}
