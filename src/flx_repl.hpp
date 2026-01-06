#ifndef FLX_REPL_HPP
#define FLX_REPL_HPP

#include <string>

#include "flx_utils.hpp"

#ifdef linux
#define clear_screen() std::ignore = system("clear")
#elif defined(_WIN32)
#define clear_screen() std::ignore = system("cls")
#endif // !linux

namespace interpreter {

	class FlexaRepl {
	public:
		static void remove_header(std::string& err);
		static void count_scopes(const std::string& input_line, size_t& open_scopes);
		static std::string read(const std::string& msg);
		static int execute(const FlexaCliArgs& args);

	};

}

#endif // !FLX_REPL_HPP
