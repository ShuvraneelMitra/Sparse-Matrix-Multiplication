`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 01.02.2026 16:14:43
// Design Name: 
// Module Name: tb
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module tb;
    logic clk, rst_n, row_sel;
    logic [7:0] matrix [0:2][0:2];
    logic [7:0] valid_bits_in[0:2][0:2];
    logic [7:0] out_data [0:4];
    logic [7:0] valid_bits_out [0:4];
    
    matrix_row_shifter #(.N(3), .DATA_WIDTH(8)) dut (
        .clk, .rst_n, .row_sel, .valid_bits_in, .matrix, .out_data, .valid_bits_out
    );
    
    always #5 clk = ~clk;
    
    initial begin
        clk = 0; rst_n = 0; row_sel = 0;
        // Load test matrix [[1,2,3],[4,5,6],[7,8,9]]
        matrix[0] = '{8'd1, 8'd2, 8'd3};
        matrix[1] = '{8'd4, 8'd5, 8'd6};
        matrix[2] = '{8'd7, 8'd8, 8'd9};
        
        valid_bits_in[0] = '{8'd1, 8'd1, 8'd1};
        valid_bits_in[1] = '{8'd1, 8'd1, 8'd1};
        valid_bits_in[2] = '{8'd1, 8'd1, 8'd1};
        
        #10 rst_n = 1;
        #10 row_sel = 0;  // Start shifting
        #10 $display("Cycle 1: %p", out_data);  // [1,2,3,0,0]
        #10 $display("Cycle 2: %p", out_data);  // [0,4,5,6,0]
        #10 $display("Cycle 3: %p", out_data);  // [0,0,7,8,9]
        #100;
        $finish;
    end
endmodule
