//
// sv-elab
//
// Copyright Martin Povišer <povik@cutebit.org>
// Distributed under the terms of the ISC license, see LICENSE
//
#pragma once

#include "slang/driver/Driver.h"
#include "slang/ast/ASTSerializer.h"

#include "slang_frontend.h"

#include <memory>
#include <optional>

namespace slang_frontend {

const std::optional<RTLIL::Const> convert_const_rtlil(
		NetlistContext &netlist, const slang::ConstantValue &constval, slang::SourceLocation loc);

const std::optional<slang::ConstantValue> convert_const_slang(const RTLIL::Const &constval);

struct ElabConfig {
	std::shared_ptr<slang::driver::Driver> driver;
	std::shared_ptr<SynthesisSettings> settings;
};

struct ModuleRename {
	IdString original;
	IdString emitted;
};

void elaborate(RTLIL::Design *design, std::optional<ModuleRename> names,
		ElabConfig& sv_elab_settings);

}; // namespace slang_frontend
