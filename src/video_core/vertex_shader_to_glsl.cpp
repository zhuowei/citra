// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <stack>

#include <boost/range/algorithm.hpp>

#include <common/file_util.h>

#include <core/mem_map.h>

#include <nihstro/shader_bytecode.h>


#include "pica.h"
#include "vertex_shader.h"
#include "debug_utils/debug_utils.h"

#include <map>
#include <cstdio>

using nihstro::Instruction;
using nihstro::RegisterType;
using nihstro::SourceRegister;
using nihstro::SwizzlePattern;

namespace ShaderUtil {
int LoadShaders(const char*, const char*);
};

namespace Pica {

namespace VertexShaderToGLSL {

const char g_fragment_shader[] = R"(
#version 110
void main() {
	gl_FragColor = vec4(0.0);
}
)";

static std::map<Instruction::OpCode, std::string[3]> two_operand_format_strings {
	{Instruction::OpCode::ADD, {"", "+", ""}},
	{Instruction::OpCode::DP3, {"dot((", ").xyz,(", ").xyz)"}},
	{Instruction::OpCode::DP4, {"dot(", ",", ")"}},
	{Instruction::OpCode::MUL, {"", "*", ""}},
	{Instruction::OpCode::MAX, {"max(", ",", ")"}},
	{Instruction::OpCode::MIN, {"min(", ",", ")"}},
};
static std::map<Instruction::OpCode, std::string[2]> one_operand_format_strings {
	{Instruction::OpCode::RCP, {"1.0/", ""}},
	{Instruction::OpCode::RSQ, {"inversesqrt(", ")"}},
	{Instruction::OpCode::MOV, {"", ""}},
};

    std::string DestMaskToStringNoUnderscore(const SwizzlePattern& swizzle) {
        std::string ret;
        for (int i = 0; i < 4; ++i) {
            if (swizzle.DestComponentEnabled(i))
                ret += "xyzw"[i];
        }
        return ret;
    }

static void ToGlsl_code(std::stringstream& out, const std::array<u32, 1024>& shader_memory, const std::array<u32, 1024>& swizzle_data) {
	bool end_loop = false;
	for (int i = 0; i < 1024; i++) {
		const Instruction& instr = (const Instruction&)shader_memory[i];
		const SwizzlePattern& swizzle = (const SwizzlePattern&)swizzle_data[instr.common.operand_desc_id];

		if (instr.opcode.GetInfo().type == Instruction::OpCodeType::Arithmetic) {
			bool src_reversed = 0 != (instr.opcode.GetInfo().subtype & Instruction::OpCodeInfo::SrcInversed);
			auto src1 = instr.common.GetSrc1(src_reversed);
			auto src2 = instr.common.GetSrc2(src_reversed);

			std::string src1_relative_address;
			if (!instr.common.AddressRegisterName().empty())
				src1_relative_address = "[" + instr.common.AddressRegisterName() + "]";

			if (instr.opcode.GetInfo().subtype &
				(Instruction::OpCodeInfo::Src1 | Instruction::OpCodeInfo::Src2 | Instruction::OpCodeInfo::Dest) &&
				two_operand_format_strings.count(instr.opcode) != 0) {
				std::string parts[3] = two_operand_format_strings[instr.opcode];
				out << instr.common.dest.GetName() << "." << DestMaskToStringNoUnderscore(swizzle)
					<< " = vec4("
					<< parts[0] 
					<< ((swizzle.negate_src1 ? "-" : "") + src1.GetName()) + src1_relative_address 
					<< "." << swizzle.SelectorToString(false)
					<< parts[1]
					<< ((swizzle.negate_src2 ? "-" : "") + src2.GetName()) 
					<< "." << swizzle.SelectorToString(true)
					<< parts[2] << ")." + DestMaskToStringNoUnderscore(swizzle) + ";" << std::endl;
			}
		    	else if (instr.opcode.GetInfo().subtype & (Instruction::OpCodeInfo::Src1 | Instruction::OpCodeInfo::Dest) &&
				one_operand_format_strings.count(instr.opcode) != 0) {
				std::string parts[2] = one_operand_format_strings[instr.opcode];
				out << instr.common.dest.GetName() << "." << DestMaskToStringNoUnderscore(swizzle)
					<< " = vec4("
					<< parts[0] 
					<< ((swizzle.negate_src1 ? "-" : "") + src1.GetName()) + src1_relative_address 
					<< "." << swizzle.SelectorToString(false)
					<< parts[1] << ")." + DestMaskToStringNoUnderscore(swizzle) + ";" << std::endl;
			} else {
				printf("Shader fail: %s\n", instr.opcode.GetInfo().name);
			}
		} else {
			switch (instr.opcode) {
				case Instruction::OpCode::END:
					end_loop = true;
					break;
			}
		}
		if (end_loop) break;
	}
}

std::string ToGlsl(const std::array<u32, 1024>& shader_memory, const std::array<u32, 1024>& swizzle_data) {
	std::stringstream out;
	out << "#version 110" << std::endl;
	// outputs
	for (int i = 0; i < 7; i++) {
		out << "varying vec4 o" << i << ";" << std::endl;
	}
	// attributes
	for (int i = 0; i < 7; i++) {
		out << "attribute vec4 v" << i << ";" << std::endl;
	}
	// uniforms
	for (int i = 0; i < 96; i++) {
		out << "uniform vec4 c" << i << ";" << std::endl;
	}
	out << "void main() {" << std::endl;
	// temps
	for (int i = 0; i < 16; i++) {
		out << "vec4 r" << i << ";" << std::endl;
	}
	ToGlsl_code(out, shader_memory, swizzle_data);
	out << "}" << std::endl;
	printf("Shader: %s\n", out.str().c_str());
	return out.str();
}

void CompileGlsl(const std::array<u32, 1024>& shader_memory, const std::array<u32, 1024>& swizzle_data) {
	std::string vertex_shader = ToGlsl(shader_memory, swizzle_data);
	ShaderUtil::LoadShaders(vertex_shader.c_str(), g_fragment_shader);
}

} // namespace VertexShaderToGLSL
} // namespace Pica
