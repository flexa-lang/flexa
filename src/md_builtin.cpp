#include "md_builtin.hpp"

#include <iostream>
#include <thread>
#if defined(_WIN32)
#include <conio.h>
#include <sys/wait.h>
#endif // defined(_WIN32)

#include "types.hpp"
#include "semantic_analysis.hpp"
#include "compiler.hpp"
#include "vm.hpp"
#include "constants.hpp"
#include "visitor.hpp"
#include "utils.hpp"

using namespace core;
using namespace core::modules;
using namespace core::runtime;
using namespace core::analysis;
using namespace utils;

ModuleBuiltin::ModuleBuiltin() {
	build_decls();
}

ModuleBuiltin::~ModuleBuiltin() = default;

void ModuleBuiltin::register_functions(SemanticAnalyser* visitor) {
	auto back_scope = visitor->get_back_scope(Constants::DEFAULT_NAMESPACE);
	back_scope->declare_struct_definition(struct_decls[BuiltinStructs::BS_ENTRY]);
	back_scope->declare_struct_definition(struct_decls[BuiltinStructs::BS_EXCEPTION]);
	back_scope->declare_struct_definition(struct_decls[BuiltinStructs::BS_CONTEXT]);

	auto func_scope = visitor->get_global_scope(Constants::BUILTIN_MODULE_NAME);

	func_scope->declare_function(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_LOG],
		func_decls[BuiltinFuncs::BF_LOG]
	);
	visitor->builtin_functions[Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_LOG]] = nullptr;

	func_scope->declare_function(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_PRINT],
		func_decls[BuiltinFuncs::BF_PRINT]
	);
	visitor->builtin_functions[Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_PRINT]] = nullptr;

	func_scope->declare_function(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_PRINTLN],
		func_decls[BuiltinFuncs::BF_PRINTLN]
	);
	visitor->builtin_functions[Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_PRINTLN]] = nullptr;

	func_scope->declare_function(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_READ],
		func_decls[BuiltinFuncs::BF_READ]
	);
	visitor->builtin_functions[Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_READ]] = nullptr;

	func_scope->declare_function(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_READCH],
		func_decls[BuiltinFuncs::BF_READCH]
	);
	visitor->builtin_functions[Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_READCH]] = nullptr;

	func_scope->declare_function(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_LEN],
		func_decls[BuiltinFuncs::BF_LEN]
	);
	func_scope->declare_function(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_LENS],
		func_decls[BuiltinFuncs::BF_LENS]
	);
	visitor->builtin_functions[Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_LEN]] = nullptr;

	func_scope->declare_function(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_SLEEP],
		func_decls[BuiltinFuncs::BF_SLEEP]
	);
	visitor->builtin_functions[Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_SLEEP]] = nullptr;

	func_scope->declare_function(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_SYSTEM],
		func_decls[BuiltinFuncs::BF_SYSTEM]
	);
	visitor->builtin_functions[Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_SYSTEM]] = nullptr;

}

