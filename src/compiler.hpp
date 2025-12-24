#ifndef COMPILER_HPP
#define COMPILER_HPP

#include <string>
#include <vector>
#include <stack>
#include <map>
#include <memory>

#include "vm_debug.hpp"
#include "bytecode.hpp"
#include "ast.hpp"

namespace core {

	namespace analysis {

		class Compiler : public Visitor {
		public:
			VmDebug vm_debug;
			std::vector<BytecodeInstruction> bytecode_program;
			std::map<std::string, std::shared_ptr<ASTExprNode>> builtin_functions;

		private:
			size_t pointer = 0;
			std::stack<std::vector<size_t>> end_pointers;
			std::stack<std::vector<size_t>> start_pointers;
			std::stack<std::vector<size_t>> if_end_pointers;
			std::vector<std::string> parsed_libs;
			std::stack<std::pair<std::string, std::string>> current_this_name;
			std::stack<size_t> scope_unwind_stack;

		private:
			template <typename T>
			size_t add_instruction(OpCode opcode, T operand);
			size_t add_instruction(OpCode opcode);
			template <typename T>
			void replace_operand(size_t pos, T operand);

			void open_start_pointers();
			void close_start_pointers(size_t sp);
			void open_end_pointers();
			void close_end_pointers();
			void open_if_end_pointers();
			void close_if_end_pointers();

			void push_scope();
			void pop_scope();

			OpCode get_opcode_operation(const std::string& operation);

			void type_definition_operations(TypeDefinition type);
			void access_sub_value_operations(std::vector<Identifier> identifier_vector);

			void declare_variable_definition(VariableDefinition var);

			bool has_sub_value(std::vector<Identifier> identifier_vector);

			void set_debug_info();

			void remove_unused_constant(std::shared_ptr<ASTNode> astnode);

		public:
			Compiler(
				std::shared_ptr<ASTModuleNode> main_module,
				std::map<std::string, std::shared_ptr<ASTModuleNode>> modules
			);
			~Compiler() = default;

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
			void visit(std::shared_ptr<ASTInstructionNode>) override;
			void visit(std::shared_ptr<ASTValueNode>) override;
			void visit(std::shared_ptr<ASTIsStructNode>) override;
			void visit(std::shared_ptr<ASTIsAnyNode>) override;
			void visit(std::shared_ptr<ASTIsArrayNode>) override;
			void visit(std::shared_ptr<ASTClassDefinitionNode>) override;

		};

	}

}

#endif // !COMPILER_HPP
