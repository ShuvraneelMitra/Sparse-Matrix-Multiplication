`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 01/29/2026 08:50:07 PM
// Design Name: 
// Module Name: mac_pe_tb
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


module mac_pe_tb(

    );
localparam DATA_WIDTH=8;
localparam OUTPUT_WIDTH=16;

logic clk,rst_n,valid_bit_in;
logic [DATA_WIDTH-1:0] a_in,b_in;
logic [OUTPUT_WIDTH-1:0] c_in;

logic [DATA_WIDTH-1:0] a_out,b_out;
logic [OUTPUT_WIDTH-1:0] s_out;
logic valid_bit_out;
mac_pe dut(.clk(clk),.a_in(a_in),.b_in(b_in),.c_in(c_in),.rst_n(rst_n),.valid_bit_in(valid_bit_in),.a_out(a_out),.b_out(b_out),.s_out(s_out),.valid_bit_out(valid_bit_out));
initial
begin
rst_n=1'b0;
clk=1'b0;
valid_bit_in=1'b1;
#9;
rst_n=1'b1;
a_in=2;
b_in=5;
c_in=3;
#3;
a_in=0;
b_in=2;
c_in=13;
#3;
valid_bit_in=1'b0;
a_in=1;
c_in=0;
#5;;
$finish;
end
always #2 clk=~clk;
endmodule
