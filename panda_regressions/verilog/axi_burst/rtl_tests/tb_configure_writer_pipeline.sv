module tb_configure_writer_pipeline;
    logic clock = 1'b0;
    always #5 clock = ~clock;

    logic reset = 1'b0;
    logic start = 1'b0;
    logic peer_fault = 1'b0;
    logic [1:0] in1 = 2'b0;
    logic [31:0] in2 = 32'b0;
    logic [31:0] in3 = 32'b0;
    logic [31:0] in4 = 32'b0;
    logic [31:0] in5 = 32'b0;
    wire done;
    wire [31:0] out1;
    wire fault;
    wire [31:0] awaddr;
    wire [7:0] awlen;
    wire [2:0] awsize;
    wire [1:0] awburst;
    wire awid;
    wire awvalid;
    wire [31:0] wdata;
    wire [3:0] wstrb;
    wire wlast;
    wire wvalid;
    wire bready;

    localparam logic [1:0] OP_CONFIGURE = 2'd2;

    MinimalAXI4MasterPipelinedWrite #(
        .B_MAX(16), .MAX_OUTSTANDING(1), .FIFO_DEPTH(32),
        .PEER_FAULT_ENABLE(1)
    ) dut (
        .clock(clock), .reset(reset), .start(start), .peer_fault(peer_fault),
        .in1(in1), .in2(in2), .in3(in3), .in4(in4), .in5(in5),
        .done(done), .out1(out1), .fault(fault),
        .m_axi_awaddr(awaddr), .m_axi_awlen(awlen), .m_axi_awsize(awsize),
        .m_axi_awburst(awburst), .m_axi_awid(awid), .m_axi_awvalid(awvalid),
        .m_axi_awready(1'b0), .m_axi_wdata(wdata), .m_axi_wstrb(wstrb),
        .m_axi_wlast(wlast), .m_axi_wvalid(wvalid), .m_axi_wready(1'b0),
        .m_axi_bresp(2'b0), .m_axi_bid(1'b0), .m_axi_bvalid(1'b0),
        .m_axi_bready(bready)
    );

    task automatic check_quiet(input string label);
        begin
            if (done || fault || dut.region_active_reg || dut.configure_ack_reg ||
                awvalid || wvalid)
                $fatal(1, "WRITER_PIPELINE_NOT_QUIET %s done=%b fault=%b active=%b ack=%b aw=%b w=%b",
                       label, done, fault, dut.region_active_reg, dut.configure_ack_reg,
                       awvalid, wvalid);
        end
    endtask

    task automatic begin_configure_e0;
        begin
            @(negedge clock);
            in1 = OP_CONFIGURE;
            in2 = 32;
            in3 = 0;
            in4 = 32'h2000;
            in5 = 4;
            start = 1'b1;
            #1;
            check_quiet("E0_BEFORE_CAPTURE");
            @(posedge clock);
            #1;
            if (!dut.cfg_pipe_valid_reg || done || fault || dut.region_active_reg ||
                dut.configure_ack_reg || awvalid || wvalid)
                $fatal(1, "WRITER_PIPELINE_E0_CAPTURE_INVALID pipe=%b done=%b fault=%b active=%b ack=%b aw=%b w=%b",
                       dut.cfg_pipe_valid_reg, done, fault, dut.region_active_reg,
                       dut.configure_ack_reg, awvalid, wvalid);
        end
    endtask

    task automatic clear_request;
        begin
            start = 1'b0;
            in1 = 0;
            in2 = 0;
            in3 = 0;
            in4 = 0;
            in5 = 0;
        end
    endtask

    task automatic recover_after_reset(input string label);
        begin
            @(negedge clock);
            reset = 1'b0;
            @(posedge clock);
            #1;
            check_quiet({label, "_RESET_ASSERTED"});
            @(negedge clock);
            reset = 1'b1;
            @(posedge clock);
            #1;
            check_quiet({label, "_RESET_RELEASED"});
            begin_configure_e0();
            @(negedge clock);
            clear_request();
            @(posedge clock);
            #1;
            if (!done || fault || !dut.region_active_reg || !dut.configure_ack_reg ||
                dut.cfg_pipe_valid_reg || dut.region_base_reg !== 32'h2000 ||
                dut.region_count_reg != 4 || awvalid || wvalid)
                $fatal(1, "WRITER_PIPELINE_RECOVERY_FAILED %s done=%b fault=%b active=%b ack=%b pipe=%b base=%h count=%0d aw=%b w=%b",
                       label, done, fault, dut.region_active_reg, dut.configure_ack_reg,
                       dut.cfg_pipe_valid_reg, dut.region_base_reg, dut.region_count_reg,
                       awvalid, wvalid);
            @(negedge clock);
            @(posedge clock);
            #1;
            if (done || fault || !dut.region_active_reg || awvalid || wvalid)
                $fatal(1, "WRITER_PIPELINE_RECOVERY_ACK_NOT_PULSED %s", label);
        end
    endtask

    initial
    begin
        string selected_case;
        if (!$value$plusargs("CASE=%s", selected_case))
            $fatal(1, "WRITER_PIPELINE_CASE_REQUIRED");

        repeat (2) @(posedge clock);
        @(negedge clock);
        reset = 1'b1;
        @(posedge clock);
        #1;
        check_quiet("INITIAL_RESET_RELEASED");

        if (selected_case == "RESET_E1")
        begin
            begin_configure_e0();
            @(negedge clock);
            clear_request();
            reset = 1'b0;
            @(posedge clock);
            #1;
            if (dut.cfg_pipe_valid_reg || dut.configure_ack_reg || done || fault ||
                dut.region_active_reg || awvalid || wvalid)
                $fatal(1, "WRITER_PIPELINE_RESET_DID_NOT_CANCEL_E0 pipe=%b ack=%b done=%b fault=%b active=%b aw=%b w=%b",
                       dut.cfg_pipe_valid_reg, dut.configure_ack_reg, done, fault,
                       dut.region_active_reg, awvalid, wvalid);
            recover_after_reset("RESET_E1");
        end
        else if (selected_case == "PEER_FAULT_E1")
        begin
            begin_configure_e0();
            @(negedge clock);
            peer_fault = 1'b1;
            @(posedge clock);
            #1;
            if (dut.cfg_pipe_valid_reg || dut.configure_ack_reg || done || !fault ||
                dut.region_active_reg || awvalid || wvalid)
                $fatal(1, "WRITER_PIPELINE_PEER_FAULT_DID_NOT_CANCEL_E0 pipe=%b ack=%b done=%b fault=%b active=%b aw=%b w=%b",
                       dut.cfg_pipe_valid_reg, dut.configure_ack_reg, done, fault,
                       dut.region_active_reg, awvalid, wvalid);
            @(negedge clock);
            clear_request();
            peer_fault = 1'b0;
            @(posedge clock);
            #1;
            if (done || !fault || dut.region_active_reg || dut.configure_ack_reg ||
                awvalid || wvalid)
                $fatal(1, "WRITER_PIPELINE_PEER_FAULT_STICKY_STATE_INVALID fault=%b active=%b ack=%b aw=%b w=%b",
                       fault, dut.region_active_reg, dut.configure_ack_reg, awvalid, wvalid);
            recover_after_reset("PEER_FAULT_E1");
        end
        else
        begin
            $fatal(1, "WRITER_PIPELINE_UNKNOWN_CASE %s", selected_case);
        end

        $display("CONFIGURE_WRITER_PIPELINE PASS CASE=%s", selected_case);
        $finish;
    end
endmodule
