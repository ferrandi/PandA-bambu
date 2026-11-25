`timescale 1ns / 1ps

module tb_duplex_alias_range_guard;
    parameter integer ADDR_BITS = `DUPLEX_ALIAS_ADDR_BITS;
    logic clock = 1'b0;
    always #5 clock = ~clock;

    logic reset = 1'b0;
    logic start_port = 1'b0;
    logic [31:0] src = 0;
    logic [31:0] dst = 0;
    logic [31:0] n = 0;

    wire arvalid;
    wire [7:0] arlen;
    wire [ADDR_BITS-1:0] araddr;
    wire awvalid;
    wire wvalid;

    kernel dut (
        .clock(clock),
        .reset(reset),
        .start_port(start_port),
        .src(src),
        .dst(dst),
        .n(n),
        .cache_reset(1'b0),
        .m_axi_shared_awready(1'b0),
        .m_axi_shared_wready(1'b0),
        .m_axi_shared_bid(6'b0),
        .m_axi_shared_bresp(2'b0),
        .m_axi_shared_buser(1'b0),
        .m_axi_shared_bvalid(1'b0),
        .m_axi_shared_arready(1'b0),
        .m_axi_shared_rid(6'b0),
        .m_axi_shared_rdata(32'b0),
        .m_axi_shared_rresp(2'b0),
        .m_axi_shared_rlast(1'b0),
        .m_axi_shared_ruser(1'b0),
        .m_axi_shared_rvalid(1'b0),
        .m_axi_shared_awvalid(awvalid),
        .m_axi_shared_wvalid(wvalid),
        .m_axi_shared_arvalid(arvalid),
        .m_axi_shared_arlen(arlen),
        .m_axi_shared_araddr(araddr)
    );

    initial
    begin
        string case_name;
        logic [7:0] expected_arlen;
        logic [ADDR_BITS-1:0] expected_araddr;
        integer wait_cycles;
        if (!$value$plusargs("CASE=%s", case_name))
            $fatal(1, "RANGE_GUARD_CASE_REQUIRED");
        $dumpfile($sformatf("range_guard_%s.vcd", case_name));
        $dumpvars(0, dut);
        case (case_name)
            "valid-disjoint":
            begin
              src=32'h00001000; dst=32'h00002000; n=32'd17; expected_arlen=8'd15; expected_araddr=ADDR_BITS'(32'h00001000);
            end
            "read-end-overflow":
            begin
              src=32'hffffffc0; dst=32'h00001000; n=32'd17; expected_arlen=8'd0; expected_araddr=ADDR_BITS'(32'hffffffc0);
            end
            "write-end-overflow":
            begin
              src=32'h00001000; dst=32'hffffffc0; n=32'd17; expected_arlen=8'd0; expected_araddr=ADDR_BITS'(32'h00001000);
            end
            "byte-count-overflow":
            begin
              src=32'h00001000; dst=32'h00002000; n=32'h40000001; expected_arlen=8'd0; expected_araddr=ADDR_BITS'(32'h00001000);
            end
            "read-end-equals-2p32":
            begin
              src=32'hffffffbc; dst=32'h00001000; n=32'd17; expected_arlen=8'd15; expected_araddr=ADDR_BITS'(32'hffffffbc);
            end
            "high-bit-valid-disjoint":
            begin
                if (ADDR_BITS != 64) $fatal(1, "RANGE_GUARD_HIGH_BIT_REQUIRES_64_BIT_BUS");
                src=32'h80001000; dst=32'h90002000; n=32'd17;
                expected_arlen=8'd15; expected_araddr=64'h0000000080001000;
            end
            default: $fatal(1, "RANGE_GUARD_UNKNOWN_CASE=%s", case_name);
        endcase
        repeat (4) @(negedge clock);
        reset = 1'b1;
        repeat (2) @(negedge clock);
        start_port = 1'b1;
        @(negedge clock);
        start_port = 1'b0;
        wait_cycles = 0;
        while (!arvalid && wait_cycles < 500)
        begin
            @(negedge clock);
            wait_cycles = wait_cycles + 1;
        end
        if (!arvalid)
            $fatal(1, "RANGE_GUARD_NO_AR CASE=%s", case_name);
        if (arlen !== expected_arlen)
            $fatal(1, "RANGE_GUARD_ARLEN_MISMATCH CASE=%s expected=%0d actual=%0d",
                   case_name, expected_arlen, arlen);
        if (araddr !== expected_araddr)
            $fatal(1, "RANGE_GUARD_ARADDR_MISMATCH CASE=%s expected=%h actual=%h",
                   case_name, expected_araddr, araddr);
        if (awvalid || wvalid)
            $fatal(1, "RANGE_GUARD_WRITE_STARTED_BEFORE_READ CASE=%s", case_name);
        if (case_name == "high-bit-valid-disjoint")
            $display("DUPLEX_ALIAS_RANGE_PASS CASE=%s ARLEN=%0d ARADDR=%016h", case_name, arlen, araddr);
        else
            $display("DUPLEX_ALIAS_RANGE_PASS CASE=%s ARLEN=%0d", case_name, arlen);
        $display("DUPLEX_ALIAS_RANGE_GUARD_PASS CASES=1 NO_AXI_HANDSHAKES=1");
        $finish;
    end
endmodule
