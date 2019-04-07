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

#include "Camera.h"
#include "sutil.h"
#include <iostream>

using namespace optix;

//------------------------------------------------------------------------------
//
// Camera implementation
//
//------------------------------------------------------------------------------
//
// Compute derived uvw frame and write to OptiX context
void sutil::Camera::apply( )
{
    const float aspect_ratio = static_cast<float>(m_width) /
        static_cast<float>(m_height);

    m_camera_rotate = Matrix4x4::identity();
    float3 camera_w = normalize(m_camera_lookat - m_camera_eye );

    m_camera_up = m_camera_up - dot(m_camera_up, camera_w) * camera_w;
    m_camera_up = normalize(m_camera_up );

    float3 camera_right = cross(camera_w, m_camera_up);
    camera_right = normalize(camera_right );
    
    m_camera_up = normalize(m_camera_up );
    camera_right = normalize(camera_right );

    float lengthV = tan(vfov/360.0 * 3.1415926535);
    float lengthU = float(m_width) / float(m_height) * lengthV;

    m_camera_u = camera_right * lengthU;
    m_camera_v = m_camera_up * lengthV;

    // Write variables to OptiX context
    m_variable_eye->setFloat( m_camera_eye );
    m_variable_u->setFloat( m_camera_u );
    m_variable_v->setFloat( m_camera_v );
    m_variable_w->setFloat( camera_w );
}

void sutil::Camera::reset_lookat()
{
    const float3 translation = m_save_camera_lookat - m_camera_lookat;
    m_camera_eye += translation;
    m_camera_lookat = m_save_camera_lookat;
    apply();
}
