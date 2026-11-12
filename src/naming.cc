//
// Yosys slang frontend
//
// Copyright Martin Povišer <povik@cutebit.org>
// Distributed under the terms of the ISC license, see LICENSE
//
#include "slang/ast/Scope.h"
#include "slang/ast/symbols/VariableSymbols.h"
#include "slang/ast/types/Type.h"
#include <cstdint>
#include <string>

#include "slang_frontend.h"
#include "variables.h"

namespace slang_frontend {

static void subfield_names(VariableChunk chunk, uint64_t type_offset, const ast::Type *type,
		std::string prefix, std::vector<NamedChunk> &ret)
{
	type = &type->getCanonicalType();

	if (!chunk.bitwidth())
		return;

	if (chunk.base >= type_offset + type->getBitstreamWidth())
		return;

	if (chunk.base + chunk.bitwidth() <= type_offset)
		return;

	if (type->isStruct()) {
		const ast::Scope *scope = &type->as<ast::Scope>();

		for (auto &field : scope->template membersOfType<ast::FieldSymbol>()) {
			uint64_t field_offset = bitstream_member_offset(field);
			subfield_names(chunk, type_offset + field_offset, &field.getType(),
					prefix + "." + std::string(field.name), ret);
		}
	} else if (type->isArray() && !type->isSimpleBitVector()) {
		assert_invariant(type->hasFixedRange());
		auto range = type->getFixedRange();
		const ast::Type *el_type = type->getArrayElementType();
		uint64_t stride = el_type->getBitstreamWidth();

		for (int i = range.lower(); i <= range.upper(); i++) {
			uint64_t elem_offset = range.translateIndex(i) * stride;
			subfield_names(chunk, type_offset + elem_offset, el_type,
					prefix + "[" + std::to_string(i) + "]", ret);
		}
	} else {
		uint64_t width = type->getBitstreamWidth();
		uint64_t lo = chunk.base > type_offset ? std::min(chunk.base - type_offset, width) : 0;
		uint64_t hi = chunk.base + chunk.bitwidth() > type_offset
							  ? std::min(chunk.base + chunk.bitwidth() - type_offset, width)
							  : 0;

		assert_invariant(hi > lo);
		if (lo == 0 && hi == width) {
			ret.emplace_back(VariableChunk{chunk.variable, type_offset, width}, prefix);
		} else {
			// Convert the bitstream offsets back to HDL indices.
			auto range = type->getFixedRange();
			int64_t right = range.right;
			auto hdl_index = [&](uint64_t offset) -> int64_t {
				return range.isDescending() ? right + (int64_t)offset : right - (int64_t)offset;
			};

			std::string suffix;
			if (hi - lo == 1)
				suffix = "[" + std::to_string(hdl_index(lo)) + "]";
			else
				suffix = "[" + std::to_string(hdl_index(hi - 1)) + ":" +
						 std::to_string(hdl_index(lo)) + "]";

			ret.emplace_back(
					VariableChunk{chunk.variable, type_offset + lo, hi - lo}, prefix + suffix);
		}
	}
}

std::vector<NamedChunk> generate_subfield_names(VariableChunk chunk, const ast::Type *type)
{
	assert_invariant((bool)chunk.variable);
	assert_invariant(type->isBitstreamType() && type->isFixedSize());
	assert_invariant(chunk.variable.bitwidth() == type->getBitstreamWidth());

	std::vector<NamedChunk> ret;
	subfield_names(chunk, 0, type, "", ret);

	uint64_t sum = 0;
	for (auto pair : ret)
		sum += pair.first.bitwidth();
	assert_invariant(sum == chunk.bitwidth());

	return ret;
}

}; // namespace slang_frontend
