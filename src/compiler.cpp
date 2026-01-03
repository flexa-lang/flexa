#include <utility>

#include "compiler.hpp"

#include "operand.hpp"
#include "token.hpp"
#include "md_builtin.hpp"
#include "constants.hpp"
#include "utils.hpp"

using namespace core;
using namespace core::modules;
using namespace core::analysis;

Compiler::Compiler(
	std::shared_ptr<ASTModuleNode> main_module,
	std::map<std::string, std::shared_ptr<ASTModuleNode>> modules
) : Visitor(modules, main_module) {
	vm_debug.add_namespace(Constants::DEFAULT_NAMESPACE);
};

void Compiler::start() {
	current_module_stack.top()->accept(this);

	if (!single_expression_state) {
		add_instruction(OpCode::OP_PUSH_INT, flx_int(0));
	}

	add_instruction(OpCode::OP_HALT);
}

void Compiler::visit(std::shared_ptr<ASTModuleNode> astnode) {
	current_this_name.push(std::make_pair("module", astnode->name));
	vm_debug.add_module(astnode->name);

	if (astnode->statements.size() == 1 && std::dynamic_pointer_cast<ASTExprNode>(astnode->statements[0])) {
		single_expression_state = true;
		astnode->statements[0]->accept(this);
	}
	else {
		for (const auto& statement : astnode->statements) {
			statement->accept(this);
			remove_unused_constant(statement);
		}
	}

	current_this_name.pop();

}

void Compiler::visit(std::shared_ptr<ASTUsingNode> astnode) {
	std::string libname = utils::StringUtils::join(astnode->library, ".");
	auto& module = modules[libname];
	vm_debug.add_module(module->name);
	vm_debug.add_namespace(module->name_space);

	if (Constants::CORE_LIBS.find(libname) != Constants::CORE_LIBS.end()) {
		add_instruction(OpCode::OP_BUILTIN_LIB, flx_string(libname));
	}

	// if can't parsed yet
	if (!utils::CollectionUtils::contains(parsed_libs, libname)) {
		parsed_libs.push_back(libname);

		current_module_stack.push(module);

		// default module scope
		add_instruction(OpCode::OP_PUSH_SCOPE, std::vector<Operand> {
			Operand(module->name_space),
			Operand(module->name)
		});
		std::make_shared<ASTIncludeNamespaceNode>(Constants::DEFAULT_NAMESPACE, 0, 0)->accept(this);
		std::make_shared<ASTIncludeNamespaceNode>(module->name_space, 0, 0)->accept(this);

		visit(module);

		current_module_stack.pop();

	}

}

void Compiler::visit(std::shared_ptr<ASTIncludeNamespaceNode> astnode) {
	add_instruction(OpCode::OP_INCLUDE_NAMESPACE, std::vector<Operand> {
		Operand(current_module_stack.top()->name),
		Operand(astnode->name_space)
	});
}

void Compiler::visit(std::shared_ptr<ASTExcludeNamespaceNode> astnode) {
	add_instruction(OpCode::OP_EXCLUDE_NAMESPACE, std::vector<Operand> {
		Operand(current_module_stack.top()->name),
		Operand(astnode->name_space)
	});
}

void Compiler::visit(std::shared_ptr<ASTEnumNode> astnode) {
	for (size_t i = 0; i < astnode->identifiers.size(); ++i) {
		add_instruction(OpCode::OP_PUSH_INT, flx_int(i));

		type_definition_operations(TypeDefinition(Type::T_INT));
		add_instruction(OpCode::OP_STORE_VAR, std::vector<Operand> {
			Operand(current_module_stack.top()->name_space),
			Operand(astnode->identifiers[i])
		});
	}
}

void Compiler::visit(std::shared_ptr<ASTDeclarationNode> astnode) {
	if (astnode->expr) {
		astnode->expr->accept(this);
	}
	else {
		add_instruction(OpCode::OP_PUSH_UNDEFINED);
	}

	if (!astnode->is_static_dim) {
		add_instruction(OpCode::OP_SET_CHECK_BUILD_ARR);
	}

	type_definition_operations(*astnode);
	add_instruction(OpCode::OP_STORE_VAR, std::vector<Operand> {
		Operand(current_module_stack.top()->name_space),
		Operand(astnode->identifier)
	});

}

void Compiler::visit(std::shared_ptr<ASTUnpackedDeclarationNode> astnode) {
	for (const auto& declaration : astnode->declarations) {
		declaration->accept(this);
	}
}

void Compiler::visit(std::shared_ptr<ASTReturnNode> astnode) {
	if (astnode->expr) {
		astnode->expr->accept(this);
	}
	else {
		add_instruction(OpCode::OP_PUSH_UNDEFINED);
	}

	add_instruction(OpCode::OP_RETURN);
}

