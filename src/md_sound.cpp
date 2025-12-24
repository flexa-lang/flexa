#include "md_sound.hpp"

#if defined(_WIN32)
#include <Windows.h>
#endif // defined(_WIN32)
#include <memory>

#include "vm.hpp"
#include "semantic_analysis.hpp"
#include "constants.hpp"

#pragma comment(lib, "winmm.lib")

using namespace core::modules;
using namespace core::runtime;
using namespace core::analysis;

ModuleSound::ModuleSound() {}

ModuleSound::~ModuleSound() = default;

void ModuleSound::register_functions(SemanticAnalyser* visitor) {
	visitor->builtin_functions["play_sound"] = nullptr;
	visitor->builtin_functions["stop_sound_once"] = nullptr;
	visitor->builtin_functions["stop_sound"] = nullptr;
	visitor->builtin_functions["set_volume"] = nullptr;
}

void ModuleSound::register_functions(VirtualMachine* vm) {

	vm->builtin_functions["play_sound"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("path"))->get_value();

		auto file_path = val->get_s();
		std::wstring wfile_path = std::wstring(file_path.begin(), file_path.end());

#ifdef linux

		throw std::runtime_error("Not implemented yet");

#elif  defined(_WIN32)

		PlaySound(wfile_path.c_str(), NULL, SND_ASYNC | SND_FILENAME | SND_LOOP);

#endif // linux

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["play_sound_once"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("path"))->get_value();

		auto file_path = val->get_s();
		std::wstring wfile_path = std::wstring(file_path.begin(), file_path.end());

#ifdef linux

		throw std::runtime_error("Not implemented yet");

#elif  defined(_WIN32)

		PlaySound(wfile_path.c_str(), NULL, SND_ASYNC | SND_FILENAME);

#endif // linux

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["stop_sound"] = [this, vm]() {

#ifdef linux

		throw std::runtime_error("Not implemented yet");

#elif  defined(_WIN32)

		PlaySound(NULL, 0, 0);

#endif // linux

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["set_volume"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("volume"))->get_value();

		unsigned long volume = val->get_f() * 65535;

#ifdef linux

		throw std::runtime_error("Not implemented yet");

#elif  defined(_WIN32)

		waveOutSetVolume(0, MAKELONG(volume, volume));

#endif // linux

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

}