void ModuleBuiltin::register_functions(VirtualMachine* vm) {
	auto back_scope = vm->get_back_scope(Constants::DEFAULT_NAMESPACE);
	back_scope->declare_struct_definition(struct_decls[BuiltinStructs::BS_ENTRY]);
	back_scope->declare_struct_definition(struct_decls[BuiltinStructs::BS_EXCEPTION]);
	back_scope->declare_struct_definition(struct_decls[BuiltinStructs::BS_CONTEXT]);

	auto func_scope = vm->get_global_scope(Constants::BUILTIN_MODULE_NAME);

	func_scope->declare_function(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_LOG],
		func_decls[BuiltinFuncs::BF_LOG]
	);
	vm->builtin_functions[Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_LOG]] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::DEFAULT_NAMESPACE);
		if (scope->already_declared_variable("args")) {
			auto var = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("args"));
			auto args = var->get_value()->get_arr();

			for (flx_int i = 0; i < args.size(); ++i) {
				std::cout << RuntimeOperations::parse_value_to_string(args[i], true);
			}
		}

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	func_scope->declare_function(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_PRINT],
		func_decls[BuiltinFuncs::BF_PRINT]
	);
	vm->builtin_functions[Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_PRINT]] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::DEFAULT_NAMESPACE);
		if (scope->already_declared_variable("args")) {
			auto var = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("args"));
			auto args = var->get_value()->get_arr();

			for (flx_int i = 0; i < args.size(); ++i) {
				std::cout << RuntimeOperations::parse_value_to_string(args[i]);
			}
		}

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	func_scope->declare_function(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_PRINTLN],
		func_decls[BuiltinFuncs::BF_PRINTLN]
	);
	vm->builtin_functions[Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_PRINTLN]] = [this, vm]() {
		vm->builtin_functions[Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_PRINT]]();
		std::cout << std::endl;
		};

	func_scope->declare_function(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_READ],
		func_decls[BuiltinFuncs::BF_READ]
	);
	vm->builtin_functions[Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_READ]] = [this, vm]() {
		vm->builtin_functions[Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_PRINT]]();
		std::string line;
		std::getline(std::cin, line);
		vm->push_new_constant(new RuntimeValue(flx_string(std::move(line))));
		};

	func_scope->declare_function(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_READCH],
		func_decls[BuiltinFuncs::BF_READCH]
	);
	vm->builtin_functions[Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_READCH]] = [this, vm]() {
		while (!_kbhit());
		char ch = _getch();
		vm->push_new_constant(new RuntimeValue(flx_char(ch)));
		};

	func_scope->declare_function(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_LEN],
		func_decls[BuiltinFuncs::BF_LEN]
	);
	func_scope->declare_function(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_LENS],
		func_decls[BuiltinFuncs::BF_LENS]
	);
	vm->builtin_functions[Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_LEN]] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::DEFAULT_NAMESPACE);
		auto var = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("it"));
		auto itval = var->get_value();

		auto val = new RuntimeValue(Type::T_INT);

		if (itval->is_array()) {
			val->set(flx_int(itval->get_arr().size()));
		}
		else {
			val->set(flx_int(itval->get_s().size()));
		}

		vm->push_new_constant(val);

		};

	func_scope->declare_function(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_SLEEP],
		func_decls[BuiltinFuncs::BF_SLEEP]
	);
	vm->builtin_functions[Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_SLEEP]] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::DEFAULT_NAMESPACE);
		auto var = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("ms"));
		auto ms = var->get_value()->get_i();

		std::this_thread::sleep_for(std::chrono::milliseconds(ms));

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	func_scope->declare_function(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_SYSTEM],
		func_decls[BuiltinFuncs::BF_SYSTEM]
	);
	vm->builtin_functions[Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_SYSTEM]] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::DEFAULT_NAMESPACE);
		auto var = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("cmd"));
		auto cmd = var->get_value()->get_s();

		int rc = system(cmd.c_str());

		flx_int res = rc;

#ifdef linux

		if (rc == -1) {
			// failed to execute the command (fork failed, etc.)
			res = -1; // or handle error appropriately
		} 
		else if (WIFEXITED(rc)) {
			// normal termination
			res = WEXITSTATUS(rc);
		} 
		else if (WIFSIGNALED(rc)) {
			// terminated by signal
			int signal_num = WTERMSIG(rc);
			// return -signal_num or handle as needed
			res = -signal_num;
		} 
		else if (WIFSTOPPED(rc)) {
			// process stopped (if using WUNTRACED)
			int signal_num = WSTOPSIG(rc);
			res = -signal_num;
		} 
		else {
			// other cases (shouldn't happen normally)
			res = -1;
		}

#endif

		vm->push_new_constant(new RuntimeValue(res));

		};

}

