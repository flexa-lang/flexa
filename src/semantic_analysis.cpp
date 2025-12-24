#include "semantic_analysis.hpp"

#include "exception_helper.hpp"
#include "token.hpp"
#include "md_builtin.hpp"
#include "constants.hpp"
#include "utils.hpp"

using namespace core;
using namespace core::analysis;

SemanticAnalyser::SemanticAnalyser(std::shared_ptr<Scope> global_scope, std::shared_ptr<ASTModuleNode> main_module,
	std::map<std::string, std::shared_ptr<ASTModuleNode>> modules, const std::vector<std::string>& args)
	: Visitor(modules, main_module), args(args) {

	push_scope(std::make_shared<Scope>(Constants::DEFAULT_NAMESPACE, Constants::BUILTIN_MODULE_NAME));
	Constants::BUILTIN_FUNCTIONS->register_functions(this);

	setup_global_namespace(global_scope);
};

void SemanticAnalyser::start() {
	current_module_stack.top()->accept(this);
}

void SemanticAnalyser::visit(std::shared_ptr<ASTModuleNode> astnode) {
	module_level++;
	for (const auto& statement : astnode->statements) {
		try {
			statement->accept(this);
		}
		catch (std::runtime_error ex) {
			if (exception) {
				throw std::runtime_error(ex.what());
			}
			exception = true;
			throw std::runtime_error(current_debug_info_stack.top().build_error_message("SemanticError", ex.what()));
		}
	}
	module_level--;

	if (module_level == 0) {
		// validate that all functions with no block
		for (const auto& func_meta : declared_functions) {
			const auto& func = std::get<0>(func_meta);
			if (!func->block) {
				current_debug_info_stack.top().row = std::get<1>(func_meta);
				current_debug_info_stack.top().col = std::get<2>(func_meta);
				throw std::runtime_error(
					current_debug_info_stack.top().build_error_message(
						"SemanticError", "function '" + func->identifier + "' was declared with no block"
					)
				);
			}
		}
		declared_functions.clear();
	}
}

void SemanticAnalyser::visit(std::shared_ptr<ASTUsingNode> astnode) {
	std::string libname = utils::StringUtils::join(astnode->library, ".");

	if (Constants::CORE_LIBS.find(libname) != Constants::CORE_LIBS.end()) {
		Constants::CORE_LIBS.find(libname)->second->register_functions(this);
	}

	if (modules.find(libname) == modules.end()) {
		throw std::runtime_error("lib '" + libname + "' not found");
	}

	auto& module = modules[libname];

	// add lib to current module
	if (utils::CollectionUtils::contains(current_module_stack.top()->libs, module)) {
		throw std::runtime_error("lib '" + libname + "' already declared in " + current_module_stack.top()->name);
	}
	current_module_stack.top()->libs.push_back(module);

	// if not parsed yet
	if (!utils::CollectionUtils::contains(parsed_libs, libname)) {
		parsed_libs.push_back(libname);

		current_module_stack.push(module);
		setup_global_namespace(std::make_shared<Scope>(module->name_space, module->name));
		start();
		current_module_stack.pop();
	}
}

void SemanticAnalyser::visit(std::shared_ptr<ASTIncludeNamespaceNode> astnode) {
	const auto& prg_name = current_module_stack.top()->name;

	validate_namespace(astnode->name_space);

	if (std::find(module_included_name_spaces[prg_name].begin(), module_included_name_spaces[prg_name].end(), astnode->name_space) == module_included_name_spaces[prg_name].end()) {
		module_included_name_spaces[prg_name].push_back(astnode->name_space);
	}
	else {
		throw std::runtime_error("namespace '" + astnode->name_space + "' already included in '" + prg_name + "'");
	}
}

void SemanticAnalyser::visit(std::shared_ptr<ASTExcludeNamespaceNode> astnode) {
	const auto& module_name = current_module_stack.top()->name;

	validate_namespace(astnode->name_space);

	size_t pos = std::distance(module_included_name_spaces[module_name].begin(),
		std::find(module_included_name_spaces[module_name].begin(),
			module_included_name_spaces[module_name].end(), astnode->name_space));
	module_included_name_spaces[module_name].erase(module_included_name_spaces[module_name].begin() + pos);
}

void SemanticAnalyser::visit(std::shared_ptr<ASTEnumNode> astnode) {
	const auto& module_name_space = current_module_stack.top()->name_space;

	for (size_t i = 0; i < astnode->identifiers.size(); ++i) {
		auto value = std::make_shared<SemanticValue>(Type::T_INT, i, true);
		auto variable = std::make_shared<SemanticVariable>(astnode->identifiers[i], Type::T_INT, true);
		variable->set_value(value);
		get_back_scope(module_name_space)->declare_variable(astnode->identifiers[i], variable);
	}
}

void SemanticAnalyser::visit(std::shared_ptr<ASTDeclarationNode> astnode) {
	const auto& current_module = current_module_stack.top();
	
	auto astnode_dim = evaluate_dimension_vector(astnode->expr_dim);

	std::shared_ptr<Scope> current_scope = get_back_scope(current_module->name_space);

	if (current_scope->already_declared_variable(astnode->identifier)) {
		throw std::runtime_error("variable '" + astnode->identifier + "' already declared");
	}

	if (astnode->is_void()) {
		throw std::runtime_error("variables cannot be declared as void type: '" + astnode->identifier + "'");
	}

	determine_object_type(astnode);

	if (astnode->expr) {
		// check and if necessary, build array constructor expression
		auto new_arr_expr = check_build_array(astnode_dim, astnode->expr);
		if (new_arr_expr) {
			astnode->expr = new_arr_expr;
			astnode->is_static_dim = true;
		}

		astnode->expr->accept(this);
		if (current_expression.is_undefined()) {
			throw std::runtime_error("'" + astnode->identifier + "' decaration expression is undefined");
		}
	}
	else {
		current_expression = SemanticValue(TypeDefinition(Type::T_UNDEFINED));
	}

	auto new_value = std::make_shared<SemanticValue>();
	new_value->copy_from(current_expression);

	if (astnode->is_constexpr && !new_value->is_constexpr) {
		throw std::runtime_error("initializer of '" + astnode->identifier + "' is not a expression constant");
	}

	if (astnode->type_name.empty()) {
		astnode->type_name = new_value->type_name;
	}

	auto new_var = std::make_shared<SemanticVariable>(
		astnode->identifier, TypeDefinition(astnode->type, astnode_dim,
			astnode->type_name_space, astnode->type_name),
		astnode->is_const || astnode->is_constexpr);
	new_var->set_value(new_value);

	if (!new_var->is_any_or_match_type_def(*new_value, false, true)
		&& astnode->expr && !new_value->is_undefined()) {
		ExceptionHelper::throw_declaration_type_err(astnode->identifier, *new_var, *new_value);
	}

	if (new_var->is_string() || new_var->is_float()) {
		new_value->type = new_var->type;
	}

	current_scope->declare_variable(astnode->identifier, new_var);
}

void SemanticAnalyser::visit(std::shared_ptr<ASTUnpackedDeclarationNode> astnode) {
	std::shared_ptr<ASTIdentifierNode> var = nullptr;
	determine_object_type(astnode);

	if (astnode->expr) {
		var = std::dynamic_pointer_cast<ASTIdentifierNode>(astnode->expr);
		if (!var) {
			throw std::runtime_error("expected variable as value of unpacked declaration, but found value");
		}
	}

	if (var) {
		var->accept(this);
		if (!astnode->is_any_or_match_type_def(current_expression)) {
			ExceptionHelper::throw_mismatched_type_err(*astnode, current_expression);
		}
	}

	for (const auto& declaration : astnode->declarations) {
		if (var) {
			auto ids = var->identifier_vector;
			ids.push_back(Identifier(declaration->identifier));
			auto access_expr = std::make_shared<ASTIdentifierNode>(ids, var->access_name_space, declaration->row, declaration->col);
			declaration->expr = access_expr;
		}

		declaration->accept(this);
	}
}

