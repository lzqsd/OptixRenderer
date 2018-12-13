#ifndef CREATECONTEXT_HEADER
#define CREATECONTEXT_HEADER

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_aabb_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>
#include <string>
#include "ptxPath.h"

using namespace optix;


void destroyContext(Context& context )
{
    if( context ){
        context->destroy();
        context = 0;
    }
}

void createContext( 
        Context& context, 
        bool use_pbo, 
        std::string cameraType,
        unsigned width, unsigned height, 
        unsigned int mode = 0,
        unsigned int sampleNum = 400, 
        unsigned maxDepth = 5, 
        unsigned rr_begin_depth = 3)
{
    unsigned sqrt_num_samples = (unsigned )(sqrt(float(sampleNum ) ) + 1.0);
    if(sqrt_num_samples == 0)
        sqrt_num_samples = 1;

    // Set up context
    context = Context::create();
    context->setRayTypeCount( 2 );
    context->setEntryPointCount( 1 );
    context->setStackSize( 600 );

    if(mode > 0){
        maxDepth = 1;
        sqrt_num_samples = 4;
    }

    context["max_depth"]->setInt(maxDepth);
    context["sqrt_num_samples"] -> setUint(sqrt_num_samples);
    context["rr_begin_depth"] -> setUint(rr_begin_depth);
    
    Buffer outputBuffer = context -> createBuffer(RT_BUFFER_OUTPUT, RT_FORMAT_FLOAT3, width, height);
    context["output_buffer"]->set( outputBuffer );

    // Ray generation program 
    std::string ptx_path( ptxPath( "path_trace_camera.cu" ) );
    Program ray_gen_program = context->createProgramFromPTXFile( ptx_path, "pinhole_camera" );
    context->setRayGenerationProgram( 0, ray_gen_program );
    
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


    // Exception program
    Program exception_program = context->createProgramFromPTXFile( ptx_path, "exception" );
    context->setExceptionProgram( 0, exception_program );
    context["bad_color"]->setFloat( 0.0f, 0.0f, 0.0f );

    // Missing Program 
    std::string miss_path( ptxPath("envmap.cu") );
    if(mode == 0){
        Program miss_program = context->createProgramFromPTXFile(miss_path, "envmap_miss");
        context->setMissProgram(0, miss_program);
    }
    else{
        Program miss_program = context->createProgramFromPTXFile(miss_path, "miss");
        context->setMissProgram(0, miss_program);
    }
}

#endif
