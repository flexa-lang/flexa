#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <array>
#include <vector>
#include <map>
#include <memory>

#include "types.hpp"
#include "operand.hpp"
#include "visitor.hpp"
#include "vm_constants.hpp"

namespace core {

	class CodePosition {
	public:
		size_t row;
		size_t col;

		CodePosition(size_t row = 0, size_t col = 0) : row(row), col(col) {};
	};

	class Identifier {
	public:
		std::string identifier;
		std::vector<std::shared_ptr<ASTExprNode>> access_vector;

		Identifier(const std::string& identifier, const std::vector<std::shared_ptr<ASTExprNode>>& access_vector);

		Identifier(const std::string& identifier);

		Identifier();
	};

	class ASTNode : public std::enable_shared_from_this<ASTNode>, public CodePosition {
	public:
		ASTNode(size_t row, size_t col)
			: CodePosition(row, col) {
		}

		virtual void accept(Visitor*) = 0;
	};

	class ASTStatementNode : public ASTNode {
	public:
		ASTStatementNode(size_t row, size_t col)
			: ASTNode(row, col) {
		}

		void accept(Visitor*) override = 0;
	};

	class ASTExprNode : public ASTNode {
	public:
		ASTExprNode(size_t row, size_t col)
			: ASTNode(row, col) {
		}

		void accept(Visitor*) override = 0;
	};

	class ASTModuleNode : public ASTNode {
	public:
		std::string name;
		std::string name_space;
		std::vector<std::shared_ptr<ASTNode>> statements;
		std::vector<std::shared_ptr<ASTModuleNode>> libs;

		explicit ASTModuleNode(
			const std::string& name,
			const std::string& name_space,
			const std::vector<std::shared_ptr<ASTNode>>& statements
		);

		void accept(Visitor*) override;
	};

	class ASTUsingNode : public ASTStatementNode {
	public:
		std::vector<std::string> library;

		ASTUsingNode(const std::vector<std::string>& library,
			size_t row, size_t col);

		void accept(Visitor*) override;
	};

	class ASTNamespaceManagerNode : public ASTStatementNode {
	public:
		std::string name_space;

		ASTNamespaceManagerNode(const std::string& name_space, size_t col, size_t row);

		void accept(Visitor*) override = 0;
	};

	class ASTIncludeNamespaceNode : public ASTNamespaceManagerNode {
	public:
		ASTIncludeNamespaceNode(const std::string& name_space, size_t col, size_t row);

		void accept(Visitor*) override;
	};

	class ASTExcludeNamespaceNode : public ASTNamespaceManagerNode {
	public:
		ASTExcludeNamespaceNode(const std::string& name_space, size_t col, size_t row);

		void accept(Visitor*) override;
	};

	class ASTDeclarationNode : public ASTStatementNode, public TypeDefinition {
	public:
		std::string identifier;
		std::shared_ptr<ASTExprNode> expr;
		bool is_const;
		bool is_constexpr;
		bool is_static_dim = false;

		ASTDeclarationNode(
			const std::string& identifier,
			Type type,
			const std::vector<std::shared_ptr<ASTExprNode>>& dim,
			const std::string& type_name_space,
			const std::string& type_name,
			std::shared_ptr<ASTExprNode> expr,
			bool is_const,
			bool is_constexpr,
			size_t row, size_t col
		);

		void accept(Visitor*) override;
	};

	class ASTUnpackedDeclarationNode : public ASTStatementNode, public TypeDefinition {
	public:
		std::vector<std::shared_ptr<ASTDeclarationNode>> declarations;
		std::shared_ptr<ASTExprNode> expr;

		ASTUnpackedDeclarationNode(
			Type type,
			const std::vector<std::shared_ptr<ASTExprNode>>& dim,
			const std::string& type_name_space,
			const std::string& type_name,
			const std::vector<std::shared_ptr<ASTDeclarationNode>>& declarations,
			std::shared_ptr<ASTExprNode> expr,
			size_t row, size_t col
		);

		void accept(Visitor*) override;
	};

	class ASTReturnNode : public ASTStatementNode {
	public:
		std::shared_ptr<ASTExprNode> expr;

		ASTReturnNode(std::shared_ptr<ASTExprNode> expr, size_t row, size_t col);

		void accept(Visitor*) override;
	};

	class ASTBlockNode : public ASTStatementNode {
	public:
		std::vector<std::shared_ptr<ASTNode>> statements;

		ASTBlockNode(const std::vector<std::shared_ptr<ASTNode>>& statements, size_t row, size_t col);

		void accept(Visitor*) override;
	};

	class ASTContinueNode : public ASTStatementNode {
	public:
		ASTContinueNode(size_t row, size_t col);

