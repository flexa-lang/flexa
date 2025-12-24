#include "md_datetime.hpp"

#include <vector>

#include "vm.hpp"
#include "semantic_analysis.hpp"
#include "constants.hpp"

using namespace core;
using namespace core::modules;
using namespace core::runtime;
using namespace core::analysis;

ModuleDateTime::ModuleDateTime() {}

ModuleDateTime::~ModuleDateTime() = default;

flx_struct ModuleDateTime::tm_to_date_time(std::function<RuntimeValue*(RuntimeValue*)> allocate_value, time_t t, tm* tm) {
	flx_struct dt_str = flx_struct();

	std::vector<std::shared_ptr<RuntimeVariable>> vars = {
		std::make_shared<RuntimeVariable>("timestamp", Type::T_INT), // timestamp
		std::make_shared<RuntimeVariable>("second", Type::T_INT), // second
		std::make_shared<RuntimeVariable>("minute", Type::T_INT), // minute
		std::make_shared<RuntimeVariable>("hour", Type::T_INT), // hour
		std::make_shared<RuntimeVariable>("day", Type::T_INT), // day
		std::make_shared<RuntimeVariable>("month", Type::T_INT), // month
		std::make_shared<RuntimeVariable>("year", Type::T_INT), // year
		std::make_shared<RuntimeVariable>("week_day", Type::T_INT), // week_day
		std::make_shared<RuntimeVariable>("year_day", Type::T_INT), // year_day
		std::make_shared<RuntimeVariable>("is_dst", Type::T_INT) // is_dst
	};

	vars[0]->set_value(allocate_value(new RuntimeValue(flx_int(t))));
	vars[1]->set_value(allocate_value(new RuntimeValue(flx_int(tm->tm_sec))));
	vars[2]->set_value(allocate_value(new RuntimeValue(flx_int(tm->tm_min))));
	vars[3]->set_value(allocate_value(new RuntimeValue(flx_int(tm->tm_hour))));
	vars[4]->set_value(allocate_value(new RuntimeValue(flx_int(tm->tm_mday))));
	vars[5]->set_value(allocate_value(new RuntimeValue(flx_int(tm->tm_mon + 1))));
	vars[6]->set_value(allocate_value(new RuntimeValue(flx_int(tm->tm_year))));
	vars[7]->set_value(allocate_value(new RuntimeValue(flx_int(tm->tm_wday))));
	vars[8]->set_value(allocate_value(new RuntimeValue(flx_int(tm->tm_yday + 1))));
	vars[9]->set_value(allocate_value(new RuntimeValue(flx_int(tm->tm_isdst))));
	for (const auto& var : vars) {
		dt_str[var->identifier] = var;
	}

	return dt_str;
}

void ModuleDateTime::register_functions(SemanticAnalyser* visitor) {
	visitor->builtin_functions["create_date_time"] = nullptr;
	visitor->builtin_functions["diff_date_time"] = nullptr;
	visitor->builtin_functions["format_date_time"] = nullptr;
	visitor->builtin_functions["format_local_date_time"] = nullptr;
	visitor->builtin_functions["ascii_date_time"] = nullptr;
	visitor->builtin_functions["ascii_local_date_time"] = nullptr;
	visitor->builtin_functions["clock"] = nullptr;
}

