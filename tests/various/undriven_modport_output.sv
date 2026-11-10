interface ifc;
  logic [1:0] x, y;
  modport mp (output x, output y);
endinterface

module user(ifc.mp p);
  assign p.x[0] = 1'b0;
  assign p.y = 2'b0;
endmodule

module top(output logic [1:0] x, output logic [1:0] y);
  ifc p();
  user u(p);
  assign p.x[1] = 1'b1;
  assign x = p.x;
  assign y = p.y;
endmodule

module user2(ifc.mp p);
  assign p.y = 2'b0;
endmodule

module top2(output logic [1:0] x, output logic [1:0] y);
  ifc p();
  user2 u(p);
  assign p.x = 2'b10;
  assign x = p.x;
  assign y = p.y;
endmodule
