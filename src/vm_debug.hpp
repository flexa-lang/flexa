#ifndef VM_DEBUG
#define VM_DEBUG

#include <iostream>
#include <string>
#include <vector>
#include <array>

#include "operand.hpp"

namespace core {

	class VmDebug {
	private:
		std::vector<std::string> ast_types = std::vector<std::string>{
			"<expression>",
			"<statement>",
			"<program>"
		};
		std::vector<std::string> module_names = std::vector<std::string>{ "" };
		std::vector<std::string> namespace_names = std::vector<std::string>{ "" };

	public:
		std::map<size_t, std::vector<Operand>> debug_info_table;

		void add_module(std::string module_name);
		void add_namespace(std::string namespace_name);

		size_t index_of_ast_type(std::string ast_type);
		size_t index_of_module(std::string module_name);
		size_t index_of_namespace(std::string namespace_name);

		std::string get_ast_type(size_t index);
		std::string get_module(size_t index);
		std::string get_namespace(size_t index);
	};

}

#endif // !VM_DEBUG
