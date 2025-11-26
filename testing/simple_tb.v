`timescale 1ns / 1ps
`include "simple.v"

module simple_tb ();
    reg clk;

    reg [7:0] in_word;
    wire [7:0] out_word;

    simple dut (
        .clock (clk),
        .data_in (in_word),
        .data_out (out_word)
    );

    initial begin
        clk = 0;
        forever begin
            #1 clk = ~clk;
        end
    end

    initial begin
        $dumpfile("signals.vcd"); // Name of the signal dump file
        $dumpvars(0); // Signals to dump

        in_word = 8'd32;
        #2;
        in_word = 8'd28;
        #8;
        in_word = 8'd109;
        #2;
        in_word = 8'd111;
        #16;
        in_word = 8'd1;
        #6;
        in_word = 8'd74;
        #40;
        in_word = 8'd0;
        #2;
        in_word = -8'd15;
        #2;
        in_word = 8'd221;
        #2;


        $finish;

    end

endmodule