void SemanticAnalyser::visit(std::shared_ptr<ASTReturnNode> astnode) {
	auto return_expr = SemanticValue();

	if (astnode->expr) {
		astnode->expr->accept(this);
		return_expr = current_expression;
		if (return_expr.is_undefined()) {
			if (!current_function.empty()) {
				throw std::runtime_error("'" + current_function.top()->identifier + "' return expression is undefined");
			}
			throw std::runtime_error("return expression is undefined");
		}
	}

	if (!current_function.empty()) {
		auto& currfun = current_function.top();
		if (!currfun->is_any_or_match_type_def(return_expr)) {
			ExceptionHelper::throw_return_type_err(currfun->identifier, *currfun, return_expr);
		}
	}
}

void SemanticAnalyser::visit(std::shared_ptr<ASTFunctionCallNode> astnode) {
	const auto& current_module = current_module_stack.top();
	const auto& normalized_name_space = normalize_name_space(astnode->access_name_space, current_module->name_space);
	bool strict = true;
	auto returned_expression = current_expression;

	std::vector<std::shared_ptr<TypeDefinition>> signature;

	for (const auto& param : astnode->parameters) {
		param->accept(this);
		if (current_expression.is_undefined()) {
			throw std::runtime_error("'" + astnode->identifier + "' parameter in position " + std::to_string(signature.size()) + " is undefined");
		}

		signature.push_back(std::make_shared<TypeDefinition>(current_expression));
	}

	// handle function return
	if (astnode->identifier.empty()) {
		if (!returned_expression.is_function() && !returned_expression.is_any()) {
			throw std::runtime_error(ExceptionHelper::buid_signature(astnode->identifier_vector, signature));
		}

		current_expression = SemanticValue(Type::T_ANY, 0, 0);

	}
	// handle subvalue call
	else if (astnode->identifier_vector.size() > 1) {
		auto idnode = std::make_shared<ASTIdentifierNode>(astnode->identifier_vector, normalized_name_space, astnode->row, astnode->col);
		idnode->accept(this);

		if (!current_expression.is_function() && !current_expression.is_any()) {
			throw std::runtime_error(ExceptionHelper::buid_signature(astnode->identifier_vector, signature));
		}

	}
	// handle regular call
	else {
		std::shared_ptr<Scope> curr_scope = get_inner_most_function_scope(current_module->name_space, current_module->name, normalized_name_space, astnode->identifier, &signature, strict);

		if (!curr_scope) {
			strict = false;
			curr_scope = get_inner_most_function_scope(current_module->name_space, current_module->name, normalized_name_space, astnode->identifier, &signature, strict);

			if (!curr_scope) {
				auto var_scope = get_inner_most_variable_scope(current_module->name_space, current_module->name, normalized_name_space, astnode->identifier);

				// if there's no variable
				if (!var_scope) {
					if (auto obj_scope = get_inner_most_class_definition_scope(current_module->name_space, current_module->name, normalized_name_space, astnode->identifier)) {

						auto obj_def = obj_scope->find_declared_class_definition(astnode->identifier);
						current_expression = SemanticValue(TypeDefinition(Type::T_CLASS, obj_scope->module_name_space, astnode->identifier));

						return;
					}

					if (!curr_scope) {
						std::string func_name = ExceptionHelper::buid_signature(astnode->identifier, signature);
						throw std::runtime_error("function '" + func_name + "' was never declared");
					}
				}

				auto var = std::dynamic_pointer_cast<SemanticVariable>(var_scope->find_declared_variable(astnode->identifier));

				// if variable is not a function type, throw error
				if (!var->is_function() && !var->is_any()) {
					ExceptionHelper::throw_undeclared_function(astnode->identifier, signature);
				}

				current_expression = SemanticValue(TypeDefinition(Type::T_ANY));

				return;

			}
		}

		auto& curr_function = curr_scope->find_declared_function(astnode->identifier, &signature, strict);

		if (curr_function->is_void()) {
			current_expression = SemanticValue(TypeDefinition(Type::T_UNDEFINED));
		}
		else {
			auto typedeg = std::make_shared<SemanticValue>(*curr_function);
			if (astnode->expression_identifier_vector.size() > 0) {
				current_expression = *access_value(typedeg, astnode->expression_identifier_vector);
			}
			else if (astnode->expression_call) {
				current_expression = SemanticValue(TypeDefinition(Type::T_FUNCTION));
			}
			else {
				current_expression = *typedeg;
			}
		}

		if (astnode->expression_call) {
			astnode->expression_call->accept(this);
		}

	}

}

void SemanticAnalyser::visit(std::shared_ptr<ASTFunctionDefinitionNode> astnode) {
	const auto& current_module = current_module_stack.top();
	std::shared_ptr<FunctionDefinition> decl_function = nullptr;
	bool declared = false;
	determine_object_type(astnode);

	for (auto& param : astnode->parameters) {
		determine_object_type(param);
	}

	std::shared_ptr<Scope> declare_scope;
	if (!class_stack.empty()) {
		declare_scope = class_stack.top();
	}
	else {
		declare_scope = get_global_scope(current_module->name);
	}

	if (declare_scope->already_declared_function(astnode->identifier, &astnode->parameters)) {
		decl_function = declare_scope->find_declared_function(astnode->identifier, &astnode->parameters);

		if (decl_function->block) {
			std::string signature = ExceptionHelper::buid_signature(astnode->identifier, astnode->parameters);
			throw std::runtime_error("function " + signature + " already defined");
		}
	}

	if (astnode->block) {
		auto has_return = returns(astnode->block);
		astnode->type = astnode->is_void() && has_return ? Type::T_ANY : astnode->type;

		if (astnode->identifier != "") {
			if (decl_function){
				decl_function->type = astnode->type;
				decl_function->block = astnode->block;
			}
			else {
				decl_function = std::make_shared<FunctionDefinition>(astnode->identifier,
					TypeDefinition(astnode->type, evaluate_dimension_vector(astnode->expr_dim), astnode->type_name_space, astnode->type_name),
					astnode->parameters, astnode->block);
				declare_scope->declare_function(astnode->identifier, decl_function);
			}

			current_function.push(decl_function);
		}

		astnode->block->accept(this);

		if (!astnode->is_void()) {
			if (!has_return) {
				throw std::runtime_error("defined function '" + astnode->identifier + "' is not guaranteed to return a value");
			}
		}

		current_function.pop();
	}
	else {
		if (astnode->identifier != "") {
			decl_function = std::make_shared<FunctionDefinition>(astnode->identifier,
				TypeDefinition(astnode->type, evaluate_dimension_vector(astnode->expr_dim), astnode->type_name_space, astnode->type_name),
				astnode->parameters, astnode->block);
			declare_scope->declare_function(astnode->identifier, decl_function);

			if (decl_function->identifier != "init"
				&& !(current_module->name_space == Constants::STD_NAMESPACE
					&& std::find(
						Constants::CORE_LIB_NAMES.begin(),
						Constants::CORE_LIB_NAMES.end(),
						current_module->name
					) != Constants::CORE_LIB_NAMES.end())) {
				declared_functions.push_back(std::make_tuple(decl_function, astnode->row, astnode->col));
			}
		}
	}

}

void SemanticAnalyser::visit(std::shared_ptr<ASTLambdaFunctionNode> astnode) {
	auto fun = std::dynamic_pointer_cast<ASTFunctionDefinitionNode>(astnode->fun);

	auto tempfun = std::make_shared<FunctionDefinition>(fun->identifier,
		TypeDefinition(fun->type, evaluate_dimension_vector(fun->expr_dim), fun->type_name_space, fun->type_name),
		fun->parameters, fun->block);

	current_function.push(tempfun);

	fun->accept(this);

	current_expression = SemanticValue(TypeDefinition(Type::T_FUNCTION, evaluate_dimension_vector(fun->expr_dim), fun->type_name_space, fun->type_name));
}