void Compiler::visit(std::shared_ptr<ASTFunctionCallNode> astnode) {
	bool self_call = astnode->identifier_vector.size() > 1 && astnode->identifier_vector[0].identifier == "self";
	
	for (const auto& param : astnode->parameters) {
		param->accept(this);
	}

	auto identifier = astnode->identifier;

	// handle function sub value access (inside structs or class function calls)
	if (astnode->identifier_vector.size() > 1 && !self_call) {
		auto idnode = std::make_shared<ASTIdentifierNode>(astnode->identifier_vector, astnode->access_name_space, astnode->row, astnode->col);
		idnode->accept(this);
		identifier = ""; // set identifier empty as we already pushed function to stack
	}

	if (self_call) {
		add_instruction(OpCode::OP_SELF_INVOKE);
	}

	add_instruction(OpCode::OP_CALL, std::vector<Operand>{
		Operand(current_module_stack.top()->name_space),
		Operand(current_module_stack.top()->name),
		Operand(astnode->access_name_space),
		Operand(identifier), // when identifier is empty, it means that function will be loaded from the stack
		Operand(astnode->parameters.size())
	});

	// handle return sub value access
	if (!astnode->expression_identifier_vector[0].identifier.empty()) {
		add_instruction(OpCode::OP_LOAD_VAR, std::vector<Operand>{
			Operand(current_module_stack.top()->name_space),
			Operand(current_module_stack.top()->name),
			Operand(astnode->access_name_space),
			Operand(flx_string(astnode->expression_identifier_vector[0].identifier))
		});
	}
	access_sub_value_operations(astnode->expression_identifier_vector);

	// handle function return expression call
	if (astnode->expression_call) {
		astnode->expression_call->accept(this);
	}

}

void Compiler::visit(std::shared_ptr<ASTFunctionDefinitionNode> astnode) {
	current_this_name.push(std::make_pair("function", astnode->identifier));

	// function will be defined here
	type_definition_operations(*astnode);
	add_instruction(OpCode::OP_FUN_START, flx_string(astnode->identifier));

	for (auto& param : astnode->parameters) {
		if (const auto& var = std::dynamic_pointer_cast<VariableDefinition>(param)){
			declare_variable_definition(*var);
			add_instruction(OpCode::OP_FUN_SET_PARAM, std::vector<Operand> {
				Operand(var->is_rest),
				Operand(flx_string(var->identifier))
			});
		}
		else if (const auto& uvar = std::dynamic_pointer_cast<UnpackedVariableDefinition>(param)) {
			type_definition_operations(*uvar);
			add_instruction(OpCode::OP_FUN_START_UNPACK_PARAM);

			for (const auto& var : uvar->variables) {
				declare_variable_definition(var);
				add_instruction(OpCode::OP_FUN_SET_SUB_PARAM, std::vector<Operand> {
					Operand(var.is_rest),
					Operand(flx_string(var.identifier))
				});
			}

			add_instruction(OpCode::OP_FUN_SET_UNPACK_PARAM);
		}
	}

	// call will start after that, function metadata will be stored with at starting pointer
	add_instruction(OpCode::OP_FUN_END, std::vector<Operand> {
		Operand(current_module_stack.top()->name_space),
		Operand(current_module_stack.top()->name),
		Operand(flx_bool(astnode->block))
	});

	if (astnode->block) {
		// at this point, vm will jump to end of block
		auto endblock_jmp_ptr = add_instruction(OpCode::OP_JUMP, size_t(0));

		astnode->block->accept(this);

		// it will return to prev
		add_instruction(OpCode::OP_PUSH_UNDEFINED);
		add_instruction(OpCode::OP_RETURN);

		replace_operand(endblock_jmp_ptr, pointer);
	}

	current_this_name.pop();
}

void Compiler::visit(std::shared_ptr<ASTLambdaFunctionNode> astnode) {
	astnode->fun->accept(this);
	add_instruction(OpCode::OP_PUSH_FUNCTION, std::vector<Operand> {
		Operand(current_module_stack.top()->name_space),
		Operand(flx_string(astnode->fun->identifier))
	});
}

void Compiler::visit(std::shared_ptr<ASTBlockNode> astnode) {
	push_scope();
	for (const auto& statement : astnode->statements) {
		statement->accept(this);
		remove_unused_constant(statement);
	}
	pop_scope();
}

void Compiler::visit(std::shared_ptr<ASTExitNode> astnode) {
	astnode->exit_code->accept(this);
	add_instruction(OpCode::OP_HALT);
}

void Compiler::visit(std::shared_ptr<ASTContinueNode>) {
	add_instruction(OpCode::OP_UNWIND);
	start_pointers.top().push_back(add_instruction(OpCode::OP_JUMP, size_t(0)));
}

void Compiler::visit(std::shared_ptr<ASTBreakNode>) {
	add_instruction(OpCode::OP_UNWIND);
	end_pointers.top().push_back(add_instruction(OpCode::OP_JUMP, size_t(0)));
}

