#include "exception_helper.hpp"

#include "constants.hpp"

using namespace core;

std::stack<std::string> ExceptionHelper::error_stack;

void ExceptionHelper::throw_stack_err() {
	if (!ExceptionHelper::error_stack.empty()) {
		std::string err = ExceptionHelper::error_stack.top();
		ExceptionHelper::error_stack.pop();
		throw std::runtime_error(err);
	}
}

void ExceptionHelper::throw_operation_err(const std::string op, const TypeDefinition& ltype, const TypeDefinition& rtype) {
	ExceptionHelper::throw_stack_err();
	throw std::runtime_error("invalid '" + op + "' operation for types '" + TypeDefinition::buid_type_str(ltype)
		+ "' and '" + TypeDefinition::buid_type_str(rtype) + "'");
}

void ExceptionHelper::throw_unary_operation_err(const std::string op, const TypeDefinition& type) {
	ExceptionHelper::throw_stack_err();
	throw std::runtime_error("incompatible unary operator '" + op +
		"' in front of " + TypeDefinition::buid_type_str(type) + " expression");
}

void ExceptionHelper::throw_declaration_type_err(const std::string& identifier, const TypeDefinition& ltype, const TypeDefinition& rtype) {
	ExceptionHelper::throw_stack_err();
	throw std::runtime_error("found " + TypeDefinition::buid_type_str(rtype)
		+ " in definition of '" + identifier
		+ "', expected " + TypeDefinition::buid_type_str(ltype) + " type");
}

void ExceptionHelper::throw_return_type_err(const std::string& identifier, const TypeDefinition& ltype, const TypeDefinition& rtype) {
	ExceptionHelper::throw_stack_err();
	throw std::runtime_error("invalid " + TypeDefinition::buid_type_str(ltype)
		+ " return type for '" + identifier
		+ "' function with " + TypeDefinition::buid_type_str(rtype)
		+ " return type");
}

void ExceptionHelper::throw_mismatched_type_err(const TypeDefinition& ltype, const TypeDefinition& rtype) {
	ExceptionHelper::throw_stack_err();
	throw std::runtime_error("mismatched types " + TypeDefinition::buid_type_str(ltype)
		+ " and " + TypeDefinition::buid_type_str(rtype));
}

void ExceptionHelper::throw_condition_type_err() {
	ExceptionHelper::throw_stack_err();
	throw std::runtime_error("conditions must be boolean expression");
}

void ExceptionHelper::throw_struct_type_err(const std::string& type_name_space, const std::string& type_name, const TypeDefinition& type) {
	ExceptionHelper::throw_stack_err();
	throw std::runtime_error("invalid type " + TypeDefinition::buid_type_str(type) +
		" trying to assign '" + TypeDefinition::buid_struct_type_name(type_name_space, type_name) + "' struct");
}

void ExceptionHelper::throw_struct_value_assign_type_err(const std::string& type_name_space, const std::string& type_name,
	const std::string& identifier, const TypeDefinition& ltype, const TypeDefinition& rtype) {
	ExceptionHelper::throw_stack_err();
	throw std::runtime_error("invalid type " + TypeDefinition::buid_type_str(rtype) +
		" trying to assign '" + identifier + "' member of '" + TypeDefinition::buid_struct_type_name(type_name_space, type_name) + "' struct, "
		"expected " + TypeDefinition::buid_type_str(ltype));
}

void ExceptionHelper::throw_struct_member_err(const std::string& type_name_space, const std::string& type_name, const std::string& variable) {
	ExceptionHelper::throw_stack_err();
	throw std::runtime_error("'" + variable + "' is not a member of '" + TypeDefinition::buid_struct_type_name(type_name_space, type_name) + "'");
}

void ExceptionHelper::throw_undeclared_function(const std::string& identifier, const std::vector<std::shared_ptr<TypeDefinition>> signature) {
	ExceptionHelper::throw_stack_err();
	std::string func_name = ExceptionHelper::buid_signature(identifier, signature);
	throw std::runtime_error("function '" + func_name + "' was never declared");
}

std::string ExceptionHelper::buid_member_name(const std::vector<Identifier>& identifier_vector) {
	std::string ss;
	
	for (auto& id : identifier_vector) {
		ss += id.identifier;
		if (id.access_vector.size() > 0) {
			for (size_t i = 0; i < id.access_vector.size(); ++i) {
				ss += "[]";
			}
		}
		ss += ".";
	}

	if (ss.ends_with(".")) {
		ss.erase(ss.end());
	}

	return ss;
}

std::string ExceptionHelper::buid_signature(const std::vector<Identifier>& identifier_vector, const std::vector<std::shared_ptr<TypeDefinition>> signature) {
	std::string ss = buid_member_name(identifier_vector) + "(";
	for (const auto& param : signature) {
		ss += TypeDefinition::buid_type_str(*param) + ", ";
	}
	if (signature.size() > 0) {
		ss.pop_back();
		ss.pop_back();
	}
	ss += ")";

	return ss;
}

std::string ExceptionHelper::buid_signature(const std::string& identifier, const std::vector<std::shared_ptr<TypeDefinition>> signature) {
	std::string ss= identifier + "(";
	for (const auto& param : signature) {
		ss += TypeDefinition::buid_type_str(*param) + ", ";
	}
	if (signature.size() > 0) {
		ss.pop_back();
		ss.pop_back();
	}
	ss += ")";

	return ss;
}