void ModuleBuiltin::build_decls() {
	struct_decls = std::vector<std::shared_ptr<StructDefinition>>(BuiltinStructs::BS_SIZE);

	struct_decls[BuiltinStructs::BS_ENTRY] = std::make_shared<StructDefinition>(
		Constants::BUILTIN_STRUCT_NAMES[BuiltinStructs::BS_ENTRY],
		std::map<std::string, std::shared_ptr<VariableDefinition>>
		{
			{
				Constants::STR_ENTRY_FIELD_NAMES[StrEntryFields::SEF_KEY], std::make_shared<VariableDefinition>(
					Constants::STR_ENTRY_FIELD_NAMES[StrEntryFields::SEF_KEY], TypeDefinition(Type::T_STRING),
					nullptr,
					false,
					true
				)
			},
			{
				Constants::STR_ENTRY_FIELD_NAMES[StrEntryFields::SEF_VALUE], std::make_shared<VariableDefinition>(
					Constants::STR_ENTRY_FIELD_NAMES[StrEntryFields::SEF_VALUE],
					TypeDefinition(Type::T_ANY),
					nullptr,
					false,
					true
				)
			}
		}
	);

	struct_decls[BuiltinStructs::BS_EXCEPTION] = std::make_shared<StructDefinition>(
		Constants::BUILTIN_STRUCT_NAMES[BuiltinStructs::BS_EXCEPTION],
		std::map<std::string, std::shared_ptr<VariableDefinition>>
		{
			{
				Constants::STR_EXCEPTION_FIELD_NAMES[StrExceptionFields::SXF_ERROR],
				std::make_shared<VariableDefinition>(
					Constants::STR_EXCEPTION_FIELD_NAMES[StrExceptionFields::SXF_ERROR], TypeDefinition(Type::T_STRING),
					nullptr,
					false,
					true
				)
			},
			{
				Constants::STR_EXCEPTION_FIELD_NAMES[StrExceptionFields::SXF_CODE],
				std::make_shared<VariableDefinition>(
					Constants::STR_EXCEPTION_FIELD_NAMES[StrExceptionFields::SXF_CODE],
					TypeDefinition(Type::T_INT),
					nullptr,
					false,
					true
				)
			}
		}
	);

	struct_decls[BuiltinStructs::BS_CONTEXT] = std::make_shared<StructDefinition>(
		Constants::BUILTIN_STRUCT_NAMES[BuiltinStructs::BS_CONTEXT],
		std::map<std::string, std::shared_ptr<VariableDefinition>>
		{
			{
				Constants::STR_CONTEXT_FIELD_NAMES[StrContextFields::SCF_NAME],
				std::make_shared<VariableDefinition>(
					Constants::STR_CONTEXT_FIELD_NAMES[StrContextFields::SCF_NAME],
					TypeDefinition(Type::T_STRING),
					nullptr,
					false,
					true
				)
			},
			{
				Constants::STR_CONTEXT_FIELD_NAMES[StrContextFields::SCF_NAMESPACE],
				std::make_shared<VariableDefinition>(
					Constants::STR_CONTEXT_FIELD_NAMES[StrContextFields::SCF_NAMESPACE],
					TypeDefinition(Type::T_STRING),
					nullptr,
					false,
					true
				)
			},
			{
				Constants::STR_CONTEXT_FIELD_NAMES[StrContextFields::SCF_TYPE],
				std::make_shared<VariableDefinition>(
					Constants::STR_CONTEXT_FIELD_NAMES[StrContextFields::SCF_TYPE],
					TypeDefinition(Type::T_STRING),
					nullptr,
					false,
					true
				)
			}
		}
	);

	func_decls = std::vector<std::shared_ptr<FunctionDefinition>>(BuiltinFuncs::BF_SIZE);

	std::vector<std::shared_ptr<TypeDefinition>> parameters;
	std::shared_ptr<VariableDefinition> variable;

	parameters = std::vector<std::shared_ptr<TypeDefinition>>();
	variable = std::make_shared<VariableDefinition>(
		"args",
		TypeDefinition(Type::T_ANY),
		std::make_shared<ASTNullNode>(0, 0),
		true
	);
	parameters.push_back(variable);
	func_decls[BuiltinFuncs::BF_LOG] = std::make_shared<FunctionDefinition>(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_LOG],
		Type::T_VOID,
		parameters,
		std::make_shared<ASTBlockNode>(
			std::vector<std::shared_ptr<ASTNode>>{}, 0, 0
		)
	);

	parameters = std::vector<std::shared_ptr<TypeDefinition>>();
	variable = std::make_shared<VariableDefinition>(
		"args",
		TypeDefinition(Type::T_ANY),
		std::make_shared<ASTNullNode>(0, 0),
		true
	);
	parameters.push_back(variable);
	func_decls[BuiltinFuncs::BF_PRINT] = std::make_shared<FunctionDefinition>(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_PRINT],
		Type::T_VOID,
		parameters,
		std::make_shared<ASTBlockNode>(
			std::vector<std::shared_ptr<ASTNode>>{}, 0, 0
		)
	);

	parameters = std::vector<std::shared_ptr<TypeDefinition>>();
	variable = std::make_shared<VariableDefinition>(
		"args",
		TypeDefinition(Type::T_ANY),
		std::make_shared<ASTNullNode>(0, 0),
		true
	);
	parameters.push_back(variable);
	func_decls[BuiltinFuncs::BF_PRINTLN] = std::make_shared<FunctionDefinition>(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_PRINTLN],
		Type::T_VOID,
		parameters,
		std::make_shared<ASTBlockNode>(
			std::vector<std::shared_ptr<ASTNode>>{}, 0, 0
		)
	);

	parameters = std::vector<std::shared_ptr<TypeDefinition>>();
	variable = std::make_shared<VariableDefinition>(
		"args",
		TypeDefinition(Type::T_ANY),
		std::make_shared<ASTNullNode>(0, 0),
		true
	);
	parameters.push_back(variable);
	func_decls[BuiltinFuncs::BF_READ] = std::make_shared<FunctionDefinition>(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_READ],
		Type::T_STRING,
		parameters,
		std::make_shared<ASTBlockNode>(
			std::vector<std::shared_ptr<ASTNode>>{}, 0, 0
		)
	);

	parameters = std::vector<std::shared_ptr<TypeDefinition>>();

	func_decls[BuiltinFuncs::BF_READCH] = std::make_shared<FunctionDefinition>(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_READCH],
		Type::T_CHAR,
		parameters,
		std::make_shared<ASTBlockNode>(
			std::vector<std::shared_ptr<ASTNode>>{}, 0, 0
		)
	);

	parameters = std::vector<std::shared_ptr<TypeDefinition>>();
	variable = std::make_shared<VariableDefinition>("it", TypeDefinition(Type::T_ANY, std::vector<size_t>{0}), nullptr);
	parameters.push_back(variable);
	func_decls[BuiltinFuncs::BF_LEN] = std::make_shared<FunctionDefinition>(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_LEN],
		Type::T_INT,
		parameters,
		std::make_shared<ASTBlockNode>(
			std::vector<std::shared_ptr<ASTNode>>{}, 0, 0
		)
	);

	parameters = std::vector<std::shared_ptr<TypeDefinition>>();
	variable = std::make_shared<VariableDefinition>("it", TypeDefinition(Type::T_STRING), nullptr);
	parameters.push_back(variable);
	func_decls[BuiltinFuncs::BF_LENS] = std::make_shared<FunctionDefinition>(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_LENS],
		Type::T_INT,
		parameters,
		std::make_shared<ASTBlockNode>(
			std::vector<std::shared_ptr<ASTNode>>{}, 0, 0
		)
	);

	parameters = std::vector<std::shared_ptr<TypeDefinition>>();
	variable = std::make_shared<VariableDefinition>("ms", TypeDefinition(Type::T_INT), nullptr);
	parameters.push_back(variable);
	func_decls[BuiltinFuncs::BF_SLEEP] = std::make_shared<FunctionDefinition>(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_SLEEP],
		Type::T_VOID, parameters, std::make_shared<ASTBlockNode>(
			std::vector<std::shared_ptr<ASTNode>>{}, 0, 0
		)
	);

	parameters = std::vector<std::shared_ptr<TypeDefinition>>();
	variable = std::make_shared<VariableDefinition>("cmd", TypeDefinition(Type::T_STRING), nullptr);
	parameters.push_back(variable);
	func_decls[BuiltinFuncs::BF_SYSTEM] = std::make_shared<FunctionDefinition>(
		Constants::BUILTIN_FUNCTION_NAMES[BuiltinFuncs::BF_SYSTEM],
		Type::T_INT, parameters, std::make_shared<ASTBlockNode>(
			std::vector<std::shared_ptr<ASTNode>>{}, 0, 0
		)
	);

}
