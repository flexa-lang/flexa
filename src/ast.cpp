#include "ast.hpp"

#include <utility>
#include <numeric>

#include "constants.hpp"
#include "utils.hpp"

using namespace core;

Identifier::Identifier(const std::string& identifier, const std::vector<std::shared_ptr<ASTExprNode>>& access_vector)
	: identifier(identifier), access_vector(access_vector) {
}

Identifier::Identifier(const std::string& identifier)
	: identifier(identifier) {
}

Identifier::Identifier()
	: identifier(""), access_vector(std::vector<std::shared_ptr<ASTExprNode>>()) {
}

ASTModuleNode::ASTModuleNode(
	const std::string& name,
	const std::string& name_space,
	const std::vector<std::shared_ptr<ASTNode>>& statements
)
	: ASTNode(0, 0),
	name(name),
	name_space(name_space),
	statements(statements),
	libs(std::vector<std::shared_ptr<ASTModuleNode>>()) {
}

ASTUsingNode::ASTUsingNode(const std::vector<std::string>& library, size_t row, size_t col)
	: ASTStatementNode(row, col), library(library) {
}

ASTNamespaceManagerNode::ASTNamespaceManagerNode(const std::string& name_space, size_t row, size_t col)
	: ASTStatementNode(row, col), name_space(name_space) {
}

ASTIncludeNamespaceNode::ASTIncludeNamespaceNode(const std::string& name_space, size_t row, size_t col)
	: ASTNamespaceManagerNode(name_space, row, col) {
}

ASTExcludeNamespaceNode::ASTExcludeNamespaceNode(const std::string& name_space, size_t row, size_t col)
	: ASTNamespaceManagerNode(name_space, row, col) {
}

ASTDeclarationNode::ASTDeclarationNode(
	const std::string& identifier,
	Type type,
	const std::vector<std::shared_ptr<ASTExprNode>>& dim,
	const std::string& type_name_space,
	const std::string& type_name,
	std::shared_ptr<ASTExprNode> expr,
	bool is_const,
	bool is_constexpr,
	size_t row, size_t col
)
	: ASTStatementNode(row, col),
	TypeDefinition(type, dim, type_name_space, type_name),
	identifier(identifier),
	expr(expr),
	is_const(is_const),
	is_constexpr(is_constexpr) {
}

ASTUnpackedDeclarationNode::ASTUnpackedDeclarationNode(
	Type type,
	const std::vector<std::shared_ptr<ASTExprNode>>& dim,
	const std::string& type_name_space,
	const std::string& type_name,
	const std::vector<std::shared_ptr<ASTDeclarationNode>>& declarations,
	std::shared_ptr<ASTExprNode> expr,
	size_t row, size_t col
)
	: ASTStatementNode(row, col),
	TypeDefinition(type, dim, type_name_space, type_name),
	declarations(declarations),
	expr(expr) {
}

ASTReturnNode::ASTReturnNode(std::shared_ptr<ASTExprNode> expr, size_t row, size_t col)
	: ASTStatementNode(row, col), expr(expr) {
}

ASTBlockNode::ASTBlockNode(const std::vector<std::shared_ptr<ASTNode>>& statements, size_t row, size_t col)
	: ASTStatementNode(row, col), statements(statements) {
}

ASTContinueNode::ASTContinueNode(size_t row, size_t col)
	: ASTStatementNode(row, col) {
}

ASTBreakNode::ASTBreakNode(size_t row, size_t col)
	: ASTStatementNode(row, col) {
}

ASTExitNode::ASTExitNode(std::shared_ptr<ASTExprNode> exit_code, size_t row, size_t col)
	: ASTStatementNode(row, col), exit_code(exit_code) {
}

ASTSwitchNode::ASTSwitchNode(
	std::shared_ptr<ASTExprNode> condition,
	const std::vector<std::shared_ptr<ASTNode>>& statements,
	const std::map<std::shared_ptr<ASTExprNode>, size_t>& case_blocks,
	size_t default_block,
	size_t row, size_t col
)
	: ASTStatementNode(row, col),
	condition(condition),
	statements(statements),
	case_blocks(case_blocks),
	default_block(default_block) {
}

