#include "scope.hpp"

#include "types.hpp"
#include "utils.hpp"

using namespace core;

Scope::Scope(std::string module_name_space, std::string module_name)
	: module_name_space(module_name_space), module_name(module_name) {
}

Scope::Scope() = default;

Scope::~Scope() = default;

std::shared_ptr<ClassDefinition> Scope::find_declared_class_definition(const std::string& identifier) {
	return class_symbol_table.at(identifier);
}

std::shared_ptr<StructDefinition> Scope::find_declared_struct_definition(const std::string& identifier) {
	return struct_symbol_table.at(identifier);
}

std::shared_ptr<Variable> Scope::find_declared_variable(const std::string& identifier) {
	auto& var = variable_symbol_table.at(identifier);
	return var;
}

std::shared_ptr<FunctionDefinition>& Scope::find_declared_function(
	const std::string& identifier,
	const std::vector<std::shared_ptr<TypeDefinition>>* signature,
	bool strict
) {
	auto funcs = function_symbol_table.equal_range(identifier);

	if (std::distance(funcs.first, funcs.second) == 0) {
		throw std::runtime_error("definition of '" + identifier + "' function signature not found");
	}

	for (auto& it = funcs.first; it != funcs.second; ++it) {
		if (!signature) {
			return it->second;
		}

		auto& func_sig = it->second->parameters;
		bool rest = false;
		auto found = true;
		std::shared_ptr<TypeDefinition> stype = nullptr;
		std::shared_ptr<TypeDefinition> ftype = nullptr;
		size_t func_sig_size = func_sig.size();
		size_t call_sig_size = signature->size();

		// if signatures size match, handle normal cases
		if (func_sig_size == call_sig_size) {
			for (size_t i = 0; i < call_sig_size; ++i) {
				ftype = func_sig.at(i);
				stype = signature->at(i);

				if (!ftype->is_any_or_match_type_def(*stype, strict)) {
					found = false;
					break;
				}
			}

			if (found) {
				return it->second;
			}
		}

		// if function signature is lesser than signature call, handle rest case
		found = true;
		if (func_sig_size >= 1 && func_sig_size < call_sig_size) {
			for (size_t i = 0; i < call_sig_size; ++i) {
				if (!rest) {
					ftype = func_sig.at(i);

					auto parameter = std::dynamic_pointer_cast<VariableDefinition>(it->second->parameters[i]);

					if (parameter && parameter->is_rest) {
						rest = true;
						if (ftype->is_array()) {
							ftype = std::make_shared<TypeDefinition>(ftype->type, ftype->type_name_space, ftype->type_name);
						}
					}

					if (parameter && !parameter->is_rest && i == func_sig.size() - 1) {
						found = false;
						break;
					}
				}
				stype = signature->at(i);

				if (!ftype->is_any_or_match_type_def(*stype, strict)) {
					found = false;
					break;
				}
			}

			if (found) {
				return it->second;
			}
		}

		// if function signature is greater than signature call, handle default value cases
		found = true;
		if (func_sig_size > call_sig_size) {
			for (size_t i = 0; i < func_sig_size; ++i) {
				if (i < call_sig_size) {
					ftype = func_sig.at(i);
					stype = signature->at(i);

					if (!ftype->is_any_or_match_type_def(*stype, strict)) {
						found = false;
						break;
					}
				}
				else {
					auto parameter = std::dynamic_pointer_cast<VariableDefinition>(it->second->parameters[i]);

					if (parameter &&
						(parameter->has_expr_default() && !parameter->get_expr_default()
							|| parameter->has_pc_default() && !parameter->get_pc_default())) {
						found = false;
						break;
					}
				}
			}

			// if found and exactly signature size (not rest)
			if (found) {
				return it->second;
			}
		}
	}

	throw std::runtime_error("something went wrong when determining the type of '" + identifier + "' function");
}

bool Scope::already_declared_class_definition(const std::string& identifier) {
	return class_symbol_table.find(identifier) != class_symbol_table.end();
}

bool Scope::already_declared_struct_definition(const std::string& identifier) {
	return struct_symbol_table.find(identifier) != struct_symbol_table.end();
}

bool Scope::already_declared_variable(const std::string& identifier) {
	return variable_symbol_table.find(identifier) != variable_symbol_table.end();
}

bool Scope::already_declared_function(
	const std::string& identifier,
	const std::vector<std::shared_ptr<TypeDefinition>>* signature,
	bool strict
) {
	try {
		find_declared_function(identifier, signature, strict);
		return true;
	}
	catch (...) {
		return false;
	}
}

size_t Scope::total_declared_variables() {
	return variable_symbol_table.size();
}

void Scope::declare_class_definition(std::shared_ptr<ClassDefinition> structure) {
	class_symbol_table[structure->identifier] = structure;
}

void Scope::declare_struct_definition(std::shared_ptr<StructDefinition> structure) {
	struct_symbol_table[structure->identifier] = structure;
}

void Scope::declare_variable(const std::string& identifier, const std::shared_ptr<Variable>& variable) {
	variable_symbol_table[identifier] = variable;
}

void Scope::declare_function(const std::string& identifier, std::shared_ptr<FunctionDefinition> function) {
	function_symbol_table.insert(std::make_pair(identifier, function));
}
