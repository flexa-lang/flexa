#include <fstream>
#include <sstream>
#include <iomanip>

#include "bytecode.hpp"
#include "utils.hpp"

using namespace core;

BytecodeInstruction::BytecodeInstruction()
	: opcode(OpCode::OP_RES), operand(nullptr, 0) {}

BytecodeInstruction::BytecodeInstruction(OpCode opcode, uint8_t* operand, size_t size)
	: opcode(opcode), operand(operand, size) {}

BytecodeInstruction::BytecodeInstruction(OpCode opcode, uint8_t operand)
	: opcode(opcode), operand(operand) {}

BytecodeInstruction::BytecodeInstruction(OpCode opcode, size_t operand)
	: opcode(opcode), operand(operand) {}

BytecodeInstruction::BytecodeInstruction(OpCode opcode, flx_bool operand)
	: opcode(opcode), operand(operand) {}

BytecodeInstruction::BytecodeInstruction(OpCode opcode, flx_int operand)
	: opcode(opcode), operand(operand) {}

BytecodeInstruction::BytecodeInstruction(OpCode opcode, flx_float operand)
	: opcode(opcode), operand(operand) {}

BytecodeInstruction::BytecodeInstruction(OpCode opcode, flx_char operand)
	: opcode(opcode), operand(operand) {}

BytecodeInstruction::BytecodeInstruction(OpCode opcode, const flx_string& operand)
	: opcode(opcode), operand(operand) {}

BytecodeInstruction::BytecodeInstruction(OpCode opcode, const std::vector<Operand>& operand)
	: opcode(opcode), operand(operand) {
}

void BytecodeInstruction::write_bytecode_table(const std::vector<BytecodeInstruction>& instructions, const std::string& filename) {
	std::ofstream file(filename);

	if (!file.is_open()) {
		std::cerr << "Error opening file: " << filename << std::endl;
		return;
	}

	auto pc_str_len = std::to_string(instructions.size()).length();
	for (size_t i = 0; i < instructions.size(); ++i) {
		const auto& instruction = instructions[i];

		file << std::setw(pc_str_len) << std::setfill('0') << i << "\t";

		file << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(instruction.opcode) << "\t";

		file << OP_NAMES.at(instruction.opcode);
		
		file << std::setw(22 - OP_NAMES.at(instruction.opcode).size()) << std::setfill(' ') << "\t" << std::dec;

		if (instruction.operand.get_raw_operand() == nullptr) {
			file << "<NO_OP>";
		}
		else {
			file << instruction.operand.string();
		}

		file << std::endl;
	}

	file.close();
}

void BytecodeInstruction::debug_instruction(const BytecodeInstruction& instruction) {
	std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(instruction.opcode) << "\t";

	std::cout << OP_NAMES.at(instruction.opcode);

	std::cout << std::setw(22 - OP_NAMES.at(instruction.opcode).size()) << std::setfill(' ') << "\t" << std::dec;

	if (instruction.operand.get_raw_operand() == nullptr) {
		std::cout << "<NO_OP>";
	}
	else {
		std::cout << instruction.operand.string();
	}

	std::cout << std::endl;
}

std::string BytecodeInstruction::string_instruction(const BytecodeInstruction& instruction) {
	std::ostringstream buffer;
	
	buffer << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(instruction.opcode) << "\t";

	buffer << OP_NAMES.at(instruction.opcode);

	buffer << std::setw(22 - OP_NAMES.at(instruction.opcode).size()) << std::setfill(' ') << "\t" << std::dec;

	if (instruction.operand.get_raw_operand() == nullptr) {
		buffer << "<NO_OP>";
	}
	else {
		buffer << instruction.operand.string();
	}

	buffer << std::endl;

	return buffer.str();
}
