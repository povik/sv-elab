// Simple rederive
module top #(parameter VALUE = 0) (output y);
    assign y = VALUE;
endmodule

// Rederive with interfaces
interface data_bus;
    logic data, result;
    modport sink (input data, output result);
endinterface

module consumer #(parameter INVERT = 0) (data_bus.sink bus);
    assign bus.result = INVERT ? ~bus.data : bus.data;
endmodule

module interfaces #(parameter INVERT = 0) (input data, output result);
    data_bus bus();
    consumer #(.INVERT(INVERT)) u (bus);
    assign bus.data = data;
    assign result = bus.result;
endmodule

