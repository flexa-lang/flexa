#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>

#include "operand.hpp"
#include "utils.hpp"

using namespace core;

Operand::Operand()
	: value(nullptr), size(0), type(OperandType::OT_RAW) {
}

Operand::Operand(uint8_t* value, size_t size)
	: value(value), size(size), type(OperandType::OT_RAW) {
}

Operand::Operand(uint8_t value)
	: value(Operand::to_byte_operand(value)), size(sizeof(value)), type(OperandType::OT_UINT8) {
}

Operand::Operand(size_t value)
	: value(Operand::to_byte_operand(value)), size(sizeof(value)), type(OperandType::OT_SIZE) {
}

Operand::Operand(flx_bool value)
	: value(Operand::to_byte_operand(value)), size(sizeof(value)), type(OperandType::OT_BOOL) {
}

Operand::Operand(flx_int value)
	: value(Operand::to_byte_operand(value)), size(sizeof(value)), type(OperandType::OT_INT) {
}

Operand::Operand(flx_float value)
	: value(Operand::to_byte_operand(value)), size(sizeof(value)), type(OperandType::OT_FLOAT) {
}

Operand::Operand(flx_char value)
	: value(Operand::to_byte_operand(value)), size(sizeof(value)), type(OperandType::OT_CHAR) {
}

Operand::Operand(const flx_string& value)
	: value(Operand::to_byte_operand(value)), size(sizeof(uint64_t) + value.size()), type(OperandType::OT_STRING) {
}

Operand::Operand(const std::vector<Operand>& value)
	: value(Operand::to_byte_operand(value)), type(OperandType::OT_VECTOR) {
	size = sizeof(uint64_t);
	for (const auto& op : value) {
		size += sizeof(uint64_t);
		size += op.size;
	}
}

uint8_t* Operand::get_raw_operand() const {
	return value;
}

uint8_t Operand::get_uint8_operand() const {
	return *value;
}

size_t Operand::get_size_operand() const {
	return Operand::to_size_operand(value);
}

flx_bool Operand::get_bool_operand() const {
	return Operand::to_bool_operand(value);
}

flx_int Operand::get_int_operand() const {
	return Operand::to_int_operand(value);
}

flx_float Operand::get_float_operand() const {
	return Operand::to_float_operand(value);
}

flx_char Operand::get_char_operand() const {
	return Operand::to_char_operand(value);
}

flx_string Operand::get_string_operand() const {
	return Operand::to_string_operand(value);
}

std::vector<Operand> Operand::get_vector_operand() const {
	return Operand::to_vector_operand(value);
}

uint8_t Operand::to_uint8_operand(uint8_t* value) {
	return *value;
}

size_t Operand::to_size_operand(uint8_t* value) {
	return *reinterpret_cast<size_t*>(value);
}

flx_bool Operand::to_bool_operand(uint8_t* value) {
	return *reinterpret_cast<flx_bool*>(value);
}

flx_int Operand::to_int_operand(uint8_t* value) {
	return *reinterpret_cast<flx_int*>(value);
}

flx_float Operand::to_float_operand(uint8_t* value) {
	return *reinterpret_cast<flx_float*>(value);
}

flx_char Operand::to_char_operand(uint8_t* value) {
	return *reinterpret_cast<flx_char*>(value);
}

flx_string Operand::to_string_operand(uint8_t* value) {
	uint64_t size;
	std::memcpy(&size, value, sizeof(uint64_t));

	std::string result(reinterpret_cast<char*>(value + sizeof(uint64_t)), size);
	return result;
}

std::vector<Operand> Operand::to_vector_operand(uint8_t* data) {
	std::vector<Operand> vec;
	uint8_t* ptr = data;

	uint64_t vec_size;
	std::memcpy(&vec_size, ptr, sizeof(uint64_t));
	ptr += sizeof(uint64_t);

	for (uint64_t i = 0; i < vec_size; ++i) {
		uint8_t type_byte;
		std::memcpy(&type_byte, ptr, sizeof(uint8_t));
		OperandType type = static_cast<OperandType>(type_byte);
		ptr += sizeof(uint8_t);

		uint64_t op_size;
		std::memcpy(&op_size, ptr, sizeof(uint64_t));
		ptr += sizeof(uint64_t);

		uint8_t* op_data = new uint8_t[op_size];
		std::memcpy(op_data, ptr, op_size);
		ptr += op_size;

		Operand op(op_data, static_cast<size_t>(op_size));
		op.type = type;

		vec.emplace_back(op);
	}

	return vec;
}

