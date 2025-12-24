#ifndef MD_BUILTIN_HPP
#define MD_BUILTIN_HPP

#include <string>
#include <memory>
#include <vector>

#include "module.hpp"

namespace core {

	class StructDefinition;
	class FunctionDefinition;

	namespace modules {

		class ModuleBuiltin : public Module {
		private:
			std::vector<std::shared_ptr<StructDefinition>> struct_decls;
			std::vector<std::shared_ptr<FunctionDefinition>> func_decls;

		public:
			ModuleBuiltin();
			~ModuleBuiltin();

			void register_functions(analysis::SemanticAnalyser* visitor) override;
			void register_functions(runtime::VirtualMachine* vm) override;

		private:
			void build_decls();
		};

	}

}

#endif // !MD_BUILTIN_HPP