void SemanticAnalyser::visit(std::shared_ptr<ASTBlockNode> astnode) {
	const auto& current_module = current_module_stack.top();

	push_scope(std::make_shared<Scope>(current_module->name_space, current_module->name));

	auto curr_scope = get_back_scope(current_module->name_space);

	if (!current_function.empty()) {
		for (auto param : current_function.top()->parameters) {
			if (const auto decl = std::dynamic_pointer_cast<VariableDefinition>(param)) {
				declare_function_parameter(curr_scope, *decl);
			}
			else if (const auto decls = std::dynamic_pointer_cast<UnpackedVariableDefinition>(param)) {
				for (auto& decl : decls->variables) {
					declare_function_parameter(curr_scope, decl);
				}
			}
		}
	}

	for (const auto& stmt : astnode->statements) {
		stmt->accept(this);
	}

	pop_scope(current_module->name_space, current_module->name);
}

void SemanticAnalyser::visit(std::shared_ptr<ASTExitNode> astnode) {
	astnode->exit_code->accept(this);

	if (current_expression.is_undefined()) {
		throw std::runtime_error("exit expression is undefined");
	}

	if (!current_expression.is_int()) {
		throw std::runtime_error("expected int value");
	}
}

void SemanticAnalyser::visit(std::shared_ptr<ASTContinueNode> astnode) {
	if (!is_loop) {
		throw std::runtime_error("continue must be inside a loop");
	}
}

void SemanticAnalyser::visit(std::shared_ptr<ASTBreakNode> astnode) {
	if (!is_loop && !is_switch) {
		throw std::runtime_error("break must be inside a loop or switch");
	}
}

void SemanticAnalyser::visit(std::shared_ptr<ASTSwitchNode> astnode) {
	const auto& current_module = current_module_stack.top();

	is_switch = true;

	push_scope(std::make_shared<Scope>(current_module->name_space, current_module->name));

	astnode->parsed_case_blocks = std::map<size_t, size_t>();

	astnode->condition->accept(this);

	if (current_expression.is_undefined()) {
		throw std::runtime_error("switch expression is undefined");
	}

	TypeDefinition cond_type = current_expression;
	auto case_type = TypeDefinition(Type::T_UNDEFINED);

	for (const auto& expr : astnode->case_blocks) {
		expr.first->accept(this);

		if (current_expression.is_undefined()) {
			throw std::runtime_error("case expression is undefined");
		}

		if (!current_expression.is_constexpr) {
			throw std::runtime_error("case expression is not an constant");
		}

		if (case_type.is_undefined()) {
			if (current_expression.is_undefined() || current_expression.is_void() || current_expression.is_any()) {
				throw std::runtime_error("case values cannot be undefined");
			}
			case_type = current_expression;
		}

		if (!case_type.match_type(current_expression)) {
			ExceptionHelper::throw_mismatched_type_err(case_type, current_expression);
		}

		intmax_t hash = current_expression.hash;

		if (astnode->parsed_case_blocks.contains(hash)) {
			throw std::runtime_error("duplicated case value: '" + std::to_string(hash) + "'");
		}

		astnode->parsed_case_blocks.emplace(hash, expr.second);
	}

	if (!cond_type.is_any_or_match_type_def(case_type)) {
		ExceptionHelper::throw_mismatched_type_err(cond_type, case_type);
	}

	for (auto& stmt : astnode->statements) {
		stmt->accept(this);
	}

	pop_scope(current_module->name_space, current_module->name);
	is_switch = false;
}

void SemanticAnalyser::visit(std::shared_ptr<ASTElseIfNode> astnode) {
	astnode->condition->accept(this);

	if (current_expression.is_undefined()) {
		throw std::runtime_error("else if expression is undefined");
	}

	if (!current_expression.is_bool() && !current_expression.is_any()) {
		ExceptionHelper::throw_condition_type_err();
	}

	astnode->block->accept(this);
}

void SemanticAnalyser::visit(std::shared_ptr<ASTIfNode> astnode) {
	astnode->condition->accept(this);

	if (current_expression.is_undefined()) {
		throw std::runtime_error("if expression is undefined");
	}

	if (!current_expression.is_bool() && !current_expression.is_any()) {
		ExceptionHelper::throw_condition_type_err();
	}

	astnode->if_block->accept(this);

	for (const auto& elif : astnode->else_ifs) {
		elif->accept(this);
	}

	if (astnode->else_block) {
		astnode->else_block->accept(this);
	}
}

void SemanticAnalyser::visit(std::shared_ptr<ASTForNode> astnode) {
	const auto& current_module = current_module_stack.top();

	is_loop = true;

	push_scope(std::make_shared<Scope>(current_module->name_space, current_module->name));

	if (astnode->expressions[0]) {
		astnode->expressions[0]->accept(this);
	}
	if (astnode->expressions[1]) {
		astnode->expressions[1]->accept(this);

		if (current_expression.is_undefined()) {
			throw std::runtime_error("for expression is undefined");
		}

		if (!current_expression.is_bool() && !current_expression.is_any()) {
			ExceptionHelper::throw_condition_type_err();
		}
	}
	if (astnode->expressions[2]) {
		astnode->expressions[2]->accept(this);
	}
	astnode->block->accept(this);

	pop_scope(current_module->name_space, current_module->name);
	is_loop = false;
}

