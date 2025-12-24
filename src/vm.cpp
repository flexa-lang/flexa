#include "vm.hpp"

#include "token.hpp"
#include "operand.hpp"
#include "exception_helper.hpp"
#include "utils.hpp"
#include "watch.hpp"
#include "constants.hpp"

using namespace core;
using namespace core::runtime;

VirtualMachine::VirtualMachine(
	std::shared_ptr<Scope> global_scope,
	VmDebug vm_debug,
	std::vector<BytecodeInstruction> instructions
)
	: vm_debug(std::move(vm_debug)),
	instructions(instructions) {
	cleanup_type_set();

	evaluation_stack = std::make_shared<std::vector<RuntimeValue*>>();

	gc.add_root_container(evaluation_stack);

	push_scope(std::make_shared<Scope>(Constants::DEFAULT_NAMESPACE, Constants::BUILTIN_MODULE_NAME));
	Constants::BUILTIN_FUNCTIONS->register_functions(this);

	push_scope(global_scope);
	module_included_name_spaces[global_scope->module_name].push_back(Constants::DEFAULT_NAMESPACE);
	module_included_name_spaces[global_scope->module_name].push_back(global_scope->module_name_space);

}

void VirtualMachine::run() {
	while (get_next()) {
		try {
			decode_operation();

			if (return_from_sub_run) {
				return_from_sub_run = false;
				return;
			}
		}
		catch (std::exception ex) {
			if (!try_stack.empty()) {
				next_pc = try_stack.top();
				try_stack.pop();

				catch_err_stack.push(std::make_pair(0, ex.what()));
			}
			else if (!generated_error) {
				generated_error = true;

				generated_error_msg = get_debug_info(next_pc - 1).build_error_message("RuntimeError", ex.what());

				while (!call_stack.empty()) {
					generated_error_msg += get_debug_info(call_stack.top()).build_error_tail();
					call_stack.pop();
				}
			}
			if (generated_error) {
				throw std::runtime_error(generated_error_msg);
			}
		}
	}

	if (evaluation_stack->empty()) {
		push_new_constant(new RuntimeValue(flx_int(-1)));
	}

	gc.collect();

}

