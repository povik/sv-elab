//
// sv-elab
//
// Copyright Martin Povišer <povik@cutebit.org>
// Distributed under the terms of the ISC license, see LICENSE
//
#include "slang_frontend.h"

#include <cassert>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace slang_frontend {

bool TritVector::has_undef() const
{
	for (auto t : bits_) {
		if (t == Trit::Sx)
			return true;
	}
	return false;
}

TritVector TritVector::zeros(uint64_t width)
{
	return TritVector(Trit::S0, width);
}

TritVector TritVector::ones(uint64_t width)
{
	return TritVector(Trit::S1, width);
}

TritVector TritVector::undef(uint64_t width)
{
	return TritVector(Trit::Sx, width);
}

TritVector TritVector::slice(uint64_t offset, uint64_t width) const
{
	assert(offset + width <= (uint64_t)size());
	return TritVector(std::vector<Trit>(bits_.begin() + offset, bits_.begin() + offset + width));
}

TritVector TritVector::concat(const TritVector &other) const
{
	std::vector<Trit> bits;
	bits.reserve(other.bits_.size() + bits_.size());
	bits.insert(bits.end(), other.bits_.begin(), other.bits_.end());
	bits.insert(bits.end(), bits_.begin(), bits_.end());
	return TritVector(bits);
}

TritVector TritVector::not_() const
{
	std::vector<Trit> bits;
	bits.reserve(bits_.size());
	for (auto t : bits_)
		bits.push_back(!t);
	return TritVector(std::move(bits));
}

TritVector TritVector::and_(const TritVector &other) const
{
	assert(width() == other.width());
	std::vector<Trit> bits;
	bits.reserve(bits_.size());
	for (int i = 0; i < width(); i++)
		bits.push_back(bits_[i] & other.bits_[i]);
	return TritVector(std::move(bits));
}

TritVector TritVector::or_(const TritVector &other) const
{
	assert(width() == other.width());
	std::vector<Trit> bits;
	bits.reserve(bits_.size());
	for (int i = 0; i < width(); i++)
		bits.push_back(bits_[i] | other.bits_[i]);
	return TritVector(std::move(bits));
}

TritVector TritVector::xor_(const TritVector &other) const
{
	assert(width() == other.width());
	std::vector<Trit> bits;
	bits.reserve(bits_.size());
	for (int i = 0; i < width(); i++)
		bits.push_back(bits_[i] ^ other.bits_[i]);
	return TritVector(std::move(bits));
}

TritVector TritVector::adc(const TritVector &other, Trit ci) const
{
	assert(width() == other.width());
	std::vector<Trit> result(width() + 1, Trit::Sx);

	if (has_undef() || other.has_undef())
		return TritVector(result);

	Trit carry = ci;
	for (uint64_t i = 0; i < width(); i++) {
		Trit a = (*this)[i], b = other[i];
		Trit ab = a ^ b;
		result[i] = ab ^ carry;
		carry = (a & b) | (carry & ab);
	}
	result[width()] = carry;

	return TritVector(result);
}

Trit TritVector::eq(const TritVector &other) const
{
	assert(width() == other.width());

	for (uint64_t i = 0; i < width(); i++) {
		Trit lhs = (*this)[i], rhs = other[i];
		if (lhs != Trit::Sx && rhs != Trit::Sx && lhs != rhs)
			return Trit::S0;
	}

	if (has_undef() || other.has_undef())
		return Trit::Sx;

	return Trit::S1;
}

Trit TritVector::ult(const TritVector &other) const
{
	assert(width() == other.width());

	if (has_undef() || other.has_undef())
		return Trit::Sx;

	for (int64_t i = width() - 1; i >= 0; i--) {
		Trit lhs = (*this)[i], rhs = other[i];

		if (lhs < rhs)
			return Trit::S1;

		if (lhs > rhs)
			return Trit::S0;
	}

	return Trit::S0;
}