void SemanticAnalyser::visit(std::shared_ptr<ASTForEachNode> astnode) {
	const auto& current_module = current_module_stack.top();
	const auto& name_space = current_module->name_space;

	is_loop = true;

	push_scope(std::make_shared<Scope>(current_module->name_space, current_module->name));
	std::shared_ptr<Scope> back_scope = get_back_scope(current_module->name_space);

	astnode->collection->accept(this);
	SemanticValue col_value = current_expression;

	if (const auto idnode = std::dynamic_pointer_cast<ASTUnpackedDeclarationNode>(astnode->itdecl)) {
		if (!col_value.is_struct() && !col_value.is_any()) {
			throw std::runtime_error("[key, value] can only be used with struct");
		}

		if (idnode->declarations.size() != 2) {
			throw std::runtime_error("invalid number of values");
		}

		auto key_node = std::make_shared<ASTLiteralNode<flx_string>>("", astnode->row, astnode->col);
		idnode->declarations[0]->expr = key_node;
		auto temp_val = std::make_shared<SemanticValue>(TypeDefinition(Type::T_ANY));
		auto val_node = std::make_shared<ASTValueNode>(temp_val.get(), astnode->row, astnode->col);
		idnode->declarations[1]->expr = val_node;
		idnode->accept(this);
		idnode->declarations[0]->expr = nullptr;
		idnode->declarations[1]->expr = nullptr;
	}
	else if (const auto idnode = std::dynamic_pointer_cast<ASTDeclarationNode>(astnode->itdecl)) {
		if (!col_value.is_iterable() && !col_value.is_any()) {
			throw std::runtime_error("expected iterable in foreach");
		}

		std::shared_ptr<SemanticValue> value;

		if (col_value.is_struct()) {
			value = std::make_shared<SemanticValue>(
				TypeDefinition(
					Type::T_STRUCT,
					Constants::DEFAULT_NAMESPACE,
					Constants::BUILTIN_STRUCT_NAMES[BuiltinStructs::BS_ENTRY]
				)
			);
		}
		else if (col_value.is_string()) {
			value = std::make_shared<SemanticValue>(TypeDefinition(Type::T_CHAR));
		}
		else if (col_value.is_any()) {
			value = std::make_shared<SemanticValue>(TypeDefinition(Type::T_ANY));
		}
		else if (col_value.dim.size() > 1) {
			auto dim = col_value.dim;
			if (dim.size() > 0) {
				dim.erase(dim.begin());
			}
			if (!idnode->is_any()) {
				value = std::make_shared<SemanticValue>(
					TypeDefinition(idnode->type, dim, idnode->type_name_space, idnode->type_name));
			}
			else {
				value = std::make_shared<SemanticValue>(
					TypeDefinition(idnode->type, dim, col_value.type_name_space, col_value.type_name));
			}
		}
		else {
			value = std::make_shared<SemanticValue>(TypeDefinition(col_value.type, current_expression.type_name_space, current_expression.type_name));
		}

		std::shared_ptr<ASTValueNode> exnode = std::make_shared<ASTValueNode>(value.get(), astnode->row, astnode->col);
		idnode->expr = exnode;
		idnode->accept(this);
		idnode->expr = nullptr;

	}
	else if (const auto idnode = std::dynamic_pointer_cast<ASTIdentifierNode>(astnode->itdecl)) {
		if (!col_value.is_array() && !col_value.is_string() && !col_value.is_struct() && !col_value.is_any()) {
			throw std::runtime_error("expected iterable in foreach");
		}

		std::shared_ptr<SemanticValue> value;

		if (col_value.is_struct()) {
			value = std::make_shared<SemanticValue>(
				TypeDefinition(
					Type::T_STRUCT,
					Constants::DEFAULT_NAMESPACE,
					Constants::BUILTIN_STRUCT_NAMES[BuiltinStructs::BS_ENTRY]
				)
			);
		}
		else if (col_value.is_string()) {
			value = std::make_shared<SemanticValue>(TypeDefinition(Type::T_CHAR));
		}
		else if (col_value.is_any()) {
			value = std::make_shared<SemanticValue>(TypeDefinition(Type::T_ANY));
		}
		else {
			value = std::make_shared<SemanticValue>(TypeDefinition(col_value.type, current_expression.type_name_space, current_expression.type_name));
		}

		auto exnode = std::make_shared<ASTValueNode>(value.get(), astnode->row, astnode->col);

		// creates an assignment node
		auto assign_node = std::make_shared<ASTBinaryExprNode>("=", idnode, exnode, idnode->row, idnode->col);
		assign_node->accept(this);

	}
	else {
		throw std::runtime_error("expected declaration or identifier");
	}

	astnode->block->accept(this);

	pop_scope(current_module->name_space, current_module->name);
	is_loop = false;
}

void SemanticAnalyser::visit(std::shared_ptr<ASTTryCatchNode> astnode) {
	const auto& current_module = current_module_stack.top();

	astnode->try_block->accept(this);

	push_scope(std::make_shared<Scope>(current_module->name_space, current_module->name));

	auto error_node = std::make_shared<ASTLiteralNode<flx_string>>("", astnode->row, astnode->col);
	auto code_node = std::make_shared<ASTLiteralNode<flx_int>>(0, astnode->row, astnode->col);

	if (const auto idnode = std::dynamic_pointer_cast<ASTUnpackedDeclarationNode>(astnode->decl)) {
		if (idnode->declarations.size() != 2) {
			throw std::runtime_error("invalid number of values");
		}
		idnode->declarations[0]->expr = error_node;
		idnode->declarations[1]->expr = code_node;
		idnode->accept(this);
		idnode->declarations[0]->expr = nullptr;
		idnode->declarations[1]->expr = nullptr;
	}
	else if (const auto idnode = std::dynamic_pointer_cast<ASTDeclarationNode>(astnode->decl)) {
		auto exnode = std::make_shared<ASTStructConstructorNode>(
			Constants::DEFAULT_NAMESPACE,
			Constants::BUILTIN_STRUCT_NAMES[BuiltinStructs::BS_EXCEPTION],
			std::map<std::string, std::shared_ptr<ASTExprNode>>{
				{ Constants::STR_EXCEPTION_FIELD_NAMES[StrExceptionFields::SXF_ERROR], error_node },
				{ Constants::STR_EXCEPTION_FIELD_NAMES[StrExceptionFields::SXF_CODE], code_node }
			},
			astnode->row,
			astnode->col
		);

		idnode->expr = exnode;
		idnode->accept(this);
		idnode->expr = nullptr;
	}
	else if (!std::dynamic_pointer_cast<ASTEllipsisNode>(astnode->decl)) {
		throw std::runtime_error("expected declaration");
	}

	astnode->catch_block->accept(this);

	pop_scope(current_module->name_space, current_module->name);
}

void SemanticAnalyser::visit(std::shared_ptr<ASTThrowNode> astnode) {
	const auto& current_module = current_module_stack.top();

	astnode->error->accept(this);

	if (!(current_expression.is_struct()
			&& current_expression.type_name_space == Constants::DEFAULT_NAMESPACE
			&& current_expression.type_name == Constants::BUILTIN_STRUCT_NAMES[BuiltinStructs::BS_EXCEPTION]
		|| current_expression.is_string())) {
		throw std::runtime_error(
			"expected "
			+ TypeDefinition::buid_struct_type_name(Constants::DEFAULT_NAMESPACE,
				Constants::BUILTIN_STRUCT_NAMES[BuiltinStructs::BS_EXCEPTION])
			+ " or string in throw"
		);
	}
}

void SemanticAnalyser::visit(std::shared_ptr<ASTEllipsisNode> astnode) {}

void SemanticAnalyser::visit(std::shared_ptr<ASTWhileNode> astnode) {
	is_loop = true;
	astnode->condition->accept(this);

	if (current_expression.is_undefined()) {
		throw std::runtime_error("while expression is undefined");
	}

	if (!current_expression.is_bool() && !current_expression.is_any()) {
		ExceptionHelper::throw_condition_type_err();
	}

	astnode->block->accept(this);
	is_loop = false;
}

void SemanticAnalyser::visit(std::shared_ptr<ASTDoWhileNode> astnode) {
	is_loop = true;
	astnode->condition->accept(this);

	if (current_expression.is_undefined()) {
		throw std::runtime_error("do-while expression is undefined");
	}

	if (!current_expression.is_bool() && !current_expression.is_any()) {
		ExceptionHelper::throw_condition_type_err();
	}

	astnode->block->accept(this);
	is_loop = false;
}

void SemanticAnalyser::visit(std::shared_ptr<ASTStructDefinitionNode> astnode) {
	const auto& current_module = current_module_stack.top();

	std::shared_ptr<Scope> current_scope = get_back_scope(current_module->name_space);

	if (current_scope->already_declared_struct_definition(astnode->identifier)) {
		throw std::runtime_error("struct '" + astnode->identifier + "' already defined");
	}

	for (auto& var : astnode->variables) {
		var.second->dim = evaluate_dimension_vector(var.second->expr_dim);
	}

	auto str = std::make_shared<StructDefinition>(astnode->identifier, astnode->variables);

	current_scope->declare_struct_definition(str);

	for (auto& var : astnode->variables) {
		determine_object_type(var.second);
	}
}

void SemanticAnalyser::visit(std::shared_ptr<ASTValueNode> astnode) {
	current_expression = *dynamic_cast<SemanticValue*>(astnode->value);
}

void SemanticAnalyser::visit(std::shared_ptr<ASTLiteralNode<flx_bool>> astnode) {
	current_expression = SemanticValue();
	current_expression.type = Type::T_BOOL;
	current_expression.is_constexpr = true;
	current_expression.set_b(astnode->value);
}

void SemanticAnalyser::visit(std::shared_ptr<ASTLiteralNode<flx_int>> astnode) {
	current_expression = SemanticValue();
	current_expression.type = Type::T_INT;
	current_expression.is_constexpr = true;
	current_expression.set_i(astnode->value);
}