uint8_t* Operand::to_byte_operand(const std::string& str) {
	uint64_t size = static_cast<uint64_t>(str.size());
	size_t total_size = sizeof(size) + str.size();

	uint8_t* buffer = new uint8_t[total_size];

	std::memcpy(buffer, &size, sizeof(size));
	std::memcpy(buffer + sizeof(size), str.data(), str.size());

	return buffer;
}

uint8_t* Operand::to_byte_operand(const std::vector<Operand>& vec) {
	size_t total_size = sizeof(uint64_t);

	for (const auto& op : vec) {
		total_size += sizeof(uint8_t);
		total_size += sizeof(uint64_t);
		total_size += op.size;
	}

	uint8_t* buffer = new uint8_t[total_size];
	uint8_t* ptr = buffer;

	uint64_t vec_size = static_cast<uint64_t>(vec.size());
	std::memcpy(ptr, &vec_size, sizeof(uint64_t));
	ptr += sizeof(uint64_t);

	for (const auto& op : vec) {
		uint8_t type = static_cast<uint8_t>(op.type);
		std::memcpy(ptr, &type, sizeof(uint8_t));
		ptr += sizeof(uint8_t);

		uint64_t op_size = static_cast<uint64_t>(op.size);
		std::memcpy(ptr, &op_size, sizeof(uint64_t));
		ptr += sizeof(uint64_t);

		std::memcpy(ptr, op.value, op.size);
		ptr += op.size;
	}

	return buffer;
}

std::string Operand::string() const {
	switch (type)
	{
	case OT_RAW: {
		std::ostringstream bytes;
		bytes << "0x" << std::hex << std::setw(2) << std::setfill('0');
		for (size_t i = 0; i < size; ++i) {
			bytes << std::to_string(static_cast<uint8_t>(value[i]));
		}
		return bytes.str();
	}
	case OT_UINT8:
		return std::to_string(get_uint8_operand());
	case OT_SIZE:
		return std::to_string(get_size_operand());
	case OT_BOOL:
		return get_bool_operand() ? "true" : "false";
	case OT_INT:
		return std::to_string(get_int_operand());
	case OT_FLOAT:
		return std::to_string(get_float_operand());
		break;
	case OT_CHAR:
		switch (get_char_operand()) {
		case '\\':
			return "'\\\\'";
		case '\n':
			return "'\\n'";
		case '\r':
			return "'\\r'";
		case '\t':
			return "'\\t'";
		case '\'':
			return "'\\''";
		case '\b':
			return "'\\b'";
		case '\0':
			return "'\\0'";
		default:
			return "'" + std::to_string(get_char_operand()) + "'";
		}
	case OT_STRING: {
		std::string str = get_string_operand();
		str = utils::StringUtils::replace(str, "\"", "\\\"");
		str = utils::StringUtils::replace(str, "\n", "\\n");
		str = utils::StringUtils::replace(str, "\r", "\\r");
		str = utils::StringUtils::replace(str, "\b", "\\b");
		str = utils::StringUtils::replace(str, "\\", "\\\\");
		return '"' + str + '"';
	}
	case OT_VECTOR: {
		std::string str;
		auto vec = get_vector_operand();
		for (const auto& value : vec) {
			str += value.string() + ", ";
		}
		if (!str.empty()) {
			str = str.substr(0, str.size() - 2);
		}
		return "[ " + str + " ]";
	}
	default:
		if (get_raw_operand()) {
			return "<UNHANDLED>\t" + std::to_string(reinterpret_cast<uintmax_t>(get_raw_operand()));
		}
		return "<EMPTY>";
	}
}

