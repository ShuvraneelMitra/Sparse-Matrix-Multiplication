`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 02/02/2026 11:10:12 AM
// Design Name: 
// Module Name: output_shift_register
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


module output_shift_register #(
    parameter N=3,
    parameter OUTPUT_WIDTH=16
)(
    input logic clk,
    input logic rst_n,
    input logic [OUTPUT_WIDTH-1:0] data_in[0:2*N-2],
    input logic [0:2*N-2] valid_bit_in,
    output logic [OUTPUT_WIDTH-1:0]data_out[0:N-1][0:N-1],
    output logic valid_bit_out
    );
    logic [OUTPUT_WIDTH-1:0]data_out_reg[0:N-1][0:N-1];
    logic valid_bit_out_reg;
    always_ff@(posedge clk)
    begin
        if(~rst_n)
        begin 
            data_out_reg<='{default:'0};
            valid_bit_out_reg<=1'b0;
        end
        else
        begin
            for( int i= 0;i< N; ++i)
            begin
                // Create a temporary mask for the bits [i : 2*N-2-i]
                logic [0:2*N-2] mask;
                mask = '0;
                for (int m = 0; m < 2*N-1; m++) begin
                     if (m >= i && m <= (2*N-2-i)) 
                        mask[m] = 1'b1;
                end

                // Now check if all bits covered by the mask are high in the input
                if ((valid_bit_in & mask) == mask) begin
                    for(int j=0;j<N-i;j++)
                    begin
                        data_out_reg[i][N-1-j]<=data_in[i+j];
                    end
                    for (int k = 1; k < (N - i); k++) 
                    begin
                        data_out_reg[i+k][i] <= data_in[N-1+k];
                    end 
                    if (i == N-1) begin
                        valid_bit_out_reg <= 1'b1;
                    end
                    break;
                end
            end
        end
    end
    assign data_out=data_out_reg;
    assign valid_bit_out=valid_bit_out_reg;
endmodule