void SemanticAnalyser::visit(std::shared_ptr<ASTLiteralNode<flx_float>> astnode) {
	current_expression = SemanticValue();
	current_expression.type = Type::T_FLOAT;
	current_expression.is_constexpr = true;
	current_expression.set_f(astnode->value);
}

void SemanticAnalyser::visit(std::shared_ptr<ASTLiteralNode<flx_char>> astnode) {
	current_expression = SemanticValue();
	current_expression.type = Type::T_CHAR;
	current_expression.is_constexpr = true;
	current_expression.set_c(astnode->value);
}

void SemanticAnalyser::visit(std::shared_ptr<ASTLiteralNode<flx_string>> astnode) {
	current_expression = SemanticValue();
	current_expression.type = Type::T_STRING;
	current_expression.is_constexpr = true;
	current_expression.set_s(astnode->value);
}

void SemanticAnalyser::visit(std::shared_ptr<ASTArrayConstructorNode> astnode) {
	flx_int arr_size = 0;

	// clean array type on start
	if (current_expression_array_dim.size() == 0) {
		current_expression_array_type = astnode->values.empty() ? TypeDefinition(Type::T_ANY) : TypeDefinition();
		// used to control nested arrays
		current_expression_array_dim_max = 0;
		// end of array dimension calculation, reached max
		is_max = false;
	}

	// increments array dimension
	++current_expression_array_dim_max;
	// if isn't reached max yet, we adds more one expr_dim
	if (!is_max) {
		current_expression_array_dim.push_back(-1);
	}

	for (size_t i = 0; i < astnode->values.size(); ++i) {
		astnode->values.at(i)->accept(this);
		if (current_expression.is_undefined()) {
			throw std::runtime_error("array value is undefined");
		}

		// if it's undefined or array (nested), it's accepts the first type encountered
		if (current_expression_array_type.is_undefined() || current_expression_array_type.is_array()) {
			current_expression_array_type = current_expression;
		}
		else {
			// else check if is any array
			if (!current_expression_array_type.match_type(current_expression.type)
				&& !current_expression.is_any() && !current_expression.is_void() && !current_expression.is_array()) {
				current_expression_array_type = TypeDefinition(Type::T_ANY);
			}
		}

		++arr_size;
	}

	auto current_sim_index = current_expression_array_dim_max - 1;
	if (current_expression_array_dim[current_sim_index] == -1) {
		current_expression_array_dim[current_sim_index] = arr_size;
	}

	// as size by dimension is fixed, it's not necessary to check after max (max nested deep)
	is_max = true;

	// here it's calculate de current dimension of array
	--current_expression_array_dim_max;
	size_t stay = current_expression_array_dim.size() - current_expression_array_dim_max;
	std::vector<size_t> current_expression_array_dim_aux;
	size_t curr_dim_i = current_expression_array_dim.size() - 1;
	for (size_t i = 0; i < stay; ++i) {
		current_expression_array_dim_aux.emplace(current_expression_array_dim_aux.begin(), current_expression_array_dim.at(curr_dim_i));
		--curr_dim_i;
	}

	// set the array type dimension
	current_expression_array_type.dim = current_expression_array_dim_aux;

	// finally, we set the dimension of the current expression
	current_expression = SemanticValue(current_expression_array_type);

	// set the AST node type for further use
	astnode->type = current_expression_array_type.is_void() ? Type::T_ANY : current_expression_array_type.type;
	astnode->dim = current_expression.dim;
	astnode->type_name = current_expression_array_type.type_name;
	astnode->type_name_space = current_expression_array_type.type_name_space;

	// final type check
	if (current_expression_array_dim_max == 0) {
		current_expression_array_dim.clear();
	}
}

void SemanticAnalyser::visit(std::shared_ptr<ASTStructConstructorNode> astnode) {
	const auto& current_module = current_module_stack.top();
	const auto& normalized_name_space = normalize_name_space(astnode->type_name_space, current_module->name_space);

	std::shared_ptr<Scope> curr_scope = get_inner_most_struct_definition_scope(current_module->name_space, current_module->name, normalized_name_space, astnode->type_name);
	if (!curr_scope) {
		throw std::runtime_error("struct '" + astnode->type_name + "' was not declared");
	}
	astnode->type_name_space = curr_scope->module_name_space;

	auto type_struct = curr_scope->find_declared_struct_definition(astnode->type_name);

	for (auto& expr : astnode->values) {
		if (type_struct->variables.find(expr.first) == type_struct->variables.end()) {
			ExceptionHelper::throw_struct_member_err(normalized_name_space, astnode->type_name, expr.first);
		}
		const auto& var_type_struct = type_struct->variables[expr.first];

		auto build_expr = check_build_array(evaluate_dimension_vector(var_type_struct->expr_dim), expr.second);
		if (build_expr) {
			expr.second = build_expr;
		}

		expr.second->accept(this);

		if (!var_type_struct->is_any_or_match_type_def(current_expression)) {
			ExceptionHelper::throw_mismatched_type_err(*var_type_struct, current_expression);
		}
	}

	current_expression = SemanticValue(TypeDefinition(Type::T_STRUCT, astnode->type_name_space, astnode->type_name));

}

void SemanticAnalyser::visit(std::shared_ptr<ASTIdentifierNode> astnode) {
	const auto& current_module = current_module_stack.top();
	const auto& normalized_name_space = normalize_name_space(astnode->access_name_space, current_module->name_space);
	auto identifier = astnode->identifier;
	auto identifier_vector = astnode->identifier_vector;
	std::shared_ptr<SemanticVariable> declared_variable = nullptr;

	std::shared_ptr<Scope> curr_scope;
	if (identifier == "self") {
		if (identifier_vector.size() == 1) {
			throw std::runtime_error("self class reference cannot be handled");
		}
		identifier_vector.erase(identifier_vector.begin());
		identifier = identifier_vector[0].identifier;
		curr_scope = class_stack.top();

		// check if variable exists in class scope
		try {
			declared_variable = std::dynamic_pointer_cast<SemanticVariable>(curr_scope->find_declared_variable(identifier));
		}
		catch (...) {
			throw std::runtime_error("'" + identifier + "' was not found in '"
				+ TypeDefinition::buid_struct_type_name(curr_scope->module_name_space, curr_scope->module_name)
				+ "' class definition");
		}
	}
	else {
		curr_scope = get_inner_most_variable_scope(current_module->name_space, current_module->name, normalized_name_space, identifier);
	}

	if (!curr_scope) {
		current_expression = SemanticValue();

		curr_scope = get_inner_most_struct_definition_scope(current_module->name_space, current_module->name, normalized_name_space, identifier);

		if (curr_scope) {
			current_expression.type = Type::T_STRUCT;
			return;
		}
		else {
			curr_scope = get_inner_most_function_scope(current_module->name_space, current_module->name, normalized_name_space, identifier, nullptr);
			if (curr_scope) {
				current_expression.type = Type::T_FUNCTION;
				return;
			}
			else {
				throw std::runtime_error("identifier '" + identifier + "' was not declared");
			}
		}
	}

	if (!declared_variable) {
		declared_variable = std::dynamic_pointer_cast<SemanticVariable>(curr_scope->find_declared_variable(identifier));
	}

	if (declared_variable->get_value()->is_undefined() && !is_assignment) {
		throw std::runtime_error("variable '" + identifier + "' is undefined");
	}
	
	auto variable_expr = access_value(declared_variable->get_value(), identifier_vector);

	if (variable_expr->is_undefined() && !is_assignment) {
		throw std::runtime_error("variable '" + ExceptionHelper::buid_member_name(identifier_vector) + "' is undefined");
	}

	current_expression = *variable_expr;

}

