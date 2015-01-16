// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <initializer_list>

#include <common/common_types.h>

#include "math.h"
#include "pica.h"

namespace Pica {

namespace VertexShaderToGLSL {
std::string ToGlsl(const std::array<u32, 1024>& shader_memory, const std::array<u32, 1024>& swizzle_data);
void CompileGlsl(const std::array<u32, 1024>& shader_memory, const std::array<u32, 1024>& swizzle_data);
} // namespace

} // namespace