void ModuleDateTime::register_functions(VirtualMachine* vm) {

	vm->builtin_functions["create_date_time"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);

		tm tm{};
		time_t t;

		if (scope->total_declared_variables() == 0) {
			t = time(nullptr);

#ifdef linux

			gmtime_r(&t, &tm);

#elif defined(_WIN32)

			gmtime_s(&tm, &t);

#endif // linux

		}
		else if (scope->total_declared_variables() == 1) {
			auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("timestamp"))->get_value();

			t = val->get_i();

#ifdef linux

			gmtime_r(&t, &tm);

#elif defined(_WIN32)

			gmtime_s(&tm, &t);

#endif // linux

		}
		else {
			auto vals = std::vector{
				std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("year"))->get_value(),
				std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("month"))->get_value(),
				std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("day"))->get_value(),
				std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("hour"))->get_value(),
				std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("min"))->get_value(),
				std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("sec"))->get_value()
			};

			tm.tm_year = vals[0]->get_i() - 1900;
			tm.tm_mon = vals[1]->get_i() - 1;
			tm.tm_mday = vals[2]->get_i();
			tm.tm_hour = vals[3]->get_i();
			tm.tm_min = vals[4]->get_i();
			tm.tm_sec = vals[5]->get_i();
			t = mktime(&tm);
		}

		vm->push_new_constant(new RuntimeValue(tm_to_date_time(std::bind(&VirtualMachine::allocate_value, vm, std::placeholders::_1), t, &tm), Constants::STD_NAMESPACE, "DateTime"));

		};

	vm->builtin_functions["diff_date_time"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto vals = std::vector{
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("left_date_time"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("right_date_time"))->get_value()
		};

		time_t lt = vals[0]->get_str()["timestamp"]->get_value()->get_i();
		time_t rt = vals[1]->get_str()["timestamp"]->get_value()->get_i();
		time_t t = difftime(lt, rt);
		tm tm{};

#ifdef linux

		gmtime_r(&t, &tm);

#elif defined(_WIN32)

		gmtime_s(&tm, &t);

#endif // linux		

		vm->push_new_constant(new RuntimeValue(tm_to_date_time(std::bind(&VirtualMachine::allocate_value, vm, std::placeholders::_1), t, &tm), Constants::STD_NAMESPACE, "DateTime"));

		};

	vm->builtin_functions["format_date_time"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto vals = std::vector{
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("date_time"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("format"))->get_value()
		};

		time_t t = vals[0]->get_str()["timestamp"]->get_value()->get_i();
		std::string fmt = vals[1]->get_s();
		tm tm{};

#ifdef linux

		gmtime_r(&t, &tm);

#elif defined(_WIN32)

		gmtime_s(&tm, &t);

#endif // linux

		char buffer[80];
		strftime(buffer, 80, fmt.c_str(), &tm);

		vm->push_new_constant(new RuntimeValue(std::string{ buffer }));

		};

	vm->builtin_functions["format_local_date_time"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto vals = std::vector{
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("date_time"))->get_value(),
			std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("format"))->get_value()
		};

		time_t t = vals[0]->get_str()["timestamp"]->get_value()->get_i();
		std::string fmt = vals[1]->get_s();
		tm tm{};

#ifdef linux

		localtime_r(&t, &tm);

#elif defined(_WIN32)

		localtime_s(&tm, &t);

#endif // linux

		char buffer[80];
		strftime(buffer, 80, fmt.c_str(), &tm);

		vm->push_new_constant(new RuntimeValue(std::string{ buffer }));

		};

	vm->builtin_functions["ascii_date_time"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("date_time"))->get_value();

		time_t t = val->get_str()["timestamp"]->get_value()->get_i();
		tm tm{};

		char buffer[26];

#ifdef linux

		gmtime_r(&t, &tm);

		if (!asctime_r(&tm, buffer)) {
			throw std::runtime_error("Error trying to convert date/time to ASCII string");
		}

#elif defined(_WIN32)

		gmtime_s(&tm, &t);

		if (asctime_s(buffer, sizeof(buffer), &tm) != 0) {
			throw std::runtime_error("Error trying to convert date/time to ASCII string");
		}

#endif // linux

		vm->push_new_constant(new RuntimeValue(std::string{ buffer }));

		};

	vm->builtin_functions["ascii_local_date_time"] = [this, vm]() {
		auto scope = vm->get_back_scope(Constants::STD_NAMESPACE);
		auto val = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable("date_time"))->get_value();

		time_t t = val->get_str()["timestamp"]->get_value()->get_i();
		tm tm{};

		char buffer[26];

#ifdef linux

		localtime_r(&t, &tm);

		if (!asctime_r(&tm, buffer)) {
			throw std::runtime_error("Error trying to convert date/time to ASCII string");
		}

#elif defined(_WIN32)

		localtime_s(&tm, &t);

		if (asctime_s(buffer, sizeof(buffer), &tm) != 0) {
			throw std::runtime_error("Error trying to convert date/time to ASCII string");
		}

#endif // linux

		vm->push_new_constant(new RuntimeValue(std::string{ buffer }));

		};

	vm->builtin_functions["clock"] = [this, vm]() {
		vm->push_new_constant(new RuntimeValue(flx_int(clock())));

		};

}
