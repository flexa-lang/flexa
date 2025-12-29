#ifndef VIRTUAL_MACHINE_HPP
#define VIRTUAL_MACHINE_HPP

#include <string>
#include <vector>
#include <stack>
#include <map>
#include <functional>
#include <memory>

#include "scope.hpp"
#include "bytecode.hpp"
#include "types.hpp"
#include "visitor.hpp"
#include "ast.hpp"
#include "debuginfo.hpp"
#include "vm_debug.hpp"
#include "scope_manager.hpp"
#include "gc.hpp"

namespace core {

	namespace runtime {

		struct RuntimeValueIterator {
			RuntimeValue* value;
			size_t index = 0;
		};

		class VirtualMachine : public ScopeManager {
		public:
			std::map<std::string, std::function<void()>> builtin_functions;
			GarbageCollector gc;

			RuntimeValue* allocate_value(RuntimeValue* value);
			void push_new_constant(RuntimeValue* value);
			void push_constant(RuntimeValue* value);
			void push_empty_constant(Type type);
			void pop_constant();
			RuntimeValue* get_evaluation_stack_top();

		private:
			std::shared_ptr<std::vector<RuntimeValue*>> evaluation_stack;
			size_t previous_pc = 0;
			size_t current_pc = 0;
			size_t next_pc = 0;
			std::vector<BytecodeInstruction> instructions;
			BytecodeInstruction current_instruction;

			std::stack<std::vector<std::pair<std::string, std::string>>> scope_unwind_stack;
			std::stack<size_t> return_unwind_stack;
			std::stack<size_t> evaluation_unwind_stack;
			std::stack<std::pair<std::string, std::string>> return_namespace;
			
			std::stack<RuntimeValueIterator> iterator_stack;
			std::stack<std::shared_ptr<Scope>> class_stack;
			std::stack<size_t> function_class_call_depth;
			std::stack<std::shared_ptr<ClassDefinition>> class_def_build_stack;
			std::stack<std::shared_ptr<StructDefinition>> struct_def_build_stack;
			std::stack< std::shared_ptr<FunctionDefinition>> func_def_build_stack;
			std::stack<std::shared_ptr<UnpackedVariableDefinition>> uvar_def_build_stack;
			std::stack<RuntimeValue*> value_build_stack;
			std::stack<size_t> return_stack;
			std::stack<size_t> try_stack;
			std::stack<std::pair<flx_int, flx_string>> catch_err_stack;

			TypeDefinition current_expression_array_type;

			std::vector<size_t> set_array_dim;
			size_t set_default_value_pc;
			bool set_check_build_array = false;
			std::stack<TypeDefinition> type_def_stack;

			bool return_from_sub_run = false;
			bool is_self_invoke = false;
			std::stack<bool> use_variable_ref;

			bool generated_error = false;
			std::string generated_error_msg;
			std::stack<size_t> call_stack;
			VmDebug vm_debug;

		private:
			void push_vm_scope(std::shared_ptr<Scope> scope);
			void pop_vm_scope(const std::string& module_name_space, const std::string& module_name);
			void push_deep();
			void pop_deep();
			void unwind();
			void unwind_scope();
			void unwind_eval_stack();

			bool get_use_variable_ref();

			void push_type_def(TypeDefinition type_def);
			TypeDefinition get_type_def();
			TypeDefinition& top_type_def();
			void cleanup_type_set();

			void check_build_array(RuntimeValue* new_value, std::vector<size_t> dim);
			flx_array build_array(const std::vector<size_t>& dim, RuntimeValue* init_value, intmax_t i);

			void binary_operation(const std::string& op);
			void unary_operation(const std::string& op);

			void decode_operation();
			bool get_next();

			DebugInfo get_debug_info(size_t dbg_pc);

			void declare_function_block_parameters(
				const std::string& func_name_space,
				std::vector<std::shared_ptr<TypeDefinition>> current_function_defined_parameters,
				std::vector<std::shared_ptr<TypeDefinition>> current_function_calling_arguments
			);
			void declare_function_parameter(
				const std::string& func_name_space,
				const std::string& identifier,
				TypeDefinition variable,
				RuntimeValue* value
			);

			std::shared_ptr<Scope> find_declared_function(
				const std::string& current_module_name_space,
				const std::string& current_module_name,
				const std::string& name_space,
				const std::string& identifier,
				const std::vector<std::shared_ptr<TypeDefinition>>& signature,
				bool& strict
			);
			std::shared_ptr<Scope> find_declared_function_strict(
				const std::string& current_module_name_space,
				const std::string& current_module_name,
				const std::string& name_space,
				const std::string& identifier,
				const std::vector<std::shared_ptr<TypeDefinition>>& signature,
				bool& strict
			);
			
			std::shared_ptr<VariableDefinition> read_param();

			void handle_include_namespace();
			void handle_exclude_namespace();
			void handle_init_array();
			void handle_set_element();
			void handle_push_array();
			void handle_init_struct();
			void handle_set_field();
			void handle_push_struct();
			void handle_struct_start();
			void handle_struct_set_var();
			void handle_struct_end();
			void handle_class_start();
			void handle_class_set_var();
			void handle_class_end();
			void handle_push_type_def();
			void handle_load_sub_id();
			void handle_load_sub_ix();
			void handle_fun_start();
			void handle_set_default_value();
			void handle_fun_set_param();
			void handle_fun_start_unpack_param();
			void handle_fun_set_sub_param();
			void handle_fun_set_unpack_param();
			void handle_fun_end();
			void handle_call();
			void handle_return();
			void handle_throw();
			void handle_get_iterator();
			void handle_has_next_element();
			void handle_next_element();
			void handle_type_parse();
			void handle_store_var();
			void handle_load_var();

		public:
			VirtualMachine(
				std::shared_ptr<Scope> global_scope,
				VmDebug vm_debug,
				std::vector<BytecodeInstruction> instructions
			);
			VirtualMachine() = default;
			~VirtualMachine() = default;

			void run();

		};

	}

}

#endif // !VIRTUAL_MACHINE_HPP
