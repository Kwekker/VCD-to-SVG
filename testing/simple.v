`timescale 1 ps / 1 ps
module simple (
        input wire clock,
		input  wire [7:0] data_in,
        output wire [7:0] data_out
	);

    reg [7:0] store;

    assign data_out = 8'd15; //store + 8'd15 + data_in;

    always @(posedge clock) begin
        store <= data_in;
    end

endmodule