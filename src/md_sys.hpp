#ifndef MD_SYS_HPP
#define MD_SYS_HPP

#include "module.hpp"

namespace core {

	namespace modules {

		class ModuleSys : public Module {
		public:
			ModuleSys();
			~ModuleSys();

			void register_functions(analysis::SemanticAnalyser* visitor) override;
			void register_functions(runtime::VirtualMachine* vm) override;
		};

	}

}

#endif // !MD_SYS_HPP