void Compiler::visit(std::shared_ptr<ASTSwitchNode> astnode) {
	push_scope();

	open_end_pointers();

	add_instruction(OpCode::OP_PUSH_DEEP);

	astnode->condition->accept(this);
	std::unordered_map<size_t, std::vector<size_t>> jmp_pointers;

	for (const auto& [consexpr, pos] : astnode->parsed_case_blocks) {
		add_instruction(OpCode::OP_DUP_CONSTANT);
		add_instruction(OpCode::OP_PUSH_INT, size_t(consexpr));
		add_instruction(OpCode::OP_EQL);
		jmp_pointers[pos].push_back(add_instruction(OpCode::OP_JUMP_IF_TRUE, size_t(0)));
	}

	if (astnode->default_block < astnode->statements.size()){
		jmp_pointers[astnode->default_block].push_back(add_instruction(OpCode::OP_JUMP, size_t(0)));
	}

	end_pointers.top().push_back(add_instruction(OpCode::OP_JUMP, size_t(0)));

	for (size_t i = 0; i < astnode->statements.size(); ++i) {
		if (jmp_pointers.find(i) != jmp_pointers.end()) {
			for (const auto& jmp_pointer : jmp_pointers[i]) {
				replace_operand(jmp_pointer, size_t(pointer));
			}
		}

		astnode->statements[i]->accept(this);
		remove_unused_constant(astnode->statements[i]);

	}

	close_end_pointers();

	// pop unpoped scopes
	add_instruction(OpCode::OP_POP_DEEP);

	pop_scope();
}

void Compiler::visit(std::shared_ptr<ASTElseIfNode> astnode) {
	astnode->condition->accept(this);

	auto ip = add_instruction(OpCode::OP_JUMP_IF_FALSE, size_t(0));

	astnode->block->accept(this);

	if_end_pointers.top().push_back(add_instruction(OpCode::OP_JUMP, size_t(0)));

	replace_operand(ip, size_t(pointer));
}

void Compiler::visit(std::shared_ptr<ASTIfNode> astnode) {
	open_if_end_pointers();

	astnode->condition->accept(this);

	auto ip = add_instruction(OpCode::OP_JUMP_IF_FALSE, size_t(0));

	astnode->if_block->accept(this);

	if_end_pointers.top().push_back(add_instruction(OpCode::OP_JUMP, size_t(0)));

	replace_operand(ip, size_t(pointer));

	for (const auto& elif : astnode->else_ifs) {
		elif->accept(this);
	}

	if (astnode->else_block) {
		astnode->else_block->accept(this);
	}

	close_if_end_pointers();

}

void Compiler::visit(std::shared_ptr<ASTForNode> astnode) {
	push_scope();

	open_end_pointers();
	open_start_pointers();

	add_instruction(OpCode::OP_PUSH_DEEP);

	if (astnode->expressions[0]) {
		astnode->expressions[0]->accept(this);
		remove_unused_constant(astnode->expressions[0]);
	}

	auto start = pointer;
	if (astnode->expressions[1]) {
		astnode->expressions[1]->accept(this);
	}
	else {
		add_instruction(OpCode::OP_PUSH_BOOL, true);
	}
	auto ip = add_instruction(OpCode::OP_JUMP_IF_FALSE, size_t(0));

	astnode->block->accept(this);

	auto continue_start = pointer;

	if (astnode->expressions[2]) {
		astnode->expressions[2]->accept(this);
		remove_unused_constant(astnode->expressions[2]);
	}

	// jump to start of loop to evaluate condition again
	add_instruction(OpCode::OP_JUMP, size_t(start));

	// replace jump if false operand with current pointer to not execute block on false condition
	replace_operand(ip, size_t(pointer));

	close_start_pointers(continue_start);
	// replace commands that breaks block with end pointer
	close_end_pointers();

	// pop unpoped scopes
	add_instruction(OpCode::OP_POP_DEEP);

	pop_scope();
}

void Compiler::visit(std::shared_ptr<ASTInstructionNode> astnode) {
	switch (astnode->operand.type)
	{
	case OperandType::OT_RAW:
		add_instruction(astnode->opcode, flx_int(astnode->operand.get_raw_operand()));
		break;
	case OperandType::OT_UINT8:
		add_instruction(astnode->opcode, astnode->operand.get_uint8_operand());
		break;
	case OperandType::OT_SIZE:
		add_instruction(astnode->opcode, astnode->operand.get_size_operand());
		break;
	case OperandType::OT_BOOL:
		add_instruction(astnode->opcode, astnode->operand.get_bool_operand());
		break;
	case OperandType::OT_INT:
		add_instruction(astnode->opcode, astnode->operand.get_int_operand());
		break;
	case OperandType::OT_FLOAT:
		add_instruction(astnode->opcode, astnode->operand.get_float_operand());
		break;
	case OperandType::OT_CHAR:
		add_instruction(astnode->opcode, astnode->operand.get_char_operand());
		break;
	case OperandType::OT_STRING:
		add_instruction(astnode->opcode, astnode->operand.get_string_operand());
		break;
	case OperandType::OT_VECTOR:
		add_instruction(astnode->opcode, astnode->operand.get_vector_operand());
		break;
	}
}

