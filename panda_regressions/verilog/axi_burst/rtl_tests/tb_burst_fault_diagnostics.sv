module tb_burst_fault_diagnostics #(parameter integer TEST_READ = 1);
    logic clock = 0;
    always #5 clock = ~clock;
    logic reset = 0;
    logic peer_fault = 0;
    logic start = 0;
    logic [1:0] in1 = 0;
    logic [31:0] in2 = 32;
    logic [31:0] in3 = 0;
    logic [31:0] in4 = 32'h1000;
    logic [31:0] in5 = 1;
    logic [31:0] rdata = 0;
    logic [1:0] rresp = 0;
    logic rid = 0, rlast = 1, rvalid = 0, arready = 1;
    wire rready, rdone, rfault, rarvalid;
    wire [31:0] rout, raraddr;
    wire [7:0] rarlen;
    wire [2:0] rarsize;
    wire [1:0] rarburst;
    wire rarid;
    logic awready = 1, wready = 1, bvalid = 0;
    logic [1:0] bresp = 0;
    logic bid = 0;
    wire awvalid, wvalid, bready, wdone, wfault;
    wire [31:0] wawaddr, wwdata, wout;
    wire [7:0] wawlen;
    wire [2:0] wawsize;
    wire [1:0] wawburst;
    wire wawid, wwlast;
    wire [3:0] wwstrb;
    wire read_reset = TEST_READ ? reset : 1'b0;
    wire write_reset = TEST_READ ? 1'b0 : reset;

    MinimalAXI4MasterPipelined #(.PEER_FAULT_ENABLE(1)) reader_dut (
        .clock(clock), .reset(read_reset), .start(start), .peer_fault(peer_fault), .in1(in1),
        .in2(in2), .in3(in3), .in4(in4), .in5(in5), .done(rdone), .out1(rout), .fault(rfault),
        .m_axi_araddr(raraddr), .m_axi_arlen(rarlen), .m_axi_arsize(rarsize),
        .m_axi_arburst(rarburst), .m_axi_arid(rarid), .m_axi_arvalid(rarvalid),
        .m_axi_arready(arready), .m_axi_rdata(rdata), .m_axi_rresp(rresp),
        .m_axi_rid(rid), .m_axi_rlast(rlast), .m_axi_rvalid(rvalid), .m_axi_rready(rready));
    MinimalAXI4MasterPipelinedWrite #(.PEER_FAULT_ENABLE(1), .FAST_STORE_ACK(0)) writer_dut (
        .clock(clock), .reset(write_reset), .start(start), .peer_fault(peer_fault), .in1(in1),
        .in2(in2), .in3(in3), .in4(in4), .in5(in5), .done(wdone), .out1(wout), .fault(wfault),
        .m_axi_awaddr(wawaddr), .m_axi_awlen(wawlen), .m_axi_awsize(wawsize),
        .m_axi_awburst(wawburst), .m_axi_awid(wawid), .m_axi_awvalid(awvalid),
        .m_axi_awready(awready), .m_axi_wdata(wwdata), .m_axi_wstrb(wwstrb),
        .m_axi_wlast(wwlast), .m_axi_wvalid(wvalid), .m_axi_wready(wready),
        .m_axi_bresp(bresp), .m_axi_bid(bid), .m_axi_bvalid(bvalid), .m_axi_bready(bready));

    task automatic tick(input integer n);
        repeat (n) @(posedge clock);
        #1;
    endtask
    task automatic command(input logic [1:0] opcode, input logic [31:0] size_value);
        @(negedge clock); in1 = opcode; in2 = size_value; start = 1;
        @(negedge clock); start = 0;
    endtask
    task automatic reset_duts;
        @(negedge clock); reset = 0; start = 0; peer_fault = 0; tick(2);
        @(negedge clock); reset = 1; tick(2);
    endtask

    string selected;
    integer i;
    initial begin
        if (!$value$plusargs("CASE=%s", selected)) $fatal(1, "CASE required");
        reset_duts();
        if (selected == "READ_CONFIG_FATAL" || selected == "READ_CONFIG_CONTINUE" || selected == "READ_REARM") begin
            command(2, 8); tick(4);
            if (selected == "READ_REARM") begin
                if (!rfault) $fatal(1, "read fault did not latch");
                reset_duts();
                command(2, 8); tick(4);
                if (!rfault) $fatal(1, "read fault did not re-arm after reset");
            end
            if (selected == "READ_CONFIG_CONTINUE" && !rfault) $fatal(1, "read fault did not latch");
            if (selected != "READ_CONFIG_FATAL") $display("BURST_DIAGNOSTICS_PASS case=%s fault=%0d", selected, rfault);
        end else if (selected == "WRITE_CONFIG_FATAL" || selected == "WRITE_CONFIG_CONTINUE") begin
            command(2, 8); tick(4);
            if (selected == "WRITE_CONFIG_CONTINUE") begin
                if (!wfault) $fatal(1, "write fault did not latch");
                $display("BURST_DIAGNOSTICS_PASS case=%s fault=%0d", selected, wfault);
            end
        end else if (selected == "READ_IGNORED" || selected == "WRITE_IGNORED" ||
                     selected == "READ_IGNORED_THEN_FAULT") begin
            command(selected == "WRITE_IGNORED" ? 0 : 3, 32); tick(3);
            if (rfault || wfault) $fatal(1, "ignored command falsely latched fault read=%0d write=%0d", rfault, wfault);
            if (selected == "READ_IGNORED_THEN_FAULT") begin
                command(2, 8); tick(4);
                if (!rfault) $fatal(1, "fault after ignored command did not latch");
            end
            $display("BURST_DIAGNOSTICS_PASS case=%s no_hardware_fault=1", selected);
        end else if (selected == "READ_RRESP_IGNORED_FATAL" || selected == "READ_RRESP_IGNORED_CONTINUE") begin
            command(2, 32); tick(3);
            command(0, 32); tick(3);
            @(negedge clock); rresp = 2'b10; rvalid = 1; start = 1; in1 = 3;
            @(posedge clock); #1;
            @(negedge clock); start = 0; rvalid = 0;
            if (selected == "READ_RRESP_IGNORED_CONTINUE") begin
                if (!rfault || rdone) $fatal(1, "simultaneous read response fault did not latch or falsely completed");
                $display("BURST_DIAGNOSTICS_PASS case=%s fault=%0d done=%0d", selected, rfault, rdone);
            end
        end else if (selected == "WRITE_BRESP_IGNORED_FATAL" || selected == "WRITE_BRESP_IGNORED_CONTINUE") begin
            command(2, 32); tick(3);
            command(1, 32); tick(5);
            if (!bready) $fatal(1, "write response was not ready before simultaneous ignored command");
            @(negedge clock); bresp = 2'b10; bvalid = 1; start = 1; in1 = 0;
            @(posedge clock); #1;
            @(negedge clock); start = 0; bvalid = 0;
            if (selected == "WRITE_BRESP_IGNORED_CONTINUE") begin
                if (!wfault || wdone) $fatal(1, "simultaneous write response fault did not latch or falsely completed");
                $display("BURST_DIAGNOSTICS_PASS case=%s fault=%0d done=%0d", selected, wfault, wdone);
            end
        end else if (selected == "READ_PEER" || selected == "WRITE_PEER") begin
            @(negedge clock); peer_fault = 1;
            tick(3);
            if ((selected == "READ_PEER" && !rfault) || (selected == "WRITE_PEER" && !wfault))
                $fatal(1, "peer fault did not propagate");
            $display("BURST_DIAGNOSTICS_PASS case=%s peer_fault=1", selected);
        end else $fatal(1, "unknown CASE=%s", selected);
        #1 $finish;
    end
endmodule
