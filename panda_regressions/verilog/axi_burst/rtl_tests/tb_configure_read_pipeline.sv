module tb_configure_read_pipeline;
    localparam logic [1:0] OP_CONFIGURE = 2'd2;

    logic clock = 1'b0;
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
    wire [31:0] araddr;
    wire [7:0] arlen;
    wire [2:0] arsize;
    wire [1:0] arburst;
    wire arvalid;
    wire rready;
    string selected_case;

    always #5 clock = ~clock;

    MinimalAXI4MasterPipelined #(
        .B_MAX(1), .MAX_OUTSTANDING(1), .FIFO_DEPTH(1), .PEER_FAULT_ENABLE(1)
    ) dut (
        .clock(clock), .reset(reset), .start(start), .peer_fault(peer_fault),
        .in1(in1), .in2(in2), .in3(in3), .in4(in4), .in5(in5),
        .done(done), .out1(out1), .fault(fault),
        .m_axi_araddr(araddr), .m_axi_arlen(arlen), .m_axi_arsize(arsize),
        .m_axi_arburst(arburst), .m_axi_arid(), .m_axi_arvalid(arvalid),
        .m_axi_arready(1'b0), .m_axi_rdata(32'b0), .m_axi_rresp(2'b0),
        .m_axi_rid(1'b0), .m_axi_rlast(1'b0), .m_axi_rvalid(1'b0),
        .m_axi_rready(rready)
    );

    task automatic begin_valid_configure;
        begin
            @(negedge clock);
            start = 1'b1;
            in1 = OP_CONFIGURE;
            in2 = 32;
            in4 = 32'h00001000;
            in5 = 1;
            @(posedge clock);
            #1;
            if (done || fault || arvalid || dut.region_active_reg)
                $fatal(1, "READ_CONFIGURE_E0_NOT_QUIET done=%b fault=%b ARVALID=%b active=%b",
                       done, fault, arvalid, dut.region_active_reg);
        end
    endtask

    task automatic poison_inputs;
        begin
            start = 1'b0;
            in1 = 2'b0;
            in2 = 32'hdeadbeef;
            in3 = 32'hcafef00d;
            in4 = 32'hbad00001;
            in5 = 32'h80000000;
        end
    endtask

    task automatic recover_after_reset;
        integer cycles;
        begin
            @(negedge clock);
            reset = 1'b1;
            start = 1'b1;
            in1 = OP_CONFIGURE;
            in2 = 32;
            in4 = 32'h00002000;
            in5 = 1;
            @(posedge clock);
            #1;
            if (done || fault || arvalid || dut.region_active_reg)
                $fatal(1, "READ_RECOVERY_E0_NOT_QUIET");
            poison_inputs();
            cycles = 0;
            while (!done && !fault && cycles < 4)
            begin
                @(posedge clock);
                #1;
                cycles = cycles + 1;
            end
            if (!done || fault || !dut.region_active_reg || dut.region_base_reg != 32'h00002000 ||
                dut.region_count_reg != 1)
                $fatal(1, "READ_RECOVERY_E1_FAILED done=%b fault=%b active=%b base=%h cycles=%0d",
                       done, fault, dut.region_active_reg, dut.region_base_reg, cycles);
        end
    endtask

    initial
    begin
        if (!$value$plusargs("CASE=%s", selected_case))
            $fatal(1, "missing +CASE selector");
        repeat (3) @(posedge clock);
        @(negedge clock);
        reset = 1'b1;
        repeat (2) @(posedge clock);

        if (selected_case == "READ_RESET")
        begin
            begin_valid_configure();
            @(negedge clock);
            poison_inputs();
            reset = 1'b0;
            @(posedge clock);
            #1;
            if (done || fault || arvalid || dut.region_active_reg || dut.cfg_pipe_valid_reg)
                $fatal(1, "READ_RESET_DID_NOT_CANCEL_E0_CONFIG");
            repeat (2)
            begin
                @(posedge clock);
                #1;
                if (done || fault || arvalid || dut.region_active_reg)
                    $fatal(1, "READ_RESET_STALE_CONFIG_REPLAYED");
            end
            recover_after_reset();
        end
        else if (selected_case == "READ_PEER_FAULT")
        begin
            begin_valid_configure();
            @(negedge clock);
            poison_inputs();
            peer_fault = 1'b1;
            @(posedge clock);
            #1;
            if (!fault || done || arvalid || dut.region_active_reg)
                $fatal(1, "READ_PEER_FAULT_DID_NOT_CANCEL_E1_CONFIG fault=%b done=%b ARVALID=%b active=%b",
                       fault, done, arvalid, dut.region_active_reg);
            @(negedge clock);
            peer_fault = 1'b0;
            repeat (2)
            begin
                @(posedge clock);
                #1;
                if (!fault || done || arvalid || dut.region_active_reg || dut.cfg_pipe_valid_reg)
                    $fatal(1, "READ_PEER_FAULT_NOT_STICKY_OR_CONFIG_REPLAYED fault=%b done=%b ARVALID=%b active=%b pending=%b",
                           fault, done, arvalid, dut.region_active_reg, dut.cfg_pipe_valid_reg);
            end
            @(negedge clock);
            reset = 1'b0;
            @(posedge clock);
            #1;
            if (fault || done || arvalid || dut.region_active_reg || dut.cfg_pipe_valid_reg)
                $fatal(1, "READ_PEER_FAULT_RESET_DID_NOT_CLEAR_STATE");
            recover_after_reset();
        end
        else
        begin
            $fatal(1, "unknown CASE %s", selected_case);
        end

        $display("READ_CONFIG_PIPELINE PASS CASE=%s", selected_case);
        $finish;
    end
endmodule
