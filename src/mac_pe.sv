`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 01.02.2026 18:47:06
// Design Name: 
// Module Name: mac_pe
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


module mac_pe #(
    parameter DATA_WIDTH=8,
    parameter OUTPUT_WIDTH=32
)(
    input logic clk,
    input logic rst_n,
   
    input logic [DATA_WIDTH-1:0] a_in,
    input logic [DATA_WIDTH-1:0] b_in,
    input logic [OUTPUT_WIDTH-1:0] c_in,
    input logic valid_bit_a_in,
    input logic valid_bit_b_in,
    input logic valid_bit_c_in,
       
    output logic [OUTPUT_WIDTH-1:0] s_out,
    output logic [DATA_WIDTH-1:0] a_out,
    output logic [DATA_WIDTH-1:0] b_out,
    output logic valid_bit_a_out,
    output logic valid_bit_b_out,
    output logic valid_bit_s_out
);
   logic [DATA_WIDTH-1:0] a_reg;
   logic [DATA_WIDTH-1:0] b_reg;
   logic [OUTPUT_WIDTH-1:0] s_reg;
   logic valid_bit_a_reg;
   logic valid_bit_b_reg;
   logic valid_bit_s_reg;
   always_ff@(posedge clk)
   begin
        if(~rst_n)
        begin
            a_reg<='b0;
            b_reg<='b0;
            s_reg<='b0;
            valid_bit_a_reg<=1'b0;
            valid_bit_b_reg<=1'b0;
            valid_bit_s_reg<=1'b0;
        end
        else
        begin
            a_reg<=a_in;
            b_reg<=b_in;
            valid_bit_a_reg<=valid_bit_a_in;
            valid_bit_b_reg<=valid_bit_b_in;
            valid_bit_s_reg<=valid_bit_a_in & valid_bit_b_in & valid_bit_c_in;
            if(valid_bit_a_in & valid_bit_b_in & valid_bit_c_in)
            begin
                s_reg<= c_in + a_in*b_in;
            end
            else
            begin
                s_reg<=c_in;
            end
        end
    end
   
    assign a_out=a_reg;
    assign b_out=b_reg;
    assign s_out=s_reg;
    assign valid_bit_a_out=valid_bit_a_reg;
    assign valid_bit_b_out=valid_bit_b_reg;
    assign valid_bit_s_out=valid_bit_s_reg;
endmodule
