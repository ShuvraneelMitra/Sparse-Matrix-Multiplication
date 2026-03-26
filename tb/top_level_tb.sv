`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 02/02/2026 03:00:55 PM
// Design Name: 
// Module Name: top_level_tb
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


module top_level_tb(

    );
    localparam N=5;
    localparam DATA_WIDTH=8;
    localparam OUTPUT_WIDTH=16;
    
    
    logic clk;
    logic rst_n;
    logic [DATA_WIDTH-1:0] a[0:N-1][0:N-1];
    logic valid_bit_a_in[0:N-1][0:N-1];
    logic [DATA_WIDTH-1:0] b[0:N-1][0:N-1];
    logic valid_bit_b_in [0:N-1][0:N-1];
    logic [OUTPUT_WIDTH-1:0] c[0:N-1][0:N-1];
    logic valid_bit_out;


    
    top_level_file #(
        .DATA_WIDTH(8),
        .OUTPUT_WIDTH(16),
        .N(N)
    ) dut(
        .clk(clk),
        .rst_n(rst_n),
        .a(a),
        .valid_bit_a_in(valid_bit_a_in),
        .b(b),
        .valid_bit_b_in(valid_bit_b_in),
        .c(c),
        .valid_bit_out(valid_bit_out)
    );
    
    initial begin
        clk = 0;
        forever #5 clk = ~clk;
    end
    
    task clear_inputs();
        
            a = '{default: '0};
            valid_bit_a_in= '{default: '0};
            b = '{default: '0};
            valid_bit_b_in= '{default: '0};
      
    endtask
    
    initial begin
        // Initialize
        rst_n = 0;
        clear_inputs();
        
        // Reset sequence
        #20 rst_n = 1;
        @(posedge clk);
        
        a='{'{1,2,3,4,5},'{6,7,8,9,10},'{11,12,13,14,15},'{16,17,18,19,20},'{21,22,23,24,25}};
        valid_bit_a_in='{'{1,1,1,1,1},'{1,1,1,1,1},'{1,1,1,1,1},'{1,1,1,1,1},'{1,1,1,1,1}};
        b='{'{25,24,23,22,21},'{20,19,18,17,16},'{15,14,13,12,11},'{10,9,8,7,6},'{5,4,3,2,1}};
        valid_bit_b_in='{'{1,1,1,1,1},'{1,1,1,1,1},'{1,1,1,1,1},'{1,1,1,1,1},'{1,1,1,1,1}};
        
        for(int i=0;i<N;++i)
        begin
            @(posedge clk);
        end
       
        // End of data
        clear_inputs();
    
        // Wait for the systolic wave to reach the end
        repeat (10) @(posedge clk);
        $finish;
    end
endmodule
