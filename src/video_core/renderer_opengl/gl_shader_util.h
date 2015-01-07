// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#ifdef ANDROID
#include <GLES2/gl2.h>
#else
#include "generated/gl_3_2_core.h"
#endif

namespace ShaderUtil {

GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path);

}
