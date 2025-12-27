#include "md_files.hpp"

#include <filesystem>

#include "vm.hpp"
#include "semantic_analysis.hpp"
#include "constants.hpp"

using namespace core::modules;
using namespace core::runtime;
using namespace core::analysis;

ModuleFiles::ModuleFiles() {}

ModuleFiles::~ModuleFiles() = default;

void ModuleFiles::register_functions(SemanticAnalyser* visitor) {
	visitor->builtin_functions["open"] = nullptr;
	visitor->builtin_functions["read"] = nullptr;
	visitor->builtin_functions["read_line"] = nullptr;
	visitor->builtin_functions["read_all_bytes"] = nullptr;
	visitor->builtin_functions["write"] = nullptr;
	visitor->builtin_functions["write_bytes"] = nullptr;
	visitor->builtin_functions["is_open"] = nullptr;
	visitor->builtin_functions["close"] = nullptr;

	visitor->builtin_functions["is_file"] = nullptr;
	visitor->builtin_functions["is_dir"] = nullptr;

	visitor->builtin_functions["create_dir"] = nullptr;
	visitor->builtin_functions["list_dir"] = nullptr;

	visitor->builtin_functions["path_exists"] = nullptr;
	visitor->builtin_functions["delete_path"] = nullptr;
}

