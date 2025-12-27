#include "flx_interpreter.hpp"

#include <filesystem>

#include "lexer.hpp"
#include "parser.hpp"
#include "utils.hpp"
#include "dependency_resolver.hpp"
#include "compiler.hpp"
#include "vm.hpp"
#include "semantic_analysis.hpp"

using namespace interpreter;
using namespace core;
using namespace core::parser;
using namespace core::analysis;
using namespace core::runtime;

FlexaInterpreter::FlexaInterpreter(const FlexaCliArgs& args)
	: project_root(utils::PathUtils::normalize_path_sep(args.workspace_path)),
	libs_root(utils::PathUtils::normalize_path_sep((args.libs_path.empty() ? utils::PathUtils::get_current_path() : args.libs_path) + "\\libs")),
	args(args) {}

core::flx_int FlexaInterpreter::execute() {
	if (!args.main_file.empty() || args.source_files.size() > 0) {
		return interpreter();
	}

	return 0;
}

FlexaSource FlexaInterpreter::load_module(const std::string& source) {
	FlexaSource source_module;

	auto current_file_path = std::string{ std::filesystem::path::preferred_separator } + utils::PathUtils::normalize_path_sep(source);
	std::string current_full_path = "";

	if (std::filesystem::exists(project_root + current_file_path)) {
		current_full_path = project_root + current_file_path;
	}
	else if (std::filesystem::exists(libs_root + current_file_path)) {
		current_full_path = libs_root + current_file_path;
	}
	else {
		throw std::runtime_error("file not found: '" + current_file_path + "'");
	}

	source_module = FlexaSource{ FlxUtils::get_lib_name(source), FlxUtils::load_source(current_full_path) };

	return source_module;
}

std::vector<FlexaSource> FlexaInterpreter::load_modules(const std::vector<std::string>& source_files) {
	std::vector<FlexaSource> source_modules;

	for (const auto& source : source_files) {
		source_modules.push_back(load_module(source));
	}

	return source_modules;
}

void FlexaInterpreter::parse_modules(const std::vector<FlexaSource>& source_modules, std::shared_ptr<ASTModuleNode>* main_module,
	std::map<std::string, std::shared_ptr<ASTModuleNode>>* modules) {

	for (const auto& source : source_modules) {
		Lexer lexer(source.name, source.source);
		parser::Parser parser(source.name , &lexer);

		std::shared_ptr<ASTModuleNode> module = parser.parse_module();

		if (!module) {
			std::cerr << "Failed to parse module: " << source.name << std::endl;
			continue;
		}

		if (!*main_module) {
			*main_module = module;
		}

		(*modules)[module->name] = module;
	}
}

core::flx_int FlexaInterpreter::interpreter() {
	FlexaSource main_module_src;
	std::vector<FlexaSource> source_modules;
	try {
		main_module_src = load_module(args.main_file);
		source_modules = load_modules(args.source_files);
		source_modules.emplace(source_modules.begin(), main_module_src);
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	try {
		std::shared_ptr<ASTModuleNode> main_module = nullptr;
		std::map<std::string, std::shared_ptr<ASTModuleNode>> modules;
		parse_modules(source_modules, &main_module, &modules);
		size_t libs_size = 0;
		do {
			DependencyResolver dependency_resolver(main_module, modules);
			dependency_resolver.start();

			libs_size = dependency_resolver.lib_names.size();

			if (libs_size > 0) {
				auto flxlib_modules = load_modules(dependency_resolver.lib_names);
				parse_modules(flxlib_modules, &main_module, &modules);
			}
		} while (libs_size > 0);

		std::shared_ptr<Scope> semantic_global_scope = std::make_shared<Scope>(main_module->name_space, main_module->name);
		std::shared_ptr<Scope> interpreter_global_scope = std::make_shared<Scope>(main_module->name_space, main_module->name);

		SemanticAnalyser semantic_analyser(semantic_global_scope, main_module, modules, args.program_args);
		semantic_analyser.start();

		core::flx_int result = 0;

		// compile
		Compiler compiler(main_module, modules);
		compiler.start();

		if (args.debug) {
			BytecodeInstruction::write_bytecode_table(compiler.bytecode_program, project_root + "\\" + main_module->name + ".flxt");
		}

		// execute
		VirtualMachine vm(interpreter_global_scope, compiler.vm_debug, compiler.bytecode_program);
		vm.run();

		result = vm.get_evaluation_stack_top()->get_i();

		return result;
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
