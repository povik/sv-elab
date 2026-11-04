//
// sv-elab
//
// Copyright Martin Povišer <povik@cutebit.org>
// Distributed under the terms of the ISC license, see LICENSE
//

#include "slang/ast/ASTSerializer.h"

#include "slang_frontend.h"

namespace slang_frontend {

const std::optional<RTLIL::Const> convert_const_rtlil(
		NetlistContext &netlist, const slang::ConstantValue &constval, slang::SourceLocation loc);

};
