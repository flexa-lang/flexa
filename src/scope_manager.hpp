#ifndef SCOPE_MANAGER_HPP
#define SCOPE_MANAGER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

#include "scope.hpp"
#include "types.hpp"

namespace core {

	class ScopeManager {
		typedef std::function<bool(const std::shared_ptr<Scope>& scope, const std::string& id)> already_decl_func_t;

	public:
		/*
			Current scopes, it will be removed when pop_scope is called.
			There are separated by namespace and contains all scopes of the namespace.
		*/
		std::unordered_map<std::string, std::vector<std::shared_ptr<Scope>>> scopes;
		/*
			Current module scopes, it will be removed when pop_scope is called.
			There are separated by module name and contains all scopes of the module.
		*/
		std::unordered_map<std::string, std::vector<std::shared_ptr<Scope>>> module_scopes;
		/*
			Global module scopes in the namespace, it will never be removed, during all runtime lifetime.
			There are separated by namespace and contains only the global scope of each module.
		*/
		std::unordered_map<std::string, std::vector<std::shared_ptr<Scope>>> global_module_scopes;
		/*
			All namespaces included in a module.
		*/
		std::unordered_map<std::string, std::vector<std::string>> module_included_name_spaces;

		ScopeManager() = default;
		virtual ~ScopeManager() = default;

		std::shared_ptr<StructDefinition> find_inner_most_struct(
			std::string module_name_space,
			std::string module_name,
			const std::string& access_name_space,
			const std::string& identifier
		);
		std::shared_ptr<Variable> find_inner_most_variable(
			std::string module_name_space,
			std::string module_name,
			const std::string& access_name_space,
			const std::string& identifier
		);

		std::shared_ptr<Scope> find_in_namespace(
			const std::string& name_space,
			const std::string& identifier,
			std::vector<std::string>& visited,
			const std::unordered_map<std::string, std::vector<std::shared_ptr<Scope>>>& scope_map,
			already_decl_func_t already_declared_pred
		);

		std::shared_ptr<Scope> get_inner_most_scope(
			std::string module_name_space,
			std::string module_name,
			const std::string& access_name_space,
			const std::string& identifier,
			std::vector<std::string> visited,
			const std::unordered_map<std::string, std::vector<std::shared_ptr<Scope>>>& namespace_scope_map,
			const std::unordered_map<std::string, std::vector<std::shared_ptr<Scope>>>& module_scope_map,
			already_decl_func_t already_declared_pred
		);

		std::shared_ptr<Scope> get_inner_most_variable_scope(
			std::string module_name_space,
			std::string module_name,
			const std::string& access_name_space,
			const std::string& identifier,
			std::vector<std::string> visited = std::vector<std::string>()
		);

		std::shared_ptr<Scope> get_inner_most_class_definition_scope(
			std::string module_name_space, std::string module_name,
			const std::string& access_name_space,
			const std::string& identifier,
			std::vector<std::string> visited = std::vector<std::string>()
		);

		std::shared_ptr<Scope> get_inner_most_struct_definition_scope(
			std::string module_name_space,
			std::string module_name,
			const std::string& access_name_space,
			const std::string& identifier,
			std::vector<std::string> visited = std::vector<std::string>()
		);

		std::shared_ptr<Scope> find_in_namespace(
			const std::string& name_space,
			const std::string& identifier,
			const std::vector<std::shared_ptr<TypeDefinition>>* signature,
			bool strict,
			std::vector<std::string>& visited
		);

		std::shared_ptr<Scope> get_inner_most_function_scope(
			std::string module_name_space,
			std::string module_name,
			const std::string& access_name_space,
			const std::string& identifier,
			const std::vector<std::shared_ptr<TypeDefinition>>* signature,
			bool strict = true,
			std::vector<std::string> visited = std::vector<std::string>()
		);

		void push_scope(std::shared_ptr<Scope> scope);
		void pop_scope(const std::string& module_name_space, const std::string& module_name);
		std::shared_ptr<Scope> get_back_scope(const std::string& module_name_space);
		std::shared_ptr<Scope> get_global_scope(const std::string& module_name);
	};

}

#endif // !SCOPE_MANAGER_HPP
