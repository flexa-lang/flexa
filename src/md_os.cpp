#include "md_os.hpp"

#include <filesystem>

#include "semantic_analysis.hpp"
#include "constants.hpp"

using namespace core::modules;
using namespace core::runtime;
using namespace core::analysis;

ModuleOS::ModuleOS() {}

ModuleOS::~ModuleOS() = default;

void ModuleOS::register_functions(SemanticAnalyser* visitor) {
	auto& module = visitor->modules[Constants::CORE_LIB_NAMES[CoreLibs::CL_OS]];
	
	auto str_constr = std::dynamic_pointer_cast<ASTStructConstructorNode>(std::dynamic_pointer_cast<ASTDeclarationNode>(module->statements[1])->expr);

	str_constr->values = std::map<std::string, std::shared_ptr<ASTExprNode>>{
		{"cwd", std::make_shared<ASTLiteralNode<flx_string>>(std::filesystem::current_path().string(), 0, 0)},
#ifdef linux
		{"name", std::make_shared<ASTLiteralNode<flx_string>>("linux", 0, 0)},
#elif defined(_WIN32)
		{"name", std::make_shared<ASTLiteralNode<flx_string>>("windows", 0, 0)},
#endif // linux
	};
	 
}

void ModuleOS::register_functions(VirtualMachine*) {}
