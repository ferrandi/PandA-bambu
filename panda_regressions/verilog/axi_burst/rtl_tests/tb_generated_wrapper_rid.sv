module tb_generated_wrapper_rid;
  logic clock = 1'b0;
  always #5 clock = ~clock;

  logic reset = 1'b0;
  logic start_port = 1'b0;
  logic [1:0] in1 = 0;
  logic [31:0] in2 = 0;
  logic [31:0] in3 = 0;
  logic [31:0] in4 = 0;
  logic [31:0] in5 = 0;
  logic arready = 1'b1;
  logic [5:0] rid = 0;
  logic [31:0] rdata = 32'h1357_9bdf;
  logic [1:0] rresp = 0;
  logic rlast = 1'b1;
  logic rvalid = 1'b0;
  logic done_port;
  logic [31:0] out1;
  logic [5:0] arid;
  logic [31:0] araddr;
  logic [7:0] arlen;
  logic arvalid;
  logic rready;
  logic awvalid;
  logic wvalid;

  bit rlast_monitor_active = 1'b0;
  bit rlast_count_load_done = 1'b0;
  bit rlast_previous_done = 1'b0;
  integer rlast_ar_handshakes = 0;
  integer rlast_r_handshakes = 0;
  integer rlast_fifo_pops = 0;
  integer rlast_load_completions = 0;
  logic [7:0] rlast_observed_arlen;
  logic [5:0] rlast_observed_arid;
  logic [31:0] rlast_observed_araddr;
  logic rlast_observed_last;

  src_bambu_artificial_ParmMgr_modgen dut (
    .clock(clock), .reset(reset), .start_port(start_port),
    .in1(in1), .in2(in2), .in3(in3), .in4(in4), .in5(in5), .cache_reset(1'b0),
    .p_m_axi_src_awready(1'b0), .p_m_axi_src_wready(1'b0),
    .p_m_axi_src_bid(6'b0), .p_m_axi_src_bresp(2'b0),
    .p_m_axi_src_buser(1'b0), .p_m_axi_src_bvalid(1'b0),
    .p_m_axi_src_arready(arready), .p_m_axi_src_rid(rid),
    .p_m_axi_src_rdata(rdata), .p_m_axi_src_rresp(rresp),
    .p_m_axi_src_rlast(rlast), .p_m_axi_src_ruser(1'b0),
    .p_m_axi_src_rvalid(rvalid), .p_src(32'b0),
    .done_port(done_port), .out1(out1),
    .p_m_axi_src_arid(arid), .p_m_axi_src_araddr(araddr),
    .p_m_axi_src_arlen(arlen), .p_m_axi_src_arvalid(arvalid),
    .p_m_axi_src_rready(rready),
    .p_m_axi_src_awvalid(awvalid), .p_m_axi_src_wvalid(wvalid)
  );

  always @(posedge clock)
  begin
    if (rlast_monitor_active)
    begin
      if (arvalid && arready)
      begin
        rlast_ar_handshakes = rlast_ar_handshakes + 1;
        rlast_observed_arlen = arlen;
        rlast_observed_arid = arid;
        rlast_observed_araddr = araddr;
      end
      if (rvalid && rready)
      begin
        rlast_r_handshakes = rlast_r_handshakes + 1;
        rlast_observed_last = rlast;
      end
      if (dut.burst_engine.fifo_load_pop)
        rlast_fifo_pops = rlast_fifo_pops + 1;
      if (done_port && !rlast_previous_done && rlast_count_load_done)
        rlast_load_completions = rlast_load_completions + 1;
      rlast_previous_done = done_port;
      if (awvalid || wvalid)
        $fatal(1, "generated read wrapper emitted AXI write traffic during RLAST test");
    end
  end

  task automatic pulse_command(input logic [1:0] opcode,
                               input logic [31:0] size_bits,
                               input logic [31:0] address,
                               input logic [31:0] count);
    @(negedge clock);
    in1 = opcode;
    in2 = size_bits;
    in4 = address;
    in5 = count;
    start_port = 1'b1;
    @(negedge clock);
    start_port = 1'b0;
  endtask

  task automatic run_rid_case(input logic [5:0] response_id,
                              input bit expect_completion);
    integer cycles;
    bit saw_completion;
    logic [31:0] captured_out1;
    begin
      // Each RID is tested from a clean engine/configuration state.
      @(negedge clock);
      reset = 1'b0;
      start_port = 1'b0;
      rvalid = 1'b0;
      rid = 0;
      repeat (2) @(negedge clock);
      reset = 1'b1;

      // Configure one word. done_port during this opcode is intentionally
      // ignored: configure completion is not the read-completion assertion.
      pulse_command(2'd2, 32'd32, 32'h0000_1000, 32'd1);

      cycles = 0;
      while (!arvalid && cycles < 40)
      begin
        @(negedge clock);
        cycles = cycles + 1;
      end
      if (!arvalid)
        $fatal(1, "watchdog waiting for AR, RID=%0d", response_id);
      if (arid !== 6'b0 || araddr !== 32'h0000_1000 || arlen !== 8'd0)
        $fatal(1, "generated wrapper emitted wrong AR descriptor for RID=%0d", response_id);
      @(negedge clock); // AR handshake: arready is held high.

      cycles = 0;
      while (!rready && cycles < 40)
      begin
        @(negedge clock);
        cycles = cycles + 1;
      end
      if (!rready)
        $fatal(1, "watchdog waiting for RREADY, RID=%0d", response_id);

      rid = response_id;
      rvalid = 1'b1;
      cycles = 0;
      saw_completion = 1'b0;
      while (!saw_completion && cycles < 40)
      begin
        @(posedge clock);
        if (rready)
          saw_completion = 1'b1; // handshake sampled at this rising edge
        cycles = cycles + 1;
      end
      if (!saw_completion)
        $fatal(1, "watchdog waiting for R handshake, RID=%0d", response_id);
      @(negedge clock);
      rvalid = 1'b0;
      rid = 0;

      // Read from the returned FIFO word. Sampling at the active command
      // edge distinguishes a true scalar-read completion from configure done.
      @(negedge clock);
      in1 = 2'd0;
      in2 = 32'd32;
      in4 = 32'h0000_1000;
      in5 = 0;
      start_port = 1'b1;
      @(posedge clock);
      saw_completion = done_port;
      captured_out1 = out1;
      @(negedge clock);
      start_port = 1'b0;
      for (cycles = 0; cycles < 8; cycles = cycles + 1)
      begin
        @(negedge clock);
        if (done_port)
          saw_completion = 1'b1;
      end
      if (expect_completion)
      begin
        if (!saw_completion)
          $fatal(1, "RID=0 response did not complete the scalar read");
        if (captured_out1 !== 32'h1357_9bdf)
          $fatal(1, "RID=0 read returned %08x", captured_out1);
      end
      else if (saw_completion)
      begin
        $fatal(1, "nonzero RID=%0d incorrectly completed a scalar read", response_id);
      end
      $display("RID_CASE PASS rid=%0d completion=%0d", response_id, saw_completion);
    end
  endtask

  task automatic run_rlast_case(input bit missing_last,
                                input logic expected_r_last,
                                input logic [7:0] expected_arlen,
                                input integer expected_remaining);
    integer cycles;
    logic [31:0] count;
    string kind_name;
    begin
      count = missing_last ? 32'd1 : 32'd2;
      kind_name = missing_last ? "missing" : "early";
      @(negedge clock);
      reset = 1'b0;
      start_port = 1'b0;
      rvalid = 1'b0;
      rid = 0;
      rresp = 0;
      rlast = 1'b1;
      repeat (2) @(negedge clock);
      reset = 1'b1;
      repeat (2) @(negedge clock);

      rlast_ar_handshakes = 0;
      rlast_r_handshakes = 0;
      rlast_fifo_pops = 0;
      rlast_load_completions = 0;
      rlast_previous_done = done_port;
      rlast_count_load_done = 1'b0;
      rlast_monitor_active = 1'b1;

      pulse_command(2'd2, 32'd32, 32'h0000_1000, count);
      cycles = 0;
      while (rlast_ar_handshakes != 1 && cycles < 40)
      begin
        @(posedge clock);
        #1;
        cycles = cycles + 1;
      end
      if (rlast_ar_handshakes != 1 || rlast_observed_arlen !== expected_arlen ||
          rlast_observed_arid !== 6'b0 || rlast_observed_araddr !== 32'h0000_1000)
        $fatal(1, "generated wrapper emitted the wrong RLAST test AR descriptor");

      @(negedge clock);
      cycles = 0;
      while (!rready && cycles < 40)
      begin
        @(negedge clock);
        cycles = cycles + 1;
      end
      if (!rready)
        $fatal(1, "watchdog waiting for RREADY in generated wrapper RLAST test");
      rid = 0;
      rresp = 0;
      rlast = expected_r_last;
      rvalid = 1'b1;
      @(posedge clock);
      #1;
      if (rlast_r_handshakes != 1 || rlast_observed_last !== expected_r_last)
        $fatal(1, "generated wrapper did not accept the malformed RLAST beat");
      if (dut.burst_engine.fault_reg !== 1'b1)
        $fatal(1, "GENERATED_WRAPPER_RLAST_FAULT_NOT_OBSERVED");
      if (done_port || dut.burst_engine.descriptor_count_reg != 1 ||
          dut.burst_engine.descriptor_remaining_reg[dut.burst_engine.descriptor_rd_ptr_reg] != expected_remaining ||
          rlast_fifo_pops != 0)
        $fatal(1, "generated wrapper did not contain the malformed RLAST response");

      @(negedge clock);
      rvalid = 1'b0;
      rlast = 1'b1;
      repeat (2) @(negedge clock);
      if (done_port)
        $fatal(1, "configure completion remained asserted before the RLAST recovery probe");

      rlast_count_load_done = 1'b1;
      pulse_command(2'd0, 32'd32, 32'h0000_1000, 32'd0);
      repeat (8)
      begin
        @(posedge clock);
        #1;
        if (done_port || arvalid || awvalid || wvalid || dut.burst_engine.fifo_load_pop)
          $fatal(1, "malformed RLAST response completed a LOAD or produced bus/FIFO activity");
      end
      rlast_count_load_done = 1'b0;
      rlast_monitor_active = 1'b0;
      if (rlast_ar_handshakes != 1 || rlast_r_handshakes != 1 ||
          rlast_load_completions != 0 || rlast_fifo_pops != 0 ||
          dut.burst_engine.fault_reg !== 1'b1)
        $fatal(1, "generated wrapper RLAST observation counts did not match the fault contract");

      $display("RLAST_CASE PASS kind=%s requested=%0d ar=%0d r=%0d observed_last=%0d remaining=%0d completion=%0d fifo_pops=%0d fault=1",
               kind_name, count,
               rlast_ar_handshakes, rlast_r_handshakes, rlast_observed_last,
               expected_remaining, rlast_load_completions, rlast_fifo_pops);
    end
  endtask

  initial
  begin
    bit negative_sensitivity;
    if ($test$plusargs("RLAST_EARLY"))
    begin
      run_rlast_case(1'b0, 1'b1, 8'd1, 2);
      if ($test$plusargs("AXI_BURST_EXPECT_FAULT"))
      begin
        run_rid_case(6'd0, 1'b1);
        $display("GENERATED_WRAPPER_RLAST PASS kind=early recovery=1");
      end
      $finish;
    end
    if ($test$plusargs("RLAST_MISSING"))
    begin
      run_rlast_case(1'b1, 1'b0, 8'd0, 1);
      run_rid_case(6'd0, 1'b1);
      $display("GENERATED_WRAPPER_RLAST PASS kind=missing recovery=1");
      $finish;
    end
    negative_sensitivity = $test$plusargs("NEGATIVE_SENSITIVITY");
    if (negative_sensitivity)
    begin
      // With the deliberately broken bit-0-only scratch wrapper, these two
      // IDs look like zero. Their successful completion proves the positive
      // test is sensitive to losing upper RID bits.
      run_rid_case(6'd2, 1'b1);
      run_rid_case(6'd32, 1'b1);
      $display("RID_NEGATIVE_SENSITIVITY PASS (bit-0-only reduction accepted 2 and 32)");
    end
    else
    begin
      run_rid_case(6'd0, 1'b1);
      run_rid_case(6'd1, 1'b0);
      run_rid_case(6'd2, 1'b0);
      run_rid_case(6'd32, 1'b0);
      run_rid_case(6'd63, 1'b0);
      $display("GENERATED_WRAPPER_RID PASS");
    end
    $finish;
  end

  initial
  begin
    #200000;
    $fatal(1, "global watchdog expired");
  end
endmodule
