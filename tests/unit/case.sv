module top(input logic [3:0] w);
	logic [1:0] m1, m2;

	always_comb
	casez (w)
		4'b?00?: m1 = 0;
		4'b??10: m1 = 1;
		4'b010?: m1 = 2;
		4'b11??, 4'b??11: m1 = 3;
	endcase

	always_comb
	casex (w)
		4'bx00?: m2 = 0;
		4'b?x10: m2 = 1;
		4'b010x: m2 = 2;
		default: m2 = 3;
	endcase

	always_comb
	if (^w !== 'x) begin
		assert(m1 === m2);
		if (w[2:1] == '0)
			assert(m1 === 0);
		else if (w[1:0] == 2'b10)
			assert(m1 === 1);
		else if (w[3:1] == 3'b010)
			assert(m1 === 2);
		else
			assert(m1 === 3);
	end
endmodule

module top2(input logic [1:0] sel, output logic [1:0] y);
	always_comb begin
		y = 2'd3;
		casez ({1'b0, sel})
			3'b000: y = 2'd0;
			3'b?10: y = 2'd1;
		endcase
	end

	always_comb
	if (^sel !== 'x) begin
		if (sel == 2)
			assert(y === 1);
		else if (sel == 0)
			assert(y === 0);
		else
			assert(y === 3);
	end
endmodule

module top3(input logic [1:0] sel, output logic [1:0] y);
	// must not cause a logic loop
	always_comb
	unique case (sel)
		2'd0: y = 0;
		2'd1: y = 1;
		2'd3: y = 3;
	endcase
endmodule
