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
    .p_m_axi_src_rready(rready)
  );

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
      while (!arvalid && cycles < 40) begin
        @(negedge clock);
        cycles = cycles + 1;
      end
      if (!arvalid)
        $fatal(1, "watchdog waiting for AR, RID=%0d", response_id);
      if (arid !== 6'b0 || araddr !== 32'h0000_1000 || arlen !== 8'd0)
        $fatal(1, "generated wrapper emitted wrong AR descriptor for RID=%0d", response_id);
      @(negedge clock); // AR handshake: arready is held high.

      cycles = 0;
      while (!rready && cycles < 40) begin
        @(negedge clock);
        cycles = cycles + 1;
      end
      if (!rready)
        $fatal(1, "watchdog waiting for RREADY, RID=%0d", response_id);

      rid = response_id;
      rvalid = 1'b1;
      cycles = 0;
      saw_completion = 1'b0;
      while (!saw_completion && cycles < 40) begin
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
      for (cycles = 0; cycles < 8; cycles = cycles + 1) begin
        @(negedge clock);
        if (done_port)
          saw_completion = 1'b1;
      end
      if (expect_completion) begin
        if (!saw_completion)
          $fatal(1, "RID=0 response did not complete the scalar read");
        if (captured_out1 !== 32'h1357_9bdf)
          $fatal(1, "RID=0 read returned %08x", captured_out1);
      end else if (saw_completion) begin
        $fatal(1, "nonzero RID=%0d incorrectly completed a scalar read", response_id);
      end
      $display("RID_CASE PASS rid=%0d completion=%0d", response_id, saw_completion);
    end
  endtask

  initial begin
    bit negative_sensitivity;
    negative_sensitivity = $test$plusargs("NEGATIVE_SENSITIVITY");
    if (negative_sensitivity) begin
      // With the deliberately broken bit-0-only scratch wrapper, these two
      // IDs look like zero. Their successful completion proves the positive
      // test is sensitive to losing upper RID bits.
      run_rid_case(6'd2, 1'b1);
      run_rid_case(6'd32, 1'b1);
      $display("RID_NEGATIVE_SENSITIVITY PASS (bit-0-only reduction accepted 2 and 32)");
    end else begin
      run_rid_case(6'd0, 1'b1);
      run_rid_case(6'd1, 1'b0);
      run_rid_case(6'd2, 1'b0);
      run_rid_case(6'd32, 1'b0);
      run_rid_case(6'd63, 1'b0);
      $display("GENERATED_WRAPPER_RID PASS");
    end
    $finish;
  end

  initial begin
    #200000;
    $fatal(1, "global watchdog expired");
  end
endmodule
