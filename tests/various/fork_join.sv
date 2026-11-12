// issue 183: fork-join blocks are not synthesizable
module top(input logic a, output logic b);
	assign b = a;
	initial fork
		$display("x");
	join
endmodule
