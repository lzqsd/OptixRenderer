/* 
 * Copyright (c) 2016, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// Note: wglew.h has to be included before sutil.h on Windows

#include <sutil/sutil.h>
//#include <sutil/PPMLoader.h>
#include <sampleConfig.h>

#include <optixu/optixu_math_namespace.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <stdint.h>

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN 1
#  endif
#  include<windows.h>
#  include<mmsystem.h>
#else // Apple and Linux both use this 
#  include<sys/time.h>
#  include <unistd.h>
#  include <dirent.h>
#endif


using namespace optix;


namespace
{

// Global variables for GLUT display functions
RTcontext   g_context           = 0;
RTbuffer    g_image_buffer      = 0;



void checkBuffer( RTbuffer buffer )
{
    // Check to see if the buffer is two dimensional
    unsigned int dimensionality;
    RT_CHECK_ERROR( rtBufferGetDimensionality(buffer, &dimensionality) );
    if (2 != dimensionality)
        throw Exception( "Attempting to display non-2D buffer" );

    // Check to see if the buffer is of type float{1,3,4} or uchar4
    RTformat format;
    RT_CHECK_ERROR( rtBufferGetFormat(buffer, &format) );
    if( RT_FORMAT_FLOAT  != format && 
            RT_FORMAT_FLOAT4 != format &&
            RT_FORMAT_FLOAT3 != format &&
            RT_FORMAT_UNSIGNED_BYTE4 != format )
        throw Exception( "Attempting to diaplay buffer with format not float, float3, float4, or uchar4");
}


bool dirExists( const char* path )
{
#if defined(_WIN32)
    DWORD attrib = GetFileAttributes( path );
    return (attrib != INVALID_FILE_ATTRIBUTES) && (attrib & FILE_ATTRIBUTE_DIRECTORY);
#else
    DIR* dir = opendir( path );
    if( dir == NULL )
        return false;
    
    closedir(dir);
    return true;
#endif
}

} // end anonymous namespace


void sutil::reportErrorMessage( const char* message )
{
    std::cerr << "OptiX Error: '" << message << "'\n";
#if defined(_WIN32) && defined(RELEASE_PUBLIC)
    {
        char s[2048];
        sprintf( s, "OptiX Error: %s", message );
        MessageBox( 0, s, "OptiX Error", MB_OK|MB_ICONWARNING|MB_SYSTEMMODAL );
    }
#endif
}


void sutil::handleError( RTcontext context, RTresult code, const char* file,
        int line)
{
    const char* message;
    char s[2048];
    rtContextGetErrorString(context, code, &message);
    sprintf(s, "%s\n(%s:%d)", message, file, line);
    reportErrorMessage( s );
}


const char* sutil::samplesDir()
{
    static char s[512];

    // Allow for overrides.
    const char* dir = getenv( "OPTIX_SAMPLES_SDK_DIR" );
    if (dir) {
        strcpy(s, dir);
        return s;
    }

    // Return hardcoded path if it exists.
    if( dirExists( SAMPLES_DIR ) )
        return SAMPLES_DIR;

    // Last resort.
    return ".";
}


const char* sutil::samplesPTXDir()
{
    static char s[512];

    // Allow for overrides.
    const char* dir = getenv( "OPTIX_SAMPLES_SDK_PTX_DIR" );
    if (dir) {
        strcpy(s, dir);
        return s;
    }

    // Return hardcoded path if it exists.
    if( dirExists(SAMPLES_PTX_DIR) )
        return SAMPLES_PTX_DIR;

    // Last resort.
    return ".";
}


optix::Buffer sutil::createOutputBuffer(
        optix::Context context,
        RTformat format,
        unsigned width,
        unsigned height,
        bool use_pbo )
{
    
    optix::Buffer buffer;
    buffer = context->createBuffer( RT_BUFFER_OUTPUT, format, width, height );

    return buffer;
}



optix::GeometryInstance sutil::createOptiXGroundPlane( optix::Context context,
                                               const std::string& parallelogram_ptx, 
                                               const optix::Aabb& aabb,
                                               optix::Material material,
                                               float scale )
{
    optix::Geometry parallelogram = context->createGeometry();
    parallelogram->setPrimitiveCount( 1u );
    parallelogram->setBoundingBoxProgram( context->createProgramFromPTXFile( parallelogram_ptx, "bounds" ) );
    parallelogram->setIntersectionProgram( context->createProgramFromPTXFile( parallelogram_ptx, "intersect" ) );
    const float extent = scale*fmaxf( aabb.extent( 0 ), aabb.extent( 2 ) );
    const float3 anchor = make_float3( aabb.center(0) - 0.5f*extent, aabb.m_min.y - 0.001f*aabb.extent( 1 ), aabb.center(2) - 0.5f*extent );
    float3 v1 = make_float3( 0.0f, 0.0f, extent );
    float3 v2 = make_float3( extent, 0.0f, 0.0f );
    const float3 normal = normalize( cross( v1, v2 ) );
    float d = dot( normal, anchor );
    v1 *= 1.0f / dot( v1, v1 );
    v2 *= 1.0f / dot( v2, v2 );
    float4 plane = make_float4( normal, d );
    parallelogram["plane"]->setFloat( plane );
    parallelogram["v1"]->setFloat( v1 );
    parallelogram["v2"]->setFloat( v2 );
    parallelogram["anchor"]->setFloat( anchor );

    optix::GeometryInstance instance = context->createGeometryInstance( parallelogram, &material, &material + 1 );
    return instance;
}



void sutil::calculateCameraVariables( float3 eye, float3 lookat, float3 up,
        float  fov, float  aspect_ratio,
        float3& U, float3& V, float3& W, bool fov_is_vertical )
{
    float ulen, vlen, wlen;
    W = lookat - eye; // Do not normalize W -- it implies focal length

    wlen = length( W ); 
    U = normalize( cross( W, up ) );
    V = normalize( cross( U, W  ) );

	if ( fov_is_vertical ) {
		vlen = wlen * tanf( 0.5f * fov * M_PIf / 180.0f );
		V *= vlen;
		ulen = vlen * aspect_ratio;
		U *= ulen;
	}
	else {
		ulen = wlen * tanf( 0.5f * fov * M_PIf / 180.0f );
		U *= ulen;
		vlen = ulen / aspect_ratio;
		V *= vlen;
	}
}


void sutil::parseDimensions( const char* arg, int& width, int& height )
{

    // look for an 'x': <width>x<height>
    size_t width_end = strchr( arg, 'x' ) - arg;
    size_t height_begin = width_end + 1;

    if ( height_begin < strlen( arg ) )
    {
        // find the beginning of the height string/
        const char *height_arg = &arg[height_begin];

        // copy width to null-terminated string
        char width_arg[32];
        strncpy( width_arg, arg, width_end );
        width_arg[width_end] = '\0';

        // terminate the width string
        width_arg[width_end] = '\0';

        width  = atoi( width_arg );
        height = atoi( height_arg );
        return;
    }

    throw Exception(
            "Failed to parse width, heigh from string '" +
            std::string( arg ) +
            "'" );
}


double sutil::currentTime()
{
#if defined(_WIN32)

    // inv_freq is 1 over the number of ticks per second.
    static double inv_freq;
    static bool freq_initialized = 0;
    static bool use_high_res_timer = 0;

    if(!freq_initialized)
    {
        LARGE_INTEGER freq;
        use_high_res_timer = ( QueryPerformanceFrequency( &freq ) != 0 );
        inv_freq = 1.0/freq.QuadPart;
        freq_initialized = 1;
    }

    if (use_high_res_timer)
    {
        LARGE_INTEGER c_time;
        if( QueryPerformanceCounter( &c_time ) )
            return c_time.QuadPart*inv_freq;
        else
            throw Exception( "sutil::currentTime: QueryPerformanceCounter failed" );
    }

    return static_cast<double>( timeGetTime() ) * 1.0e-3;

#else

    struct timeval tv;
    if( gettimeofday( &tv, 0 ) )
        throw Exception( "sutil::urrentTime(): gettimeofday failed!\n" );

    return  tv.tv_sec+ tv.tv_usec * 1.0e-6;

#endif 
}


void sutil::sleep( int seconds )
{
#if defined(_WIN32)
    Sleep( seconds * 1000 );
#else
    ::sleep( seconds );
#endif
}

