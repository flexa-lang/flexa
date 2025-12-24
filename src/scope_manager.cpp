#include "scope_manager.hpp"

#include "utils.hpp"

using namespace core;

std::shared_ptr<StructDefinition> ScopeManager::find_inner_most_struct(
	std::string module_name_space,
	std::string module_name,
	const std::string& name_space,
	const std::string& identifier
) {
	std::shared_ptr<Scope> scope = get_inner_most_struct_definition_scope(module_name_space, module_name, name_space, identifier);
	if (!scope) {
		throw std::runtime_error("struct '" + identifier + "' not found");
	}
	return scope->find_declared_struct_definition(identifier);
}

std::shared_ptr<Variable> ScopeManager::find_inner_most_variable(
	std::string module_name_space,
	std::string module_name,
	const std::string& name_space,
	const std::string& identifier
) {
	std::shared_ptr<Scope> scope = get_inner_most_variable_scope(module_name_space, module_name, name_space, identifier);
	if (!scope) {
		throw std::runtime_error("variable '" + identifier + "' not found");
	}
	return scope->find_declared_variable(identifier);
}

std::shared_ptr<Scope> ScopeManager::find_in_namespace(
	const std::string& name_space,
	const std::string& identifier,
	std::vector<std::string>& visited,
	const std::unordered_map<std::string, std::vector<std::shared_ptr<Scope>>>& scope_map,
	already_decl_func_t already_declared_pred
) {
	if (utils::CollectionUtils::contains(visited, name_space)) {
		return nullptr;
	}
	visited.push_back(name_space);

	auto it = scope_map.find(name_space);
	if (it == scope_map.end()) return nullptr;

	auto& vec = it->second;
	for (intmax_t i = vec.size() - 1; i >= 0; --i) {
		if (already_declared_pred(vec[i], identifier)) {
			return vec[i];
		}
	}
	return nullptr;
}

std::shared_ptr<Scope> ScopeManager::get_inner_most_scope(
	std::string module_name_space,
	std::string module_name,
	const std::string& access_name_space,
	const std::string& identifier,
	std::vector<std::string> visited,
	const std::unordered_map<std::string, std::vector<std::shared_ptr<Scope>>>& namespace_scope_map,
	const std::unordered_map<std::string, std::vector<std::shared_ptr<Scope>>>& module_scope_map,
	already_decl_func_t already_declared_pred
) {
	if (!access_name_space.empty() && access_name_space != module_name_space) {
		// try find at given namespace
		return find_in_namespace(access_name_space, identifier, visited, namespace_scope_map, already_declared_pred);
	}
	else {
		// try find at current module
		if (auto scope = find_in_namespace(module_name, identifier, visited, module_scope_map, already_declared_pred)) {
			return scope;
		}

		// try find at module included namespace
		for (const auto& module_included_name_space : module_included_name_spaces[module_name]) {
			if (auto scope = find_in_namespace(module_included_name_space, identifier, visited, namespace_scope_map, already_declared_pred)) {
				return scope;
			}
		}
	}

	return nullptr;
}

std::shared_ptr<Scope> ScopeManager::get_inner_most_variable_scope(
	std::string module_name_space,
	std::string module_name,
	const std::string& access_name_space,
	const std::string& identifier,
	std::vector<std::string> visited
) {
	return get_inner_most_scope(
		module_name_space,
		module_name,
		access_name_space,
		identifier,
		visited,
		scopes,
		module_scopes,
		[](const std::shared_ptr<Scope>& scope, const std::string& id) { return scope->already_declared_variable(id); }
	);
}

std::shared_ptr<Scope> ScopeManager::get_inner_most_struct_definition_scope(
	std::string module_name_space,
	std::string module_name,
	const std::string& access_name_space,
	const std::string& identifier,
	std::vector<std::string> visited
) {
	return get_inner_most_scope(
		module_name_space,
		module_name,
		access_name_space,
		identifier,
		visited,
		scopes,
		module_scopes,
		[](const std::shared_ptr<Scope>& scope, const std::string& id) { return scope->already_declared_struct_definition(id); }
	);
}

std::shared_ptr<Scope> ScopeManager::get_inner_most_class_definition_scope(
	std::string module_name_space,
	std::string module_name,
	const std::string& access_name_space,
	const std::string& identifier,
	std::vector<std::string> visited
) {
	return get_inner_most_scope(
		module_name_space,
		module_name,
		access_name_space,
		identifier,
		visited,
		scopes,
		module_scopes,
		[](const std::shared_ptr<Scope>& scope, const std::string& id) {
			return scope->already_declared_class_definition(id);
		}
	);
}

std::shared_ptr<Scope> ScopeManager::find_in_namespace(
	const std::string& name_space,
	const std::string& identifier,
	const std::vector<std::shared_ptr<TypeDefinition>>* signature,
	bool strict,
	std::vector<std::string>& visited
) {
	if (utils::CollectionUtils::contains(visited, name_space)) {
		return nullptr;
	}
	visited.push_back(name_space);

	auto it = global_module_scopes.find(name_space);
	if (it == global_module_scopes.end()) return nullptr;

	auto& vec = it->second;
	for (intmax_t i = vec.size() - 1; i >= 0; --i) {
		if (vec[i]->already_declared_function(identifier, signature, strict)) {
			return vec[i];
		}
	}
	return nullptr;
}

std::shared_ptr<Scope> ScopeManager::get_inner_most_function_scope(
	std::string module_name_space,
	std::string module_name,
	const std::string& access_name_space,
	const std::string& identifier,
	const std::vector<std::shared_ptr<TypeDefinition>>* signature,
	bool strict,
	std::vector<std::string> visited
) {
	if (!access_name_space.empty() && access_name_space != module_name_space) {
		// try find at given namespace
		return find_in_namespace(access_name_space, identifier, signature, strict, visited);
	}
	else {
		// try find at current module
		if (module_scopes.at(module_name)[0]->already_declared_function(identifier, signature, strict)) {
			return module_scopes.at(module_name)[0];
		}

		// try find at module included namespace
		std::shared_ptr<Scope> scope = nullptr;
		for (const auto& module_included_name_space : module_included_name_spaces[module_name]) {
			scope = find_in_namespace(module_included_name_space, identifier, signature, strict, visited);
			if (scope) {
				return scope;
			}
		}
	}

	return nullptr;
}

void ScopeManager::push_scope(std::shared_ptr<Scope> scope) {
	module_scopes[scope->module_name].push_back(scope);
	if (module_scopes[scope->module_name].size() == 1) {
		global_module_scopes[scope->module_name_space].push_back(scope);
	}
	scopes[scope->module_name_space].push_back(scope);
}

void ScopeManager::pop_scope(const std::string& module_name_space, const std::string& module_name) {
	module_scopes[module_name].pop_back();
	scopes[module_name_space].pop_back();
}

std::shared_ptr<Scope> ScopeManager::get_back_scope(const std::string& module_name_space) {
	return scopes[module_name_space].back();
}

std::shared_ptr<Scope> ScopeManager::get_global_scope(const std::string& module_name) {
	return module_scopes[module_name].at(0);
}
