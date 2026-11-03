module undef();	
	// has 1 1-bit anyseq
	wire  single_wire;
	// has 1 1-bit anyseq
	reg   single_reg;

	// has 1 2-bit anyseq
	wire [1:0] bus_wire;

	// has 1 1-bit anyseq
	wire [1:0] bus_wire_partial;
	assign bus_wire_partial[0] = 1'b0;

	// is only initialized and is not driven so has 1 1-bit anyseq
	reg initialized_reg = 1'b1;

	// is driven but not initialized so has 1 1-bit anyseq
	reg driven_reg;
	always @* begin
		driven_reg = 1'b0;
	end

endmodule
