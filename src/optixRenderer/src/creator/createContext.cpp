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
    
    if(width * height < 2700){
        scale = (unsigned )(sqrt(2700.0 / float(width * height) ) + 1.0);
    }
    unsigned bWidth = width * scale;
    unsigned bHeight = height * scale;

    if(mode != 7){
        Buffer outputBuffer = context -> createBuffer(RT_BUFFER_OUTPUT, RT_FORMAT_FLOAT3, bWidth, bHeight);
        context["output_buffer"]->set( outputBuffer );
    }
    else{
        Buffer normal1Buffer = context -> createBuffer(RT_BUFFER_OUTPUT, RT_FORMAT_FLOAT3, bWidth, bHeight);
        context["normal1_buffer"]->set( normal1Buffer ); 

        Buffer normal2Buffer = context -> createBuffer(RT_BUFFER_OUTPUT, RT_FORMAT_FLOAT3, bWidth, bHeight);
        context["normal2_buffer"]->set( normal2Buffer );

        Buffer normal3Buffer = context -> createBuffer(RT_BUFFER_OUTPUT, RT_FORMAT_FLOAT3, bWidth, bHeight);
        context["normal3_buffer"]->set( normal3Buffer ); 

        Buffer normal4Buffer = context -> createBuffer(RT_BUFFER_OUTPUT, RT_FORMAT_FLOAT3, bWidth, bHeight);
        context["normal4_buffer"]->set( normal4Buffer ); 

        Buffer depth1Buffer = context -> createBuffer(RT_BUFFER_OUTPUT, RT_FORMAT_FLOAT3, bWidth, bHeight);
        context["depth1_buffer"]->set( depth1Buffer ); 

        Buffer depth2Buffer = context -> createBuffer(RT_BUFFER_OUTPUT, RT_FORMAT_FLOAT3, bWidth, bHeight);
        context["depth2_buffer"]->set( depth2Buffer ); 

        Buffer depth3Buffer = context -> createBuffer(RT_BUFFER_OUTPUT, RT_FORMAT_FLOAT3, bWidth, bHeight);
        context["depth3_buffer"]->set( depth3Buffer ); 

        Buffer depth4Buffer = context -> createBuffer(RT_BUFFER_OUTPUT, RT_FORMAT_FLOAT3, bWidth, bHeight);
        context["depth4_buffer"]->set( depth4Buffer ); 
    }
    
    unsigned sqrt_num_samples = (unsigned )(sqrt(float(sampleNum ) / float(scale * scale) ) + 1.0);
    if(sqrt_num_samples == 0){
        sqrt_num_samples = 1;
    }
    if(mode > 0 && mode !=7 ){
        maxDepth = 1;
        sqrt_num_samples = 4;
    }
    else if(mode == 7){
        maxDepth = 2;
        sqrt_num_samples = 4;
    }

    context["max_depth"]->setInt(maxDepth);
    context["sqrt_num_samples"] -> setUint(sqrt_num_samples);

    // Ray generation program 
    if(mode != 7){
        std::string ptx_path( ptxPath( "path_trace_camera.cu" ) );
        Program ray_gen_program = context->createProgramFromPTXFile( ptx_path, "pinhole_camera" );
        context->setRayGenerationProgram( 0, ray_gen_program );
        
        // Exception program
        Program exception_program = context->createProgramFromPTXFile( ptx_path, "exception" );
        context->setExceptionProgram( 0, exception_program );
        context["bad_color"]->setFloat( 0.0f, 0.0f, 0.0f );
    }
    else{
        std::string ptx_path( ptxPath("four_bounce_camera.cu") );
        Program ray_gen_program = context->createProgramFromPTXFile( ptx_path, "pinhole_camera" );
        context->setRayGenerationProgram( 0, ray_gen_program );
        
        // Exception program
        Program exception_program = context->createProgramFromPTXFile( ptx_path, "exception" );
        context->setExceptionProgram( 0, exception_program );
        context["bad_color"]->setFloat( 0.0f, 0.0f, 0.0f );
    }
    
    if(cameraType == std::string("perspective") ){
        context["cameraMode"] -> setInt(0);
    }
    else if(cameraType == std::string("envmap") ){
        context["cameraMode"] -> setInt(1);
    }
    else if(cameraType == std::string("hemisphere") ){
        context["cameraMode"] -> setInt(2);
    }
    else{ 
        std::cout<<"Wrong: unrecognizable camera type!"<<std::endl;
        exit(1);
    }


    // Missing Program 
    std::string miss_path( ptxPath("envmap.cu") );
    if(mode == 0){
        Program miss_program = context->createProgramFromPTXFile(miss_path, "envmap_miss");
        context->setMissProgram(0, miss_program);
    }
    else{
        if(mode == 7){
            Program miss_program = context->createProgramFromPTXFile(miss_path, "missFourBounce");
            context->setMissProgram(0, miss_program);
        }
        else{
            Program miss_program = context->createProgramFromPTXFile(miss_path, "miss");
            context->setMissProgram(0, miss_program);
        }
    }

    // Set light sampling program 
    const std::string area_path = ptxPath("areaLight.cu");
    Program sampleAreaLight = context -> createProgramFromPTXFile(area_path, "sampleAreaLight");
    context["sampleAreaLight"]->set(sampleAreaLight );

    const std::string env_path = ptxPath("envmap.cu");
    Program sampleEnvLight = context -> createProgramFromPTXFile(env_path, "sampleEnvironmapLight");
    context["sampleEnvironmapLight"]->set(sampleEnvLight );
        

    return scale;
}
