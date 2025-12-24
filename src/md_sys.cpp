#include "md_sys.hpp"

#include "semantic_analysis.hpp"
#include "constants.hpp"

using namespace core::modules;
using namespace core::runtime;
using namespace core::analysis;

ModuleSys::ModuleSys() {}

ModuleSys::~ModuleSys() = default;

void ModuleSys::register_functions(SemanticAnalyser* visitor) {
	auto& module = visitor->modules[Constants::CORE_LIB_NAMES[CoreLibs::CL_SYS]];

	auto str_constr = std::dynamic_pointer_cast<ASTStructConstructorNode>(std::dynamic_pointer_cast<ASTDeclarationNode>(module->statements[1])->expr);

	std::vector<std::shared_ptr<ASTExprNode>> arr_values = std::vector<std::shared_ptr<ASTExprNode>>();
	for (const auto& arg : visitor->args) {
		arr_values.push_back(std::make_shared<ASTLiteralNode<flx_string>>(arg, 0, 0));
	}

	str_constr->values = std::map<std::string, std::shared_ptr<ASTExprNode>>{
		{"argv", std::make_shared<ASTArrayConstructorNode>(arr_values, 0, 0)}
	};

}

void ModuleSys::register_functions(VirtualMachine*) {}
