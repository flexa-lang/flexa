#ifndef DEBUGINFO_HPP
#define DEBUGINFO_HPP

#include <string>

namespace core {

	class DebugInfo {
	public:
		std::string module_name_space;
		std::string module_name;
		std::string ast_type;
		std::string access_name_space;
		std::string identifier;
		size_t row = 0;
		size_t col = 0;

		DebugInfo(
			std::string module_name_space,
			std::string module_name,
			std::string ast_type,
			std::string access_name_space,
			std::string identifier,
			size_t row = 0,
			size_t col = 0
		);

		virtual void set_dbg_info(
			std::string module_name_space,
			std::string module_name,
			std::string ast_type,
			std::string access_name_space,
			std::string identifier,
			size_t row,
			size_t col
		);

		virtual std::string build_error_message(const std::string& error_type, const std::string& error);
		virtual std::string build_error_tail();

	};

}

#endif // !DEBUGINFO_HPP
