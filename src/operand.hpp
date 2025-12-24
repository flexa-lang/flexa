#ifndef OPERAND_HPP
#define OPERAND_HPP

#include <iostream>
#include <string>
#include <cstring>

#include "types.hpp"

namespace core {

	enum OperandType : uint8_t {
		OT_RAW,
		OT_UINT8,
		OT_SIZE,
		OT_BOOL,
		OT_INT,
		OT_FLOAT,
		OT_CHAR,
		OT_STRING,
		OT_VECTOR
	};

	class Operand {
	public:
		uint8_t* value;
		size_t size;
		OperandType type;

		Operand();
		Operand(uint8_t* value, size_t size);
		Operand(uint8_t value);
		Operand(size_t value);
		Operand(flx_bool value);
		Operand(flx_int value);
		Operand(flx_float value);
		Operand(flx_char value);
		Operand(const flx_string& value);
		Operand(const std::vector<Operand>& value);

		uint8_t* get_raw_operand() const;
		uint8_t get_uint8_operand() const;
		size_t get_size_operand() const;
		flx_bool get_bool_operand() const;
		flx_int get_int_operand() const;
		flx_float get_float_operand() const;
		flx_char get_char_operand() const;
		flx_string get_string_operand() const;
		std::vector<Operand> get_vector_operand() const;

		static uint8_t to_uint8_operand(uint8_t* value);
		static size_t to_size_operand(uint8_t* value);
		static flx_bool to_bool_operand(uint8_t* value);
		static flx_int to_int_operand(uint8_t* value);
		static flx_float to_float_operand(uint8_t* value);
		static flx_char to_char_operand(uint8_t* value);
		static flx_string to_string_operand(uint8_t* value);
		static std::vector<Operand> to_vector_operand(uint8_t* buffer);

		std::string string() const;

		template <typename T>
		static uint8_t* to_byte_operand(T value) {
			static_assert(std::is_arithmetic<T>::value, "Only arithmetic types are supported");

			uint8_t* buffer = new uint8_t[sizeof(T)];

			std::memcpy(buffer, &value, sizeof(T));

			return buffer;
		}

		static uint8_t* to_byte_operand(const std::string& str);

		static uint8_t* to_byte_operand(const std::vector<Operand>& vec);

	};

}

#endif // !OPERAND_HPP
