`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company:
// Engineer:
//
// Create Date: 04.02.2026 15:29:59
// Design Name:
// Module Name: matrix_row_shifter
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


module matrix_row_shifter #(
    parameter N = 3,
    parameter DATA_WIDTH = 8,
    parameter SHIFT_LEN = 2*N - 1
)(
    input  logic clk,
    input  logic rst_n,
    input  logic row_sel,

    input  logic valid_bits_in [0:N-1][0:N-1],
    input  logic [DATA_WIDTH-1:0] matrix [0:N-1][0:N-1],

    output logic [DATA_WIDTH-1:0] out_data [0:SHIFT_LEN-1],
    output logic valid_bits_out [0:SHIFT_LEN-1] 
);

    

    logic [DATA_WIDTH-1:0] shift_reg [0:SHIFT_LEN-1];
    logic valid_shift_reg [0:SHIFT_LEN-1];
    logic [$clog2(N)-1:0] state;
    integer i;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= 0;
            valid_shift_reg <= '{SHIFT_LEN{1'b0}};
        end
        else begin

            /* Clear entire register */
            for (i = 0; i < SHIFT_LEN; i++) begin
                shift_reg[i] <= '0;
                valid_shift_reg[i] <= 1'b1;
            end

            /* Inject data depending on row/column mode */
            for (i = 0; i < N; i++) begin

                if (row_sel) begin
                    shift_reg[state + i] <= matrix[state][i];
                    valid_shift_reg[state + i] <= valid_bits_in[state][i];
                end
                else begin
                    shift_reg[state + i] <= matrix[i][state];
                    valid_shift_reg[state + i] <= valid_bits_in[i][state];
                end

            end

            /* State update */
            if (state == N-1)
                state <= 0;
            else
                state <= state + 1;

        end
    end

    assign out_data = shift_reg;
    assign valid_bits_out = valid_shift_reg;

endmodule