		void accept(Visitor*) override;
	};

	class ASTBreakNode : public ASTStatementNode {
	public:
		ASTBreakNode(size_t row, size_t col);

		void accept(Visitor*) override;
	};

	class ASTExitNode : public ASTStatementNode {
	public:
		std::shared_ptr<ASTExprNode> exit_code;

		ASTExitNode(std::shared_ptr<ASTExprNode> exit_code, size_t row, size_t col);

		void accept(Visitor*) override;
	};

	class ASTSwitchNode : public ASTStatementNode {
	public:
		std::shared_ptr<ASTExprNode> condition;
		std::vector<std::shared_ptr<ASTNode>> statements;
		std::map<std::shared_ptr<ASTExprNode>, size_t> case_blocks;
		size_t default_block;
		std::map<size_t, size_t> parsed_case_blocks;

		ASTSwitchNode(
			std::shared_ptr<ASTExprNode> condition,
			const std::vector<std::shared_ptr<ASTNode>>& statements,
			const std::map<std::shared_ptr<ASTExprNode>, size_t>& case_blocks,
			size_t default_block,
			size_t row, size_t col
		);

		void accept(Visitor*) override;
	};

	class ASTIfNode : public ASTStatementNode {
	public:
		std::shared_ptr<ASTExprNode> condition;
		std::shared_ptr<ASTBlockNode> if_block;
		std::vector<std::shared_ptr<ASTElseIfNode>> else_ifs;
		std::shared_ptr<ASTBlockNode> else_block;

		ASTIfNode(
			std::shared_ptr<ASTExprNode> condition,
			std::shared_ptr<ASTBlockNode> if_block,
			const std::vector<std::shared_ptr<ASTElseIfNode>>& else_ifs,
			std::shared_ptr<ASTBlockNode> else_block,
			size_t row, size_t col
		);

		void accept(Visitor*) override;
	};

	class ASTElseIfNode : public ASTStatementNode {
	public:
		std::shared_ptr<ASTExprNode> condition;
		std::shared_ptr<ASTBlockNode> block;

		ASTElseIfNode(
			std::shared_ptr<ASTExprNode> condition,
			std::shared_ptr<ASTBlockNode> block,
			size_t row, size_t col
		);

		void accept(Visitor*) override;
	};

	class ASTEnumNode : public ASTStatementNode {
	public:
		std::vector<std::string> identifiers;

		ASTEnumNode(const std::vector<std::string>& identifiers, size_t row, size_t col);

		void accept(Visitor*) override;
	};

	class ASTTryCatchNode : public ASTStatementNode {
	public:
		std::shared_ptr<ASTStatementNode> decl;
		std::shared_ptr<ASTBlockNode> try_block;
		std::shared_ptr<ASTBlockNode> catch_block;

		ASTTryCatchNode(
			std::shared_ptr<ASTStatementNode> decl,
			std::shared_ptr<ASTBlockNode> try_block,
			std::shared_ptr<ASTBlockNode> catch_block,
			size_t row, size_t col
		);

		void accept(Visitor*) override;
	};

	class ASTThrowNode : public ASTStatementNode {
	public:
		std::shared_ptr<ASTExprNode> error;

		ASTThrowNode(std::shared_ptr<ASTExprNode> error, size_t row, size_t col);

		void accept(Visitor*) override;
	};

	class ASTEllipsisNode : public ASTStatementNode {
	public:
		ASTEllipsisNode(size_t row, size_t col);

		void accept(Visitor*) override;
	};

	class ASTForNode : public ASTStatementNode {
	public:
		std::array<std::shared_ptr<ASTNode>, 3> expressions;
		std::shared_ptr<ASTBlockNode> block;

		ASTForNode(
			const std::array<std::shared_ptr<ASTNode>, 3>& expressions,
			std::shared_ptr<ASTBlockNode> block,
			size_t row, size_t col
		);

		void accept(Visitor*) override;
	};

	class ASTForEachNode : public ASTStatementNode {
	public:
		std::shared_ptr<ASTNode> itdecl;
		std::shared_ptr<ASTNode> collection;
		std::shared_ptr<ASTBlockNode> block;

		ASTForEachNode(
			std::shared_ptr<ASTNode> itdecl,
			std::shared_ptr<ASTNode> collection,
			std::shared_ptr<ASTBlockNode> block,
			size_t row, size_t col
		);

		void accept(Visitor*) override;
	};

	class ASTWhileNode : public ASTStatementNode {
	public:
		std::shared_ptr<ASTExprNode> condition;
		std::shared_ptr<ASTBlockNode> block;

