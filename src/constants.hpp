#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "module.hpp"

namespace core {

	enum BuiltinStructs {
		BS_ENTRY,
		BS_EXCEPTION,
		BS_CONTEXT,
		BS_SIZE
	};

	enum StrEntryFields {
		SEF_KEY,
		SEF_VALUE,
		SEF_SIZE
	};

	enum StrExceptionFields {
		SXF_ERROR,
		SXF_CODE,
		SXF_SIZE
	};

	enum StrContextFields {
		SCF_NAME,
		SCF_NAMESPACE,
		SCF_TYPE,
		SCF_SIZE
	};

	enum BuiltinFuncs {
		BF_LOG,
		BF_PRINT,
		BF_PRINTLN,
		BF_READ,
		BF_READCH,
		BF_LEN,
		BF_LENS,
		BF_SLEEP,
		BF_SYSTEM,
		BF_SIZE
	};

	enum CoreLibs {
		CL_GC,
		CL_GRAPHICS,
		CL_FILES,
		CL_CONSOLE,
		CL_DATETIME,
		CL_INPUT,
		CL_SOUND,
		CL_HTTP,
		CL_SYS,
		CL_OS,
		CL_SIZE
	};

	class Constants {
	public:
		bool static debug;

		std::string static const NAME;
		std::string static const VER;
		std::string static const YEAR;

		std::string static const DEFAULT_NAMESPACE;
		std::string static const STD_NAMESPACE;

		std::array<std::string, BuiltinStructs::BS_SIZE> static const BUILTIN_STRUCT_NAMES;
		std::array<std::string, StrEntryFields::SEF_SIZE> static const STR_ENTRY_FIELD_NAMES;
		std::array<std::string, StrExceptionFields::SXF_SIZE> static const STR_EXCEPTION_FIELD_NAMES;
		std::array<std::string, StrContextFields::SCF_SIZE> static const STR_CONTEXT_FIELD_NAMES;

		std::string static const BUILTIN_MODULE_NAME;
		std::array<std::string, BuiltinFuncs::BF_SIZE> static const BUILTIN_FUNCTION_NAMES;
		std::shared_ptr<modules::Module> static const BUILTIN_FUNCTIONS;

		std::vector<std::string> static const STD_LIB_NAMES;
		std::array<std::string, CoreLibs::CL_SIZE> static const CORE_LIB_NAMES;
		std::unordered_map<std::string, std::shared_ptr<modules::Module>> static const CORE_LIBS;

	};

}

#endif // !CONSTANTS_HPP