void VirtualMachine::decode_operation() {
	switch (current_instruction.opcode) {
	case OP_RES:
		throw std::runtime_error("Reserved operation");
		break;

		// vm operations
	case OP_PUSH_SCOPE: {
		auto params = current_instruction.operand.get_vector_operand();
		auto module_name_space = params[0].get_string_operand();
		auto module_name = params[1].get_string_operand();
		push_vm_scope(std::make_shared<Scope>(module_name_space, module_name));
		break;
	}
	case OP_POP_SCOPE: {
		auto params = current_instruction.operand.get_vector_operand();
		auto module_name_space = params[0].get_string_operand();
		auto module_name = params[1].get_string_operand();
		pop_vm_scope(module_name_space, module_name);
		break;
	}
	case OP_PUSH_DEEP:
		push_deep();
		break;
	case OP_POP_DEEP:
		pop_deep();
		break;
	case OP_UNWIND:
		unwind();
		break;

		// namespace operations
	case OP_BUILTIN_LIB:
		Constants::CORE_LIBS.find(current_instruction.operand.get_string_operand())->second->register_functions(this);
		break;
	case OP_INCLUDE_NAMESPACE:
		handle_include_namespace();
		break;
	case OP_EXCLUDE_NAMESPACE:
		handle_exclude_namespace();
		break;

		// constant operations
	case OP_POP_CONSTANT:
		pop_constant();
		break;
	case OP_DUP_CONSTANT:
		push_new_constant(new RuntimeValue(evaluation_stack->back()));
		break;
	case OP_PUSH_UNDEFINED:
		push_empty_constant(Type::T_UNDEFINED);
		break;
	case OP_PUSH_VOID:
		push_empty_constant(Type::T_VOID);
		break;
	case OP_PUSH_TYPE:
		push_new_constant(new RuntimeValue(get_type_def()));
		break;
	case OP_PUSH_BOOL:
		push_new_constant(new RuntimeValue(current_instruction.operand.get_bool_operand()));
		break;
	case OP_PUSH_INT:
		push_new_constant(new RuntimeValue(current_instruction.operand.get_int_operand()));
		break;
	case OP_PUSH_FLOAT:
		push_new_constant(new RuntimeValue(current_instruction.operand.get_float_operand()));
		break;
	case OP_PUSH_CHAR:
		push_new_constant(new RuntimeValue(current_instruction.operand.get_char_operand()));
		break;
	case OP_PUSH_STRING:
		push_new_constant(new RuntimeValue(current_instruction.operand.get_string_operand()));
		break;
	case OP_PUSH_FUNCTION: {
		auto params = current_instruction.operand.get_vector_operand();
		push_new_constant(new RuntimeValue(flx_function(params[0].get_string_operand(), params[1].get_string_operand())));
		break;
	}
	case OP_INIT_ARRAY:
		handle_init_array();
		break;
	case OP_SET_ELEMENT:
		handle_set_element();
		break;
	case OP_PUSH_ARRAY:
		handle_push_array();
		break;
	case OP_INIT_STRUCT:
		handle_init_struct();
		break;
	case OP_SET_FIELD:
		handle_set_field();
		break;
	case OP_PUSH_STRUCT:
		handle_push_struct();
		break;
	case OP_PUSH_VALUE_FROM_STRUCT:
		push_constant(evaluation_stack->back()->get_field(current_instruction.operand.get_string_operand()));
		break;

		// struct definition operations
	case OP_STRUCT_START:
		handle_struct_start();
		break;
	case OP_STRUCT_SET_VAR:
		handle_struct_set_var();
		break;
	case OP_STRUCT_END:
		handle_struct_end();
		break;

		// class class definition operations
	case OP_CLASS_START:
		handle_class_start();
		break;
	case OP_CLASS_SET_VAR:
		handle_class_set_var();
		break;
	case OP_CLASS_END:
		handle_class_end();
		break;
	case OP_SELF_INVOKE:
		is_self_invoke = true;
		break;

		// typing operations
	case OP_SET_ARRAY_SIZE:
		set_array_dim.push_back(get_evaluation_stack_top()->get_i());
		break;
	case OP_PUSH_TYPE_DEF:
		handle_push_type_def();
		break;

		// variable operations
	case OP_LOAD_VAR:
		handle_load_var();
		break;
	case OP_STORE_VAR:
		handle_store_var();
		break;
	case OP_SET_CHECK_BUILD_ARR:
		set_check_build_array = true;
		break;
	case OP_LOAD_SUB_ID:
		handle_load_sub_id();
		break;
	case OP_LOAD_SUB_IX:
		handle_load_sub_ix();
		break;
	case OP_PUSH_VAR_REF:
		use_variable_ref.push(current_instruction.operand.get_bool_operand());
		break;
	case OP_POP_VAR_REF:
		use_variable_ref.pop();
		break;

		// function operations
	case OP_FUN_START:
		handle_fun_start();
		break;
	case OP_SET_DEFAULT_VALUE:
		handle_set_default_value();
		break;
	case OP_FUN_SET_PARAM:
		handle_fun_set_param();
		break;
	case OP_FUN_START_UNPACK_PARAM:
		handle_fun_start_unpack_param();
		break;
	case OP_FUN_SET_SUB_PARAM:
		handle_fun_set_sub_param();
		break;
	case OP_FUN_SET_UNPACK_PARAM:
		handle_fun_set_unpack_param();
		break;
	case OP_FUN_END:
		handle_fun_end();
		break;
	case OP_CALL:
		handle_call();
		break;
	case OP_RETURN:
		handle_return();
		break;

		// conditional operations
	case OP_TRY:
		try_stack.push(current_instruction.operand.get_size_operand());
		break;
	case OP_TRY_END:
		try_stack.pop();
		break;
	case OP_THROW:
		handle_throw();
		break;
	case OP_PUSH_ERROR_DESC:
		push_new_constant(new RuntimeValue(catch_err_stack.top().second));
		break;
	case OP_PUSH_ERROR_CODE:
		push_new_constant(new RuntimeValue(catch_err_stack.top().first));
		break;
	case OP_POP_ERROR:
		catch_err_stack.pop();
		break;
	case OP_GET_ITERATOR:
		handle_get_iterator();
		break;
	case OP_HAS_NEXT_ELEMENT:
		handle_has_next_element();
		break;
	case OP_NEXT_ELEMENT:
		handle_next_element();
		break;
	case OP_JUMP:
		next_pc = current_instruction.operand.get_size_operand();
		break;
	case OP_JUMP_IF_FALSE:
		if (!get_evaluation_stack_top()->get_b()) {
			next_pc = current_instruction.operand.get_size_operand();
		}
		break;
	case OP_JUMP_IF_TRUE:
		if (get_evaluation_stack_top()->get_b()) {
			next_pc = current_instruction.operand.get_size_operand();
		}
		break;

		// expression operations
		break;
	case OP_IS_STRUCT:
		push_new_constant(
			new RuntimeValue(
				flx_bool(
					get_evaluation_stack_top()->is_struct()
				)
			)
		);
		break;
	case OP_IS_ARRAY: {
		auto value_stack_top = get_evaluation_stack_top();
		push_new_constant(
			new RuntimeValue(
				flx_bool(
					value_stack_top->is_array()
				)
			)
		);
		break;
	}
	case OP_IS_ANY: {
		auto value_stack_top = get_evaluation_stack_top();
		push_new_constant(
			new RuntimeValue(
				flx_bool(
					value_stack_top->ref.lock() && value_stack_top->ref.lock()->is_any()
				)
			)
		);
		break;
	}
	case OP_REFID:
		push_new_constant(new RuntimeValue(flx_int(get_evaluation_stack_top())));
		break;
	case OP_TYPEID:
		push_new_constant(
			new RuntimeValue(
				flx_int(
					utils::StringUtils::hashcode(TypeDefinition::buid_type_str(*get_evaluation_stack_top()))
				)
			)
		);
		break;
	case OP_TYPEOF:
		push_new_constant(
			new RuntimeValue(
				flx_string(
					TypeDefinition::buid_type_str(*get_evaluation_stack_top())
				)
			)
		);
		break;
	case OP_TYPE_PARSE:
		handle_type_parse();
		break;
	case OP_IN:
		binary_operation("in");
		break;
	case OP_OR:
		binary_operation("or");
		break;
	case OP_AND:
		binary_operation("and");
		break;
	case OP_BIT_OR:
		binary_operation("|");
		break;
	case OP_BIT_XOR:
		binary_operation("^");
		break;
	case OP_BIT_AND:
		binary_operation("&");
		break;
	case OP_EQL:
		binary_operation("==");
		break;
	case OP_DIF:
		binary_operation("!=");
		break;
	case OP_LT:
		binary_operation("<");
		break;
	case OP_LTE:
		binary_operation("<=");
		break;
	case OP_GT:
		binary_operation(">");
		break;
	case OP_GTE:
		binary_operation(">=");
		break;
	case OP_SPACE_SHIP:
		binary_operation("<=>");
		break;
	case OP_LEFT_SHIFT:
		binary_operation("<<");
		break;
	case OP_RIGHT_SHIFT:
		binary_operation(">>");
		break;
	case OP_ADD:
		binary_operation("+");
		break;
	case OP_SUB:
		binary_operation("-");
		break;
	case OP_MUL:
		binary_operation("*");
		break;
	case OP_DIV:
		binary_operation("/");
		break;
	case OP_REMAINDER:
		binary_operation("%");
		break;
	case OP_FLOOR_DIV:
		binary_operation("/%");
		break;
	case OP_UNARY_SUB:
		unary_operation("-");
		break;
	case OP_NOT:
		unary_operation("not");
		break;
	case OP_BIT_NOT:
		unary_operation("~");
		break;
	case OP_EXP:
		binary_operation("**");
		break;
	case OP_INC:
		unary_operation("++");
		break;
	case OP_DEC:
		unary_operation("--");
		break;
	case OP_ASSIGN:
		binary_operation("=");
		break;
	case OP_ADD_ASSIGN:
		binary_operation("+=");
		break;
	case OP_SUB_ASSIGN:
		binary_operation("-=");
		break;
	case OP_MUL_ASSIGN:
		binary_operation("*=");
		break;
	case OP_DIV_ASSIGN:
		binary_operation("/=");
		break;
	case OP_REMAINDER_ASSIGN:
		binary_operation("%=");
		break;
	case OP_FLOOR_DIV_ASSIGN:
		binary_operation("/%=");
		break;
	case OP_EXP_ASSIGN:
		binary_operation("**=");
		break;
	case OP_BIT_OR_ASSIGN:
		binary_operation("|=");
		break;
	case OP_BIT_XOR_ASSIGN:
		binary_operation("^=");
		break;
	case OP_BIT_AND_ASSIGN:
		binary_operation("&=");
		break;
	case OP_LEFT_SHIFT_ASSIGN:
		binary_operation("<<=");
		break;
	case OP_RIGHT_SHIFT_ASSIGN:
		binary_operation(">>=");
		break;

	case OP_SKIP:
		// skip operation, do nothing
		break;
	case OP_HALT:
		next_pc = instructions.size();
		break;
	case OP_TRAP:
		return_from_sub_run = true;
		break;

	case OP_ERROR:
		throw std::runtime_error("Operation error");

	case OP_SIZE:
		throw std::runtime_error("Invalid operation");

	default:
		throw std::runtime_error("Unknow operation");
	}
}

RuntimeValue* VirtualMachine::get_evaluation_stack_top() {
	auto value = evaluation_stack->back();
	pop_constant();
	return value;

}

bool VirtualMachine::get_next() {
	previous_pc = current_pc;
	current_pc = next_pc;
	if (next_pc >= instructions.size()) {
		return false;
	}
	current_instruction = instructions[next_pc++];
	return true;
}

void VirtualMachine::cleanup_type_set() {
	set_array_dim = std::vector<size_t>();
}

