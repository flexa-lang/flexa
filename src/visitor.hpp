#ifndef VISITOR_HPP
#define VISITOR_HPP

#include <string>
#include <vector>
#include <stack>
#include <map>
#include <memory>

#include "types.hpp"
#include "debuginfo.hpp"

namespace core {

	class ASTModuleNode;
	class ASTUsingNode;
	class ASTIncludeNamespaceNode;
	class ASTExcludeNamespaceNode;
	class ASTDeclarationNode;
	class ASTUnpackedDeclarationNode;
	class ASTFunctionCallNode;
	class ASTReturnNode;
	class ASTBlockNode;
	class ASTContinueNode;
	class ASTBreakNode;
	class ASTSwitchNode;
	class ASTExitNode;
	class ASTEnumNode;
	class ASTElseIfNode;
	class ASTIfNode;
	class ASTTryCatchNode;
	class ASTThrowNode;
	class ASTEllipsisNode;
	class ASTForNode;
	class ASTForEachNode;
	class ASTWhileNode;
	class ASTDoWhileNode;
	class ASTFunctionDefinitionNode;
	class ASTStructDefinitionNode;
	class ASTClassDefinitionNode;
	template <typename T>
	class ASTLiteralNode;
	class ASTLambdaFunctionNode;
	class ASTExprNode;
	class ASTArrayConstructorNode;
	class ASTStructConstructorNode;
	class ASTBinaryExprNode;
	class ASTUnaryExprNode;
	class ASTIdentifierNode;
	class ASTTernaryNode;
	class ASTFunctionCallNode;
	class ASTTypeCastNode;
	class ASTTypeNode;
	class ASTNullNode;
	class ASTThisNode;
	class ASTTypeOfNode;
	class ASTTypeIdNode;
	class ASTRefIdNode;
	class ASTIsStructNode;
	class ASTIsArrayNode;
	class ASTIsAnyNode;
	class ASTInstructionNode;
	class ASTValueNode;

	class Visitor {
	public:
		std::map<std::string, std::shared_ptr<ASTModuleNode>> modules;
		std::shared_ptr<ASTModuleNode> main_module;
		std::stack<std::shared_ptr<ASTModuleNode>> current_module_stack;
		std::stack<DebugInfo> current_debug_info_stack;

		Visitor(
			const std::map<std::string, std::shared_ptr<ASTModuleNode>>& modules,
			std::shared_ptr<ASTModuleNode> main_module
		)
			: modules(modules),
			main_module(main_module) {
			current_module_stack.push(main_module);
		};

		virtual void visit(std::shared_ptr<ASTModuleNode>) = 0;
		virtual void visit(std::shared_ptr<ASTUsingNode>) = 0;
		virtual void visit(std::shared_ptr<ASTIncludeNamespaceNode>) = 0;
		virtual void visit(std::shared_ptr<ASTExcludeNamespaceNode>) = 0;
		virtual void visit(std::shared_ptr<ASTDeclarationNode>) = 0;
		virtual void visit(std::shared_ptr<ASTUnpackedDeclarationNode>) = 0;
		virtual void visit(std::shared_ptr<ASTReturnNode>) = 0;
		virtual void visit(std::shared_ptr<ASTBlockNode>) = 0;
		virtual void visit(std::shared_ptr<ASTContinueNode>) = 0;
		virtual void visit(std::shared_ptr<ASTBreakNode>) = 0;
		virtual void visit(std::shared_ptr<ASTExitNode>) = 0;
		virtual void visit(std::shared_ptr<ASTSwitchNode>) = 0;
		virtual void visit(std::shared_ptr<ASTElseIfNode>) = 0;
		virtual void visit(std::shared_ptr<ASTEnumNode>) = 0;
		virtual void visit(std::shared_ptr<ASTTryCatchNode>) = 0;
		virtual void visit(std::shared_ptr<ASTThrowNode>) = 0;
		virtual void visit(std::shared_ptr<ASTEllipsisNode>) = 0;
		virtual void visit(std::shared_ptr<ASTIfNode>) = 0;
		virtual void visit(std::shared_ptr<ASTForNode>) = 0;
		virtual void visit(std::shared_ptr<ASTForEachNode>) = 0;
		virtual void visit(std::shared_ptr<ASTWhileNode>) = 0;
		virtual void visit(std::shared_ptr<ASTDoWhileNode>) = 0;
		virtual void visit(std::shared_ptr<ASTFunctionDefinitionNode>) = 0;
		virtual void visit(std::shared_ptr<ASTStructDefinitionNode>) = 0;
		virtual void visit(std::shared_ptr<ASTLiteralNode<flx_bool>>) = 0;
		virtual void visit(std::shared_ptr<ASTLiteralNode<flx_int>>) = 0;
		virtual void visit(std::shared_ptr<ASTLiteralNode<flx_float>>) = 0;
		virtual void visit(std::shared_ptr<ASTLiteralNode<flx_char>>) = 0;
		virtual void visit(std::shared_ptr<ASTLiteralNode<flx_string>>) = 0;
		virtual void visit(std::shared_ptr<ASTLambdaFunctionNode>) = 0;
		virtual void visit(std::shared_ptr<ASTArrayConstructorNode>) = 0;
		virtual void visit(std::shared_ptr<ASTStructConstructorNode>) = 0;
		virtual void visit(std::shared_ptr<ASTBinaryExprNode>) = 0;
		virtual void visit(std::shared_ptr<ASTUnaryExprNode>) = 0;
		virtual void visit(std::shared_ptr<ASTIdentifierNode>) = 0;
		virtual void visit(std::shared_ptr<ASTTernaryNode>) = 0;
		virtual void visit(std::shared_ptr<ASTFunctionCallNode>) = 0;
		virtual void visit(std::shared_ptr<ASTTypeCastNode>) = 0;
		virtual void visit(std::shared_ptr<ASTTypeNode>) = 0;
		virtual void visit(std::shared_ptr<ASTNullNode>) = 0;
		virtual void visit(std::shared_ptr<ASTThisNode>) = 0;
		virtual void visit(std::shared_ptr<ASTTypeOfNode>) = 0;
		virtual void visit(std::shared_ptr<ASTTypeIdNode>) = 0;
		virtual void visit(std::shared_ptr<ASTRefIdNode>) = 0;
		virtual void visit(std::shared_ptr<ASTIsStructNode>) = 0;
		virtual void visit(std::shared_ptr<ASTIsArrayNode>) = 0;
		virtual void visit(std::shared_ptr<ASTIsAnyNode>) = 0;
		virtual void visit(std::shared_ptr<ASTInstructionNode>) = 0;
		virtual void visit(std::shared_ptr<ASTValueNode>) = 0;
		virtual void visit(std::shared_ptr<ASTClassDefinitionNode>) = 0;

	};

}

#endif // !VISITOR_HPP
