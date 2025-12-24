#ifndef MD_OS_HPP
#define MD_OS_HPP

#include "module.hpp"

namespace core {

	namespace modules {

		class ModuleOS : public Module {
		public:
			ModuleOS();
			~ModuleOS();

			void register_functions(analysis::SemanticAnalyser* visitor) override;
			void register_functions(runtime::VirtualMachine* vm) override;
		};

	}

}

#endif // !MD_OS_HPP