RuntimeValue* VirtualMachine::allocate_value(RuntimeValue* value) {
	return dynamic_cast<RuntimeValue*>(gc.allocate(value));
}

void VirtualMachine::push_empty_constant(Type type) {
	push_new_constant(new RuntimeValue(type));
}

void VirtualMachine::push_new_constant(RuntimeValue* value) {
	push_constant(dynamic_cast<RuntimeValue*>(allocate_value(value)));
}

void VirtualMachine::push_constant(RuntimeValue* value) {
	evaluation_stack->push_back(value);
	if (!evaluation_unwind_stack.empty()) {
		evaluation_unwind_stack.top()++;
	}
}

void VirtualMachine::pop_constant() {
	evaluation_stack->pop_back();
	if (!evaluation_unwind_stack.empty()) {
		evaluation_unwind_stack.top()--;
	}
}

void VirtualMachine::binary_operation(const std::string& op) {
	RuntimeValue* rval = get_evaluation_stack_top();
	RuntimeValue* lval = get_evaluation_stack_top();

	auto res = RuntimeOperations::do_operation(op, lval, rval);

	if (res != lval && res != rval) {
		push_new_constant(res);
	}
	else {
		push_constant(res);
	}

}

void VirtualMachine::unary_operation(const std::string& op) {
	RuntimeValue* value = get_evaluation_stack_top();

	switch (value->type) {
	case Type::T_INT:
		if (op == "-") {
			push_new_constant(new RuntimeValue(flx_int(-value->get_i())));
		}
		else if (op == "~") {
			push_new_constant(new RuntimeValue(flx_int(~value->get_i())));
		}
		else if (op == "++") {
			value->set(flx_int(value->get_i() + 1));
			push_constant(value);
		}
		else if (op == "--") {
			value->set(flx_int(value->get_i() - 1));
			push_constant(value);
		}
		else {
			throw std::runtime_error("incompatible unary operator '" + op +
				"' in front of " + TypeDefinition::type_str(value->type) + " expression");
		}
		break;
	case Type::T_FLOAT:
		if (op == "-") {
			push_new_constant(new RuntimeValue(flx_float(-value->get_f())));
		}
		else if (op == "++") {
			value->set(flx_float(value->get_f() + 1.0));
			push_constant(value);
		}
		else if (op == "--") {
			value->set(flx_float(value->get_f() - 1.0));
			push_constant(value);
		}
		else {
			throw std::runtime_error("incompatible unary operator '" + op +
				"' in front of " + TypeDefinition::type_str(value->type) + " expression");
		}
		break;
	case Type::T_BOOL:
		if (op == "not") {
			push_new_constant(new RuntimeValue(flx_bool(!value->get_b())));
		}
		else {
			throw std::runtime_error("incompatible unary operator '" + op +
				"' in front of " + TypeDefinition::type_str(value->type) + " expression");
		}
		break;
	default:
		throw std::runtime_error("incompatible unary operator '" + op +
			"' in front of " + TypeDefinition::type_str(value->type) + " expression");
	}
}

void VirtualMachine::handle_call() {
	auto params = current_instruction.operand.get_vector_operand();
	auto module_name_space = params.at(0).get_string_operand();
	auto module_name = params.at(1).get_string_operand();
	auto name_space = params.at(2).get_string_operand();
	auto identifier = params.at(3).get_string_operand();
	auto param_count = params.at(4).get_size_operand();
	auto call_identifier = identifier;
	auto as_identifier = std::string();
	auto curr_pc = next_pc;
	auto dbg_info = get_debug_info(curr_pc);
	auto curr_row = dbg_info.row;
	auto curr_col = dbg_info.col;
	RuntimeValue* returned_expression_value = nullptr;

	if (identifier == "gc_collect") {
		int x = 0;
	}

	// if identifier is empty, it means that the call is a lambda function
	if (identifier.empty()) {
		returned_expression_value = get_evaluation_stack_top();
	}

	bool strict = true;
	std::vector<std::shared_ptr<TypeDefinition>> signature;
	std::shared_ptr<std::vector<RuntimeValue*>> function_arguments = std::make_shared<std::vector<RuntimeValue*>>();

	gc.add_root_container(function_arguments);

	while (param_count-- > 0) {
		std::shared_ptr<RuntimeValue> value(get_evaluation_stack_top(), [](RuntimeValue*) {});
		signature.insert(signature.begin(), value);
		function_arguments->insert(function_arguments->begin(), value.get());
	}

	std::shared_ptr<Scope> func_scope;

	// handle function return
	if (identifier.empty()) {
		if (!returned_expression_value) {
			ExceptionHelper::throw_undeclared_function("lambda", signature);
		}

		name_space = returned_expression_value->get_fun().first;
		identifier = returned_expression_value->get_fun().second;

		func_scope = find_declared_function_strict(module_name_space, module_name, name_space, identifier, signature, strict);

		if (!func_scope) {
			ExceptionHelper::throw_undeclared_function(identifier, signature);
		}

	}
	// handle regular call
	else {
		// check if is a common declared function
		func_scope = find_declared_function_strict(module_name_space, module_name, name_space, identifier, signature, strict);

		if (!func_scope) {
			auto var_scope = get_inner_most_variable_scope(module_name_space, module_name, name_space, identifier);

			// if there's no variable
			if (!var_scope) {

				if (auto obj_scope = get_inner_most_class_definition_scope(module_name_space, module_name, name_space, identifier)) {
					auto obj_def = obj_scope->find_declared_class_definition(identifier);

					auto obj_value = new RuntimeValue(flx_class(module_name_space, obj_def->identifier), module_name_space, obj_def->identifier);

					obj_value->get_raw_cls()->function_symbol_table = obj_def->functions_scope->function_symbol_table;

					for (const auto& var_def : obj_def->variables) {
						auto var = std::make_shared<RuntimeVariable>(var_def.first, var_def.second);

						if (var_def.second.get_pc_default() > 0) {
							auto current_pc = next_pc;
							next_pc = var_def.second.get_pc_default();
							run();
							next_pc = current_pc;
							var->set_value(get_evaluation_stack_top());
						}
						else {
							var->set_value(allocate_value(new RuntimeValue(Type::T_UNDEFINED)));
						}

						gc.add_var_root(var);

						obj_value->get_raw_cls()->declare_variable(var_def.first, var);
					}

					std::shared_ptr<flx_class> obj_as_scope = obj_value->get_raw_cls();

					class_stack.push(obj_as_scope);
					push_vm_scope(obj_as_scope);

					std::shared_ptr<FunctionDefinition> cls_const;
					try {
						cls_const = obj_value->get_raw_cls()->find_declared_function("init", &signature, true);
					}
					catch (...) {
						cls_const = obj_value->get_raw_cls()->find_declared_function("init", &signature, false);
					}

					push_vm_scope(std::make_shared<Scope>(obj_as_scope->module_name_space, obj_as_scope->module_name));

					declare_function_block_parameters(obj_as_scope->module_name_space, obj_as_scope->module_name, module_name, cls_const->parameters, signature);

					vm_debug.debug_info_table[curr_pc] = std::vector<Operand>{
						vm_debug.debug_info_table[curr_pc][0],
						vm_debug.debug_info_table[curr_pc][1],
						Operand(size_t(0)),
						Operand(vm_debug.index_of_namespace(obj_as_scope->module_name_space)),
						Operand(identifier),
						Operand(curr_row),
						Operand(curr_col)
					};
					call_stack.push(curr_pc);

					return_namespace.push(std::make_pair(obj_as_scope->module_name_space, obj_as_scope->module_name));
					return_stack.push(next_pc);
					return_unwind_stack.push(0);
					push_deep();

					next_pc = cls_const->pointer;
					run();

					// after return from constructor
					pop_constant(); // pop the constructor undefined return
					class_stack.pop();
					pop_vm_scope(obj_as_scope->module_name_space, obj_as_scope->module_name);
					
					push_new_constant(obj_value);

					gc.remove_root_container(function_arguments);

					return;
				}

				ExceptionHelper::throw_undeclared_function(identifier, signature);
			}

			// gets variable value
			auto var = std::dynamic_pointer_cast<RuntimeVariable>(var_scope->find_declared_variable(identifier));
			auto var_value = var->get_value();

			// if variable is not a function type, throw error
			if (!var_value->is_function()) {
				ExceptionHelper::throw_undeclared_function(identifier, signature);
			}

			// get function namespace and name
			name_space = var_value->get_fun().first;
			identifier = var_value->get_fun().second;

			as_identifier = identifier;

			// set strict to true again to try find from variable
			strict = true;

			func_scope = find_declared_function_strict(module_name_space, module_name, name_space, identifier, signature, strict);

			if (!func_scope) {
				ExceptionHelper::throw_undeclared_function(identifier, signature);
			}

		}

	}

	auto& declfun = func_scope->find_declared_function(identifier, &signature, strict);

	push_vm_scope(std::make_shared<Scope>(func_scope->module_name_space, func_scope->module_name));

	declare_function_block_parameters(func_scope->module_name_space, func_scope->module_name, module_name, declfun->parameters, signature);

	if (call_identifier.starts_with("lambda@")) {
		call_identifier = "<lambda>";
	}
	if (as_identifier.starts_with("lambda@")) {
		as_identifier = "<lambda>";
	}
	auto stack_identifier = call_identifier + (as_identifier.empty() ? "" : " as " + as_identifier);
	vm_debug.debug_info_table[curr_pc] = std::vector<Operand>{
		vm_debug.debug_info_table[curr_pc][0],
		vm_debug.debug_info_table[curr_pc][1],
		Operand(size_t(0)),
		Operand(vm_debug.index_of_namespace(func_scope->module_name_space)),
		Operand(stack_identifier),
		Operand(curr_row),
		Operand(curr_col)
	};
	call_stack.push(curr_pc);

	if (declfun->pointer) {
		return_namespace.push(std::make_pair(func_scope->module_name_space, func_scope->module_name));
		return_stack.push(next_pc);
		return_unwind_stack.push(0);

		push_deep();

		next_pc = declfun->pointer;
	}
	else {
		builtin_functions[identifier]();

		pop_vm_scope(func_scope->module_name_space, func_scope->module_name);
		call_stack.pop();
	}

	gc.remove_root_container(function_arguments);

}