void ModuleFiles::register_functions(VirtualMachine* vm) {

	vm->builtin_functions["open"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto vals = std::vector{
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("path"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("mode"))->get_value()
		};

		auto path_var = std::make_shared<RuntimeVariable>("path", Type::T_STRING);
		path_var->set_value(vm->allocate_value(new RuntimeValue(vals[0])));
		vm->gc.add_var_root(path_var);

		auto mode_var = std::make_shared<RuntimeVariable>("mode", Type::T_INT);
		mode_var->set_value(vm->allocate_value(new RuntimeValue(vals[1])));
		vm->gc.add_var_root(mode_var);

		flx_struct str = flx_struct();
		str["path"] = path_var;
		str["mode"] = mode_var;

		auto parmode = vals[1]->get_i();

		std::fstream* fs = nullptr;
		try {
			std::ios_base::openmode mode = std::ios_base::openmode(parmode);
			fs = new std::fstream(vals[0]->get_s(), mode);

			auto instance_id_var = std::make_shared<RuntimeVariable>(INSTANCE_ID_NAME, Type::T_INT);
			instance_id_var->set_value(vm->allocate_value(new RuntimeValue((flx_int)fs)));
			vm->gc.add_var_root(instance_id_var);

			str[INSTANCE_ID_NAME] = instance_id_var;

			// initialize file struct values
			RuntimeValue* flxfile = vm->allocate_value(new RuntimeValue(str, Constants::STD_NAMESPACE, "File"));

			vm->push_constant(flxfile);
		}
		catch (const std::runtime_error& ex) {
			throw std::runtime_error(ex.what());
		}

		};

	vm->builtin_functions["read"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("file"))->get_value();

		if (val->is_void()) {
			throw std::runtime_error("Cannot read from a null");
		}

		auto rval = vm->allocate_value(new RuntimeValue(Type::T_STRING));

		std::fstream* fs = ((std::fstream*)val->get_str()[INSTANCE_ID_NAME]->get_value()->get_i());

		fs->seekg(0);

		std::stringstream ss;
		std::string line;
		while (std::getline(*fs, line)) {
			ss << line << std::endl;
		}
		rval->set(ss.str());

		vm->push_constant(rval);

		};

	vm->builtin_functions["read_line"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("file"))->get_value();

		if (val->is_void()) {
			throw std::runtime_error("Cannot read line from a null");
		}

		auto rval = vm->allocate_value(new RuntimeValue(Type::T_STRING));

		std::fstream* fs = ((std::fstream*)val->get_str()[INSTANCE_ID_NAME]->get_value()->get_i());

		std::string line;
		std::getline(*fs, line);
		rval->set(line);

		vm->push_constant(rval);

		};

	vm->builtin_functions["read_all_bytes"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("file"))->get_value();

		if (val->is_void()) {
			throw std::runtime_error("Cannot read bytes from a null");
		}

		std::fstream* fs = ((std::fstream*)val->get_str()[INSTANCE_ID_NAME]->get_value()->get_i());

		fs->seekg(0);

		// find file size
		fs->seekg(0, std::ios::end);
		std::streamsize buffer_size = fs->tellg();
		fs->seekg(0, std::ios::beg);

		// buffer to store readed data
		char* buffer = new char[buffer_size];

		flx_array arr = flx_array(buffer_size);

		// read all bytes
		if (fs->read(buffer, buffer_size)) {
			for (std::streamsize i = 0; i < buffer_size; ++i) {
				RuntimeValue* val = vm->allocate_value(new RuntimeValue(Type::T_CHAR));
				val->set(buffer[i]);
				arr[i] = val;
			}
		}

		auto rval = vm->allocate_value(new RuntimeValue(arr, Type::T_CHAR, std::vector<size_t>{(size_t)arr.size()}));

		delete[] buffer;

		vm->push_constant(rval);

		};

	vm->builtin_functions["write"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto vals = std::vector{
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("file"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("data"))->get_value()
		};

		RuntimeValue* cpfile = vals[0];
		if (cpfile->is_void()) {
			throw std::runtime_error("Cannot write to a null");
		}

		std::fstream* fs = ((std::fstream*)cpfile->get_str()[INSTANCE_ID_NAME]->get_value()->get_i());
		*fs << vals[1]->get_s();

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["write_bytes"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto vals = std::vector{
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("file"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("bytes"))->get_value()
		};

		RuntimeValue* cpfile = vals[0];
		if (cpfile->is_void()) {
			throw std::runtime_error("Cannot write to a null");
		}

		std::fstream* fs = ((std::fstream*)cpfile->get_str()[INSTANCE_ID_NAME]->get_value()->get_i());

		auto arr = vals[1]->get_arr();

		std::streamsize buffer_size = arr.size();

		char* buffer = new char[buffer_size];

		for (std::streamsize i = 0; i < buffer_size; ++i) {
			buffer[i] = arr[i]->get_c();
		}

		fs->write(buffer, sizeof(buffer));

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["is_open"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("file"))->get_value();

		if (val->is_void()) {
			throw std::runtime_error("Cannot check is_open on a null");
		}
		auto rval = vm->allocate_value(new RuntimeValue(Type::T_BOOL));
		rval->set(flx_bool(((std::fstream*)val->get_str()[INSTANCE_ID_NAME]->get_value()->get_i())->is_open()));
		vm->push_constant(rval);

		};

	vm->builtin_functions["close"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("file"))->get_value();

		auto instance_id = val->get_raw_str()->at(INSTANCE_ID_NAME);
		if (auto rawfile = (std::fstream*)instance_id->get_value()->get_i()) {
			rawfile->close();
			delete rawfile;
			instance_id->get_value()->set(flx_int(0));
		}

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["is_file"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("path"))->get_value();

		std::filesystem::path path = val->get_s();

		vm->push_new_constant(new RuntimeValue(std::filesystem::is_regular_file(path)));

		};

	vm->builtin_functions["is_dir"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("path"))->get_value();

		std::filesystem::path path = val->get_s();

		vm->push_new_constant(new RuntimeValue(std::filesystem::is_directory(path)));

		};

	vm->builtin_functions["create_dir"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("path"))->get_value();

		std::filesystem::path path = val->get_s();

		if (!std::filesystem::create_directories(path)) {
			throw std::runtime_error("cannot create directory");
		}

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

	vm->builtin_functions["list_dir"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("path"))->get_value();

		std::filesystem::path path = val->get_s();

		std::vector<std::string> files;

		for (const auto& entry : std::filesystem::directory_iterator(path)) {
			files.push_back(entry.path().filename().string());
		}

		flx_array values = flx_array(files.size());

		for (size_t i = 0; i < files.size(); ++i) {
			const auto& file = files[i];
			values[i] = vm->allocate_value(new RuntimeValue(file));
		}

		vm->push_new_constant(new RuntimeValue(values, Type::T_STRING, std::vector<size_t>{size_t(values.size())}));

		};

	vm->builtin_functions["path_exists"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("path"))->get_value();

		std::filesystem::path path = val->get_s();

		vm->push_new_constant(new RuntimeValue(std::filesystem::exists(path)));

		};

	vm->builtin_functions["delete_path"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("path"))->get_value();

		std::filesystem::path path = val->get_s();

		std::filesystem::remove_all(path);

		vm->push_empty_constant(Type::T_UNDEFINED);

		};

}
