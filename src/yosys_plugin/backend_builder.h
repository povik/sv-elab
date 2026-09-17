//
// Yosys slang frontend
//
// Copyright Martin Povišer <povik@cutebit.org>
// Distributed under the terms of the ISC license, see LICENSE
//
#pragma once

#include "slang_frontend.h"

namespace slang_frontend {

struct BackendGraphBuilder : BackendGraphBuilderBase
{
	const slang::SourceManager *source_mgr;
	RTLIL::Module *canvas = nullptr;
	Yosys::dict<RTLIL::IdString, RTLIL::Const> staged_attributes;
	// Source ranges are kept unformatted until bless_cell() actually emits a
	// cell; many expression leaves never need an `src` string.
	slang::SourceRange staged_source_range;
	bool staged_source_range_valid = false;

	unsigned next_id = 0;
	std::string new_id(std::string base = std::string());

	ir::Value Unop(ast::UnaryOperator op, ir::Value a, bool a_signed, uint64_t y_width) override;
	ir::Value Biop(ast::BinaryOperator op, ir::Value a, ir::Value b, bool a_signed, bool b_signed,
			uint64_t y_width) override;
	ir::Value Demux(ir::Value a, ir::Value s) override;
	ir::Value Bwmux(ir::Value a, ir::Value b, ir::Value s) override;
	ir::Value Bmux(ir::Value a, ir::Value s) override;
	ir::Value Shift(ir::Value a, ir::Value s, bool s_signed, uint64_t result_width) override;
	ir::Value Shiftx(ir::Value a, ir::Value s, bool s_signed, uint64_t result_width) override;
	ir::Value Mux(ir::Value a, ir::Value b, ir::Net s) override;

	ir::Value add_placeholder_signal(uint64_t width, std::string_view name_suggestion = ""sv,
			bool public_name = false) override;
	void connect(ir::Value target, ir::Value source) override;
	void set_initialization(ir::Value signal, ir::Const init_value) override;
	void add_memory_init(
			std::string_view name, uint64_t bit_offset, bool big_endian, ir::Const data) override;
	void add_dual_edge_aldff(const std::string &base_name, ir::Net clk, ir::Net aload,
			const ir::Value &d, const ir::Value &q, const ir::Value &ad,
			bool aload_polarity) override;
	void add_dff(std::string_view name, const ir::Net clk, const ir::Value &d, const ir::Value &q,
			bool clk_polarity = true) override;
	void add_dffe(std::string_view name, const ir::Net clk, const ir::Net en, const ir::Value &d,
			const ir::Value &q, bool clk_polarity = true, bool en_polarity = true) override;
	void add_aldff(std::string_view name, const ir::Net clk, const ir::Net aload,
			const ir::Value &d, const ir::Value &q, const ir::Value &ad, bool clk_polarity = true,
			bool aload_polarity = true) override;
	void add_aldffe(std::string_view name, const ir::Net clk, const ir::Net en, const ir::Net aload,
			const ir::Value &d, const ir::Value &q, const ir::Value &ad, bool clk_polarity = true,
			bool en_polarity = true, bool aload_polarity = true) override;
	std::unique_ptr<BackendGraphBuilder> start_new_graph(std::string_view graph_name);
	void finalize();

private:
	ir::Value UnopInternal(RTLIL::IdString op, ir::Value a, bool a_signed, uint64_t y_width);
	ir::Value BiopInternal(RTLIL::IdString op, ir::Value a, ir::Value b, bool a_signed,
			bool b_signed, uint64_t y_width);

	int meminit_prio_counter = 0;
	void emit_meminit_cell(RTLIL::Memory *mem, uint64_t word_offset, bool big_endian,
			ir::Const data, ir::Const mask);

	std::pair<std::string, ir::Value> add_y_wire(int width);
	// apply attributes to newly created cell
	void bless_cell(RTLIL::Cell *cell);
};

class AttributeGuard
{
public:
	AttributeGuard(GraphBuilder &graph_builder) : builder(*graph_builder.backend)
	{
		save.swap(builder.staged_attributes);
		save_source_range = builder.staged_source_range;
		save_source_range_valid = builder.staged_source_range_valid;
		builder.staged_source_range_valid = false;
	}

	~AttributeGuard()
	{
		save.swap(builder.staged_attributes);
		builder.staged_source_range = save_source_range;
		builder.staged_source_range_valid = save_source_range_valid;
	}

	void set(RTLIL::IdString id, RTLIL::Const value) { builder.staged_attributes[id] = value; }

	void set_source(slang::SourceRange source_range)
	{
		builder.staged_source_range = source_range;
		builder.staged_source_range_valid = true;
	}

private:
	BackendGraphBuilder &builder;
	Yosys::dict<RTLIL::IdString, RTLIL::Const> save;
	slang::SourceRange save_source_range;
	bool save_source_range_valid = false;
};

} // namespace slang_frontend
