#include "vm_debug.hpp"

using namespace core;

void VmDebug::add_module(std::string module_name) {
	if (std::find(module_names.begin(), module_names.end(), module_name) == module_names.end()) {
		module_names.push_back(module_name);
	}
}

void VmDebug::add_namespace(std::string namespace_name) {
	if (std::find(namespace_names.begin(), namespace_names.end(), namespace_name) == namespace_names.end()) {
		namespace_names.push_back(namespace_name);
	}
}

size_t VmDebug::index_of_ast_type(std::string ast_type) {
	auto it = std::find(ast_types.begin(), ast_types.end(), ast_type);
	if (it != ast_types.end()) {
		return std::distance(ast_types.begin(), it);
	}
	throw std::runtime_error("invalid type index");
}

size_t VmDebug::index_of_module(std::string module_name) {
	auto it = std::find(module_names.begin(), module_names.end(), module_name);
	if (it != module_names.end()) {
		return std::distance(module_names.begin(), it);
	}
	throw std::runtime_error("invalid module index");
}

size_t VmDebug::index_of_namespace(std::string namespace_name) {
	auto it = std::find(namespace_names.begin(), namespace_names.end(), namespace_name);
	if (it != namespace_names.end()) {
		return std::distance(namespace_names.begin(), it);
	}
	throw std::runtime_error("invalid namespace index");
}

std::string VmDebug::get_ast_type(size_t index) {
	return ast_types[index];
}

std::string VmDebug::get_module(size_t index) {
	return module_names[index];
}

std::string VmDebug::get_namespace(size_t index) {
	return namespace_names[index];
}