		ASTWhileNode(
			std::shared_ptr<ASTExprNode> condition,
			std::shared_ptr<ASTBlockNode> block,
			size_t row, size_t col
		);

		void accept(Visitor*) override;
	};

	class ASTDoWhileNode : public ASTWhileNode {
	public:
		ASTDoWhileNode(
			std::shared_ptr<ASTExprNode> condition,
			std::shared_ptr<ASTBlockNode> block,
			size_t row, size_t col
		);

		void accept(Visitor*) override;
	};

	class ASTStructDefinitionNode : public ASTStatementNode {
	public:
		std::string identifier;
		std::map<std::string, std::shared_ptr<VariableDefinition>> variables;

		ASTStructDefinitionNode(
			const std::string& identifier,
			const std::map<std::string, std::shared_ptr<VariableDefinition>>& variables,
			size_t row, size_t col
		);

		void accept(Visitor*) override;
	};

	class ASTFunctionDefinitionNode : public ASTStatementNode, public TypeDefinition {
	public:
		std::string identifier;
		std::vector<std::shared_ptr<TypeDefinition>> parameters;
		std::shared_ptr<ASTBlockNode> block;
		bool is_class_function = false;

		ASTFunctionDefinitionNode(
			const std::string& identifier,
			const std::vector<std::shared_ptr<TypeDefinition>>& parameters,
			Type type,
			const std::string& type_name_space,
			const std::string& type_name,
			const std::vector<std::shared_ptr<ASTExprNode>>& dim,
			std::shared_ptr<ASTBlockNode> block,
			size_t row, size_t col
		);

		void accept(Visitor*) override;
	};

	class ASTClassDefinitionNode : public ASTStatementNode {
	public:
		std::string identifier;
		std::vector<std::shared_ptr<ASTDeclarationNode>> declarations;
		std::vector<std::shared_ptr<ASTFunctionDefinitionNode>> functions;

		ASTClassDefinitionNode(
			const std::string& identifier,
			const std::vector<std::shared_ptr<ASTDeclarationNode>>& declarations,
			const std::vector<std::shared_ptr<ASTFunctionDefinitionNode>>& functions,
			size_t row, size_t col
		);

		void accept(Visitor*) override;
	};

	template<typename T>
	class ASTLiteralNode : public ASTExprNode {
	public:
		T value;

		ASTLiteralNode(T value, size_t row, size_t col) : ASTExprNode(row, col), value(value) {};

		void accept(Visitor*) override;
	};

	class ASTLambdaFunctionNode : public ASTExprNode {
	public:
		std::shared_ptr<ASTFunctionDefinitionNode> fun;

		ASTLambdaFunctionNode(std::shared_ptr<ASTFunctionDefinitionNode> fun, size_t row, size_t col);

		void accept(Visitor*) override;
	};

	class ASTArrayConstructorNode : public ASTExprNode, public TypeDefinition {
	public:
		std::vector<std::shared_ptr<ASTExprNode>> values;

		ASTArrayConstructorNode(const std::vector<std::shared_ptr<ASTExprNode>>& values, size_t row, size_t col);

		void accept(Visitor*) override;
	};

	class ASTStructConstructorNode : public ASTExprNode {
	public:
		std::string type_name_space;
		std::string type_name;
		std::map<std::string, std::shared_ptr<ASTExprNode>> values;

		ASTStructConstructorNode(
			const std::string& type_name_space,
			const std::string& type_name,
			const std::map<std::string, std::shared_ptr<ASTExprNode>>& values,
			size_t row, size_t col
		);

		void accept(Visitor*) override;
	};

	class ASTNullNode : public ASTExprNode {
	public:
		ASTNullNode(size_t row, size_t col);

		void accept(Visitor*) override;
	};

	class ASTThisNode : public ASTExprNode {
	public:
		std::vector<Identifier> access_vector;

		ASTThisNode(std::vector<Identifier> access_vector, size_t row, size_t col);

		void accept(Visitor*) override;
	};

	class ASTBinaryExprNode : public ASTExprNode {
	public:
		std::string op;
		std::shared_ptr<ASTExprNode> left;
		std::shared_ptr<ASTExprNode> right;

		ASTBinaryExprNode(
			const std::string& op,
			std::shared_ptr<ASTExprNode> left,
			std::shared_ptr<ASTExprNode> right,
			size_t row, size_t col
		);

		void accept(Visitor*) override;
	};

	class ASTUnaryExprNode : public ASTExprNode {
	public:
		std::string unary_op;
		std::shared_ptr<ASTExprNode> expr;

		ASTUnaryExprNode(const std::string& unary_op, std::shared_ptr<ASTExprNode> expr, size_t row, size_t col);

		void accept(Visitor*) override;
	};

