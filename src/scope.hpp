#ifndef SCOPE_HPP
#define SCOPE_HPP

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>

namespace core {

	class ClassDefinition;
	class StructDefinition;
	class FunctionDefinition;
	class TypeDefinition;
	class Variable;

	class Scope {
	public:
		std::unordered_multimap<std::string, std::shared_ptr<FunctionDefinition>> function_symbol_table;
		std::unordered_map<std::string, std::shared_ptr<ClassDefinition>> class_symbol_table;
		std::unordered_map<std::string, std::shared_ptr<StructDefinition>> struct_symbol_table;
		std::unordered_map<std::string, std::shared_ptr<Variable>> variable_symbol_table;

	public:
		std::string module_name_space;
		std::string module_name;
		bool is_class = false;

		Scope(std::string module_name_space, std::string module_name, bool is_class = false);
		Scope(bool is_class = false);
		~Scope();

		bool already_declared_variable(const std::string& identifier);
		bool already_declared_struct_definition(const std::string& identifier);
		bool already_declared_class_definition(const std::string& identifier);
		bool already_declared_function(
			const std::string& identifier,
			const std::vector<std::shared_ptr<TypeDefinition>>* signature,
			bool strict = true
		);

		void declare_variable(const std::string& identifier, const std::shared_ptr<Variable>& variable);
		void declare_struct_definition(std::shared_ptr<StructDefinition> structure);
		void declare_class_definition(std::shared_ptr<ClassDefinition> structure);
		void declare_function(const std::string& identifier, std::shared_ptr<FunctionDefinition> function);

		std::shared_ptr<Variable> find_declared_variable(const std::string& identifier);
		std::shared_ptr<StructDefinition> find_declared_struct_definition(const std::string& identifier);
		std::shared_ptr<ClassDefinition> find_declared_class_definition(const std::string& identifier);
		std::shared_ptr<FunctionDefinition>& find_declared_function(
			const std::string& identifier,
			const std::vector<std::shared_ptr<TypeDefinition>>* signature,
			bool strict = true
		);

		size_t total_declared_variables();

	};

}

#endif // !SCOPE_HPP