void Compiler::visit(std::shared_ptr<ASTForEachNode> astnode) {
	push_scope();

	open_end_pointers();
	open_start_pointers();

	add_instruction(OpCode::OP_PUSH_DEEP);

	astnode->collection->accept(this);

	add_instruction(OpCode::OP_GET_ITERATOR);

	auto start = pointer;

	add_instruction(OpCode::OP_HAS_NEXT_ELEMENT);

	auto ip = add_instruction(OpCode::OP_JUMP_IF_FALSE, size_t(0));

	if (const auto idnode = std::dynamic_pointer_cast<ASTUnpackedDeclarationNode>(astnode->itdecl)) {
		add_instruction(OpCode::OP_NEXT_ELEMENT);

		for (const auto& decl : idnode->declarations) {
			decl->expr = std::make_shared<ASTInstructionNode>(OpCode::OP_PUSH_VALUE_FROM_STRUCT, Operand(decl->identifier), decl->row, decl->col);
			decl->accept(this);
			decl->expr = nullptr;
		}
		add_instruction(OpCode::OP_POP_CONSTANT);
	}
	else if (const auto idnode = std::dynamic_pointer_cast<ASTIdentifierNode>(astnode->itdecl)) {
		add_instruction(OpCode::OP_PUSH_VAR_REF, flx_bool(true));
		idnode->accept(this);
		add_instruction(OpCode::OP_POP_VAR_REF);

		add_instruction(OpCode::OP_NEXT_ELEMENT);

		add_instruction(OpCode::OP_ASSIGN);
	}
	else if (const auto& idnode = std::dynamic_pointer_cast<ASTDeclarationNode>(astnode->itdecl)) {
		add_instruction(OpCode::OP_NEXT_ELEMENT);

		idnode->expr = std::make_shared<ASTInstructionNode>(OpCode::OP_SKIP, Operand(new uint8_t), idnode->row, idnode->col);
		idnode->accept(this);
		idnode->expr = nullptr;
	}

	astnode->block->accept(this);

	// jump to start of loop to evaluate condition again
	add_instruction(OpCode::OP_JUMP, size_t(start));
	close_start_pointers(start);

	// jump to end of loop
	replace_operand(ip, size_t(pointer));

	// replace commands that breaks block with end pointer
	close_end_pointers();

	// pop unpoped scopes
	add_instruction(OpCode::OP_POP_DEEP);

	pop_scope();
}

void Compiler::visit(std::shared_ptr<ASTTryCatchNode> astnode) {
	auto tryip = add_instruction(OpCode::OP_TRY, size_t(0));

	add_instruction(OpCode::OP_PUSH_DEEP);

	push_scope();
	
	astnode->try_block->accept(this);

	pop_scope();

	add_instruction(OpCode::OP_TRY_END);

	auto ip = add_instruction(OpCode::OP_JUMP, size_t(0));

	replace_operand(tryip, size_t(pointer));

	// pop unpoped scopes
	add_instruction(OpCode::OP_POP_DEEP);

	push_scope();

	if (const auto& idnode = std::dynamic_pointer_cast<ASTUnpackedDeclarationNode>(astnode->decl)) {
		add_instruction(OpCode::OP_PUSH_ERROR_DESC);
		type_definition_operations(*idnode->declarations[0]);
		add_instruction(OpCode::OP_STORE_VAR, std::vector<Operand> {
			Operand(current_module_stack.top()->name_space),
			Operand(flx_string(idnode->declarations[0]->identifier))
		});

		add_instruction(OpCode::OP_PUSH_ERROR_CODE);
		type_definition_operations(*idnode->declarations[1]);
		add_instruction(OpCode::OP_STORE_VAR, std::vector<Operand> {
			Operand(current_module_stack.top()->name_space),
			Operand(flx_string(idnode->declarations[1]->identifier))
		});
	}
	else if (const auto& idnode = std::dynamic_pointer_cast<ASTDeclarationNode>(astnode->decl)) {
		add_instruction(OpCode::OP_INIT_STRUCT, std::vector<Operand>{
			Operand(current_module_stack.top()->name_space),
			Operand(current_module_stack.top()->name),
			Operand(Constants::DEFAULT_NAMESPACE),
			Operand(flx_string(Constants::BUILTIN_STRUCT_NAMES[BuiltinStructs::BS_EXCEPTION]))
		});

		add_instruction(OpCode::OP_PUSH_ERROR_CODE);
		add_instruction(OpCode::OP_SET_FIELD, std::vector<Operand>{
			Operand(current_module_stack.top()->name_space),
			Operand(current_module_stack.top()->name),
			Operand(flx_string(Constants::STR_EXCEPTION_FIELD_NAMES[StrExceptionFields::SXF_CODE]))
		});

		add_instruction(OpCode::OP_PUSH_ERROR_DESC);
		add_instruction(OpCode::OP_SET_FIELD, std::vector<Operand>{
			Operand(current_module_stack.top()->name_space),
			Operand(current_module_stack.top()->name),
			Operand(flx_string(Constants::STR_EXCEPTION_FIELD_NAMES[StrExceptionFields::SXF_ERROR]))
		});

		add_instruction(OpCode::OP_PUSH_STRUCT);

		type_definition_operations(*idnode);
		add_instruction(OpCode::OP_STORE_VAR, std::vector<Operand> {
			Operand(current_module_stack.top()->name_space),
			Operand(flx_string(idnode->identifier))
		});
	}
	add_instruction(OpCode::OP_POP_ERROR);

	astnode->catch_block->accept(this);

	pop_scope();

	auto endip = add_instruction(OpCode::OP_JUMP, size_t(0));

	// try executed with no erros and execution will continue here
	replace_operand(ip, size_t(pointer));

	add_instruction(OpCode::OP_POP_DEEP);

	replace_operand(endip, size_t(pointer));

}

