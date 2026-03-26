`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 02/01/2026 06:10:59 PM
// Design Name: 
// Module Name: dense_mult_tb
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


module dense_mult_tb(

    );
    localparam N = 4;
    localparam DATA_WIDTH = 8;
    localparam OUTPUT_WIDTH = 16;

    logic clk;
    logic rst_n;
    logic [DATA_WIDTH-1:0] a_in_bus [0:2*N-2]; 
    logic valid_bit_a_in [0:2*N-2];
    logic [DATA_WIDTH-1:0] b_in_bus [0:2*N-2];
    logic valid_bit_b_in [0:2*N-2];
    
    logic [OUTPUT_WIDTH-1:0] s_out_bus [0:2*N-2];
    logic [0:2*N-2]valid_bit_s_out;

    // Instantiate the Unit Under Test (UUT)
    dense_mult #(N, DATA_WIDTH, OUTPUT_WIDTH) uut (
        .clk(clk),
        .rst_n(rst_n),
        .a_in_bus(a_in_bus),
        .valid_bit_a_in(valid_bit_a_in),
        .b_in_bus(b_in_bus),
        .valid_bit_b_in(valid_bit_b_in),
        .s_out_bus(s_out_bus),
        .valid_bit_s_out(valid_bit_s_out)
    );

    // Clock Generation
    initial begin
        clk = 0;
        forever #5 clk = ~clk;
    end

    // Task to clear inputs
    task clear_inputs();
        for (int i = 0; i <= 2*N-2; i++) begin
            a_in_bus[i] = 0;
            valid_bit_a_in[i] = 0;
            b_in_bus[i] = 0;
            valid_bit_b_in[i] = 0;
        end
    endtask

    initial begin
        // Initialize
        rst_n = 0;
        clear_inputs();
        
        // Reset sequence
        #20 rst_n = 1;
        @(posedge clk);

        // --- Staggered Data Injection ---
        // T1: a11, b11
        a_in_bus[0] = 1; valid_bit_a_in[0] = 1;
        a_in_bus[1] = 2; valid_bit_a_in[1] = 1;
        a_in_bus[2] = 3; valid_bit_a_in[2] = 1;
        a_in_bus[3] = 4; valid_bit_a_in[3] = 1;
        a_in_bus[4] = 0; valid_bit_a_in[4] = 1;
        a_in_bus[5] = 0; valid_bit_a_in[5] = 1;
        a_in_bus[6]=  0; valid_bit_a_in[6] = 1;
        b_in_bus[0] = 1; valid_bit_b_in[0] = 1;
        b_in_bus[1] = 0; valid_bit_b_in[1] = 1;
        b_in_bus[2] = 0; valid_bit_b_in[2] = 1;
        b_in_bus[3] = 0; valid_bit_b_in[3] = 1;
        b_in_bus[4] = 0; valid_bit_b_in[4] = 1;
        b_in_bus[5] = 0; valid_bit_b_in[5] = 1;
        b_in_bus[6] = 0; valid_bit_b_in[6] = 1;
        @(posedge clk);

        // T2: a21, a12 | b12, b21
        a_in_bus[0] = 0; valid_bit_a_in[0] = 1;
        a_in_bus[1] = 5; valid_bit_a_in[1] = 1;
        a_in_bus[2] = 6; valid_bit_a_in[2] = 1;
        a_in_bus[3] = 7; valid_bit_a_in[3] = 1;
        a_in_bus[4] = 8; valid_bit_a_in[4] = 1;
        a_in_bus[5] = 0; valid_bit_a_in[5] = 1;
        a_in_bus[6]=  0; valid_bit_a_in[6] = 1;
        b_in_bus[0] = 0; valid_bit_b_in[0] = 1;
        b_in_bus[1] = 0; valid_bit_b_in[1] = 1;
        b_in_bus[2] = 1; valid_bit_b_in[2] = 1;
        b_in_bus[3] = 0; valid_bit_b_in[3] = 1;
        b_in_bus[4] = 0; valid_bit_b_in[4] = 1;
        b_in_bus[5] = 0; valid_bit_b_in[5] = 1;
        b_in_bus[6] = 0; valid_bit_b_in[6] = 1;
        @(posedge clk);

        // T3: a31, a22, a13 | b13, b22, b31
        a_in_bus[0] = 0; valid_bit_a_in[0] = 1;
        a_in_bus[1] = 0; valid_bit_a_in[1] = 1;
        a_in_bus[2] = 9; valid_bit_a_in[2] = 1;
        a_in_bus[3] = 10; valid_bit_a_in[3] = 1;
        a_in_bus[4] = 11; valid_bit_a_in[4] = 1;
        a_in_bus[5] = 12; valid_bit_a_in[5] = 1;
        a_in_bus[6]=  0; valid_bit_a_in[6] = 1;
        b_in_bus[0] = 0; valid_bit_b_in[0] = 1;
        b_in_bus[1] = 0; valid_bit_b_in[1] = 1;
        b_in_bus[2] = 0; valid_bit_b_in[2] = 1;
        b_in_bus[3] = 0; valid_bit_b_in[3] = 1;
        b_in_bus[4] = 1; valid_bit_b_in[4] = 1;
        b_in_bus[5] = 0; valid_bit_b_in[5] = 1;
        b_in_bus[6] = 0; valid_bit_b_in[6] = 1;
        
        @(posedge clk);
        a_in_bus[0] = 0; valid_bit_a_in[0] = 1;
        a_in_bus[1] = 0; valid_bit_a_in[1] = 1;
        a_in_bus[2] = 0; valid_bit_a_in[2] = 1;
        a_in_bus[3] = 13; valid_bit_a_in[3] = 1;
        a_in_bus[4] = 14; valid_bit_a_in[4] = 1;
        a_in_bus[5] = 15; valid_bit_a_in[5] = 1;
        a_in_bus[6]=  16; valid_bit_a_in[6] = 1;
        b_in_bus[0] = 0; valid_bit_b_in[0] = 1;
        b_in_bus[1] = 0; valid_bit_b_in[1] = 1;
        b_in_bus[2] = 0; valid_bit_b_in[2] = 1;
        b_in_bus[3] = 0; valid_bit_b_in[3] = 1;
        b_in_bus[4] = 0; valid_bit_b_in[4] = 1;
        b_in_bus[5] = 0; valid_bit_b_in[5] = 1;
        b_in_bus[6] = 1; valid_bit_b_in[6] = 1;
        // End of data
        @(posedge clk);
        clear_inputs();
    
        // Wait for the systolic wave to reach the end
        repeat (10) @(posedge clk);
        
       // $display("Test Complete. Check waveforms for result emergence.");
        $finish;
    end

endmodule