ASTIfNode::ASTIfNode(
	std::shared_ptr<ASTExprNode> condition,
	std::shared_ptr<ASTBlockNode> if_block,
	const std::vector<std::shared_ptr<ASTElseIfNode>>& else_ifs,
	std::shared_ptr<ASTBlockNode> else_block,
	size_t row, size_t col
)
	: ASTStatementNode(row, col),
	condition(condition),
	if_block(if_block),
	else_ifs(else_ifs),
	else_block(else_block) {
}

ASTElseIfNode::ASTElseIfNode(
	std::shared_ptr<ASTExprNode> condition,
	std::shared_ptr<ASTBlockNode> block,
	size_t row, size_t col
)
	: ASTStatementNode(row, col),
	condition(condition),
	block(block) {
}

ASTEnumNode::ASTEnumNode(const std::vector<std::string>& identifiers, size_t row, size_t col)
	: ASTStatementNode(row, col), identifiers(identifiers) {
}

ASTTryCatchNode::ASTTryCatchNode(
	std::shared_ptr<ASTStatementNode> decl,
	std::shared_ptr<ASTBlockNode> try_block,
	std::shared_ptr<ASTBlockNode> catch_block,
	size_t row, size_t col
)
	: ASTStatementNode(row, col),
	decl(decl),
	try_block(try_block),
	catch_block(catch_block) {
}

ASTThrowNode::ASTThrowNode(std::shared_ptr<ASTExprNode> error, size_t row, size_t col)
	: ASTStatementNode(row, col), error(error) {
}

ASTEllipsisNode::ASTEllipsisNode(size_t row, size_t col)
	: ASTStatementNode(row, col) {
}

ASTForNode::ASTForNode(
	const std::array<std::shared_ptr<ASTNode>, 3>& expressions,
	std::shared_ptr<ASTBlockNode> block,
	size_t row, size_t col
)
	: ASTStatementNode(row, col),
	expressions(expressions),
	block(block) {
}

ASTForEachNode::ASTForEachNode(
	std::shared_ptr<ASTNode> itdecl,
	std::shared_ptr<ASTNode> collection,
	std::shared_ptr<ASTBlockNode> block,
	size_t row, size_t col
)
	: ASTStatementNode(row, col),
	itdecl(itdecl),
	collection(collection),
	block(block) {
}

ASTWhileNode::ASTWhileNode(
	std::shared_ptr<ASTExprNode> condition,
	std::shared_ptr<ASTBlockNode> block,
	size_t row, size_t col
)
	: ASTStatementNode(row, col),
	condition(condition),
	block(block) {
}

ASTDoWhileNode::ASTDoWhileNode(
	std::shared_ptr<ASTExprNode> condition,
	std::shared_ptr<ASTBlockNode> block,
	size_t row, size_t col
)
	: ASTWhileNode(condition, block, row, col) {
}

ASTStructDefinitionNode::ASTStructDefinitionNode(
	const std::string& identifier,
	const std::map<std::string, std::shared_ptr<VariableDefinition>>& variables,
	size_t row, size_t col
)
	: ASTStatementNode(row, col),
	identifier(identifier),
	variables(variables) {
}

ASTFunctionDefinitionNode::ASTFunctionDefinitionNode(
	const std::string& identifier,
	const std::vector<std::shared_ptr<TypeDefinition>>& parameters,
	Type type,
	const std::string& type_name_space,
	const std::string& type_name,
	const std::vector<std::shared_ptr<ASTExprNode>>& dim,
	std::shared_ptr<ASTBlockNode> block,
	size_t row, size_t col
)
	: ASTStatementNode(row, col),
	TypeDefinition(type, dim, type_name_space, type_name),
	identifier(identifier),
	parameters(parameters),
	block(block) {
}

ASTClassDefinitionNode::ASTClassDefinitionNode(
	const std::string& identifier,
	const std::vector<std::shared_ptr<ASTDeclarationNode>>& declarations,
	const std::vector<std::shared_ptr<ASTFunctionDefinitionNode>>& functions,
	size_t row, size_t col
)
	: ASTStatementNode(row, col),
	identifier(identifier),
	declarations(declarations),
	functions(functions) {
}