Trit TritVector::slt(const TritVector &other) const
{
	assert(width() == other.width());

	if (has_undef() || other.has_undef())
		return Trit::Sx;

	if (msb() > other.msb())
		return Trit::S1;

	if (msb() < other.msb())
		return Trit::S0;

	for (int64_t i = width() - 2; i >= 0; i--) {
		Trit lhs = (*this)[i], rhs = other[i];

		if (lhs < rhs)
			return Trit::S1;

		if (lhs > rhs)
			return Trit::S0;
	}

	return Trit::S0;
}

Trit TritVector::case_eq(const TritVector &other) const
{
	assert(width() == other.width());
	for (int i = 0; i < width(); i++) {
		if (bits_[i] != other.bits_[i])
			return Trit::S0;
	}
	return Trit::S1;
}

namespace {

// Interpret `amt` as an unsigned shift count in units of `stride` bits. Returns
// false if the count is not fully defined.
bool decode_shift_amount(const TritVector &amt, uint64_t stride, uint64_t &count)
{
	uint64_t value = 0;
	for (int i = 0; i < amt.size(); i++) {
		if (amt[i] == Trit::Sx)
			return false;
		if (amt[i] == Trit::S1) {
			if (i >= 64)
				value = std::numeric_limits<uint64_t>::max();
			else
				value |= uint64_t(1) << i;
		}
	}

	const uint64_t max = std::numeric_limits<uint64_t>::max();
	if (stride != 0 && value > max / stride)
		count = max;
	else
		count = value * stride;

	return true;
}

} // namespace

TritVector TritVector::shl(const TritVector &amt, uint64_t stride) const
{
	uint64_t amount;
	if (!decode_shift_amount(amt, stride, amount))
		return TritVector::undef(width());

	const uint64_t w = width();
	TritVector result = TritVector::zeros(w);
	if (amount >= w)
		return result;

	for (uint64_t i = 0; i + amount < w; i++)
		result.set(i + amount, bits_[i]);
	return result;
}

TritVector TritVector::ushr(const TritVector &amt, uint64_t stride) const
{
	uint64_t amount;
	if (!decode_shift_amount(amt, stride, amount))
		return TritVector::undef(width());

	const uint64_t w = width();
	TritVector result = TritVector::zeros(w);
	if (amount >= w)
		return result;

	for (uint64_t i = 0; i + amount < w; i++)
		result.set(i, bits_[i + amount]);
	return result;
}

TritVector TritVector::sshr(const TritVector &amt, uint64_t stride) const
{
	uint64_t amount;
	if (!decode_shift_amount(amt, stride, amount))
		return TritVector::undef(width());

	const uint64_t w = width();
	if (w == 0)
		return TritVector();

	Trit sign = msb();
	TritVector result = TritVector::zeros(w);
	if (amount >= w) {
		for (uint64_t i = 0; i < w; i++)
			result.set(i, sign);
		return result;
	}

	for (uint64_t i = 0; i < w; i++) {
		if (i + amount < w)
			result.set(i, bits_[i + amount]);
		else
			result.set(i, sign);
	}
	return result;
}

TritVector TritVector::mul(const TritVector &other) const
{
	assert(width() == other.width());

	if (has_undef() || other.has_undef())
		return TritVector::undef(width());

	const uint64_t w = width();
	TritVector result = TritVector::zeros(w);
	for (uint64_t i = 0; i < w; i++) {
		if (other[i] != Trit::S1)
			continue;

		TritVector term = TritVector::zeros(w);
		for (uint64_t j = 0; i + j < w; j++)
			term.set(i + j, bits_[j]);

		result = result.adc(term, Trit::S0).slice(0, w);
	}
	return result;
}

TritVector TritVector::extended(uint64_t width, bool is_signed) const
{
	if (width <= size())
		return slice(0, width);

	Trit pad = (is_signed && !empty()) ? msb() : Trit::S0;
	return TritVector(pad, width - size()).concat(*this);
}

} // namespace slang_frontend
