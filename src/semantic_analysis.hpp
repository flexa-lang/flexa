#ifndef SEMANTIC_ANALYSIS_HPP
#define SEMANTIC_ANALYSIS_HPP

#include <string>
#include <vector>
#include <stack>
#include <map>
#include <functional>
#include <memory>
#include <tuple>

#include "ast.hpp"
#include "scope.hpp"
#include "scope_manager.hpp"

namespace core {

	namespace analysis {

		class SemanticAnalyser : public Visitor, public ScopeManager {
		public:
			std::map<std::string, std::shared_ptr<ASTExprNode>> builtin_functions;
			std::vector<std::string> args;

		private:
			std::stack<std::shared_ptr<Scope>> class_stack;
			std::vector<std::string> all_name_spaces;
			std::vector<std::string> parsed_libs;
			SemanticValue current_expression;
			std::stack<std::shared_ptr<FunctionDefinition>> current_function;
			std::vector<std::tuple<std::shared_ptr<FunctionDefinition>, size_t, size_t>> declared_functions;
			bool is_assignment = false;
			bool exception = false;
			bool is_switch = false;
			bool is_loop = false;
			size_t module_level = 0;

			std::vector<size_t> current_expression_array_dim;
			int current_expression_array_dim_max = 0;
			TypeDefinition current_expression_array_type;
			bool is_max = false;

		private:
			bool is_return_node(std::shared_ptr<ASTNode> astnode);
			bool returns(std::shared_ptr<ASTNode> astnode);

			std::shared_ptr<ASTExprNode> check_build_array(
				const std::vector<size_t>& dim,
				std::shared_ptr<ASTExprNode> init_expr
			);
			std::shared_ptr<ASTExprNode> build_array(
				const std::vector<size_t>& dim,
				std::shared_ptr<ASTExprNode> init_value,
				size_t level
			);

			void declare_function_parameter(std::shared_ptr<Scope> scope, const VariableDefinition& param);

			std::vector<size_t> evaluate_access_vector(
				const std::vector<std::shared_ptr<ASTExprNode>>& expr_access_vector
			);
			std::vector<size_t> evaluate_dimension_vector(
				const std::vector<std::shared_ptr<ASTExprNode>>& expr_dimension_vector
			);

			std::shared_ptr<SemanticValue> access_value(
				std::shared_ptr<SemanticValue> value,
				const std::vector<Identifier>& identifier_vector,
				size_t i = 0
			);
			bool has_sub_value(std::vector<Identifier> identifier_vector);

			void check_is_struct_exists(Type type, const std::string& name_space, const std::string& identifier);
			void determine_object_type(std::shared_ptr<TypeDefinition> typedefinition);
			const std::string& normalize_name_space(
				std::string& astnode_name_space,
				const std::string& name_space
			) const;
			bool namespace_exists(const std::string& name_space);
			void validate_namespace(const std::string& name_space) const;
			void setup_global_namespace(std::shared_ptr<Scope> scope);

		public:
			SemanticAnalyser(
				std::shared_ptr<Scope> global_scope,
				std::shared_ptr<ASTModuleNode> main_module,
				std::map<std::string, std::shared_ptr<ASTModuleNode>> modules,
				const std::vector<std::string>& args
			);
			~SemanticAnalyser() = default;

			void start();

			void visit(std::shared_ptr<ASTModuleNode>) override;
			void visit(std::shared_ptr<ASTUsingNode>) override;
			void visit(std::shared_ptr<ASTIncludeNamespaceNode>) override;
			void visit(std::shared_ptr<ASTExcludeNamespaceNode>) override;
			void visit(std::shared_ptr<ASTDeclarationNode>) override;
			void visit(std::shared_ptr<ASTUnpackedDeclarationNode>) override;
			void visit(std::shared_ptr<ASTReturnNode>) override;
			void visit(std::shared_ptr<ASTExitNode>) override;
			void visit(std::shared_ptr<ASTBlockNode>) override;
			void visit(std::shared_ptr<ASTContinueNode>) override;
			void visit(std::shared_ptr<ASTBreakNode>) override;
			void visit(std::shared_ptr<ASTSwitchNode>) override;
			void visit(std::shared_ptr<ASTEnumNode>) override;
			void visit(std::shared_ptr<ASTTryCatchNode>) override;
			void visit(std::shared_ptr<ASTThrowNode>) override;
			void visit(std::shared_ptr<ASTEllipsisNode>) override;
			void visit(std::shared_ptr<ASTElseIfNode>) override;
			void visit(std::shared_ptr<ASTIfNode>) override;
			void visit(std::shared_ptr<ASTForNode>) override;
			void visit(std::shared_ptr<ASTForEachNode>) override;
			void visit(std::shared_ptr<ASTWhileNode>) override;
			void visit(std::shared_ptr<ASTDoWhileNode>) override;
			void visit(std::shared_ptr<ASTFunctionDefinitionNode>) override;
			void visit(std::shared_ptr<ASTStructDefinitionNode>) override;
			void visit(std::shared_ptr<ASTLiteralNode<flx_bool>>) override;
			void visit(std::shared_ptr<ASTLiteralNode<flx_int>>) override;
			void visit(std::shared_ptr<ASTLiteralNode<flx_float>>) override;
			void visit(std::shared_ptr<ASTLiteralNode<flx_char>>) override;
			void visit(std::shared_ptr<ASTLiteralNode<flx_string>>) override;
			void visit(std::shared_ptr<ASTLambdaFunctionNode>) override;
			void visit(std::shared_ptr<ASTArrayConstructorNode>) override;
			void visit(std::shared_ptr<ASTStructConstructorNode>) override;
			void visit(std::shared_ptr<ASTBinaryExprNode>) override;
			void visit(std::shared_ptr<ASTUnaryExprNode>) override;
			void visit(std::shared_ptr<ASTIdentifierNode>) override;
			void visit(std::shared_ptr<ASTTernaryNode>) override;
			void visit(std::shared_ptr<ASTFunctionCallNode>) override;
			void visit(std::shared_ptr<ASTTypeCastNode>) override;
			void visit(std::shared_ptr<ASTTypeNode>) override;
			void visit(std::shared_ptr<ASTNullNode>) override;
			void visit(std::shared_ptr<ASTThisNode>) override;
			void visit(std::shared_ptr<ASTTypeOfNode>) override;
			void visit(std::shared_ptr<ASTTypeIdNode>) override;
			void visit(std::shared_ptr<ASTRefIdNode>) override;
			void visit(std::shared_ptr<ASTIsStructNode>) override;
			void visit(std::shared_ptr<ASTIsArrayNode>) override;
			void visit(std::shared_ptr<ASTIsAnyNode>) override;
			void visit(std::shared_ptr<ASTInstructionNode>) override;
			void visit(std::shared_ptr<ASTValueNode>) override;
			void visit(std::shared_ptr<ASTClassDefinitionNode>) override;

		};

	}

}

#endif // !SEMANTIC_ANALYSIS_HPP