ASTArrayConstructorNode::ASTArrayConstructorNode(
	const std::vector<std::shared_ptr<ASTExprNode>>& values,
	size_t row, size_t col
)
	: ASTExprNode(row, col),
	values(values) {
}

ASTStructConstructorNode::ASTStructConstructorNode(
	const std::string& type_name_space,
	const std::string& type_name,
	const std::map<std::string, std::shared_ptr<ASTExprNode>>& values,
	size_t row, size_t col
)
	: ASTExprNode(row, col),
	type_name_space(type_name_space),
	type_name(type_name),
	values(values) {
}

ASTNullNode::ASTNullNode(size_t row, size_t col)
	: ASTExprNode(row, col) {
}

ASTThisNode::ASTThisNode(std::vector<Identifier> access_vector, size_t row, size_t col)
	: ASTExprNode(row, col), access_vector(access_vector) {
}

ASTBinaryExprNode::ASTBinaryExprNode(
	const std::string& op,
	std::shared_ptr<ASTExprNode> left,
	std::shared_ptr<ASTExprNode> right,
	size_t row, size_t col
)
	: ASTExprNode(row, col),
	op(op),
	left(left),
	right(right) {
}

ASTUnaryExprNode::ASTUnaryExprNode(
	const std::string& unary_op,
	std::shared_ptr<ASTExprNode> expr,
	size_t row, size_t col
)
	: ASTExprNode(row, col),
	unary_op(unary_op),
	expr(expr) {
}

ASTIdentifierNode::ASTIdentifierNode(
	const std::vector<Identifier>& identifier_vector,
	std::string access_name_space,
	size_t row, size_t col
)
	: ASTExprNode(row, col),
	identifier_vector(identifier_vector),
	access_name_space(access_name_space),
	identifier(identifier_vector[0].identifier) {
}

ASTTernaryNode::ASTTernaryNode(
	std::shared_ptr<ASTExprNode> condition,
	std::shared_ptr<ASTExprNode> value_if_true,
	std::shared_ptr<ASTExprNode> value_if_false,
	size_t row, size_t col
)
	: ASTExprNode(row, col),
	condition(condition),
	value_if_true(value_if_true),
	value_if_false(value_if_false) {
}

ASTFunctionCallNode::ASTFunctionCallNode(
	const std::string& access_name_space,
	const std::vector<Identifier>& identifier_vector,
	const std::vector<std::shared_ptr<ASTExprNode>>& parameters,
	std::vector<Identifier> expression_identifier_vector,
	std::shared_ptr<ASTFunctionCallNode> expression_call,
	size_t row, size_t col
)
	: ASTExprNode(row, col),
	access_name_space(access_name_space),
	identifier_vector(identifier_vector),
	parameters(parameters),
	expression_identifier_vector(expression_identifier_vector),
	expression_call(expression_call),
	identifier(identifier_vector[0].identifier) {
}

ASTTypeCastNode::ASTTypeCastNode(Type type, std::shared_ptr<ASTExprNode> expr, size_t row, size_t col)
	: ASTExprNode(row, col), type(type), expr(expr) {
}

ASTTypeNode::ASTTypeNode(TypeDefinition type, size_t row, size_t col)
	: ASTExprNode(row, col), type(type) {
}

ASTCallOperatorNode::ASTCallOperatorNode(std::shared_ptr<ASTExprNode> expr, size_t row, size_t col)
	: ASTExprNode(row, col), expr(expr) {
}

ASTTypeOfNode::ASTTypeOfNode(std::shared_ptr<ASTExprNode> expr, size_t row, size_t col)
	: ASTCallOperatorNode(expr, row, col) {
}

ASTTypeIdNode::ASTTypeIdNode(std::shared_ptr<ASTExprNode> expr, size_t row, size_t col)
	: ASTCallOperatorNode(expr, row, col) {
}

ASTRefIdNode::ASTRefIdNode(std::shared_ptr<ASTExprNode> expr, size_t row, size_t col)
	: ASTCallOperatorNode(expr, row, col) {
}

