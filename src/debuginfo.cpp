#include "debuginfo.hpp"

#include "constants.hpp"

using namespace core;

DebugInfo::DebugInfo(
	std::string module_name_space,
	std::string module_name,
	std::string ast_type,
	std::string access_name_space,
	std::string identifier,
	size_t row,
	size_t col
)
	: module_name_space(module_name_space),
	module_name(module_name),
	ast_type(ast_type),
	access_name_space(access_name_space),
	identifier(identifier),
	row(row),
	col(col) {
}

void DebugInfo::set_dbg_info(
	std::string module_name_space,
	std::string module_name,
	std::string ast_type,
	std::string access_name_space,
	std::string identifier,
	size_t row,
	size_t col
) {
	this->module_name_space = module_name_space;
	this->module_name = module_name;
	this->access_name_space = access_name_space;
	this->identifier = identifier;
	this->ast_type = ast_type;
	this->row = row;
	this->col = col;
}

std::string DebugInfo::build_error_message(const std::string& error_type, const std::string& error) {
	return "" + error_type + ": " + error + build_error_tail();
}

std::string DebugInfo::build_error_tail() {
	std::string error_tail = "\n at ";

	if (identifier.empty()) {
		error_tail += ast_type;
	}
	else {
		//error_tail += access_name_space + (access_name_space.empty() ? "" : "::");
		error_tail += identifier;
	}

	error_tail += " (";
	if (module_name_space != Constants::DEFAULT_NAMESPACE) {
		error_tail += module_name_space + (module_name_space.empty() ? "" : "::");
	}
	error_tail += module_name + ":";

	error_tail += std::to_string(row) + ":" + std::to_string(col) + ")";

	return error_tail;
}
