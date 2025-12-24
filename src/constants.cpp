#include "constants.hpp"

#include "md_builtin.hpp"
#include "md_datetime.hpp"
#include "md_graphics.hpp"
#include "md_files.hpp"
#include "md_gc.hpp"
#include "md_console.hpp"
#include "md_input.hpp"
#include "md_sound.hpp"
#include "md_http.hpp"
#include "md_sys.hpp"
#include "md_os.hpp"

using namespace core;

bool Constants::debug = false;

std::string const Constants::NAME = "Flexa";
std::string const Constants::VER = "v0.4.3";
std::string const Constants::YEAR = "2025";

std::string const Constants::STD_NAMESPACE = "flx";
std::string const Constants::DEFAULT_NAMESPACE = "__default__";

std::array<std::string, BuiltinStructs::BS_SIZE> const Constants::BUILTIN_STRUCT_NAMES = {
	"Entry",
	"Exception",
	"Context"
};
std::array<std::string, StrEntryFields::SEF_SIZE> const Constants::STR_ENTRY_FIELD_NAMES = {
	"key",
	"value"
};
std::array<std::string, StrExceptionFields::SXF_SIZE> const Constants::STR_EXCEPTION_FIELD_NAMES = {
	"error",
	"code"
};
std::array<std::string, StrContextFields::SCF_SIZE> const Constants::STR_CONTEXT_FIELD_NAMES = {
	"name",
	"ns",
	"type"
};

std::string const Constants::BUILTIN_MODULE_NAME = "__builtin__";
std::array<std::string, BuiltinFuncs::BF_SIZE> const Constants::BUILTIN_FUNCTION_NAMES = {
	"log",
	"print",
	"println",
	"read",
	"readch",
	"len",
	"len",
	"sleep",
	"system"
};
std::shared_ptr<modules::Module> const Constants::BUILTIN_FUNCTIONS = std::shared_ptr<modules::ModuleBuiltin>(new modules::ModuleBuiltin());

std::vector<std::string> const Constants::STD_LIB_NAMES = {
	"flx.std.collections.collection",
	"flx.std.collections.dictionary",
	"flx.std.collections.hashtable",
	"flx.std.collections.list",
	"flx.std.collections.queue",
	"flx.std.collections.stack",
	"flx.std.arrays",
	"flx.std.math",
	"flx.std.print",
	"flx.std.random",
	"flx.std.strings",
	"flx.std.types",
	"flx.std.testing",
	"flx.std.utils",
	"flx.std.DSL.FML",
	"flx.std.DSL.JSON",
	"flx.std.DSL.YAML",
	"flx.std.DSL.XML"
};

std::array<std::string, CoreLibs::CL_SIZE> const Constants::CORE_LIB_NAMES = {
	"flx.core.gc",
	"flx.core.graphics",
	"flx.core.files",
	"flx.core.console",
	"flx.core.datetime",
	"flx.core.input",
	"flx.core.sound",
	"flx.core.HTTP",
	"flx.core.sys",
	"flx.core.os"
};

std::unordered_map<std::string, std::shared_ptr<modules::Module>> const Constants::CORE_LIBS = {
	{CORE_LIB_NAMES[CoreLibs::CL_GC], std::shared_ptr<modules::ModuleGC>(new modules::ModuleGC())},
	{CORE_LIB_NAMES[CoreLibs::CL_GRAPHICS], std::shared_ptr<modules::ModuleGraphics>(new modules::ModuleGraphics())},
	{CORE_LIB_NAMES[CoreLibs::CL_FILES], std::shared_ptr<modules::ModuleFiles>(new modules::ModuleFiles())},
	{CORE_LIB_NAMES[CoreLibs::CL_CONSOLE], std::shared_ptr<modules::ModuleConsole>(new modules::ModuleConsole())},
	{CORE_LIB_NAMES[CoreLibs::CL_DATETIME], std::shared_ptr<modules::ModuleDateTime>(new modules::ModuleDateTime())},
	{CORE_LIB_NAMES[CoreLibs::CL_INPUT], std::shared_ptr<modules::ModuleInput>(new modules::ModuleInput())},
	{CORE_LIB_NAMES[CoreLibs::CL_SOUND], std::shared_ptr<modules::ModuleSound>(new modules::ModuleSound())},
	{CORE_LIB_NAMES[CoreLibs::CL_HTTP], std::shared_ptr<modules::ModuleHTTP>(new modules::ModuleHTTP())},
	{CORE_LIB_NAMES[CoreLibs::CL_SYS], std::shared_ptr<modules::ModuleSys>(new modules::ModuleSys())},
	{CORE_LIB_NAMES[CoreLibs::CL_OS], std::shared_ptr<modules::ModuleOS>(new modules::ModuleOS())}
};
