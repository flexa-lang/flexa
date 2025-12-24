#ifndef BYTECODE_HPP
#define BYTECODE_HPP

#include <iostream>
#include <string>
#include <vector>

#include "vm_constants.hpp"
#include "types.hpp"
#include "operand.hpp"

namespace core {

	class BytecodeInstruction {
	public:
		OpCode opcode;
		Operand operand;

		BytecodeInstruction();
		BytecodeInstruction(OpCode opcode, uint8_t* operand, size_t size);
		BytecodeInstruction(OpCode opcode, uint8_t operand);
		BytecodeInstruction(OpCode opcode, size_t operand);
		BytecodeInstruction(OpCode opcode, flx_bool operand);
		BytecodeInstruction(OpCode opcode, flx_int operand);
		BytecodeInstruction(OpCode opcode, flx_float operand);
		BytecodeInstruction(OpCode opcode, flx_char operand);
		BytecodeInstruction(OpCode opcode, const flx_string& operand);
		BytecodeInstruction(OpCode opcode, const std::vector<Operand>& operand);

		static void write_bytecode_table(
			const std::vector<BytecodeInstruction>& instructions,
			const std::string& filename
		);
	};

}

#endif // !BYTECODE_HPP