void Compiler::visit(std::shared_ptr<ASTThrowNode> astnode) {
	astnode->error->accept(this);
	add_instruction(OpCode::OP_THROW);
}

void Compiler::visit(std::shared_ptr<ASTEllipsisNode>) {}

void Compiler::visit(std::shared_ptr<ASTWhileNode> astnode) {
	open_end_pointers();
	open_start_pointers();

	add_instruction(OpCode::OP_PUSH_DEEP);

	auto start = pointer;

	astnode->condition->accept(this);

	auto ip = add_instruction(OpCode::OP_JUMP_IF_FALSE, size_t(0));

	astnode->block->accept(this);

	add_instruction(OpCode::OP_JUMP, size_t(start));

	replace_operand(ip, size_t(pointer));

	close_end_pointers();
	close_start_pointers(start);

	add_instruction(OpCode::OP_POP_DEEP);
}

void Compiler::visit(std::shared_ptr<ASTDoWhileNode> astnode) {
	open_end_pointers();
	open_start_pointers();

	add_instruction(OpCode::OP_PUSH_DEEP);

	auto start = pointer;

	astnode->block->accept(this);

	auto continue_start = pointer;

	astnode->condition->accept(this);

	add_instruction(OpCode::OP_JUMP_IF_TRUE, size_t(start));

	close_end_pointers();
	close_start_pointers(continue_start);

	add_instruction(OpCode::OP_POP_DEEP);
}

void Compiler::visit(std::shared_ptr<ASTStructDefinitionNode> astnode) {
	add_instruction(OpCode::OP_STRUCT_START, flx_string(astnode->identifier));

	for (const auto& var : astnode->variables) {
		declare_variable_definition(*var.second);
		add_instruction(OpCode::OP_STRUCT_SET_VAR, flx_string(var.first));
	}

	add_instruction(OpCode::OP_STRUCT_END, current_module_stack.top()->name_space);
}

void Compiler::visit(std::shared_ptr<ASTClassDefinitionNode> astnode) {
	current_this_name.push(std::make_pair("class", astnode->identifier));

	// here we will create a temporary namespace to store class variables and functions
	add_instruction(OpCode::OP_CLASS_START, std::vector<Operand>{
		Operand(current_module_stack.top()->name_space),
		Operand(current_module_stack.top()->name),
		Operand(flx_string(astnode->identifier))
	});
	for (const auto& var : astnode->declarations) {
		declare_variable_definition(VariableDefinition(var->identifier, *var, var->expr));
		add_instruction(OpCode::OP_CLASS_SET_VAR, flx_string(var->identifier));
	}
	for (const auto& fun : astnode->functions) {
		fun->accept(this);
	}
	// finally, we get all variables and functions and store them in the definition to be used in instantiation
	add_instruction(OpCode::OP_CLASS_END, std::vector<Operand>{
		Operand(current_module_stack.top()->name_space),
		Operand(current_module_stack.top()->name)
	});

	current_this_name.pop();
}

void Compiler::visit(std::shared_ptr<ASTValueNode>) {}

void Compiler::visit(std::shared_ptr<ASTLiteralNode<flx_bool>> astnode) {
	add_instruction(OpCode::OP_PUSH_BOOL, flx_bool(astnode->value));
}

void Compiler::visit(std::shared_ptr<ASTLiteralNode<flx_int>> astnode) {
	add_instruction(OpCode::OP_PUSH_INT, flx_int(astnode->value));
}

void Compiler::visit(std::shared_ptr<ASTLiteralNode<flx_float>> astnode) {
	add_instruction(OpCode::OP_PUSH_FLOAT, flx_float(astnode->value));
}

void Compiler::visit(std::shared_ptr<ASTLiteralNode<flx_char>> astnode) {
	add_instruction(OpCode::OP_PUSH_CHAR, flx_char(astnode->value));
}

void Compiler::visit(std::shared_ptr<ASTLiteralNode<flx_string>> astnode) {
	add_instruction(OpCode::OP_PUSH_STRING, flx_string(astnode->value));
}

void Compiler::visit(std::shared_ptr<ASTArrayConstructorNode> astnode) {
	auto size = astnode->values.size();

	type_definition_operations(*astnode);
	add_instruction(OpCode::OP_INIT_ARRAY, size_t(size));

	for (size_t i = 0; i < size; ++i) {
		astnode->values[i]->accept(this);
		add_instruction(OpCode::OP_SET_ELEMENT, size_t(i));
	}

	add_instruction(OpCode::OP_PUSH_ARRAY);
}

void Compiler::visit(std::shared_ptr<ASTStructConstructorNode> astnode) {
	add_instruction(OpCode::OP_INIT_STRUCT, std::vector<Operand>{
		Operand(current_module_stack.top()->name_space),
		Operand(current_module_stack.top()->name),
		Operand(astnode->type_name_space),
		Operand(flx_string(astnode->type_name))
	});

	for (const auto& expr : astnode->values) {
		expr.second->accept(this);
		add_instruction(OpCode::OP_SET_FIELD, std::vector<Operand>{
			Operand(current_module_stack.top()->name_space),
			Operand(current_module_stack.top()->name),
			Operand(flx_string(expr.first))
		});
	}

	add_instruction(OpCode::OP_PUSH_STRUCT);
}

