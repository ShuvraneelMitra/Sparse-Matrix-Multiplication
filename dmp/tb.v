`timescale 1ns / 1ps

module tb();

    // Clock and reset
    reg ap_clk;
    reg ap_rst_n;
    wire interrupt;
    
    // AXI Lite control interface
    reg         s_axi_control_AWVALID;
    wire        s_axi_control_AWREADY;
    reg  [7:0]  s_axi_control_AWADDR;
    reg         s_axi_control_WVALID;
    wire        s_axi_control_WREADY;
    reg  [31:0] s_axi_control_WDATA;
    reg  [3:0]  s_axi_control_WSTRB;
    reg         s_axi_control_ARVALID;
    wire        s_axi_control_ARREADY;
    reg  [7:0]  s_axi_control_ARADDR;
    wire        s_axi_control_RVALID;
    reg         s_axi_control_RREADY;
    wire [31:0] s_axi_control_RDATA;
    wire [1:0]  s_axi_control_RRESP;
    wire        s_axi_control_BVALID;
    reg         s_axi_control_BREADY;
    wire [1:0]  s_axi_control_BRESP;
    
    // gmem0 - Inputs to DUT (driven by testbench) - use reg
    reg         m_axi_gmem0_AWREADY;
    reg         m_axi_gmem0_WREADY;
    reg         m_axi_gmem0_ARREADY;
    reg         m_axi_gmem0_RREADY;
    reg         m_axi_gmem0_BREADY;
    
    // gmem0 - Outputs from DUT (monitored by testbench) - use wire
    wire        m_axi_gmem0_AWVALID;
    wire [63:0] m_axi_gmem0_AWADDR;
    wire [5:0]  m_axi_gmem0_AWID;
    wire [7:0]  m_axi_gmem0_AWLEN;
    wire [2:0]  m_axi_gmem0_AWSIZE;
    wire [1:0]  m_axi_gmem0_AWBURST;
    wire [1:0]  m_axi_gmem0_AWLOCK;
    wire [3:0]  m_axi_gmem0_AWCACHE;
    wire [2:0]  m_axi_gmem0_AWPROT;
    wire [3:0]  m_axi_gmem0_AWQOS;
    wire [3:0]  m_axi_gmem0_AWREGION;
    wire [0:0]  m_axi_gmem0_AWUSER;
    
    wire        m_axi_gmem0_WVALID;
    wire [31:0] m_axi_gmem0_WDATA;
    wire [3:0]  m_axi_gmem0_WSTRB;
    wire        m_axi_gmem0_WLAST;
    wire [5:0]  m_axi_gmem0_WID;
    wire [0:0]  m_axi_gmem0_WUSER;
    
    wire        m_axi_gmem0_ARVALID;
    wire [63:0] m_axi_gmem0_ARADDR;
    wire [5:0]  m_axi_gmem0_ARID;
    wire [7:0]  m_axi_gmem0_ARLEN;
    wire [2:0]  m_axi_gmem0_ARSIZE;
    wire [1:0]  m_axi_gmem0_ARBURST;
    wire [1:0]  m_axi_gmem0_ARLOCK;
    wire [3:0]  m_axi_gmem0_ARCACHE;
    wire [2:0]  m_axi_gmem0_ARPROT;
    wire [3:0]  m_axi_gmem0_ARQOS;
    wire [3:0]  m_axi_gmem0_ARREGION;
    wire [0:0]  m_axi_gmem0_ARUSER;
    
    wire        m_axi_gmem0_RVALID;
    wire [31:0] m_axi_gmem0_RDATA;
    wire        m_axi_gmem0_RLAST;
    wire [5:0]  m_axi_gmem0_RID;
    wire [0:0]  m_axi_gmem0_RUSER;
    wire [1:0]  m_axi_gmem0_RRESP;
    
    wire        m_axi_gmem0_BVALID;
    wire [1:0]  m_axi_gmem0_BRESP;
    wire [5:0]  m_axi_gmem0_BID;
    wire [0:0]  m_axi_gmem0_BUSER;
    
    // Instantiate DUT
    top dut (
        .ap_clk(ap_clk),
        .ap_rst_n(ap_rst_n),
        .interrupt(interrupt),
        
        .s_axi_control_AWVALID(s_axi_control_AWVALID),
        .s_axi_control_AWREADY(s_axi_control_AWREADY),
        .s_axi_control_AWADDR(s_axi_control_AWADDR),
        .s_axi_control_WVALID(s_axi_control_WVALID),
        .s_axi_control_WREADY(s_axi_control_WREADY),
        .s_axi_control_WDATA(s_axi_control_WDATA),
        .s_axi_control_WSTRB(s_axi_control_WSTRB),
        .s_axi_control_ARVALID(s_axi_control_ARVALID),
        .s_axi_control_ARREADY(s_axi_control_ARREADY),
        .s_axi_control_ARADDR(s_axi_control_ARADDR),
        .s_axi_control_RVALID(s_axi_control_RVALID),
        .s_axi_control_RREADY(s_axi_control_RREADY),
        .s_axi_control_RDATA(s_axi_control_RDATA),
        .s_axi_control_RRESP(s_axi_control_RRESP),
        .s_axi_control_BVALID(s_axi_control_BVALID),
        .s_axi_control_BREADY(s_axi_control_BREADY),
        .s_axi_control_BRESP(s_axi_control_BRESP),
        
        .m_axi_gmem0_AWVALID(m_axi_gmem0_AWVALID),
        .m_axi_gmem0_AWREADY(m_axi_gmem0_AWREADY),
        .m_axi_gmem0_AWADDR(m_axi_gmem0_AWADDR),
        .m_axi_gmem0_AWID(m_axi_gmem0_AWID),
        .m_axi_gmem0_AWLEN(m_axi_gmem0_AWLEN),
        .m_axi_gmem0_AWSIZE(m_axi_gmem0_AWSIZE),
        .m_axi_gmem0_AWBURST(m_axi_gmem0_AWBURST),
        .m_axi_gmem0_AWLOCK(m_axi_gmem0_AWLOCK),
        .m_axi_gmem0_AWCACHE(m_axi_gmem0_AWCACHE),
        .m_axi_gmem0_AWPROT(m_axi_gmem0_AWPROT),
        .m_axi_gmem0_AWQOS(m_axi_gmem0_AWQOS),
        .m_axi_gmem0_AWREGION(m_axi_gmem0_AWREGION),
        .m_axi_gmem0_AWUSER(m_axi_gmem0_AWUSER),
        .m_axi_gmem0_WVALID(m_axi_gmem0_WVALID),
        .m_axi_gmem0_WREADY(m_axi_gmem0_WREADY),
        .m_axi_gmem0_WDATA(m_axi_gmem0_WDATA),
        .m_axi_gmem0_WSTRB(m_axi_gmem0_WSTRB),
        .m_axi_gmem0_WLAST(m_axi_gmem0_WLAST),
        .m_axi_gmem0_WID(m_axi_gmem0_WID),
        .m_axi_gmem0_WUSER(m_axi_gmem0_WUSER),
        .m_axi_gmem0_ARVALID(m_axi_gmem0_ARVALID),
        .m_axi_gmem0_ARREADY(m_axi_gmem0_ARREADY),
        .m_axi_gmem0_ARADDR(m_axi_gmem0_ARADDR),
        .m_axi_gmem0_ARID(m_axi_gmem0_ARID),
        .m_axi_gmem0_ARLEN(m_axi_gmem0_ARLEN),
        .m_axi_gmem0_ARSIZE(m_axi_gmem0_ARSIZE),
        .m_axi_gmem0_ARBURST(m_axi_gmem0_ARBURST),
        .m_axi_gmem0_ARLOCK(m_axi_gmem0_ARLOCK),
        .m_axi_gmem0_ARCACHE(m_axi_gmem0_ARCACHE),
        .m_axi_gmem0_ARPROT(m_axi_gmem0_ARPROT),
        .m_axi_gmem0_ARQOS(m_axi_gmem0_ARQOS),
        .m_axi_gmem0_ARREGION(m_axi_gmem0_ARREGION),
        .m_axi_gmem0_ARUSER(m_axi_gmem0_ARUSER),
        .m_axi_gmem0_RVALID(m_axi_gmem0_RVALID),
        .m_axi_gmem0_RREADY(m_axi_gmem0_RREADY),
        .m_axi_gmem0_RDATA(m_axi_gmem0_RDATA),
        .m_axi_gmem0_RLAST(m_axi_gmem0_RLAST),
        .m_axi_gmem0_RID(m_axi_gmem0_RID),
        .m_axi_gmem0_RUSER(m_axi_gmem0_RUSER),
        .m_axi_gmem0_RRESP(m_axi_gmem0_RRESP),
        .m_axi_gmem0_BVALID(m_axi_gmem0_BVALID),
        .m_axi_gmem0_BREADY(m_axi_gmem0_BREADY),
        .m_axi_gmem0_BRESP(m_axi_gmem0_BRESP),
        .m_axi_gmem0_BID(m_axi_gmem0_BID),
        .m_axi_gmem0_BUSER(m_axi_gmem0_BUSER)
    );
    
    // Clock generation (100 MHz)
    initial begin
        ap_clk = 0;
        forever #5 ap_clk = ~ap_clk;
    end
    
    // Test sequence
    initial begin
        // Set ready signals (inputs to DUT, so reg type)
        m_axi_gmem0_AWREADY = 1;
        m_axi_gmem0_WREADY = 1;
        m_axi_gmem0_ARREADY = 1;
        m_axi_gmem0_RREADY = 1;
        m_axi_gmem0_BREADY = 1;
        
        // Initialize AXI Lite
        s_axi_control_AWVALID = 0;
        s_axi_control_WVALID = 0;
        s_axi_control_ARVALID = 0;
        s_axi_control_RREADY = 1;
        s_axi_control_BREADY = 1;
        s_axi_control_AWADDR = 0;
        s_axi_control_ARADDR = 0;
        s_axi_control_WDATA = 0;
        s_axi_control_WSTRB = 0;
        
        $display("========================================");
        $display("DMP SpMSpM Accelerator RTL Testbench");
        $display("========================================");
        
        // Reset
        ap_rst_n = 0;
        repeat(10) @(posedge ap_clk);
        ap_rst_n = 1;
        repeat(2) @(posedge ap_clk);
        
        $display("[TB] Reset complete at time %0t", $time);
        
        // Write configuration registers
        write_control(8'h10, 32'h00000000);
        write_control(8'h18, 32'h00000100);
        write_control(8'h20, 32'h00000200);
        write_control(8'h28, 32'd3);
        write_control(8'h30, 32'd5);
        
        // Start accelerator
        write_control(8'h00, 32'h00000001);
        
        $display("[TB] Accelerator started at time %0t", $time);
        
        // Wait for interrupt (completion)
        wait (interrupt == 1);
        $display("[TB] Accelerator finished at time %0t", $time);
        
        $finish;
    end
    
    // AXI Lite write task
    task write_control(input [7:0] addr, input [31:0] data);
        begin
            @(posedge ap_clk);
            s_axi_control_AWVALID = 1;
            s_axi_control_AWADDR = addr;
            s_axi_control_WVALID = 1;
            s_axi_control_WDATA = data;
            s_axi_control_WSTRB = 4'b1111;
            
            @(posedge ap_clk);
            while (!s_axi_control_AWREADY || !s_axi_control_WREADY) begin
                @(posedge ap_clk);
            end
            
            s_axi_control_AWVALID = 0;
            s_axi_control_WVALID = 0;
            
            @(posedge ap_clk);
            s_axi_control_BREADY = 1;
            @(posedge ap_clk);
            while (!s_axi_control_BVALID) @(posedge ap_clk);
            s_axi_control_BREADY = 0;
            
            $display("[TB] Wrote 0x%h to addr 0x%h at time %0t", data, addr, $time);
        end
    endtask
    
endmodule