void VirtualMachine::handle_return() {
	return_from_sub_run = current_instruction.operand.get_bool_operand();

	next_pc = return_stack.top();
	return_stack.pop();

	// store return value to perform unwind
	auto return_value = get_evaluation_stack_top();
	gc.add_root(return_value);

	auto total_pop = return_unwind_stack.top();
	for (size_t i = 0; i < total_pop; ++i) {
		pop_deep();
	}
	return_unwind_stack.pop();

	// push back return value
	push_constant(return_value);
	gc.remove_root(return_value);

	const auto& scope_data = return_namespace.top();
	pop_vm_scope(scope_data.first, scope_data.second);
	return_namespace.pop();

	if (!generated_error) {
		call_stack.pop();
	}
}

void VirtualMachine::declare_function_block_parameters(
	const std::string& func_name_space, const std::string& func_module_name, const std::string& module_name,
	std::vector<std::shared_ptr<TypeDefinition>> current_function_defined_parameters,
	std::vector<std::shared_ptr<TypeDefinition>> current_function_calling_arguments) {
	auto rest_name = std::string();
	auto vec = std::vector<RuntimeValue*>();
	size_t i = 0;

	// adds function arguments
	for (i = 0; i < current_function_calling_arguments.size(); ++i) {
		RuntimeValue* current_value = allocate_value(new RuntimeValue(std::dynamic_pointer_cast<RuntimeValue>(current_function_calling_arguments[i]).get()));

		if (current_function_defined_parameters.size() > i) {
			current_value = RuntimeOperations::normalize_type(current_function_defined_parameters[i], current_value, true);
		}

		if (i >= current_function_defined_parameters.size()) {
			vec.push_back(current_value);
		}
		else {
			if (const auto decl = std::dynamic_pointer_cast<VariableDefinition>(current_function_defined_parameters[i])) {
				// is rest
				if (decl->is_rest) {
					rest_name = decl->identifier;
					// if it's the last defined parameter
					// and it's the last calling parameter
					// and the current passed parameter is an array
					if (current_function_defined_parameters.size() - 1 == i
						&& current_function_calling_arguments.size() - 1 == i
						&& current_value->is_array()) {
						for (size_t i = 0; i < current_value->get_arr().size(); ++i) {
							vec.push_back(current_value->get_arr()[i]);
						}
					}
					else {
						vec.push_back(current_value);
					}
				}
				else {
					declare_function_parameter(func_name_space, func_module_name, module_name, decl->identifier, *decl, current_value);
				}
			}
			else if (const auto decls = std::dynamic_pointer_cast<UnpackedVariableDefinition>(current_function_defined_parameters[i])) {
				for (auto& decl : decls->variables) {
					auto original_sub_value = current_value->get_str()[decl.identifier]->get_value();
					auto sub_value = allocate_value(new RuntimeValue(original_sub_value));
					declare_function_parameter(func_name_space, func_module_name, module_name, decl.identifier, decl, sub_value);
				}
			}
		}
	}

	// adds default values
	for (; i < current_function_defined_parameters.size(); ++i) {
		if (const auto decl = std::dynamic_pointer_cast<VariableDefinition>(current_function_defined_parameters[i])) {
			if (decl->is_rest) {
				break;
			}

			auto current_pc = next_pc;
			next_pc = decl->get_pc_default();
			run();
			next_pc = current_pc;
			auto current_value = get_evaluation_stack_top();

			declare_function_parameter(func_name_space, func_module_name, module_name, decl->identifier, *decl, current_value);
		}
	}

	if (vec.size() > 0) {
		auto arr = flx_array(vec.size());
		for (size_t i = 0; i < vec.size(); ++i) {
			arr[i] = vec[i];
		}
		auto rest = allocate_value(new RuntimeValue(arr, Type::T_ANY, std::vector<size_t>{(size_t)arr.size()}));
		auto var = std::make_shared<RuntimeVariable>(rest_name, *current_function_defined_parameters.back());
		var->set_value(rest);
		gc.add_var_root(var);
		get_back_scope(func_name_space)->declare_variable(rest_name, var);
	}

}