void Compiler::visit(std::shared_ptr<ASTIdentifierNode> astnode) {
	auto identifier = astnode->identifier;
	auto identifier_vector = astnode->identifier_vector;

	if (identifier == "self") {
		add_instruction(OpCode::OP_SELF_INVOKE);
		identifier_vector.erase(identifier_vector.begin());
		identifier = identifier_vector[0].identifier;
	}

	add_instruction(OpCode::OP_LOAD_VAR, std::vector<Operand>{
		Operand(current_module_stack.top()->name_space),
		Operand(current_module_stack.top()->name),
		Operand(astnode->access_name_space),
		Operand(flx_string(identifier))
	});

	if (has_sub_value(identifier_vector)) {
		access_sub_value_operations(identifier_vector);
	}
}

void Compiler::visit(std::shared_ptr<ASTBinaryExprNode> astnode) {
	add_instruction(OpCode::OP_PUSH_VAR_REF, flx_bool(Token::is_assignment_op(astnode->op)));
	astnode->left->accept(this);
	add_instruction(OpCode::OP_POP_VAR_REF);

	if (astnode->op == "and") {
		add_instruction(OpCode::OP_DUP_CONSTANT);
		size_t skip_right = add_instruction(OpCode::OP_JUMP_IF_FALSE, size_t(0));
		add_instruction(OpCode::OP_POP_CONSTANT);
		astnode->right->accept(this);
		replace_operand(skip_right, size_t(pointer));
	}
	else if (astnode->op == "or") {
		add_instruction(OpCode::OP_DUP_CONSTANT);
		size_t skip_right = add_instruction(OpCode::OP_JUMP_IF_TRUE, size_t(0));
		add_instruction(OpCode::OP_POP_CONSTANT);
		astnode->right->accept(this);
		replace_operand(skip_right, size_t(pointer));
	}
	else {
		// other operations
		astnode->right->accept(this);
		add_instruction(get_opcode_operation(astnode->op));
	}
}

void Compiler::visit(std::shared_ptr<ASTUnaryExprNode> astnode) {
	auto op = OpCode::OP_RES;
	if (astnode->unary_op == "-") {
		op = OpCode::OP_UNARY_SUB;
	}
	else if (astnode->unary_op == "not") {
		op = OpCode::OP_NOT;
	}
	else if (astnode->unary_op == "~") {
		op = OpCode::OP_BIT_NOT;
	}
	else if (astnode->unary_op == "++") {
		op = OpCode::OP_INC;
	}
	else if (astnode->unary_op == "--") {
		op = OpCode::OP_DEC;
	}

	astnode->expr->accept(this);

	add_instruction(op);
}

void Compiler::visit(std::shared_ptr<ASTTernaryNode> astnode) {
	astnode->condition->accept(this);
	size_t skip_false = add_instruction(OpCode::OP_JUMP_IF_FALSE, size_t(0));
	astnode->value_if_true->accept(this);
	size_t skip_end = add_instruction(OpCode::OP_JUMP, size_t(0));
	replace_operand(skip_false, size_t(pointer));
	astnode->value_if_false->accept(this);
	replace_operand(skip_end, size_t(pointer));
}

void Compiler::visit(std::shared_ptr<ASTTypeCastNode> astnode) {
	astnode->expr->accept(this);
	add_instruction(OpCode::OP_TYPE_PARSE, uint8_t(astnode->type));
}

void Compiler::visit(std::shared_ptr<ASTTypeNode> astnode) {
	type_definition_operations(astnode->type);
	add_instruction(OpCode::OP_PUSH_TYPE);
}

void Compiler::visit(std::shared_ptr<ASTNullNode>) {
	add_instruction(OpCode::OP_PUSH_VOID);
}

void Compiler::visit(std::shared_ptr<ASTThisNode> astnode) {
	std::make_shared<ASTStructConstructorNode>(
		Constants::DEFAULT_NAMESPACE,
		Constants::BUILTIN_STRUCT_NAMES[BuiltinStructs::BS_CONTEXT],
		std::map<std::string, std::shared_ptr<ASTExprNode>>{
			{ Constants::STR_CONTEXT_FIELD_NAMES[StrContextFields::SCF_NAME],
				std::make_shared<ASTLiteralNode<flx_string>>(current_this_name.top().second, astnode->row, astnode->col) },
			{ Constants::STR_CONTEXT_FIELD_NAMES[StrContextFields::SCF_NAMESPACE],
				std::make_shared<ASTLiteralNode<flx_string>>(current_module_stack.top()->name_space, astnode->row, astnode->col) },
			{ Constants::STR_CONTEXT_FIELD_NAMES[StrContextFields::SCF_TYPE],
				std::make_shared<ASTLiteralNode<flx_string>>(current_this_name.top().first, astnode->row, astnode->col)}
		},
		astnode->row, astnode->col
	)->accept(this);

	if (has_sub_value(astnode->access_vector)) {
		access_sub_value_operations(astnode->access_vector);
	}
}