	class ASTIdentifierNode : public ASTExprNode {
	public:
		std::vector<Identifier> identifier_vector;
		std::string access_name_space;
		std::string identifier;

		explicit ASTIdentifierNode(
			const std::vector<Identifier>& identifier_vector,
			std::string access_name_space,
			size_t row, size_t col
		);

		void accept(Visitor*) override;
	};

	class ASTTernaryNode : public ASTExprNode {
	public:
		std::shared_ptr<ASTExprNode> condition;
		std::shared_ptr<ASTExprNode> value_if_true;
		std::shared_ptr<ASTExprNode> value_if_false;

		ASTTernaryNode(
			std::shared_ptr<ASTExprNode> condition,
			std::shared_ptr<ASTExprNode> value_if_true,
			std::shared_ptr<ASTExprNode> value_if_false,
			size_t row, size_t col
		);

		void accept(Visitor*) override;
	};

	class ASTFunctionCallNode : public ASTExprNode {
	public:
		std::string access_name_space;
		std::vector<Identifier> identifier_vector;
		std::vector<std::shared_ptr<ASTExprNode>> parameters;
		std::vector<Identifier> expression_identifier_vector;
		std::shared_ptr<ASTFunctionCallNode> expression_call;
		std::string identifier;

		ASTFunctionCallNode(
			const std::string& access_name_space,
			const std::vector<Identifier>& identifier_vector,
			const std::vector<std::shared_ptr<ASTExprNode>>& parameters,
			std::vector<Identifier> expression_identifier_vector,
			std::shared_ptr<ASTFunctionCallNode> expression_call,
			size_t row, size_t col
		);

		void accept(Visitor*) override;
	};

	class ASTTypeCastNode : public ASTExprNode {
	public:
		Type type;
		std::shared_ptr<ASTExprNode> expr;

		ASTTypeCastNode(Type type, std::shared_ptr<ASTExprNode> expr, size_t row, size_t col);

		void accept(Visitor*) override;
	};

	class ASTTypeNode : public ASTExprNode {
	public:
		TypeDefinition type;

		ASTTypeNode(TypeDefinition type, size_t row, size_t col);

		void accept(Visitor*) override;
	};

	class ASTCallOperatorNode : public ASTExprNode {
	public:
		std::shared_ptr<ASTExprNode> expr;

		ASTCallOperatorNode(std::shared_ptr<ASTExprNode> expr, size_t row, size_t col);

		void accept(Visitor*) override = 0;
	};

	class ASTTypeOfNode : public ASTCallOperatorNode {
	public:
		ASTTypeOfNode(std::shared_ptr<ASTExprNode> expr, size_t row, size_t col);

		void accept(Visitor*) override;
	};

	class ASTTypeIdNode : public ASTCallOperatorNode {
	public:
		ASTTypeIdNode(std::shared_ptr<ASTExprNode> expr, size_t row, size_t col);

		void accept(Visitor*) override;
	};

	class ASTRefIdNode : public ASTCallOperatorNode {
	public:
		ASTRefIdNode(std::shared_ptr<ASTExprNode> expr, size_t row, size_t col);

		void accept(Visitor*) override;
	};

	class ASTIsStructNode : public ASTCallOperatorNode {
	public:
		ASTIsStructNode(std::shared_ptr<ASTExprNode> expr, size_t row, size_t col);

		void accept(Visitor*) override;
	};

	class ASTIsArrayNode : public ASTCallOperatorNode {
	public:
		ASTIsArrayNode(std::shared_ptr<ASTExprNode> expr, size_t row, size_t col);

		void accept(Visitor*) override;
	};

	class ASTIsAnyNode : public ASTCallOperatorNode {
	public:
		ASTIsAnyNode(std::shared_ptr<ASTExprNode> expr, size_t row, size_t col);

		void accept(Visitor*) override;
	};

	// ASTInstructionNode
	// Auxiliary node used only during compilation.
	// Represents a low-level instruction or placeholder
	// to be directly emitted as bytecode during code generation.
	class ASTInstructionNode : public ASTExprNode {
	public:
		OpCode opcode;
		Operand operand;

		ASTInstructionNode(OpCode opcode, Operand operand, size_t row, size_t col);

		void accept(Visitor*) override;
	};

	// ASTValueNode
	// Auxiliary node used only during semantic analysis.
	// Represents a temporary SemanticValue to ensure type correctness
	// in contexts where an expression must exist syntactically.
	class ASTValueNode : public ASTExprNode {
	public:
		Value* value;

		ASTValueNode(Value* value, size_t row, size_t col);

		void accept(Visitor*) override;
	};

}

#endif // !AST_HPP