void VirtualMachine::declare_function_parameter(
	const std::string& func_name_space, const std::string& func_module_name, const std::string& module_name,
	const std::string& identifier, TypeDefinition variable, RuntimeValue* value) {
	auto curr_scope = get_back_scope(func_name_space);

	auto var = std::make_shared<RuntimeVariable>(identifier, variable);
	var->set_value(value);
	gc.add_var_root(var);
	curr_scope->declare_variable(identifier, var);
}

void VirtualMachine::handle_throw() {
	auto value = get_evaluation_stack_top();

	if (value->is_struct()
		&& value->type_name_space == Constants::DEFAULT_NAMESPACE
		&& value->type_name == Constants::BUILTIN_STRUCT_NAMES[BuiltinStructs::BS_EXCEPTION]) {
		auto var_value = value->get_str()[Constants::STR_EXCEPTION_FIELD_NAMES[StrExceptionFields::SXF_ERROR]]->get_value();
		throw std::runtime_error(var_value->get_s().c_str());
	}
	else if (value->is_string()) {
		throw std::runtime_error(value->get_s());
	}
	else {
		throw std::runtime_error(
			"expected "
			+ TypeDefinition::buid_struct_type_name(Constants::DEFAULT_NAMESPACE,
				Constants::BUILTIN_STRUCT_NAMES[BuiltinStructs::BS_EXCEPTION])
			+ " or string in throw"
		);
	}

}

void VirtualMachine::handle_get_iterator() {
	auto value_stack_top = get_evaluation_stack_top();
	gc.add_root(value_stack_top);
	iterator_stack.push(RuntimeValueIterator{ value_stack_top });
}

void VirtualMachine::handle_has_next_element() {
	if (iterator_stack.empty()) {
		throw std::runtime_error("no iterator on stack");
	}
	bool has_next = false;

	auto& it_value = iterator_stack.top();
	auto value = it_value.value;
	auto& index = it_value.index;

	if (value->is_array()) {
		if (index < value->get_arr().size()) {
			has_next = true;
		}

	}
	else if (value->is_string()) {
		if (index < value->get_s().size()) {
			has_next = true;
		}

	}
	else if (value->is_struct()) {
		if (index < value->get_str().size()) {
			has_next = true;
		}

	}
	else {
		throw std::runtime_error("invalid iterable type");

	}

	push_new_constant(new RuntimeValue(flx_bool(has_next)));

}

void VirtualMachine::handle_next_element() {
	if (iterator_stack.empty()) {
		throw std::runtime_error("no iterator on stack");
	}

	auto& it_value = iterator_stack.top();
	auto value = it_value.value;
	auto& index = it_value.index;

	if (value->is_array()) {
		if (index >= value->get_arr().size()) {
			gc.remove_root(value);
			iterator_stack.pop();
			return;
		}
		auto arr_value = value->get_item(index++);
		push_constant(arr_value);
		
	}
	else if (value->is_string()) {
		if (index >= value->get_s().size()) {
			gc.remove_root(value);
			iterator_stack.pop();
			return;
		}
		auto ch_value = value->get_s()[index++];
		push_new_constant(new RuntimeValue(flx_char(ch_value)));

	}
	else if (value->is_struct()) {
		if (index >= value->get_str().size()) {
			gc.remove_root(value);
			iterator_stack.pop();
			return;
		}

		auto it = value->get_raw_str()->begin();
		std::advance(it, index++);

		auto str_value = it->second;

		auto key_var = std::make_shared<RuntimeVariable>("key", Type::T_STRING);
		key_var->set_value(allocate_value(new RuntimeValue(it->first)));
		gc.add_var_root(key_var);

		push_new_constant(new RuntimeValue(
			flx_struct{
				{"key", key_var},
				{"value", str_value}
			},
			Constants::DEFAULT_NAMESPACE, Constants::BUILTIN_STRUCT_NAMES[BuiltinStructs::BS_ENTRY]
		));

	}
	else {
		throw std::runtime_error("invalid iterable type");

	}

}

void VirtualMachine::handle_type_parse() {
	Type type = Type(current_instruction.operand.get_uint8_operand());
	auto value = get_evaluation_stack_top();
	RuntimeValue* new_value = new RuntimeValue();

	switch (type) {
	case Type::T_BOOL:
		switch (value->type) {
		case Type::T_BOOL:
			new_value->copy_from(value);
			break;
		case Type::T_INT:
			new_value->set(flx_bool(value->get_i() != 0));
			break;
		case Type::T_FLOAT:
			new_value->set(flx_bool(value->get_f() != .0));
			break;
		case Type::T_CHAR:
			new_value->set(flx_bool(value->get_c() != '\0'));
			break;
		case Type::T_STRING:
			new_value->set(flx_bool(!value->get_s().empty()));
			break;
		}
		break;

	case Type::T_INT:
		switch (value->type) {
		case Type::T_BOOL:
			new_value->set(flx_int(value->get_b()));
			break;
		case Type::T_INT:
			new_value->copy_from(value);
			break;
		case Type::T_FLOAT:
			new_value->set(flx_int(value->get_f()));
			break;
		case Type::T_CHAR:
			new_value->set(flx_int(value->get_c()));
			break;
		case Type::T_STRING:
			try {
				new_value->set(flx_int(std::stoll(value->get_s())));
			}
			catch (...) {
				throw std::runtime_error("'" + value->get_s() + "' is not a valid value to parse int");
			}
			break;
		}
		break;

	case Type::T_FLOAT:
		switch (value->type) {
		case Type::T_BOOL:
			new_value->set(flx_float(value->get_b()));
			break;
		case Type::T_INT:
			new_value->set(flx_float(value->get_i()));
			break;
		case Type::T_FLOAT:
			new_value->copy_from(value);
			break;
		case Type::T_CHAR:
			new_value->set(flx_float(value->get_c()));
			break;
		case Type::T_STRING:
			try {
				new_value->set(flx_float(std::stold(value->get_s())));
			}
			catch (...) {
				throw std::runtime_error("'" + value->get_s() + "' is not a valid value to parse float");
			}
			break;
		}
		break;

	case Type::T_CHAR:
		switch (value->type) {
		case Type::T_BOOL:
			new_value->set(flx_char(value->get_b()));
			break;
		case Type::T_INT:
			new_value->set(flx_char(value->get_i()));
			break;
		case Type::T_FLOAT:
			new_value->set(flx_char(value->get_f()));
			break;
		case Type::T_CHAR:
			new_value->copy_from(value);
			break;
		case Type::T_STRING:
			if (new_value->get_s().size() > 1) {
				throw std::runtime_error("'" + value->get_s() + "' is not a valid value to parse char");
			}
			else {
				new_value->set(flx_char(value->get_s()[0]));
			}
			break;
		}
		break;

	case Type::T_STRING:
		new_value->set(flx_string(RuntimeOperations::parse_value_to_string(value, true)));

	}

	new_value->type = type;

	push_new_constant(new_value);
}

