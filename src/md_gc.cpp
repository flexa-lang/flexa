#include "md_gc.hpp"

#include "vm.hpp"
#include "semantic_analysis.hpp"
#include "constants.hpp"

using namespace core::modules;
using namespace core::runtime;
using namespace core::analysis;

ModuleGC::ModuleGC() {}

ModuleGC::~ModuleGC() = default;

void ModuleGC::register_functions(SemanticAnalyser* visitor) {
	visitor->builtin_functions["gc_is_enabled"] = nullptr;
	visitor->builtin_functions["gc_enable"] = nullptr;
	visitor->builtin_functions["gc_collect"] = nullptr;
	visitor->builtin_functions["gc_maybe_collect"] = nullptr;
	visitor->builtin_functions["gc_get_max_heap"] = nullptr;
	visitor->builtin_functions["gc_set_max_heap"] = nullptr;
}

void ModuleGC::register_functions(VirtualMachine* vm) {

	vm->builtin_functions["gc_is_enabled"] = [this, vm]() {
		vm->push_new_constant(new RuntimeValue(flx_bool(vm->gc.enable)));

		};

	vm->builtin_functions["gc_enable"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("enable"))->get_value();

		vm->gc.enable = val->get_b();

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["gc_maybe_collect"] = [this, vm]() {

		vm->gc.maybe_collect();

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["gc_collect"] = [this, vm]() {

		vm->gc.collect();

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["gc_get_max_heap"] = [this, vm]() {
		vm->push_new_constant(new RuntimeValue(flx_int(vm->gc.max_heap)));

		};

	vm->builtin_functions["gc_set_max_heap"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("max_heap"))->get_value();

		vm->gc.max_heap = val->get_i();

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

}