void SemanticAnalyser::visit(std::shared_ptr<ASTBinaryExprNode> astnode) {
	if (Token::is_assignment_op(astnode->op)) {
		is_assignment = true;
	}
	astnode->left->accept(this);

	if (current_expression.is_undefined() && !is_assignment) {
		throw std::runtime_error("left expression is undefined");
	}
	is_assignment = false;

	auto lexpr = current_expression;

	astnode->right->accept(this);

	if (current_expression.is_undefined()) {
		throw std::runtime_error("right expression is undefined");
	}

	auto rexpr = current_expression;

	current_expression = SemanticOperations::do_operation(astnode->op, lexpr, rexpr);
	current_expression.is_constexpr = lexpr.is_constexpr && rexpr.is_constexpr;

	// handles constant expressions evaluation
	if (current_expression.is_constexpr) {
		auto rt_lexpr = std::make_shared<RuntimeValue>(lexpr);

		switch (lexpr.type) {
		case Type::T_BOOL:
			rt_lexpr->set(lexpr.b);
			break;
		case Type::T_INT:
			rt_lexpr->set(lexpr.i);
			break;
		case Type::T_FLOAT:
			rt_lexpr->set(lexpr.f);
			break;
		case Type::T_CHAR:
			rt_lexpr->set(lexpr.c);
			break;
		case Type::T_STRING:
			rt_lexpr->set(lexpr.s);
			break;
		}

		auto rt_rexpr = std::make_shared<RuntimeValue>(rexpr);

		switch (rexpr.type) {
		case Type::T_BOOL:
			rt_rexpr->set(rexpr.b);
			break;
		case Type::T_INT:
			rt_rexpr->set(rexpr.i);
			break;
		case Type::T_FLOAT:
			rt_rexpr->set(rexpr.f);
			break;
		case Type::T_CHAR:
			rt_rexpr->set(rexpr.c);
			break;
		case Type::T_STRING:
			rt_rexpr->set(rexpr.s);
			break;
		}

		std::shared_ptr<RuntimeValue> rt_res;

		try {
			rt_res = std::make_shared<RuntimeValue>(RuntimeOperations::do_operation(astnode->op, rt_lexpr.get(), rt_rexpr.get()));
		}
		catch (...) {
			current_expression.is_constexpr = false;
			return;
		}

		switch (rt_res->type) {
		case Type::T_BOOL:
			current_expression.set_b(rt_res->get_b());
			break;
		case Type::T_INT:
			current_expression.set_i(rt_res->get_i());
			break;
		case Type::T_FLOAT:
			current_expression.set_f(rt_res->get_f());
			break;
		case Type::T_CHAR:
			current_expression.set_c(rt_res->get_c());
			break;
		case Type::T_STRING:
			current_expression.set_s(rt_res->get_s());
			break;
		}

	}

}

void SemanticAnalyser::visit(std::shared_ptr<ASTUnaryExprNode> astnode) {
	astnode->expr->accept(this);

	if (current_expression.is_undefined()) {
		throw std::runtime_error("unary expression is undefined");
	}

	switch (current_expression.type) {
	case Type::T_INT:
		if (astnode->unary_op != "+" && astnode->unary_op != "-"
			&& astnode->unary_op != "--" && astnode->unary_op != "++"
			&& astnode->unary_op != "~") {
			throw std::runtime_error("operator '" + astnode->unary_op + "' in front of int expression");
		}
		break;
	case Type::T_FLOAT:
		if (astnode->unary_op != "+" && astnode->unary_op != "-"
			&& astnode->unary_op != "--" && astnode->unary_op != "++") {
			throw std::runtime_error("operator '" + astnode->unary_op + "' in front of float expression");
		}
		break;
	case Type::T_BOOL:
		if (astnode->unary_op != "not") {
			throw std::runtime_error("operator '" + astnode->unary_op + "' in front of boolean expression");
		}
		break;
	case Type::T_ANY:
		if (astnode->unary_op != "not" && astnode->unary_op != "~"
			&& astnode->unary_op != "+" && astnode->unary_op != "-"
			&& astnode->unary_op != "--" && astnode->unary_op != "++") {
			throw std::runtime_error("operator '" + astnode->unary_op + "' in front of boolean expression");
		}
		break;
	default:
		throw std::runtime_error("incompatible unary operator '" + astnode->unary_op +
			"' in front of " + TypeDefinition::type_str(current_expression.type) + " expression");
	}
}

void SemanticAnalyser::visit(std::shared_ptr<ASTTernaryNode> astnode) {
	astnode->condition->accept(this);

	if (current_expression.is_undefined()) {
		throw std::runtime_error("ternary condition is undefined");
	}

	astnode->value_if_true->accept(this);

	if (current_expression.is_undefined()) {
		throw std::runtime_error("left ternary expression is undefined");
	}

	astnode->value_if_false->accept(this);

	if (current_expression.is_undefined()) {
		throw std::runtime_error("right ternary expression is undefined");
	}
}

void SemanticAnalyser::visit(std::shared_ptr<ASTTypeCastNode> astnode) {
	astnode->expr->accept(this);

	if (current_expression.is_undefined()) {
		throw std::runtime_error("cast expression is undefined");
	}

	if ((current_expression.is_array() || current_expression.is_struct())
		&& !TypeDefinition(astnode->type).is_string()) {
		throw std::runtime_error("invalid type conversion from "
			+ TypeDefinition::buid_type_str(current_expression) + " to " + TypeDefinition::type_str(astnode->type));
	}

	current_expression = SemanticValue(TypeDefinition(astnode->type));
}

void SemanticAnalyser::visit(std::shared_ptr<ASTTypeNode> astnode) {
	current_expression = SemanticValue(TypeDefinition(astnode->type), 0, true);

}

void SemanticAnalyser::visit(std::shared_ptr<ASTNullNode> astnode) {
	current_expression = SemanticValue(TypeDefinition(Type::T_VOID));
}

void SemanticAnalyser::visit(std::shared_ptr<ASTThisNode> astnode) {
	current_expression = SemanticValue(TypeDefinition(
		Type::T_STRUCT,
		Constants::DEFAULT_NAMESPACE,
		Constants::BUILTIN_STRUCT_NAMES[BuiltinStructs::BS_CONTEXT])
	);

	current_expression = *access_value(std::make_shared<SemanticValue>(current_expression), astnode->access_vector);
}

void SemanticAnalyser::visit(std::shared_ptr<ASTTypeOfNode> astnode) {
	astnode->expr->accept(this);

	if (current_expression.is_undefined()) {
		throw std::runtime_error("typeof expression is undefined");
	}

	current_expression = SemanticValue(TypeDefinition(Type::T_STRING));
}

void SemanticAnalyser::visit(std::shared_ptr<ASTTypeIdNode> astnode) {
	astnode->expr->accept(this);

	if (current_expression.is_undefined()) {
		throw std::runtime_error("typeid expression is undefined");
	}

	current_expression = SemanticValue(TypeDefinition(Type::T_INT));
}

void SemanticAnalyser::visit(std::shared_ptr<ASTRefIdNode> astnode) {
	astnode->expr->accept(this);

	if (current_expression.is_undefined()) {
		throw std::runtime_error("refid expression is undefined");
	}

	current_expression = SemanticValue(TypeDefinition(Type::T_INT));
}

void SemanticAnalyser::visit(std::shared_ptr<ASTIsStructNode> astnode) {
	astnode->expr->accept(this);

	if (current_expression.is_undefined()) {
		throw std::runtime_error("is_struct expression is undefined");
	}

	current_expression = SemanticValue(TypeDefinition(Type::T_BOOL));
}

void SemanticAnalyser::visit(std::shared_ptr<ASTIsArrayNode> astnode) {
	astnode->expr->accept(this);

	if (current_expression.is_undefined()) {
		throw std::runtime_error("is_array expression is undefined");
	}

	current_expression = SemanticValue(TypeDefinition(Type::T_BOOL));
}

