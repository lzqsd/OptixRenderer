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

#include <optixu/optixu_math_namespace.h>
#include "OptiXMesh.h"
#include "sutil.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace optix {
  float3 make_float3( const float* a )
  {
    return make_float3( a[0], a[1], a[2] );
  }
}


namespace
{

struct MeshBuffers
{
  optix::Buffer tri_indices;
  optix::Buffer mat_indices;
  optix::Buffer positions;
  optix::Buffer normals;
  optix::Buffer texcoords;
};


void unmap( MeshBuffers& buffers, Mesh& mesh )
{
  buffers.tri_indices->unmap();
  buffers.mat_indices->unmap();
  buffers.positions->unmap();
  if( mesh.has_normals )
    buffers.normals->unmap();
  if( mesh.has_texcoords)
    buffers.texcoords->unmap();

  mesh.tri_indices = 0; 
  mesh.mat_indices = 0;
  mesh.positions   = 0;
  mesh.normals     = 0;
  mesh.texcoords   = 0;
}


void createMaterialPrograms(
    optix::Context         context,
    bool                   use_textures,
    optix::Program&        closest_hit,
    optix::Program&        any_hit
    )
{
  const std::string path = std::string( sutil::samplesPTXDir() ) + 
                          "/cuda_compile_ptx_generated_phong.cu.ptx";
  const std::string closest_name = use_textures ?
                                   "closest_hit_radiance_textured" :
                                   "closest_hit_radiance";

  if( !closest_hit )
      closest_hit = context->createProgramFromPTXFile( path, closest_name );
  if( !any_hit )
      any_hit     = context->createProgramFromPTXFile( path, "any_hit_shadow" );
}


optix::Program createBoundingBoxProgram( optix::Context context )
{
  std::string path = std::string( sutil::samplesPTXDir() ) +
                     "/cuda_compile_ptx_generated_triangle_mesh.cu.ptx";
  return context->createProgramFromPTXFile( path, "mesh_bounds" );
}


optix::Program createIntersectionProgram( optix::Context context )
{
  std::string path = std::string( sutil::samplesPTXDir() ) +
                     "/cuda_compile_ptx_generated_triangle_mesh.cu.ptx";
  return context->createProgramFromPTXFile( path, "mesh_intersect" );
}


void translateMeshToOptiX(
    const Mesh&        mesh,
    const MeshBuffers& buffers,
    OptiXMesh&         optix_mesh
    )
{
  optix::Context ctx       = optix_mesh.context;
  optix_mesh.bbox_min      = optix::make_float3( mesh.bbox_min );
  optix_mesh.bbox_max      = optix::make_float3( mesh.bbox_max );
  optix_mesh.num_triangles = mesh.num_triangles;

  std::vector<optix::Material> optix_materials;
  if( optix_mesh.material )
  {
    // Rewrite all mat_indices to point to single override material
    memset( mesh.mat_indices, 0, mesh.num_triangles*sizeof(int32_t) );
    optix_materials.push_back( optix_mesh.material ); 
  }

  optix::Geometry geometry = ctx->createGeometry();  
  geometry[ "vertex_buffer"   ]->setBuffer( buffers.positions ); 
  geometry[ "normal_buffer"   ]->setBuffer( buffers.normals); 
  geometry[ "texcoord_buffer" ]->setBuffer( buffers.texcoords ); 
  geometry[ "material_buffer" ]->setBuffer( buffers.mat_indices); 
  geometry[ "index_buffer"    ]->setBuffer( buffers.tri_indices); 
  geometry->setPrimitiveCount     ( mesh.num_triangles );
  geometry->setBoundingBoxProgram ( optix_mesh.bounds ?
                                    optix_mesh.bounds :
                                    createBoundingBoxProgram( ctx ) );
  geometry->setIntersectionProgram( optix_mesh.intersection ?
                                    optix_mesh.intersection :
                                    createIntersectionProgram( ctx ) );

  optix_mesh.geom_instance = ctx->createGeometryInstance(
                                 geometry,
                                 optix_materials.begin(),
                                 optix_materials.end()
                                 );
}


} // namespace end