void VirtualMachine::check_build_array(RuntimeValue* new_value, std::vector<size_t> dim) {
	// if the new value is not an array or the first dimension is 0, do nothing
	if (!new_value->is_array() || dim.size() == 0 || dim[0] == 0) {
		return;
	}

	auto arr = new_value->get_arr();
	auto arrsize = arr.size();

	// if the new value is an array with more than one element, do nothing
	if (arrsize > 1) {
		return;
	}

	if (dim.size() > 1) { // multi dimension arrays case
		auto val = arrsize == 1 ? arr[0] : allocate_value(new RuntimeValue(Type::T_VOID));

		flx_array rarr = build_array(dim, val, dim.size() - 1);

		new_value->set(
			rarr,
			current_expression_array_type.is_void() ? Type::T_ANY : current_expression_array_type.type,
			dim,
			current_expression_array_type.type_name_space,
			current_expression_array_type.type_name
		);

	}
	else if (dim.size() == 1) { // one dimension arrays case
		switch (arrsize)
		{
		case 0: { // no initialization value, all nulls
			flx_array rarr = flx_array(dim[0]);

			new_value->set(rarr, new_value->is_void() ? Type::T_ANY : new_value->type, dim);

			break;
		}
		case 1: { // with initialization value, fill with it
			auto val = arr[0];

			flx_array rarr = build_array(dim, val, 0);

			new_value->set(
				rarr,
				current_expression_array_type.is_void() ? Type::T_ANY : current_expression_array_type.type,
				dim,
				current_expression_array_type.type_name_space,
				current_expression_array_type.type_name
			);

			break;
		}
		}

	}

}

flx_array VirtualMachine::build_array(const std::vector<size_t>& dim, RuntimeValue* init_value, intmax_t i) {
	flx_array raw_arr;

	if (dim.size() - 1 == i) {
		current_expression_array_type = TypeDefinition();
	}

	size_t size = 0;
	if (dim.size() == 0) {
		size = 1;
	}
	else {
		size = dim[i];
	}

	raw_arr = flx_array(size);

	for (size_t j = 0; j < size; ++j) {
		RuntimeValue* val;

		if (i > 0) {
			// for superior dimentions, its creates a new array for each element
			std::vector<size_t> sub_dims(dim.begin(), dim.begin() + i);
			val = allocate_value(new RuntimeValue(init_value)); // copy the initial value
			raw_arr[j] = allocate_value(new RuntimeValue(build_array(sub_dims, val, i - 1),
				val->type, val->dim, val->type_name_space, val->type_name));
		}
		else {
			// for the last dimension, we create independent values
			val = allocate_value(new RuntimeValue(init_value)); // create a independent copy
			raw_arr[j] = val;
		}

		if (current_expression_array_type.is_undefined() || current_expression_array_type.is_array()) {
			current_expression_array_type = *val;
		}
	}

	return raw_arr;
}

void VirtualMachine::handle_store_var() {
	auto params = current_instruction.operand.get_vector_operand();
	auto name_space = params.at(0).get_string_operand();
	auto identifier = params.at(1).get_string_operand();

	RuntimeValue* new_value = get_evaluation_stack_top();

	TypeDefinition var_type_def = get_type_def();

	if (set_check_build_array) {
		set_check_build_array = false;
		check_build_array(new_value, var_type_def.dim);
	}

	auto new_var = std::make_shared<RuntimeVariable>(identifier, var_type_def);

	new_value = RuntimeOperations::normalize_type(new_var, new_value, true);

	new_var->set_value(new_value);
	gc.add_var_root(new_var);

	if (!new_var->is_any_or_match_type_def(*new_value) && !new_value->is_undefined()) {
		ExceptionHelper::throw_declaration_type_err(identifier, *new_var, *new_value);
	}

	get_back_scope(name_space)->declare_variable(identifier, new_var);

}

void VirtualMachine::handle_include_namespace() {
	auto params = current_instruction.operand.get_vector_operand();
	auto module_name = params.at(0).get_string_operand();
	auto name_space = params.at(1).get_string_operand();

	module_included_name_spaces[module_name].push_back(name_space);
}

void VirtualMachine::handle_exclude_namespace() {
	auto params = current_instruction.operand.get_vector_operand();
	auto module_name = params.at(0).get_string_operand();
	auto name_space = params.at(1).get_string_operand();

	size_t pos = std::distance(module_included_name_spaces[module_name].begin(),
		std::find(module_included_name_spaces[module_name].begin(),
			module_included_name_spaces[module_name].end(), name_space));

	module_included_name_spaces[module_name].erase(module_included_name_spaces[module_name].begin() + pos);
}

void VirtualMachine::handle_init_array() {
	auto size = current_instruction.operand.get_size_operand();
	auto type_def = get_type_def();
	value_build_stack.push(new RuntimeValue(flx_array(size), type_def.type, type_def.dim, type_def.type_name_space, type_def.type_name));
}

void VirtualMachine::handle_set_element() {
	RuntimeValue* value = get_evaluation_stack_top();
	RuntimeValue* arr_value = value_build_stack.top();
	arr_value->set_item(current_instruction.operand.get_size_operand(), value);
}

void VirtualMachine::handle_push_array() {
	push_new_constant(value_build_stack.top());
	value_build_stack.pop();
}

void VirtualMachine::handle_init_struct() {
	auto params = current_instruction.operand.get_vector_operand();
	auto module_name_space = params.at(0).get_string_operand();
	auto module_name = params.at(1).get_string_operand();
	auto name_space = params.at(2).get_string_operand();
	auto identifier = params.at(3).get_string_operand();

	auto type_struct = find_inner_most_struct(module_name_space, module_name, name_space, identifier);

	auto str_build = new RuntimeValue(flx_struct(), name_space, identifier);

	for (auto& struct_var_def : type_struct->variables) {
		RuntimeValue* str_sub_value = allocate_value(new RuntimeValue(Type::T_VOID));
		auto sub_value_var = std::make_shared<RuntimeVariable>(struct_var_def.first, *struct_var_def.second);
		sub_value_var->set_value(str_sub_value);
		gc.add_var_root(sub_value_var);
		str_build->get_raw_str()->emplace(struct_var_def.first, sub_value_var);
	}

	value_build_stack.push(str_build);
}

void VirtualMachine::handle_set_field() {
	RuntimeValue* value = get_evaluation_stack_top();
	auto params = current_instruction.operand.get_vector_operand();
	auto module_name_space = params.at(0).get_string_operand();
	auto module_name = params.at(1).get_string_operand();
	auto identifier = params.at(2).get_string_operand();

	auto str_build = value_build_stack.top();
	const auto& str_field_var = str_build->get_raw_str()->at(identifier);

	if (!str_field_var->is_any_or_match_type_def(*value)) {
		ExceptionHelper::throw_struct_value_assign_type_err(str_build->type_name_space, str_build->type_name,
			identifier, *str_field_var, *value);
	}

	value = RuntimeOperations::normalize_type(str_field_var, value, true);

	if (!str_field_var->is_any() && !value->is_void()) {
		value->type = str_field_var->type;
		value->type_name = str_field_var->type_name;
		value->type_name_space = str_field_var->type_name_space;
	}

	str_build->set_field(identifier, value);

}

