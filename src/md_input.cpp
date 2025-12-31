#include "md_input.hpp"

#if defined(_WIN32)
#include <Windows.h>
#endif // defined(_WIN32)

#include "vm.hpp"
#include "semantic_analysis.hpp"
#include "constants.hpp"

using namespace core::modules;
using namespace core::runtime;
using namespace core::analysis;

ModuleInput::ModuleInput() : running(false) {
	start();
}

ModuleInput::~ModuleInput() {
	stop();
}

void ModuleInput::register_functions(SemanticAnalyser* visitor) {
	visitor->builtin_functions["update_key_states"] = nullptr;
	visitor->builtin_functions["is_key_pressed"] = nullptr;
	visitor->builtin_functions["is_key_released"] = nullptr;
	visitor->builtin_functions["get_mouse_position"] = nullptr;
	visitor->builtin_functions["set_mouse_position"] = nullptr;
	visitor->builtin_functions["is_mouse_button_pressed"] = nullptr;
}

void ModuleInput::key_update_loop() {
	while (running) {
		{
			std::lock_guard<std::mutex> lock(state_mutex);
			for (int i = 0; i < KEY_COUNT; ++i) {
				bool current = false;

#if defined(_WIN32)

				current = GetAsyncKeyState(i) & 0x8000;

#endif // defined(_WIN32)

				previous_key_state[i] = previous_key_state[i] || current;
				current_key_state[i] = current;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void ModuleInput::start() {
	if (!running) {
		running = true;
		key_update_thread = std::thread(&ModuleInput::key_update_loop, this);
	}
}

void ModuleInput::stop() {
	if (running) {
		running = false;
		if (key_update_thread.joinable()) {
			key_update_thread.join();
		}
	}
}

void ModuleInput::register_functions(VirtualMachine* vm) {

	vm->builtin_functions["update_key_states"] = [this, vm]() {

#ifdef linux

		throw std::runtime_error("Not implemented yet");

#elif  defined(_WIN32)

		previous_key_state = current_key_state;

		for (int i = 0; i < KEY_COUNT; ++i) {
			current_key_state[i] = GetAsyncKeyState(i) & 0x8000;
		}

#endif // linux

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["is_key_pressed"] = [this, vm]() {
		bool is_pressed = false;

#ifdef linux

		throw std::runtime_error("Not implemented yet");

#elif  defined(_WIN32)

		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("key"))->get_value();

		auto key = val->get_i();

		{
			std::lock_guard<std::mutex> lock(state_mutex);
			is_pressed = current_key_state[key];
		}

#endif // linux

		vm->push_new_constant(new RuntimeValue(flx_bool(is_pressed)));

		};

	vm->builtin_functions["is_key_released"] = [this, vm]() {
		bool is_released = false;

#ifdef linux

		throw std::runtime_error("Not implemented yet");

#elif  defined(_WIN32)
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("key"))->get_value();

		auto key = val->get_i();

		{
			std::lock_guard<std::mutex> lock(state_mutex);
			is_released = previous_key_state[key] && !current_key_state[key];
			if (is_released) {
				previous_key_state[key] = false;
			}
		}

#endif // linux

		vm->push_new_constant(new RuntimeValue(flx_bool(is_released)));

		};

	vm->builtin_functions["get_mouse_position"] = [this, vm]() {

		long long x, y = 0;

#ifdef linux

		throw std::runtime_error("Not implemented yet");

#elif  defined(_WIN32)

		POINT point;
		GetCursorPos(&point);
		x = point.x;
		y = point.y;

#endif // linux

		flx_struct str = flx_struct();

		auto x_var = std::make_shared<RuntimeVariable>("x", Type::T_INT);
		x_var->set_value(vm->allocate_value(new RuntimeValue(flx_int(x * 2 * 0.905))));
		vm->gc.add_var_root(x_var);

		auto y_var = std::make_shared<RuntimeVariable>("y", Type::T_INT);
		y_var->set_value(vm->allocate_value(new RuntimeValue(flx_int(y * 2 * 0.875))));
		vm->gc.add_var_root(y_var);

		str["x"] = x_var;
		str["y"] = y_var;

		vm->push_new_constant(new RuntimeValue(str, Constants::STD_NAMESPACE, "Point"));

		};

	vm->builtin_functions["set_mouse_position"] = [this, vm]() {

#ifdef linux

		throw std::runtime_error("Not implemented yet");

#elif  defined(_WIN32)

		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto vals = std::vector{
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("x"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("y"))->get_value()
		};

		auto x = static_cast<int>(vals[0]->get_i());
		auto y = static_cast<int>(vals[1]->get_i());
		SetCursorPos(x, y);

#endif // linux

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["is_mouse_button_pressed"] = [this, vm]() {

		bool is_pressed = false;

#ifdef linux

		throw std::runtime_error("Not implemented yet");

#elif  defined(_WIN32)

		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("button"))->get_value();

		auto button = static_cast<int>(val->get_i());

		is_pressed = (GetAsyncKeyState(button) & 0x8000) != 0;

#endif // linux

		vm->push_new_constant(new RuntimeValue(flx_bool(is_pressed)));

		};

}
