// SIZE guard for the minimal AXI adapters.
//
// The ARSIZE/AWSIZE field of MinimalAXI4Adapter and MinimalAXI4AdapterSingleBeat comes from the
// byte width of the memory port: log2(Mout_data_ram_size / 8). The library computes it in two
// branches, with $clog2 when _SIM_HAVE_CLOG2 is defined (the simulation tools that support it) and
// with the Verilog log2 function otherwise (the synthesis case, where PandA emits no macro
// prologue). Writing `log2(...) with a backtick in the second branch would invoke a macro that
// does not exist: the preprocessor drops the name and the expression degenerates into the byte
// count itself, i.e. SIZE=4 instead of 2 for a 32-bit port, which declares 16-byte beats on a
// 4-byte bus. This bench pins the value of size_next, and of the SIZE register in the
// single-beat adapter where the load path is directly reachable, for 32-bit and 64-bit ports.
`timescale 1ns/1ps
module tb_adapter_size;
    localparam integer EXPECT_32 = 2; // 32-bit memory port -> 4 bytes per beat
    localparam integer EXPECT_64 = 3; // 64-bit memory port -> 8 bytes per beat

    logic clock = 1'b0;
    logic reset = 1'b1;
    logic [31:0] size_bits_32 = 32;
    logic [31:0] size_bits_64 = 64;
    logic m_oe = 1'b0;
    logic m_we = 1'b0;
    logic [31:0] m_addr = 32'h0000_0010;
    logic [63:0] m_wdata = 64'h5a5a_1234_0000_1234;

    always #5 clock = ~clock;

    MinimalAXI4Adapter #(
        .BURST_TYPE(2'b01),
        .BITSIZE_Mout_addr_ram(32),
        .BITSIZE_Mout_Wdata_ram(32),
        .BITSIZE_Mout_data_ram_size(32),
        .BITSIZE_M_Rdata_ram(32)
    ) adapter32 (.clock(clock), .reset(reset), .Mout_data_ram_size(size_bits_32));

    MinimalAXI4Adapter #(
        .BURST_TYPE(2'b01),
        .BITSIZE_Mout_addr_ram(32),
        .BITSIZE_Mout_Wdata_ram(64),
        .BITSIZE_Mout_data_ram_size(32),
        .BITSIZE_M_Rdata_ram(64)
    ) adapter64 (.clock(clock), .reset(reset), .Mout_data_ram_size(size_bits_64));

    MinimalAXI4AdapterSingleBeat #(
        .BURST_TYPE(2'b01),
        .BITSIZE_Mout_addr_ram(32),
        .BITSIZE_Mout_Wdata_ram(32),
        .BITSIZE_Mout_data_ram_size(32),
        .BITSIZE_M_Rdata_ram(32)
    ) single_beat32 (.clock(clock), .reset(reset), .Mout_oe_ram(m_oe), .Mout_we_ram(m_we),
        .Mout_addr_ram(m_addr[31:0]), .Mout_Wdata_ram(m_wdata[31:0]),
        .Mout_data_ram_size(size_bits_32));

    MinimalAXI4AdapterSingleBeat #(
        .BURST_TYPE(2'b01),
        .BITSIZE_Mout_addr_ram(32),
        .BITSIZE_Mout_Wdata_ram(64),
        .BITSIZE_Mout_data_ram_size(32),
        .BITSIZE_M_Rdata_ram(64)
    ) single_beat64 (.clock(clock), .reset(reset), .Mout_oe_ram(m_oe), .Mout_we_ram(m_we),
        .Mout_addr_ram(m_addr[31:0]), .Mout_Wdata_ram(m_wdata[63:32]),
        .Mout_data_ram_size(size_bits_64));

    task automatic expect_size(input string name, input integer got, input integer want);
        begin
            if (got !== want)
                $fatal(1, "ADAPTER_SIZE_MISMATCH adapter=%s size_next=%0d expected=%0d", name, got, want);
        end
    endtask

    initial begin
        repeat (3) @(posedge clock);
        reset = 1'b0;
        repeat (2) @(posedge clock);

        expect_size("MinimalAXI4Adapter:32", adapter32.size_next, EXPECT_32);
        expect_size("MinimalAXI4Adapter:64", adapter64.size_next, EXPECT_64);
        expect_size("MinimalAXI4AdapterSingleBeat:32", single_beat32.size_next, EXPECT_32);
        expect_size("MinimalAXI4AdapterSingleBeat:64", single_beat64.size_next, EXPECT_64);

        // Single-beat write path: the SIZE register takes size_next directly when the core side
        // issues a write, so the value that reaches the bus is checked as well.
        m_we = 1'b1;
        @(negedge clock);
        @(negedge clock);
        @(negedge clock);
        $display("ADAPTER_SIZE adapter=MinimalAXI4AdapterSingleBeat:32 size_next=%0d awsize_reg=%0d",
                 single_beat32.size_next, single_beat32.m_axi_awsize_reg);
        if (single_beat32.m_axi_awsize_reg === EXPECT_32)
            $display("ADAPTER_SIZE_BUS_REG_MATCH size=%0d", EXPECT_32);
        else
            $display("ADAPTER_SIZE_BUS_REG_NOT_LOADED awsize_reg=%b", single_beat32.m_axi_awsize_reg);

`ifdef _SIM_HAVE_CLOG2
        $display("ADAPTER_SIZE_PASS size32=%0d size64=%0d path=clog2", EXPECT_32, EXPECT_64);
`else
        $display("ADAPTER_SIZE_PASS size32=%0d size64=%0d path=log2_function", EXPECT_32, EXPECT_64);
`endif
        $finish;
    end
endmodule