void Compiler::visit(std::shared_ptr<ASTTypeOfNode> astnode) {
	astnode->expr->accept(this);

	add_instruction(OpCode::OP_TYPEOF);
}

void Compiler::visit(std::shared_ptr<ASTTypeIdNode> astnode) {
	astnode->expr->accept(this);

	add_instruction(OpCode::OP_TYPEID);
}

void Compiler::visit(std::shared_ptr<ASTRefIdNode> astnode) {
	astnode->expr->accept(this);

	add_instruction(OpCode::OP_REFID);
}

void Compiler::visit(std::shared_ptr<ASTIsStructNode> astnode) {
	astnode->expr->accept(this);
	add_instruction(OpCode::OP_IS_STRUCT);
}

void Compiler::visit(std::shared_ptr<ASTIsArrayNode> astnode) {
	astnode->expr->accept(this);
	add_instruction(OpCode::OP_IS_ARRAY);
}

void Compiler::visit(std::shared_ptr<ASTIsAnyNode> astnode) {
	astnode->expr->accept(this);
	add_instruction(OpCode::OP_IS_ANY);
}

bool Compiler::has_sub_value(std::vector<Identifier> identifier_vector) {
	return identifier_vector.size() > 1 || identifier_vector[0].access_vector.size() > 0;
}

void Compiler::type_definition_operations(TypeDefinition type) {
	if (type.dim.size() > 0) {
		for (const auto& s : type.dim) {
			add_instruction(OpCode::OP_PUSH_INT, size_t(s));
			add_instruction(OpCode::OP_SET_ARRAY_SIZE);
		}
	}
	else if (type.expr_dim.size() > 0) {
		for (const auto& s : type.expr_dim) {
			if (s) {
				s->accept(this);
			}
			else {
				add_instruction(OpCode::OP_PUSH_INT, size_t(0));
			}
			add_instruction(OpCode::OP_SET_ARRAY_SIZE);
		}
	}

	add_instruction(OpCode::OP_PUSH_TYPE_DEF, std::vector<Operand> {
		Operand(uint8_t(type.type)),
		Operand(flx_string(type.type_name_space)),
		Operand(flx_string(type.type_name))
	});
}

void Compiler::access_sub_value_operations(std::vector<Identifier> identifier_vector) {
	if (has_sub_value(identifier_vector)) {
		for (size_t i = 0; i < identifier_vector.size(); ++i) {
			const auto& id = identifier_vector[i];

			if (i > 0) {
				add_instruction(OpCode::OP_LOAD_SUB_ID, flx_string(id.identifier));
			}

			for (auto& av : id.access_vector) {
				if (av) {
					av->accept(this);
				}
				else {
					add_instruction(OpCode::OP_PUSH_INT, size_t(0));
				}
				add_instruction(OpCode::OP_LOAD_SUB_IX);
			}
		}
	}
}

void Compiler::remove_unused_constant(std::shared_ptr<ASTNode> astnode) {
	if (const auto& expr = std::dynamic_pointer_cast<ASTExprNode>(astnode)) {
		add_instruction(OpCode::OP_POP_CONSTANT);
	}
}

void Compiler::set_debug_info() {
	if (current_debug_info_stack.empty()) {
		vm_debug.debug_info_table[pointer] = std::vector<Operand>{
			Operand(vm_debug.index_of_namespace(current_module_stack.top()->name_space)),
			Operand(vm_debug.index_of_module(current_module_stack.top()->name)),
			Operand(vm_debug.index_of_ast_type("<program>")),
			Operand(size_t(0)),
			Operand(flx_string("")),
			Operand(size_t(0)),
			Operand(size_t(0))
		};
		return;
	}

	const auto& debug_info = current_debug_info_stack.top();
	vm_debug.debug_info_table[pointer] = std::vector<Operand>{
		Operand(vm_debug.index_of_namespace(debug_info.module_name_space)),
		Operand(vm_debug.index_of_module(debug_info.module_name)),
		Operand(vm_debug.index_of_ast_type(debug_info.ast_type)),
		Operand(vm_debug.index_of_namespace(debug_info.access_name_space)),
		Operand(debug_info.identifier),
		Operand(debug_info.row),
		Operand(debug_info.col)
	};
}

template <typename T>
size_t Compiler::add_instruction(OpCode opcode, T operand) {
	set_debug_info();
	auto ins_pointer = pointer;
	bytecode_program.push_back(BytecodeInstruction(opcode, operand));
	++pointer;
	return ins_pointer;
}

size_t Compiler::add_instruction(OpCode opcode) {
	set_debug_info();
	auto ins_pointer = pointer;
	bytecode_program.push_back(BytecodeInstruction(opcode, nullptr, 0));
	++pointer;
	return ins_pointer;
}

template <typename T>
void Compiler::replace_operand(size_t pos, T operand) {
	bytecode_program[pos].operand = Operand(operand);
}

void Compiler::open_start_pointers() {
	start_pointers.push(std::vector<size_t>());
}