void SemanticAnalyser::visit(std::shared_ptr<ASTIsAnyNode> astnode) {
	astnode->expr->accept(this);

	if (current_expression.is_undefined()) {
		throw std::runtime_error("is_any expression is undefined");
	}

	current_expression = SemanticValue(TypeDefinition(Type::T_BOOL));
}

void SemanticAnalyser::visit(std::shared_ptr<ASTInstructionNode>) {}

void SemanticAnalyser::declare_function_parameter(std::shared_ptr<Scope> scope, const VariableDefinition& param) {
	if (auto expr = param.get_expr_default()) {
		expr->accept(this);
	}

	auto var_expr = std::make_shared<SemanticValue>();
	var_expr->type = param.type;
	var_expr->type_name = param.type_name;
	var_expr->type_name_space = param.type_name_space;
	var_expr->dim = evaluate_dimension_vector(param.expr_dim);

	auto v = std::make_shared<SemanticVariable>(param.identifier, TypeDefinition(param.type,
		evaluate_dimension_vector(param.expr_dim), param.type_name_space, param.type_name));

	v->set_value(var_expr);

	scope->declare_variable(param.identifier, v);
}

bool SemanticAnalyser::has_sub_value(std::vector<Identifier> identifier_vector) {
	return identifier_vector.size() > 1 || identifier_vector[0].access_vector.size() > 0;
}

bool SemanticAnalyser::namespace_exists(const std::string& name_space) {
	return scopes.find(name_space) != scopes.end();
}

void SemanticAnalyser::validate_namespace(const std::string& name_space) const {
	if (!utils::CollectionUtils::contains(all_name_spaces, name_space)) {
		throw std::runtime_error("namespace '" + name_space + "' not found");
	}
	if (name_space == Constants::DEFAULT_NAMESPACE) {
		throw std::runtime_error("namespace '" + name_space + "' is default included");
	}
}

const std::string& SemanticAnalyser::normalize_name_space(std::string& astnode_name_space, const std::string& name_space) const {
	if (astnode_name_space.empty()) {
		astnode_name_space = name_space;
	}
	return astnode_name_space;
}

std::shared_ptr<SemanticValue> SemanticAnalyser::access_value(std::shared_ptr<SemanticValue> value, const std::vector<Identifier>& identifier_vector, size_t i) {
	const auto& current_module = current_module_stack.top();
	const auto& name_space = value->type_name_space.empty() ? current_module->name_space : value->type_name_space;
	std::shared_ptr<SemanticValue> next_value = value;

	auto access_vector = evaluate_access_vector(identifier_vector[i].access_vector);

	// array access
	if (access_vector.size() > 0) {
		// last element access
		if (access_vector.size() == next_value->dim.size()) {
			next_value = std::make_shared<SemanticValue>(TypeDefinition(next_value->type,
				std::vector<size_t>(), next_value->type_name_space, next_value->type_name));
			//next_value->type_ref = value;
		}
		// mid element access
		else if (access_vector.size() < next_value->dim.size()) {
			std::vector<size_t> calc_dim;
			for (size_t i = access_vector.size(); i < next_value->dim.size(); ++i) {
				calc_dim.push_back(next_value->dim[i]);
			}
			next_value = std::make_shared<SemanticValue>(*next_value, next_value->hash, next_value->is_constexpr);
			next_value->dim = calc_dim;
		}
		// string access
		else if (access_vector.size() - 1 == next_value->dim.size() && next_value->is_string()) {
			next_value = std::make_shared<SemanticValue>(TypeDefinition(Type::T_CHAR));
		}
	}

	++i;

	// struct member access
	if (i < identifier_vector.size()) {
		// get value in class members
		if (next_value->is_class()) {
			// todo: finalize class member access
			next_value = std::make_shared<SemanticValue>(TypeDefinition(Type::T_ANY));
			next_value->ref = std::make_shared<SemanticVariable>(identifier_vector[i].identifier, Type::T_ANY, next_value->is_constexpr);
		}
		// get value in any members
		else if (next_value->type_name.empty()) {
			next_value = std::make_shared<SemanticValue>(TypeDefinition(Type::T_ANY));
			next_value->ref = std::make_shared<SemanticVariable>(identifier_vector[i].identifier, Type::T_ANY, next_value->is_constexpr);
		}
		// get value in struct members
		else {
			std::shared_ptr<Scope> curr_scope = get_inner_most_struct_definition_scope(current_module->name_space, current_module->name, name_space, next_value->type_name);
			if (!curr_scope) {
				throw std::runtime_error("cannot find '" + TypeDefinition::buid_struct_type_name(name_space, next_value->type_name) + "' struct");
			}
			auto type_struct = curr_scope->find_declared_struct_definition(next_value->type_name);

			if (type_struct->variables.find(identifier_vector[i].identifier) == type_struct->variables.end()) {
				ExceptionHelper::throw_struct_member_err(next_value->type_name_space, next_value->type_name, identifier_vector[i].identifier);
			}

			const auto& value_type = type_struct->variables[identifier_vector[i].identifier];

			next_value = std::make_shared<SemanticValue>(*value_type);

			next_value->ref = std::make_shared<SemanticVariable>(value_type->identifier, *value_type, value_type->is_const);
		}

		if (identifier_vector[i].access_vector.size() > 0 || i < identifier_vector.size()) {
			return access_value(next_value, identifier_vector, i);
		}
	}

	return next_value;
}

void SemanticAnalyser::check_is_struct_exists(Type type, const std::string& name_space, const std::string& type_name) {
	if (type == Type::T_STRUCT) {
		const auto& current_module = current_module_stack.top();
		if (!get_inner_most_struct_definition_scope(current_module->name_space, current_module->name, name_space, type_name)) {
			throw std::runtime_error("struct '" + TypeDefinition::buid_struct_type_name(name_space, type_name) + "' was not defined");
		}
	}
}

std::vector<size_t> SemanticAnalyser::evaluate_access_vector(const std::vector<std::shared_ptr<ASTExprNode>>& expr_access_vector) {
	auto access_vector = std::vector<size_t>();
	for (const auto& expr_ptr : expr_access_vector) {
		auto expr = std::dynamic_pointer_cast<ASTExprNode>(expr_ptr);
		intmax_t val = -1; // size_t max = complile time check
		if (expr) {
			expr->accept(this);
			if (current_expression.is_constexpr) {
				val = current_expression.hash;
			}
			if (!current_expression.is_int() && !current_expression.is_any()) {
				throw std::runtime_error("array index access must be an integer expression");
			}
		}
		access_vector.push_back(val);
	}
	return access_vector;
}

std::vector<size_t> SemanticAnalyser::evaluate_dimension_vector(const std::vector<std::shared_ptr<ASTExprNode>>& expr_dimension_vector) {
	auto access_vector = std::vector<size_t>();
	for (const auto& expr_ptr : expr_dimension_vector) {
		auto expr = std::dynamic_pointer_cast<ASTExprNode>(expr_ptr);
		intmax_t val = 0;
		if (expr) {
			expr->accept(this);
			if (current_expression.is_constexpr) {
				val = current_expression.hash;
			}
			if (!current_expression.is_int() && !current_expression.is_any()) {
				throw std::runtime_error("array index access must be an integer expression");
			}
		}
		access_vector.push_back(val);
	}
	return access_vector;
}

