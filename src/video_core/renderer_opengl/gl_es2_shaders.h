// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

// GLES2 uses GLSL ES 1.0, which has slightly different syntax from the standard shaders.
// For example, instead of in/out, attribute/varying is used.

namespace GLShaders {

const char g_vertex_shader[] = R"(
attribute vec2 vert_position;
attribute vec2 vert_tex_coord;
varying vec2 frag_tex_coord;

// This is a truncated 3x3 matrix for 2D transformations:
// The upper-left 2x2 submatrix performs scaling/rotation/mirroring.
// The third column performs translation.
// The third row could be used for projection, which we don't need in 2D. It hence is assumed to
// implicitly be [0, 0, 1]

// above comment from OpenGL shader; ES doesn't have a 3x2 matrix
uniform mat3 modelview_matrix;

void main() {
    // Multiply input position by the rotscale part of the matrix and then manually translate by
    // the last column. This is equivalent to using a full 3x3 matrix and expanding the vector
    // to `vec3(vert_position.xy, 1.0)`
    gl_Position = vec4(mat2(modelview_matrix) * vert_position + modelview_matrix[2].xy, 0.0, 1.0);
    frag_tex_coord = vert_tex_coord;
}
)";

const char g_fragment_shader[] = R"(
varying highp vec2 frag_tex_coord;

uniform sampler2D color_texture;

void main() {
    gl_FragColor = texture2D(color_texture, frag_tex_coord);
}
)";

}