void Compiler::close_start_pointers(size_t sp) {
	for (auto eip : start_pointers.top()) {
		replace_operand(eip, size_t(sp));
	}

	start_pointers.pop();
}

void Compiler::open_end_pointers() {
	end_pointers.push(std::vector<size_t>());
}

void Compiler::close_end_pointers() {
	for (auto eip : end_pointers.top()) {
		replace_operand(eip, size_t(pointer));
	}

	end_pointers.pop();
}

void Compiler::open_if_end_pointers() {
	if_end_pointers.push(std::vector<size_t>());
}

void Compiler::close_if_end_pointers() {
	for (auto eip : if_end_pointers.top()) {
		replace_operand(eip, size_t(pointer));
	}

	if_end_pointers.pop();
}

void Compiler::push_scope() {
	add_instruction(OpCode::OP_PUSH_SCOPE, std::vector<Operand> {
		Operand(current_module_stack.top()->name_space),
		Operand(current_module_stack.top()->name)
	});
}

void Compiler::pop_scope() {
	add_instruction(OpCode::OP_POP_SCOPE, std::vector<Operand> {
		Operand(current_module_stack.top()->name_space),
		Operand(current_module_stack.top()->name)
	});
}

OpCode Compiler::get_opcode_operation(const std::string& operation) {
	if (operation == "or") {
		return OpCode::OP_OR;
	}
	else if (operation == "and") {
		return OpCode::OP_AND;
	}
	else if (operation == "|") {
		return OpCode::OP_BIT_OR;
	}
	else if (operation == "^") {
		return OpCode::OP_BIT_XOR;
	}
	else if (operation == "&") {
		return OpCode::OP_BIT_AND;
	}
	else if (operation == "==") {
		return OpCode::OP_EQL;
	}
	else if (operation == "!=") {
		return OpCode::OP_DIF;
	}
	else if (operation == "<") {
		return OpCode::OP_LT;
	}
	else if (operation == "<=") {
		return OpCode::OP_LTE;
	}
	else if (operation == ">") {
		return OpCode::OP_GT;
	}
	else if (operation == ">=") {
		return OpCode::OP_GTE;
	}
	else if (operation == "<=>") {
		return OpCode::OP_SPACE_SHIP;
	}
	else if (operation == "<<") {
		return OpCode::OP_LEFT_SHIFT;
	}
	else if (operation == ">>") {
		return OpCode::OP_RIGHT_SHIFT;
	}
	else if (operation == "+") {
		return OpCode::OP_ADD;
	}
	else if (operation == "-") {
		return OpCode::OP_SUB;
	}
	else if (operation == "*") {
		return OpCode::OP_MUL;
	}
	else if (operation == "/") {
		return OpCode::OP_DIV;
	}
	else if (operation == "%") {
		return OpCode::OP_REMAINDER;
	}
	else if (operation == "/%") {
		return OpCode::OP_FLOOR_DIV;
	}
	else if (operation == "**") {
		return OpCode::OP_EXP;
	}
	else if (operation == "++") {
		return OpCode::OP_INC;
	}
	else if (operation == "--") {
		return OpCode::OP_DEC;
	}
	else if (operation == "=") {
		return OpCode::OP_ASSIGN;
	}
	else if (operation == "+=") {
		return OpCode::OP_ADD_ASSIGN;
	}
	else if (operation == "-=") {
		return OpCode::OP_SUB_ASSIGN;
	}
	else if (operation == "*=") {
		return OpCode::OP_MUL_ASSIGN;
	}
	else if (operation == "/=") {
		return OpCode::OP_DIV_ASSIGN;
	}
	else if (operation == "%=") {
		return OpCode::OP_REMAINDER_ASSIGN;
	}
	else if (operation == "/%=") {
		return OpCode::OP_FLOOR_DIV_ASSIGN;
	}
	else if (operation == "**=") {
		return OpCode::OP_EXP_ASSIGN;
	}
	else if (operation == "|=") {
		return OpCode::OP_BIT_OR_ASSIGN;
	}
	else if (operation == "^=") {
		return OpCode::OP_BIT_XOR_ASSIGN;
	}
	else if (operation == "&=") {
		return OpCode::OP_BIT_AND_ASSIGN;
	}
	else if (operation == "<<=") {
		return OpCode::OP_LEFT_SHIFT_ASSIGN;
	}
	else if (operation == ">>=") {
		return OpCode::OP_RIGHT_SHIFT_ASSIGN;
	}
	else if (operation == "ref") {
		return OpCode::OP_REF;
	}
	else if (operation == "unref") {
		return OpCode::OP_UNREF;
	}
	else if (operation == "in") {
		return OpCode::OP_IN;
	}

	throw std::runtime_error("Unknown operation: " + operation);
}

void Compiler::declare_variable_definition(VariableDefinition var) {
	if (var.get_expr_default()) {
		auto id = add_instruction(OpCode::OP_JUMP, size_t(0));
		auto start_def = pointer;
		var.get_expr_default()->accept(this);
		add_instruction(OpCode::OP_TRAP);
		replace_operand(id, size_t(pointer));
		add_instruction(OpCode::OP_SET_DEFAULT_VALUE, start_def);
	}
	type_definition_operations(var);
}