bool SemanticAnalyser::returns(std::shared_ptr<ASTNode> astnode) {
	if (is_return_node(astnode)) {
		return true;
	}

	if (const auto& block_node = std::dynamic_pointer_cast<ASTBlockNode>(astnode)) {
		bool block_return = false;
		bool sub_return = block_node->statements.size() > 0;
		for (const auto& block_stmt : block_node->statements) {
			if (is_return_node(block_stmt)) {
				block_return = true;
				break;
			}

			if (sub_return) {
				if (!returns(block_stmt)) {
					sub_return = false;
				}

				if (std::dynamic_pointer_cast<ASTBreakNode>(block_stmt) || std::dynamic_pointer_cast<ASTContinueNode>(block_stmt)) {
					sub_return = false;
				}
			}
		}
		return block_return || sub_return;
	}

	if (const auto& if_node = std::dynamic_pointer_cast<ASTIfNode>(astnode)) {
		bool if_return = returns(if_node->if_block);
		bool elif_return = true;
		bool else_return = true;
		for (const auto& elif_stmt : if_node->else_ifs) {
			if (!returns(elif_stmt->block)) {
				elif_return = false;
				break;
			}
		}
		if (if_node->else_block) {
			else_return = returns(if_node->else_block);
		}
		return if_return && elif_return && else_return;
	}

	if (const auto& trycatch_node = std::dynamic_pointer_cast<ASTTryCatchNode>(astnode)) {
		return returns(trycatch_node->try_block) && returns(trycatch_node->catch_block);
	}

	if (const auto& switch_node = std::dynamic_pointer_cast<ASTSwitchNode>(astnode)) {
		std::vector<size_t> positions;
		for (const auto& [key, value] : switch_node->case_blocks) {
			positions.push_back(value);
		}
		std::sort(positions.begin(), positions.end());

		for (size_t pi = 0; pi < positions.size() + 2; ++pi) {
			size_t current_block_start = 0;
			size_t current_block_end = 0;

			if (pi < positions.size()) {
				current_block_start = positions[pi];
				current_block_end = pi < positions.size() - 1 ? positions[pi + 1] : switch_node->default_block;
			}
			else {
				current_block_start = switch_node->default_block;
				current_block_end = switch_node->statements.size();
			}


			bool block_return = false;
			for (size_t i = current_block_start; i < current_block_end; ++i) {
				const auto& switch_stmt = switch_node->statements[i];

				if (returns(switch_stmt)) {
					block_return = true;
					break;
				}

				if (const auto& break_node = std::dynamic_pointer_cast<ASTBreakNode>(switch_stmt)) {
					break;
				}
			}

			if (!block_return) {
				return false;
			}
		}

		return true;
	}

	if (const auto& for_node = std::dynamic_pointer_cast<ASTForNode>(astnode)) {
		return returns(for_node->block);
	}

	if (const auto& foreach_node = std::dynamic_pointer_cast<ASTForEachNode>(astnode)) {
		return returns(foreach_node->block);
	}

	if (const auto& while_node = std::dynamic_pointer_cast<ASTWhileNode>(astnode)) {
		return returns(while_node->block);
	}

	return false;
}

void SemanticAnalyser::visit(std::shared_ptr<ASTClassDefinitionNode> astnode) {
	const auto& current_module = current_module_stack.top();

	std::shared_ptr<Scope> current_scope = get_back_scope(current_module->name_space);

	if (current_scope->already_declared_class_definition(astnode->identifier)) {
		throw std::runtime_error("class '" + astnode->identifier + "' already defined");
	}

	// declare the class in the current scope
	auto cls = std::make_shared<ClassDefinition>(astnode->identifier, astnode->declarations, astnode->functions);
	current_scope->declare_class_definition(cls);

	// first we process the constructor if any
	for (const auto& constructor : astnode->functions) {
		if (constructor->identifier == "init") {
			if (returns(constructor->block) || !constructor->is_void()) {
				throw std::runtime_error("constructors cannot have return");
			}

			push_scope(std::make_shared<Scope>(current_module->name_space, astnode->identifier));
			class_stack.push(get_back_scope(current_module->name_space));

			for (const auto& decl : astnode->declarations) {
				decl->accept(this);
			}

			for (const auto& func : astnode->functions) {
				if (func->identifier != "init") {
					func->accept(this);
				}
			}

			constructor->accept(this);

			class_stack.pop();
			pop_scope(current_module->name_space, astnode->identifier);
		}
	}

	// then we process the rest of the class
	push_scope(std::make_shared<Scope>(current_module->name_space, astnode->identifier));
	class_stack.push(get_back_scope(current_module->name_space));

	for (const auto& decl : astnode->declarations) {
		decl->accept(this);
	}

	for (const auto& func : astnode->functions) {
		if (func->identifier == "init") {
			auto constructor = std::make_shared<ASTFunctionDefinitionNode>(*func);
			constructor->block = nullptr;
			constructor->accept(this);
		}
		else {
			func->accept(this);
		}
	}

	class_stack.pop();
	pop_scope(current_module->name_space, astnode->identifier);

}

void SemanticAnalyser::determine_object_type(std::shared_ptr<TypeDefinition> typedefinition) {
	const auto& current_module = current_module_stack.top();
	Type type = Type::T_UNDEFINED;

	if (typedefinition->is_object()) {
		std::shared_ptr<Scope> curr_scope;
		if (curr_scope = get_inner_most_struct_definition_scope(current_module->name_space, current_module->name, typedefinition->type_name_space, typedefinition->type_name)) {
			type = Type::T_STRUCT;
		}
		else if (curr_scope = get_inner_most_class_definition_scope(current_module->name_space, current_module->name, typedefinition->type_name_space, typedefinition->type_name)) {
			type = Type::T_CLASS;
		}
		else {
			throw std::runtime_error("object '" + TypeDefinition::buid_struct_type_name(typedefinition->type_name_space, typedefinition->type_name) + "' not found");
		}

		if (typedefinition->type_name_space.empty()) {
			typedefinition->type_name_space = curr_scope->module_name_space;
		}
	}

	if (type != Type::T_UNDEFINED) {
		typedefinition->type = type;
	}
}

void SemanticAnalyser::setup_global_namespace(std::shared_ptr<Scope> scope) {
	const auto& module = current_module_stack.top();

	// check if module has default namespace
	module_included_name_spaces[module->name].push_back(module->name_space);
	module_included_name_spaces[module->name].push_back(Constants::DEFAULT_NAMESPACE);

	// check if namespace alread included, if not, we include in all namespaces list
	if (!module->name_space.empty() && !utils::CollectionUtils::contains(all_name_spaces, module->name_space)) {
		all_name_spaces.push_back(module->name_space);
	}

	// add the global module namespace, it will never be removed, during all runtime lifetime
	push_scope(scope);
}

std::shared_ptr<ASTExprNode> SemanticAnalyser::check_build_array(const std::vector<size_t>& dim, std::shared_ptr<ASTExprNode> init_expr) {
	if (dim.empty()) return nullptr;
	for (auto d : dim) if (d == 0) return nullptr;

	if (auto arr = std::dynamic_pointer_cast<ASTArrayConstructorNode>(init_expr)) {
		if (arr->values.size() == 1) {
			return build_array(dim, arr->values[0], 0);
		}
		else if (arr->values.empty()) {
			return build_array(dim, std::make_shared<ASTNullNode>(current_debug_info_stack.top().row, current_debug_info_stack.top().col), 0);
		}
	}

	return nullptr;
}

std::shared_ptr<ASTExprNode> SemanticAnalyser::build_array(const std::vector<size_t>& dim, std::shared_ptr<ASTExprNode> init_value, size_t level) {
	std::vector<std::shared_ptr<ASTExprNode>> values;
	const size_t size = dim[level];
	values.reserve(size);

	for (size_t j = 0; j < size; ++j) {
		if (level + 1 < dim.size()) {
			values.push_back(build_array(dim, init_value, level + 1));
		}
		else {
			values.push_back(init_value);
		}
	}
	return std::make_shared<ASTArrayConstructorNode>(values, current_debug_info_stack.top().row, current_debug_info_stack.top().col);
}

bool SemanticAnalyser::is_return_node(std::shared_ptr<ASTNode> astnode) {
	return std::dynamic_pointer_cast<ASTReturnNode>(astnode)
		|| std::dynamic_pointer_cast<ASTThrowNode>(astnode);
}
