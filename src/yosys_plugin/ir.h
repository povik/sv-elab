//
// Yosys slang frontend
//
// Copyright Martin Povišer <povik@cutebit.org>
// Distributed under the terms of the ISC license, see LICENSE
//
// IR abstraction layer: thin wrappers around RTLIL signal types exposing
// only the interface required by the frontend. Consumers targeting a different
// IR can swap out the implementation while keeping the same surface.
//
#pragma once
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <utility>
#include <vector>

#include "kernel/log.h"
#include "kernel/rtlil.h"

namespace slang_frontend {
enum Trit : uint8_t;
class TritVector;
} // namespace slang_frontend

namespace ir {

namespace RTLIL = ::Yosys::RTLIL;

// Convert the frontend's constant representation to an RTLIL constant.
RTLIL::Const to_rtlil(const slang_frontend::TritVector &c);

struct Value;

// Single-bit signal reference
struct Net
{
	RTLIL::SigBit raw_;

	Net() : raw_() {}
	Net(RTLIL::SigBit b) : raw_(b) {}
	Net(slang_frontend::Trit t);

	operator RTLIL::SigBit() const { return raw_; }
	RTLIL::SigBit &raw() { return raw_; }
	const RTLIL::SigBit &raw() const { return raw_; }

	Value repeat(uint64_t width);

	slang_frontend::Trit as_trit() const;

	bool operator==(Net o) const { return raw_ == o.raw_; }
	bool operator!=(Net o) const { return raw_ != o.raw_; }
};

// Proxy returned by Value::operator[] for read and write access
class NetProxy
{
	RTLIL::SigSpec &spec_;
	int idx_;

public:
	NetProxy(RTLIL::SigSpec &s, int i) : spec_(s), idx_(i) {}
	operator Net() const { return spec_[idx_]; }
	NetProxy &operator=(Net n)
	{
		spec_[idx_] = n.raw_;
		return *this;
	}
	NetProxy &operator=(const NetProxy &o)
	{
		spec_[idx_] = o.spec_[o.idx_];
		return *this;
	}
	bool operator==(Net n) const { return Net(spec_[idx_]) == n; }
	bool operator!=(Net n) const { return Net(spec_[idx_]) != n; }
	slang_frontend::Trit as_trit() const { return Net(spec_[idx_]).as_trit(); }
};

// Multi-bit signal
struct Value
{
	RTLIL::SigSpec raw_;

	// Construction
	Value() {}
	Value(RTLIL::SigSpec s) : raw_(std::move(s)) {}
	Value(Net bit) : raw_(bit.raw_) {}
	Value(RTLIL::SigBit bit) : raw_(bit) {}
	Value(const slang_frontend::TritVector &c);
	Value(RTLIL::Const c) : raw_(std::move(c)) {}
	Value(RTLIL::Wire *wire) : raw_(wire) {}

	// Concatenation (MSB-first, matching RTLIL::SigSpec and Verilog semantics)
	Value(std::initializer_list<Value> parts)
	{
		for (auto it = parts.end(); it != parts.begin();)
			raw_.append((--it)->raw_);
	}
	Value(std::initializer_list<Net> parts)
	{
		for (auto it = parts.end(); it != parts.begin();)
			raw_.append((--it)->raw_);
	}

	operator RTLIL::SigSpec() const { return raw_; }
	RTLIL::SigSpec &raw() { return raw_; }
	const RTLIL::SigSpec &raw() const { return raw_; }

	// Sizing
	uint64_t size() const { return raw_.size(); }
	uint64_t width() const { return raw_.size(); }
	bool empty() const { return raw_.empty(); }

	// Element access
	Net operator[](uint64_t i) const { return raw_[i]; }
	NetProxy operator[](uint64_t i) { return NetProxy(raw_, i); }

	// Extraction / manipulation
	Value extract(uint64_t offset, uint64_t width) const { return raw_.extract(offset, width); }
	Value extract_end(uint64_t offset) const { return raw_.extract_end(offset); }
	void append(Net bit) { raw_.append(bit.raw_); }
	void append(const NetProxy &p) { raw_.append(Net(p).raw_); }
	void append(slang_frontend::Trit t);
	void append(Value other) { raw_.append(other.raw_); }
	void remove(uint64_t index, uint64_t width = 1) { raw_.remove(index, width); }
	Value repeat(uint64_t count) const { return raw_.repeat(count); }
	void extend_u0(uint64_t width, bool is_signed = false) { raw_.extend_u0(width, is_signed); }
	Net msb() const { return raw_.msb(); }
	void reserve(uint64_t) { /* not available on all SigSpec implementations */ }

	// Queries
	bool is_fully_const() const { return raw_.is_fully_const(); }
	bool is_fully_def() const { return raw_.is_fully_def(); }
	bool is_fully_zero() const { return raw_.is_fully_zero(); }
	bool is_fully_ones() const { return raw_.is_fully_ones(); }
	bool is_wire() const { return raw_.is_wire(); }

	// Conversion
	Net as_net() const
	{
		log_assert(raw_.size() == 1);
		return raw_[0];
	}
	slang_frontend::TritVector as_const() const;
	bool as_bool() const { return raw_.as_bool(); }

	// For code that needs the raw chunks (e.g. initialization.cc wire access)
	auto chunks() const { return raw_.chunks(); }

	// Iteration (yields RTLIL::SigBit; auto-converts to Net via implicit ctor)
	auto begin() const { return raw_.begin(); }
	auto end() const { return raw_.end(); }

	// Comparison
	bool operator==(const Value &o) const { return raw_ == o.raw_; }
	bool operator!=(const Value &o) const { return raw_ != o.raw_; }
};

} // namespace ir
