`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 02/02/2026 02:08:58 PM
// Design Name: 
// Module Name: top_level_file
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


module top_level_file #(
    parameter DATA_WIDTH=8,
    parameter OUTPUT_WIDTH=16,
    parameter N=3
    )(
    input logic clk,
    input logic rst_n,
    input logic [DATA_WIDTH-1:0] a[0:N-1][0:N-1],
    input logic valid_bit_a_in[0:N-1][0:N-1],
    input logic [DATA_WIDTH-1:0] b[0:N-1][0:N-1],
    input logic valid_bit_b_in [0:N-1][0:N-1],
    output logic [OUTPUT_WIDTH-1:0] c[0:N-1][0:N-1],
    output logic valid_bit_out
  );
    
    logic [DATA_WIDTH-1:0]a_staggered_output[0:2*N-2];
    logic valid_bit_a_staggered_output[0:2*N-2];
    logic [DATA_WIDTH-1:0]b_staggered_output[0:2*N-2];
    logic valid_bit_b_staggered_output[0:2*N-2];
    logic [OUTPUT_WIDTH-1:0] s_out_staggered[0:2*N-2];
    logic [0:2*N-2] valid_bit_out_staggered;
    
    
    matrix_row_shifter #(
        .N(N),
        .DATA_WIDTH(8)
    ) row_shifter(
        .clk(clk),
        .rst_n(rst_n),
        .row_sel(1'b1),
        .valid_bits_in(valid_bit_a_in),
        .matrix(a),
        .out_data(a_staggered_output),
        .valid_bits_out(valid_bit_a_staggered_output)
    );
    
    matrix_row_shifter #(
        .N(N),
        .DATA_WIDTH(8)
    ) col_shifter(
        .clk(clk),
        .rst_n(rst_n),
        .row_sel(1'b0),
        .valid_bits_in(valid_bit_b_in),
        .matrix(b),
        .out_data(b_staggered_output),
        .valid_bits_out(valid_bit_b_staggered_output)
    );
    
    dense_mult #(
        .N(N),
        .DATA_WIDTH(8),
        .OUTPUT_WIDTH(16)
    )   sys_array(
        .clk(clk),
        .rst_n(rst_n),
        .a_in_bus(a_staggered_output),
        .valid_bit_a_in(valid_bit_a_staggered_output),
        .b_in_bus(b_staggered_output),
        .valid_bit_b_in(valid_bit_b_staggered_output),
        .s_out_bus(s_out_staggered),
        .valid_bit_s_out(valid_bit_out_staggered)
    );
    
    output_shift_register #(
        .N(N),
        .OUTPUT_WIDTH(16)
    )   output_shifter(
        .clk(clk),
        .rst_n(rst_n),
        .data_in(s_out_staggered),
        .valid_bit_in(valid_bit_out_staggered),
        .data_out(c),
        .valid_bit_out(valid_bit_out)
    );
endmodule
