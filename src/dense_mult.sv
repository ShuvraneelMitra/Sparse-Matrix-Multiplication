`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 01/29/2026 07:49:31 PM
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


module dense_mult #(
    parameter N = 3,      // Grid size (N x N)
    parameter DATA_WIDTH = 8, // Data bit-width
    parameter OUTPUT_WIDTH =16
)(
    input  logic clk,
    input  logic rst_n,
    
    // Inputs from the bottom (a values) - N columns
    input  logic [DATA_WIDTH-1:0] a_in_bus [0:2*N-2], 
    input logic  valid_bit_a_in [0:2*N-2],
    // Inputs from the left (b values) - N rows
    input  logic [DATA_WIDTH-1:0] b_in_bus [0:2*N-2],
    input logic valid_bit_b_in[0:2*N-2],
    // Outputs (Final accumulated values)
    output logic [OUTPUT_WIDTH-1:0] s_out_bus [0:2*N-2],
    output logic [0:2*N-2] valid_bit_s_out
);

    // Internal wires with generic dimensions
    // a_wire: N+1 rows, N columns (data flows up)
    logic [DATA_WIDTH-1:0] a_wire [0:2*N-1][0:2*N-2];
    logic valid_bit_a_wire [0:2*N-1][0:2*N-2];
    // b_wire: N rows, N+1 columns (data flows right)
    logic [DATA_WIDTH-1:0] b_wire [0:2*N-2][0:2*N-1];
    logic valid_bit_b_wire [0:2*N-2][0:2*N-1];
    // c_wire: N+1 rows, N+1 columns (data flows diagonally)
    logic [OUTPUT_WIDTH-1:0] c_wire [0:2*N-1][0:2*N-1];
    logic valid_bit_c_wire [0:2*N-1][0:2*N-1];
    // --- Boundary Assignments ---
    generate
        for (genvar i = 0; i < 2*N-1; i++) begin : boundaries
            assign a_wire[0][i] = a_in_bus[i]; // Bottom inputs
            assign valid_bit_a_wire[0][i]=valid_bit_a_in[i];
            assign b_wire[i][0] = b_in_bus[i]; // Left inputs
            assign valid_bit_b_wire[i][0]=valid_bit_b_in[i];
            // Initialize the starting diagonal partial sums to 0
            if(i<N && i>0)
            begin
                assign c_wire[i][0] = '0; 
                assign valid_bit_c_wire[i][0]=1'b1;
                assign c_wire[0][i] = '0;
                assign valid_bit_c_wire[0][i]=1'b1;
            end
        end
        assign c_wire[0][0] = '0; // Corner case'
        assign valid_bit_c_wire[0][0]=1'b1;
        
    endgenerate

    // --- PE Grid Generation ---
    generate
        for (genvar r = 0; r < N; r++) begin : row_gen
            for (genvar c = 0; c < N+r; c++) begin : col_gen
                mac_pe #(8,16) pe_inst (
                    .clk   (clk),
                    .rst_n (rst_n),
                    .a_in  (a_wire[r][c]),
                    .valid_bit_a_in(valid_bit_a_wire[r][c]),
                    .b_in  (b_wire[r][c]),
                    .valid_bit_b_in(valid_bit_b_wire[r][c]),
                    .c_in  (c_wire[r][c]),
                    .valid_bit_c_in(valid_bit_c_wire[r][c]),
                    .a_out (a_wire[r+1][c]),
                    .b_out (b_wire[r][c+1]),
                    .s_out (c_wire[r+1][c+1]),
                    .valid_bit_a_out(valid_bit_a_wire[r+1][c]),
                    .valid_bit_b_out(valid_bit_b_wire[r][c+1]),
                    .valid_bit_s_out(valid_bit_c_wire[r+1][c+1])
                );
                
                // Assign the internal c_wire to the result matrix
                // In a real chip, you'd capture this when the "wave" finishes
                //assign result[r][c] = c_wire[r+1][c+1];
            end
        end
        for(genvar r=N;r<2*N-1;r++)begin :row_gen1
            for (genvar c= r-N+1;c<2*N-1;c++) begin: col_gen1
                mac_pe #(8,16) pe_inst1 (
                    .clk   (clk),
                    .rst_n (rst_n),
                    .a_in  (a_wire[r][c]),
                    .valid_bit_a_in(valid_bit_a_wire[r][c]),
                    .b_in  (b_wire[r][c]),
                    .valid_bit_b_in(valid_bit_b_wire[r][c]),
                    .c_in  (c_wire[r][c]),
                    .valid_bit_c_in(valid_bit_c_wire[r][c]),
                    .a_out (a_wire[r+1][c]),
                    .b_out (b_wire[r][c+1]),
                    .s_out (c_wire[r+1][c+1]),
                    .valid_bit_a_out(valid_bit_a_wire[r+1][c]),
                    .valid_bit_b_out(valid_bit_b_wire[r][c+1]),
                    .valid_bit_s_out(valid_bit_c_wire[r+1][c+1])
                );
            end
        end
        
        for(genvar r=0;r<N-1;r++) begin : buffer_row_gen1
            for (genvar c=N+r; c<2*N-1;c++) begin : buffer_col_gen1
                    buffer #(8) b_a(
                        .clk(clk),
                        .rst_n(rst_n),
                        .valid_bit_in(valid_bit_a_wire[r][c]),
                        .data_in(a_wire[r][c]),
                        .data_out(a_wire[r+1][c]),
                        .valid_bit_out(valid_bit_a_wire[r+1][c])        
                    );
            end
        end
        
        for(genvar r=N;r<2*N-1;r++) begin : buffer_row_gen2
            for (genvar c=0; c<r-N+1;c++) begin : buffer_col_gen2
                    buffer #(8) b_a(
                        .clk(clk),
                        .rst_n(rst_n),
                        .valid_bit_in(valid_bit_b_wire[r][c]),
                        .data_in(b_wire[r][c]),
                        .data_out(b_wire[r][c+1]),
                        .valid_bit_out(valid_bit_b_wire[r][c+1])        
                    );
            end
        end
       
        for(genvar c= N;c<=2*N-1;c++) begin: output_col_ass
            assign s_out_bus[c-N]=c_wire[2*N-1][c];
            assign valid_bit_s_out[c-N]=valid_bit_c_wire[2*N-1][c];
        end
        
        for (genvar r=2*N-2;r>=N-1;--r) begin: output_row_ass
            assign s_out_bus[3*N-2-r]=c_wire[r][2*N-1];
            assign valid_bit_s_out[3*N-2-r]=valid_bit_c_wire[r][2*N-1];
        end   
            
    endgenerate
endmodule
