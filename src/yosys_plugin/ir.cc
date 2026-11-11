//
// Yosys slang frontend
//
// Copyright Martin Povišer <povik@cutebit.org>
// Distributed under the terms of the ISC license, see LICENSE
//

#include "ir.h"

#include <cstdint>
#include <vector>

#include "slang_frontend.h"

namespace ir {

static RTLIL::State to_rtlil(slang_frontend::Trit t)
{
	switch (t) {
	case slang_frontend::S0:
		return RTLIL::S0;
	case slang_frontend::S1:
		return RTLIL::S1;
	default:
		return RTLIL::Sx;
	}
}

static slang_frontend::Trit from_rtlil(RTLIL::State s)
{
	switch (s) {
	case RTLIL::S0:
		return slang_frontend::S0;
	case RTLIL::S1:
		return slang_frontend::S1;
	default:
		return slang_frontend::Sx;
	}
}

Net::Net(slang_frontend::Trit t) : raw_(to_rtlil(t)) {}

slang_frontend::Trit Net::as_trit() const
{
	return from_rtlil(raw_.data);
}

Value::Value(const slang_frontend::TritVector &c)
{
	std::vector<RTLIL::State> states;
	states.reserve(c.size());
	for (auto t : c)
		states.push_back(to_rtlil(t));
	raw_ = RTLIL::Const(std::move(states));
}

void Value::append(slang_frontend::Trit t)
{
	raw_.append(RTLIL::SigBit(to_rtlil(t)));
}

slang_frontend::TritVector Value::as_const() const
{
	std::vector<slang_frontend::Trit> bits;
	auto c = raw_.as_const();
	bits.reserve(c.size());
	for (auto s : c)
		bits.push_back(from_rtlil(s));
	return slang_frontend::TritVector(bits);
}

RTLIL::Const to_rtlil(const slang_frontend::TritVector &c)
{
	std::vector<RTLIL::State> states;
	states.reserve(c.size());
	for (auto t : c)
		states.push_back(to_rtlil(t));
	return RTLIL::Const(std::move(states));
}

Value Net::repeat(uint64_t width)
{
	return Value(*this).repeat(width);
}

}; // namespace ir
