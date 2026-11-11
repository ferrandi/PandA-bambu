// SIZE guard for the minimal AXI adapters: check active address transactions,
// stalls, and response handshakes for both adapter implementations.
`timescale 1ns/1ps

module adapter_size_probe #(
    parameter integer SINGLE_BEAT = 0,
    parameter integer DATA_BITS = 32
) (
    input wire clock,
    input wire reset,
    input wire read_req,
    input wire write_req,
    input wire [31:0] address,
    input wire [63:0] write_data,
    input wire arready,
    input wire awready,
    input wire wready,
    input wire [63:0] rdata,
    input wire rvalid,
    input wire bvalid,
    output wire [2:0] size_next,
    output wire [2:0] arsize,
    output wire [2:0] awsize,
    output wire arvalid,
    output wire awvalid,
    output wire wvalid,
    output wire rready,
    output wire bready
);
    wire [31:0] data_ram_size = DATA_BITS;

    generate
        if (SINGLE_BEAT) begin : single
            MinimalAXI4AdapterSingleBeat #(
                .BURST_TYPE(2'b01),
                .BITSIZE_Mout_addr_ram(32),
                .BITSIZE_Mout_Wdata_ram(64),
                .BITSIZE_Mout_data_ram_size(32),
                .BITSIZE_M_Rdata_ram(64),
                .BITSIZE_m_axi_arsize(3),
                .BITSIZE_m_axi_awsize(3),
                .BITSIZE_m_axi_rdata(64)
            ) dut (
                .clock(clock), .reset(reset),
                .Mout_oe_ram(read_req), .Mout_we_ram(write_req),
                .Mout_addr_ram(address), .Mout_Wdata_ram(write_data),
                .Mout_data_ram_size(data_ram_size),
                .m_axi_arsize(arsize), .m_axi_awsize(awsize),
                .m_axi_arvalid(arvalid), .m_axi_awvalid(awvalid), .m_axi_wvalid(wvalid),
                .m_axi_arready(arready), .m_axi_awready(awready), .m_axi_wready(wready),
                .m_axi_rid(1'b0), .m_axi_rdata(rdata), .m_axi_rresp(2'b00),
                .m_axi_rlast(1'b1), .m_axi_rvalid(rvalid), .m_axi_rready(rready),
                .m_axi_bid(1'b0), .m_axi_bresp(2'b00), .m_axi_bvalid(bvalid),
                .m_axi_bready(bready)
            );
            assign size_next = single.dut.size_next;
        end else begin : burst_adapter
            MinimalAXI4Adapter #(
                .BURST_TYPE(2'b01),
                .BITSIZE_Mout_addr_ram(32),
                .BITSIZE_Mout_Wdata_ram(64),
                .BITSIZE_Mout_data_ram_size(32),
                .BITSIZE_Mout_tag(1),
                .BITSIZE_M_Rdata_ram(64),
                .BITSIZE_M_tag(1),
                .BITSIZE_m_axi_arsize(3),
                .BITSIZE_m_axi_awsize(3),
                .BITSIZE_m_axi_rdata(64)
            ) dut (
                .clock(clock), .reset(reset),
                .Mout_oe_ram(read_req), .Mout_we_ram(write_req),
                .Mout_addr_ram(address), .Mout_Wdata_ram(write_data),
                .Mout_data_ram_size(data_ram_size), .Mout_tag(1'b0),
                .M_back_pressure(1'b0),
                .m_axi_arsize(arsize), .m_axi_awsize(awsize),
                .m_axi_arvalid(arvalid), .m_axi_awvalid(awvalid), .m_axi_wvalid(wvalid),
                .m_axi_arready(arready), .m_axi_awready(awready), .m_axi_wready(wready),
                .m_axi_rid(1'b0), .m_axi_rdata(rdata), .m_axi_rresp(2'b00),
                .m_axi_rlast(1'b1), .m_axi_rvalid(rvalid), .m_axi_rready(rready),
                .m_axi_bid(1'b0), .m_axi_bresp(2'b00), .m_axi_bvalid(bvalid),
                .m_axi_bready(bready)
            );
            assign size_next = burst_adapter.dut.size_next;
        end
    endgenerate
endmodule

module tb_adapter_size;
    logic clock = 1'b0;
    logic reset = 1'b0;
    logic read_req = 1'b0;
    logic write_req = 1'b0;
    logic [31:0] address = 32'h0000_0010;
    logic [63:0] write_data = 64'h5a5a_1234_0000_1234;
    logic arready = 1'b0;
    logic awready = 1'b0;
    logic wready = 1'b0;
    logic [63:0] rdata = 64'h1234_5678_9abc_def0;
    logic rvalid = 1'b0;
    logic bvalid = 1'b0;

    wire [2:0] a8_next, a32_next, a64_next, s8_next, s32_next, s64_next;
    wire [2:0] a8_ar, a32_ar, a64_ar, s8_ar, s32_ar, s64_ar;
    wire [2:0] a8_aw, a32_aw, a64_aw, s8_aw, s32_aw, s64_aw;
    wire a8_arvalid, a32_arvalid, a64_arvalid, s8_arvalid, s32_arvalid, s64_arvalid;
    wire a8_awvalid, a32_awvalid, a64_awvalid, s8_awvalid, s32_awvalid, s64_awvalid;
    wire a8_wvalid, a32_wvalid, a64_wvalid, s8_wvalid, s32_wvalid, s64_wvalid;
    wire a8_rready, a32_rready, a64_rready, s8_rready, s32_rready, s64_rready;
    wire a8_bready, a32_bready, a64_bready, s8_bready, s32_bready, s64_bready;

    always #5 clock = ~clock;

`define PROBE(NAME, SINGLE, WIDTH) \
    adapter_size_probe #(.SINGLE_BEAT(SINGLE), .DATA_BITS(WIDTH)) NAME ( \
        .clock(clock), .reset(reset), .read_req(read_req), .write_req(write_req), \
        .address(address), .write_data(write_data), \
        .arready(arready), .awready(awready), .wready(wready), \
        .rdata(rdata), .rvalid(rvalid), .bvalid(bvalid), \
        .size_next(NAME``_next), .arsize(NAME``_ar), .awsize(NAME``_aw), \
        .arvalid(NAME``_arvalid), .awvalid(NAME``_awvalid), .wvalid(NAME``_wvalid), \
        .rready(NAME``_rready), .bready(NAME``_bready))

    `PROBE(a8, 0, 8);
    `PROBE(a32, 0, 32);
    `PROBE(a64, 0, 64);
    `PROBE(s8, 1, 8);
    `PROBE(s32, 1, 32);
    `PROBE(s64, 1, 64);