void VirtualMachine::handle_push_struct() {
	auto str_build = value_build_stack.top();
	value_build_stack.pop();
	push_new_constant(str_build);
}

void VirtualMachine::handle_struct_start() {
	struct_def_build_stack.push(std::make_shared<StructDefinition>(current_instruction.operand.get_string_operand()));
}

void VirtualMachine::handle_struct_set_var() {
	auto var_id = current_instruction.operand.get_string_operand();

	auto var = std::make_shared<VariableDefinition>(var_id, get_type_def(), 0);

	struct_def_build_stack.top()->variables[var_id] = var;

}

void VirtualMachine::handle_struct_end() {
	auto name_space = current_instruction.operand.get_string_operand();
	auto str = struct_def_build_stack.top();
	struct_def_build_stack.pop();
	get_back_scope(name_space)->declare_struct_definition(str);
}

void VirtualMachine::handle_class_start() {
	auto params = current_instruction.operand.get_vector_operand();
	auto module_name_space = params.at(0).get_string_operand();
	auto module_name = params.at(1).get_string_operand();
	auto identifier = params.at(2).get_string_operand();

	auto cls = std::make_shared<ClassDefinition>(identifier);

	class_def_build_stack.push(cls);

	push_scope(std::make_shared<Scope>(module_name_space, module_name));

	cls->functions_scope = get_back_scope(module_name_space);

}

void VirtualMachine::handle_class_set_var() {
	auto var_id = current_instruction.operand.get_string_operand();
	auto var = VariableDefinition(var_id, get_type_def(), set_default_value_pc, false);
	set_default_value_pc = 0;
	class_def_build_stack.top()->variables[var_id] = var;

}

void VirtualMachine::handle_class_end() {
	auto params = current_instruction.operand.get_vector_operand();
	auto module_name_space = params.at(0).get_string_operand();
	auto module_name = params.at(1).get_string_operand();
	auto cls = class_def_build_stack.top();
	class_def_build_stack.pop();

	pop_scope(module_name_space, module_name);

	get_back_scope(module_name_space)->declare_class_definition(cls);

}

void VirtualMachine::handle_push_type_def() {
	auto param = current_instruction.operand.get_vector_operand();
	push_type_def(TypeDefinition(
		static_cast<Type>(param[0].get_uint8_operand()),
		set_array_dim,
		param[1].get_string_operand(),
		param[2].get_string_operand()
	));
}

bool VirtualMachine::get_use_variable_ref() {
	if (use_variable_ref.empty()) {
		return false;
	}

	return use_variable_ref.top();
}

void VirtualMachine::handle_load_var() {
	auto params = current_instruction.operand.get_vector_operand();
	auto module_name_space = params.at(0).get_string_operand();
	auto module_name = params.at(1).get_string_operand();
	auto name_space = params.at(2).get_string_operand();
	auto identifier = params.at(3).get_string_operand();

	// handle class self identifier invoke
	if (is_self_invoke) {
		is_self_invoke = false;
		auto variable = std::dynamic_pointer_cast<RuntimeVariable>(class_stack.top()->find_declared_variable(identifier));
		auto value = variable->get_value(get_use_variable_ref());
		push_constant(value);
	}
	// handle regular identifier
	else if (const auto& id_scope = get_inner_most_variable_scope(module_name_space, module_name, name_space, identifier)) {
		auto variable = std::dynamic_pointer_cast<RuntimeVariable>(id_scope->find_declared_variable(identifier));
		auto value = variable->get_value(get_use_variable_ref());
		push_constant(value);
	}
	// handle struct type names for type checks
	else if (get_inner_most_struct_definition_scope(module_name_space, module_name, name_space, identifier)) {
		std::vector<size_t> dim;

		while (next_pc + 1 < instructions.size() && instructions[next_pc + 1].opcode == OP_LOAD_SUB_IX) {
			// execute push constant operation
			get_next();
			decode_operation();

			// loads dimension
			get_next();
			dim.push_back(get_evaluation_stack_top()->get_i());

		}

		if (dim.size() > 0) {
			push_new_constant(new RuntimeValue(flx_array(), Type::T_STRUCT, dim, name_space, identifier));
		}
		else {
			push_new_constant(new RuntimeValue(flx_struct(), name_space, identifier));
		}

	}
	// handle expression function calls
	else if (get_inner_most_function_scope(module_name_space, module_name, name_space, identifier, nullptr)) {
		auto fun = flx_function{ name_space, identifier };
		push_new_constant(new RuntimeValue(fun));

	}
	else {
		throw std::runtime_error("identifier '" + identifier + "' was not declared");

	}

}

void VirtualMachine::handle_load_sub_id() {
	auto id = current_instruction.operand.get_string_operand();
	auto val = get_evaluation_stack_top();

	switch (val->type)
	{
	case Type::T_STRUCT: {
		//auto str_def = find_inner_most_struct("", "", val->type_name_space, val->type_name);
		//const auto& type_def = str_def->variables.at(id);

		auto sub_value = val->get_field(id, get_use_variable_ref());
		//sub_value->type_ref = type_def.get();
		push_constant(sub_value);
		break;
	}
	case Type::T_CLASS: {
		if (instructions[next_pc].opcode == OP_CALL && instructions[next_pc].operand.get_vector_operand()[3].get_string_operand().empty()) {
			auto fopnd = instructions[next_pc].operand.get_vector_operand();
			fopnd[3] = Operand(id);
			instructions[next_pc].operand = Operand(fopnd);

			std::shared_ptr<flx_class> obj_as_scope = val->get_raw_cls();
			class_stack.push(obj_as_scope);
			push_vm_scope(obj_as_scope);

			get_next();

			decode_operation();

			run();

			class_stack.pop();
			pop_vm_scope(obj_as_scope->module_name_space, obj_as_scope->module_name);

		}
		else {
			auto variable = std::dynamic_pointer_cast<RuntimeVariable>(val->get_raw_cls()->find_declared_variable(id));
			auto sub_value = variable->get_value();
			push_constant(sub_value);

		}
		break;
	}
	default:
		throw std::runtime_error("invalid " + TypeDefinition::type_str(val->type) + " access, this operation can only be performed on object values");
	}
}

void VirtualMachine::handle_load_sub_ix() {
	auto i = get_evaluation_stack_top();
	if (!i->is_int()) {
		throw std::runtime_error("Invalid type " + TypeDefinition::type_str(i->type) + " trying to access array");
	}
	auto val = get_evaluation_stack_top();
	if (val->is_array()) {
		auto access_index = i->get_i();
		auto sub_value = val->get_item(access_index, get_use_variable_ref());
		if (!sub_value) {
			val->set_item(access_index, allocate_value(new RuntimeValue(Type::T_VOID)));
			sub_value = val->get_item(access_index, get_use_variable_ref());
		}
		push_constant(sub_value);
	}
	else if (val->is_string()) {
		auto sub_value = val->get_char(i->get_i(), get_use_variable_ref());
		push_constant(sub_value);
	}
	else {
		throw std::runtime_error("Invalid " + TypeDefinition::type_str(val->type) + " index access, this operation can only be performed on array or string values");
	}

}

