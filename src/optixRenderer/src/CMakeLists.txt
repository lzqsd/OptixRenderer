#
# Copyright (c) 2016, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#


include_directories(${SAMPLES_INCLUDE_DIR})
find_package(OpenCV REQUIRED)

# See top level CMakeLists.txt file for documentation of OPTIX_add_sample_executable.
OPTIX_add_sample_executable( optixRenderer
    optixRenderer.cpp

    camera/path_trace_camera.cu
    creator/createAreaLight.cpp
    creator/createCamera.cpp
    creator/createContext.cpp
    creator/createEnvmap.cpp
    creator/createGeometry.cpp
    creator/createMaterial.cpp
    creator/createPointFlashLight.cpp
    creator/createTextureSampler.cpp

    inout/readXML.cpp
    inout/loadBsdfFromXML.cpp
    inout/loadSensorFromXML.cpp
    inout/loadLightFromXML.cpp
    inout/loadShapeFromXML.cpp
    inout/relativePath.cpp
    inout/rgbe.cpp

    intersect/triangle_mesh.cu

    light/areaLight.cu
    light/envmap.cu

    material/albedo.cpp
    material/albedo.cu
    material/depth.cpp
    material/depth.cu
    material/diffuse.cpp
    material/diffuse.cu
    material/mask.cpp
    material/mask.cu
    material/metallic.cpp
    material/metallic.cu
    material/microfacet.cpp
    material/microfacet.cu
    material/normal.cpp
    material/normal.cu
    material/phong.cpp
    material/phong.cu
    material/roughness.cpp
    material/roughness.cu
    material/dielectric.cpp
    material/dielectric.cu
    material/conductor.cpp
    material/conductor.cu

    sampler/sampler.cpp

    utils/ptxPath.cpp

    postprocessing/filter.cpp

    # common headers
    ${SAMPLES_INCLUDE_DIR}/helpers.h
    ${SAMPLES_INCLUDE_DIR}/intersection_refinement.h
    ${SAMPLES_INCLUDE_DIR}/random.h
    )
target_link_libraries(optixRenderer ${OpenCV_LIBS} )

