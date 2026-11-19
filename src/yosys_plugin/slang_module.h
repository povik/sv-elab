
#pragma once

#include "driver.h"
#include "kernel/rtlil.h"

#include "slang/numeric/ConstantValue.h"

namespace Yosys {

	struct SlangModule : Yosys::RTLIL::Module {
		explicit SlangModule(RTLIL::Design *design);

		slang_frontend::ElabConfig settings;

		RTLIL::IdString derive(RTLIL::Design *design, const dict<RTLIL::IdString, RTLIL::Const> &parameters, bool mayfail) override;
		RTLIL::IdString derive(RTLIL::Design *design, const dict<RTLIL::IdString, RTLIL::Const> &parameters, const dict<RTLIL::IdString, RTLIL::Module*> &interfaces, const dict<RTLIL::IdString, RTLIL::IdString> &modports, bool mayfail) override;
		RTLIL::Module *clone() const override;
#if YOSYS_TWINES
		RTLIL::Module *clone(RTLIL::Design *dst) const override;
		RTLIL::Module *clone(RTLIL::Design *dst, IdString target_name) const override;
#endif
		void copy_config_into(SlangModule *new_mod) const;
	};

	std::string compute_module_name(std::string name, std::vector<std::string> params);
	const slang::ConstantValue convert_const_slang(const RTLIL::Const &constval);
}