void VirtualMachine::handle_fun_start() {
	func_def_build_stack.push(std::make_shared<FunctionDefinition>(current_instruction.operand.get_string_operand(), get_type_def()));
}

void VirtualMachine::handle_set_default_value() {
	set_default_value_pc = current_instruction.operand.get_size_operand();
}

std::shared_ptr<VariableDefinition> VirtualMachine::read_param() {
	auto params = current_instruction.operand.get_vector_operand();
	auto is_rest = params.at(0).get_bool_operand();
	auto var_id = params.at(1).get_string_operand();
	auto default_value_pc = set_default_value_pc;
	set_default_value_pc = 0;

	return std::make_shared<VariableDefinition>(var_id, get_type_def(), default_value_pc, is_rest);
}

void VirtualMachine::handle_fun_set_param() {
	func_def_build_stack.top()->parameters.push_back(read_param());
}

void VirtualMachine::handle_fun_start_unpack_param() {
	uvar_def_build_stack.push(std::make_shared<UnpackedVariableDefinition>(get_type_def(), std::vector<VariableDefinition>()));
}

void VirtualMachine::handle_fun_set_sub_param() {
	uvar_def_build_stack.top()->variables.push_back(*read_param());
}

void VirtualMachine::handle_fun_set_unpack_param() {
	func_def_build_stack.top()->parameters.push_back(uvar_def_build_stack.top());
	uvar_def_build_stack.pop();
}

void VirtualMachine::handle_fun_end() {
	auto params = current_instruction.operand.get_vector_operand();
	auto name_space = params.at(0).get_string_operand();
	auto module_name = params.at(1).get_string_operand();
	auto has_block = params.at(2).get_bool_operand();

	auto fun = func_def_build_stack.top();
	func_def_build_stack.pop();

	std::shared_ptr<Scope> scope;
	if (class_def_build_stack.empty()) {
		scope = get_global_scope(module_name);
	}
	else {
		scope = get_back_scope(name_space);
	}

	if (scope->already_declared_function(fun->identifier, &fun->parameters, true)) {
		scope->find_declared_function(fun->identifier, &fun->parameters, true)->pointer = next_pc + 1;
	}
	else {
		fun->pointer = has_block ? next_pc + 1 : 0;
		scope->declare_function(fun->identifier, fun);
	}
}

void VirtualMachine::push_type_def(TypeDefinition type_def) {
	type_def_stack.push(type_def);
	cleanup_type_set();
}

TypeDefinition VirtualMachine::get_type_def() {
	if (type_def_stack.empty()) {
		throw std::runtime_error("No parameters to get");
	}
	auto value = type_def_stack.top();
	type_def_stack.pop();
	return value;
}

TypeDefinition& VirtualMachine::top_type_def() {
	if (type_def_stack.empty()) {
		throw std::runtime_error("No parameters to get top");
	}
	return type_def_stack.top();
}

void VirtualMachine::push_vm_scope(std::shared_ptr<Scope> scope) {
	if (!scope_unwind_stack.empty()) {
		scope_unwind_stack.top().push_back(std::make_pair(scope->module_name_space, scope->module_name));
	}

	push_scope(scope);
}

void VirtualMachine::pop_vm_scope(const std::string& module_name_space, const std::string& module_name) {
	if (!scope_unwind_stack.empty()) {
		scope_unwind_stack.top().pop_back();
	}

	pop_scope(module_name_space, module_name);
	gc.maybe_collect();
}

void VirtualMachine::push_deep() {
	scope_unwind_stack.push(std::vector<std::pair<std::string, std::string>>());
	evaluation_unwind_stack.push(0);

	if (!return_unwind_stack.empty()) {
		return_unwind_stack.top()++;
	}
}

void VirtualMachine::pop_deep() {
	if (!return_unwind_stack.empty()) {
		return_unwind_stack.top()--;
	}

	unwind();

	if (!scope_unwind_stack.empty()) {
		scope_unwind_stack.pop();
	}
	if (!evaluation_unwind_stack.empty()) {
		evaluation_unwind_stack.pop();
	}
}

void VirtualMachine::unwind() {
	unwind_eval_stack();
	unwind_scope();

	gc.maybe_collect();
}

void VirtualMachine::unwind_scope() {
	if (scope_unwind_stack.empty()) {
		return;
	}

	auto total_pop = scope_unwind_stack.top().size();
	for (size_t i = 0; i < total_pop; ++i) {
		auto scope_data = scope_unwind_stack.top().back();
		pop_vm_scope(scope_data.first, scope_data.second);
	}
}
void VirtualMachine::unwind_eval_stack() {
	if (evaluation_unwind_stack.empty()) {
		return;
	}

	// unwind to the try block
	for (size_t i = 0; i < evaluation_unwind_stack.top(); ++i) {
		evaluation_stack->pop_back();
	}
	evaluation_unwind_stack.top() = 0;
}

std::shared_ptr<Scope> VirtualMachine::find_declared_function(const std::string& current_module_name_space, const std::string& current_module_name,
	const std::string& name_space, const std::string& identifier, const std::vector<std::shared_ptr<TypeDefinition>>& signature,
	bool& strict) {

	std::shared_ptr<Scope> func_scope = get_inner_most_function_scope(current_module_name_space, current_module_name, name_space, identifier, &signature, strict);

	return func_scope;

}

std::shared_ptr<Scope> VirtualMachine::find_declared_function_strict(const std::string& current_module_name_space, const std::string& current_module_name,
	const std::string& name_space, const std::string& identifier, const std::vector<std::shared_ptr<TypeDefinition>>& signature,
	bool& strict) {

	std::shared_ptr<Scope> func_scope = find_declared_function(current_module_name_space, current_module_name, name_space, identifier, signature, strict);

	if (!func_scope) {
		strict = false;

		func_scope = find_declared_function(current_module_name_space, current_module_name, name_space, identifier, signature, strict);

	}

	return func_scope;

}

DebugInfo VirtualMachine::get_debug_info(size_t dbg_pc) {
	return DebugInfo(
		vm_debug.get_namespace(vm_debug.debug_info_table[dbg_pc][0].get_int_operand()),
		vm_debug.get_module(vm_debug.debug_info_table[dbg_pc][1].get_int_operand()),
		vm_debug.get_ast_type(vm_debug.debug_info_table[dbg_pc][2].get_int_operand()),
		vm_debug.get_namespace(vm_debug.debug_info_table[dbg_pc][3].get_int_operand()),
		vm_debug.debug_info_table[dbg_pc][4].get_string_operand(),
		vm_debug.debug_info_table[dbg_pc][5].get_int_operand(),
		vm_debug.debug_info_table[dbg_pc][6].get_int_operand()
	);
}
