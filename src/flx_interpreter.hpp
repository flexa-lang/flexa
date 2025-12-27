#ifndef FLX_INTERPRETER_HPP
#define FLX_INTERPRETER_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "ast.hpp"
#include "flx_utils.hpp"

namespace interpreter {

	class FlexaInterpreter {
	private:
		std::string project_root;
		std::string libs_root;
		FlexaCliArgs args;

	public:
		FlexaInterpreter(const FlexaCliArgs& args);

		core::flx_int execute();

	private:
		FlexaSource load_module(const std::string& source);
		std::vector<FlexaSource> load_modules(const std::vector<std::string>& source_files);

		void parse_modules(
			const std::vector<FlexaSource>& source_modules,
			std::shared_ptr<core::ASTModuleNode>* main_module,
			std::map<std::string, std::shared_ptr<core::ASTModuleNode>>* modules
		);

		core::flx_int interpreter();

	};

}

#endif // !FLX_INTERPRETER_HPP