`undef PROBE

    task automatic check_size_next;
        begin
            if ({a8_next, a32_next, a64_next, s8_next, s32_next, s64_next} !==
                {3'd0, 3'd2, 3'd3, 3'd0, 3'd2, 3'd3})
                $fatal(1, "ADAPTER_SIZE_MISMATCH phase=size_next values=%0d,%0d,%0d,%0d,%0d,%0d",
                       a8_next, a32_next, a64_next, s8_next, s32_next, s64_next);
        end
    endtask

    task automatic check_read(input string phase, input logic expected_valid);
        begin
            if ({a8_arvalid, a32_arvalid, a64_arvalid, s8_arvalid, s32_arvalid, s64_arvalid} !==
                {6{expected_valid}})
                $fatal(1, "ADAPTER_SIZE_MISMATCH phase=%s ARVALID", phase);
            if ({a8_ar, a32_ar, a64_ar, s8_ar, s32_ar, s64_ar} !==
                {3'd0, 3'd2, 3'd3, 3'd0, 3'd2, 3'd3})
                $fatal(1, "ADAPTER_SIZE_MISMATCH phase=%s ARSIZE values=%0d,%0d,%0d,%0d,%0d,%0d",
                       phase, a8_ar, a32_ar, a64_ar, s8_ar, s32_ar, s64_ar);
        end
    endtask

    task automatic check_write(input string phase, input logic expected_valid);
        begin
            if ({a8_awvalid, a32_awvalid, a64_awvalid, s8_awvalid, s32_awvalid, s64_awvalid} !==
                {6{expected_valid}} ||
                {a8_wvalid, a32_wvalid, a64_wvalid, s8_wvalid, s32_wvalid, s64_wvalid} !==
                {6{expected_valid}})
                $fatal(1, "ADAPTER_SIZE_MISMATCH phase=%s AW/WVALID", phase);
            if ({a8_aw, a32_aw, a64_aw, s8_aw, s32_aw, s64_aw} !==
                {3'd0, 3'd2, 3'd3, 3'd0, 3'd2, 3'd3})
                $fatal(1, "ADAPTER_SIZE_MISMATCH phase=%s AWSIZE values=%0d,%0d,%0d,%0d,%0d,%0d",
                       phase, a8_aw, a32_aw, a64_aw, s8_aw, s32_aw, s64_aw);
        end
    endtask

    task automatic wait_read_response;
        integer cycle;
        begin
            rvalid = 1'b1;
            for (cycle = 0; cycle < 10; cycle = cycle + 1) begin
                @(posedge clock);
                if (&{a8_rready, a32_rready, a64_rready, s8_rready, s32_rready, s64_rready})
                    cycle = 10;
            end
            if (!(&{a8_rready, a32_rready, a64_rready, s8_rready, s32_rready, s64_rready}))
                $fatal(1, "ADAPTER_SIZE_MISMATCH phase=read-response RREADY timeout");
            @(negedge clock);
            rvalid = 1'b0;
        end
    endtask

    task automatic wait_write_response;
        integer cycle;
        begin
            bvalid = 1'b1;
            for (cycle = 0; cycle < 10; cycle = cycle + 1) begin
                @(posedge clock);
                if (&{a8_bready, a32_bready, a64_bready, s8_bready, s32_bready, s64_bready})
                    cycle = 10;
            end
            if (!(&{a8_bready, a32_bready, a64_bready, s8_bready, s32_bready, s64_bready}))
                $fatal(1, "ADAPTER_SIZE_MISMATCH phase=write-response BREADY timeout");
            @(negedge clock);
            bvalid = 1'b0;
        end
    endtask

    initial
    begin
        repeat (3) @(posedge clock);
        @(negedge clock);
        reset = 1'b1;
        repeat (2) @(posedge clock);
        #1 check_size_next();

        @(negedge clock);
        read_req = 1'b1;
        @(posedge clock); #1 check_read("read-valid", 1'b1);
        @(negedge clock);
        read_req = 1'b0;
        repeat (2) begin
            @(posedge clock); #1 check_read("read-stall", 1'b1);
        end
        @(negedge clock);
        arready = 1'b1;
        @(posedge clock); #1 check_read("read-accepted", 1'b0);
        @(negedge clock);
        arready = 1'b0;
        wait_read_response();

        repeat (2) @(posedge clock);
        @(negedge clock);
        write_req = 1'b1;
        @(posedge clock); #1 check_write("write-valid", 1'b1);
        @(negedge clock);
        write_req = 1'b0;
        repeat (2) begin
            @(posedge clock); #1 check_write("write-stall", 1'b1);
        end
        @(negedge clock);
        awready = 1'b1;
        wready = 1'b1;
        @(posedge clock); #1 check_write("write-accepted", 1'b0);
        @(negedge clock);
        awready = 1'b0;
        wready = 1'b0;
        wait_write_response();

        `ifdef _SIM_HAVE_CLOG2
            $display("ADAPTER_SIZE_PASS widths=8,32,64 adapters=both path=clog2");
        `else
            $display("ADAPTER_SIZE_PASS widths=8,32,64 adapters=both path=log2_function");
        `endif
        $finish;
    end
endmodule