ASTIsStructNode::ASTIsStructNode(std::shared_ptr<ASTExprNode> expr, size_t row, size_t col)
	: ASTCallOperatorNode(expr, row, col) {
}

ASTIsArrayNode::ASTIsArrayNode(std::shared_ptr<ASTExprNode> expr, size_t row, size_t col)
	: ASTCallOperatorNode(expr, row, col) {
}

ASTIsAnyNode::ASTIsAnyNode(std::shared_ptr<ASTExprNode> expr, size_t row, size_t col)
	: ASTCallOperatorNode(expr, row, col) {
}

ASTLambdaFunctionNode::ASTLambdaFunctionNode(std::shared_ptr<ASTFunctionDefinitionNode> fun, size_t row, size_t col)
	: ASTExprNode(row, col), fun(fun) {
}

ASTInstructionNode::ASTInstructionNode(OpCode opcode, Operand operand, size_t row, size_t col)
	: ASTExprNode(row, col), opcode(opcode), operand(operand) {
}

ASTValueNode::ASTValueNode(Value* value, size_t row, size_t col)
	: ASTExprNode(row, col), value(value) {
}

#ifdef linux
template<>
#endif // linux
void ASTLiteralNode<flx_bool>::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTLiteralNode<flx_bool>>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

#ifdef linux
template<>
#endif // linux
void ASTLiteralNode<flx_int>::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTLiteralNode<flx_int>>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

#ifdef linux
template<>
#endif // linux
void ASTLiteralNode<flx_float>::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTLiteralNode<flx_float>>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

#ifdef linux
template<>
#endif // linux
void ASTLiteralNode<flx_char>::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTLiteralNode<flx_char>>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

#ifdef linux
template<>
#endif // linux
void ASTLiteralNode<flx_string>::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTLiteralNode<flx_string>>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTArrayConstructorNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		this->type_name_space,
		this->type_name,
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTArrayConstructorNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTStructConstructorNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		this->type_name_space,
		this->type_name,
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTStructConstructorNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTBinaryExprNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTBinaryExprNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTFunctionCallNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		this->access_name_space,
		this->identifier,
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTFunctionCallNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTIdentifierNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTIdentifierNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTTypeOfNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTTypeOfNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTTypeIdNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTTypeIdNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTRefIdNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTRefIdNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTIsStructNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTIsStructNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTIsArrayNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTIsArrayNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTIsAnyNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTIsAnyNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTUnaryExprNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTUnaryExprNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTTernaryNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTTernaryNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTTypeCastNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTTypeCastNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTTypeNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTTypeNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTNullNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTNullNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTThisNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTThisNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTLambdaFunctionNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTLambdaFunctionNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTInstructionNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTInstructionNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTValueNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<expression>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTValueNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTDeclarationNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTDeclarationNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTUnpackedDeclarationNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTUnpackedDeclarationNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTIncludeNamespaceNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTIncludeNamespaceNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTExcludeNamespaceNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTExcludeNamespaceNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTReturnNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTReturnNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTExitNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTExitNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTBlockNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTBlockNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTContinueNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTContinueNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTBreakNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTBreakNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTSwitchNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTSwitchNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTEnumNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTEnumNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTTryCatchNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTTryCatchNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTThrowNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTThrowNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTEllipsisNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTEllipsisNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTElseIfNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTElseIfNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTIfNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTIfNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTForNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTForNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTForEachNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTForEachNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTWhileNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTWhileNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTDoWhileNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTDoWhileNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTFunctionDefinitionNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTFunctionDefinitionNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTClassDefinitionNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		this->identifier,
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTClassDefinitionNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTStructDefinitionNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		this->identifier,
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTStructDefinitionNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTUsingNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<statement>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTUsingNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}

void ASTModuleNode::accept(Visitor* v) {
	v->current_debug_info_stack.push(DebugInfo(
		v->current_module_stack.top()->name_space,
		v->current_module_stack.top()->name,
		"<program>",
		"",
		"",
		this->row,
		this->col
	));
	v->visit(std::dynamic_pointer_cast<ASTModuleNode>(shared_from_this()));
	v->current_debug_info_stack.pop();
}
