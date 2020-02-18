#include "creator/createContext.h"

void destroyContext(Context& context )
{
    if( context ){
        context->destroy();
        context = 0;
    }
}

unsigned createContext( 
        Context& context, 
        bool use_pbo, 
        std::string cameraType,
        unsigned width, unsigned height, 
        unsigned envWidth, unsigned envHeight, 
        unsigned int mode,
        unsigned int sampleNum, 
        unsigned maxDepth, 
        unsigned rr_begin_depth)
{
    unsigned scale = 1;

    // Set up context
    context = Context::create();
    context->setRayTypeCount( 2 );
    context->setEntryPointCount( 1 );
    context->setStackSize( 600 );

    context["rr_begin_depth"] -> setUint(rr_begin_depth);
    context["envWidth"] -> setUint(envWidth );
    context["envHeight"] -> setUint(envHeight );
    
    float* ib = NULL; 
    Buffer intensityBuffer = context -> createBuffer(
            RT_BUFFER_INPUT_OUTPUT, RT_FORMAT_FLOAT3, 
            envWidth*width, envHeight*height );
    ib = reinterpret_cast<float*>(intensityBuffer -> map() );
    for(int i = 0; i < envWidth * envHeight * width * height * 3; i++){
        ib[i] = 0;
    } 
    
    intensityBuffer -> unmap(); 
    context["intensity_buffer"]->set( intensityBuffer );
    
    maxDepth = 7;
    unsigned sqrt_num_samples = sqrt(sampleNum );
    std::cout<<"sqrt_num_samples: "<<sqrt_num_samples<<std::endl;

    context["max_depth"]->setInt(maxDepth);
    context["sqrt_num_samples"] -> setUint(sqrt_num_samples);

    // Ray generation program 
    std::cout<<"light_camera.cu"<<std::endl;
    std::string ptx_path( ptxPath( "light_camera.cu" ) );
    Program ray_gen_program = context->createProgramFromPTXFile( ptx_path, "pinhole_camera" );
    context->setRayGenerationProgram( 0, ray_gen_program );
    
    // Exception program
    Program exception_program = context->createProgramFromPTXFile( ptx_path, "exception" );
    context->setExceptionProgram( 0, exception_program );
    context["bad_color"]->setFloat( 0.0f, 0.0f, 0.0f );
    
    context["cameraMode"] -> setInt(0);

    // Missing Program 
    std::string miss_path( ptxPath("envmap.cu") );
    if(mode == 0 || mode == 7){
        Program miss_program = context->createProgramFromPTXFile(miss_path, "envmap_miss");
        context->setMissProgram(0, miss_program);
    }
    else{
        Program miss_program = context->createProgramFromPTXFile(miss_path, "miss");
        context->setMissProgram(0, miss_program);
    }

    // Set light sampling program 
    const std::string area_path = ptxPath("areaLight.cu");
    Program sampleAreaLight = context -> createProgramFromPTXFile(area_path, "sampleAreaLight");
    context["sampleAreaLight"]->set(sampleAreaLight );

    const std::string env_path = ptxPath("envmap.cu");
    Program sampleEnvLight = context -> createProgramFromPTXFile(env_path, "sampleEnvironmapLight");
    context["sampleEnvironmapLight"]->set(sampleEnvLight );
        
    return 1;
}
