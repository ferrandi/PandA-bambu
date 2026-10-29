module tb_generated_memaxi_response;
  logic clock = 1'b0;
  always #5 clock = ~clock;

  logic reset = 1'b0;
  logic start_port = 1'b0;
  logic [31:0] n = 1;
  logic [31:0] src = 32'h0000_1000;
  logic [31:0] dst = 32'h0000_2000;
  logic [31:0] seed = 32'hcafe_0000;
  logic cache_reset = 1'b0;
  wire done_port;
  wire idle_port;
  wire [31:0] return_port;

  wire [5:0] arid;
  wire [31:0] araddr;
  wire [7:0] arlen;
  wire [2:0] arsize;
  wire [1:0] arburst;
  wire arlock;
  wire [3:0] arcache;
  wire [2:0] arprot;
  wire [3:0] arqos;
  wire [3:0] arregion;
  wire aruser;
  wire arvalid;
  wire arready;
  wire [5:0] rid;
  wire model_rid;
  wire [31:0] rdata;
  wire [1:0] rresp;
  wire rlast;
  wire rvalid;
  wire rready;
  wire [5:0] awid;
  wire [31:0] awaddr;
  wire [7:0] awlen;
  wire [2:0] awsize;
  wire [1:0] awburst;
  wire awlock;
  wire [3:0] awcache;
  wire [2:0] awprot;
  wire [3:0] awqos;
  wire [3:0] awregion;
  wire awuser;
  wire awvalid;
  wire awready;
  wire [31:0] wdata;
  wire [3:0] wstrb;
  wire wlast;
  wire wuser;
  wire wvalid;
  wire wready;
  wire [5:0] bid;
  wire model_bid;
  wire [1:0] bresp;
  wire bvalid;
  wire bready;

  integer read_fault_handshakes = 0;
  integer write_fault_handshakes = 0;
  integer read_okay_handshakes = 0;
  integer write_okay_handshakes = 0;
  integer done_pulses = 0;
  assign rid = {5'b0, model_rid};
  assign bid = {5'b0, model_bid};

`ifdef RESPONSE_READ
  kernel dut (
    .clock(clock), .reset(reset), .start_port(start_port), .src(src), .n(n),
    .cache_reset(cache_reset),
    .m_axi_src_awready(awready), .m_axi_src_wready(wready), .m_axi_src_bid(bid),
    .m_axi_src_bresp(bresp), .m_axi_src_buser(1'b0), .m_axi_src_bvalid(bvalid),
    .m_axi_src_arready(arready), .m_axi_src_rid(rid), .m_axi_src_rdata(rdata),
    .m_axi_src_rresp(rresp), .m_axi_src_rlast(rlast), .m_axi_src_ruser(1'b0),
    .m_axi_src_rvalid(rvalid), .done_port(done_port), .idle_port(idle_port),
    .return_port(return_port), .m_axi_src_awid(awid), .m_axi_src_awaddr(awaddr),
    .m_axi_src_awlen(awlen), .m_axi_src_awsize(awsize), .m_axi_src_awburst(awburst),
    .m_axi_src_awlock(awlock), .m_axi_src_awcache(arcache), .m_axi_src_awprot(awprot),
    .m_axi_src_awqos(awqos), .m_axi_src_awregion(awregion), .m_axi_src_awuser(awuser),
    .m_axi_src_awvalid(awvalid), .m_axi_src_wdata(wdata), .m_axi_src_wstrb(wstrb),
    .m_axi_src_wlast(wlast), .m_axi_src_wuser(wuser), .m_axi_src_wvalid(wvalid),
    .m_axi_src_bready(bready), .m_axi_src_arid(arid), .m_axi_src_araddr(araddr),
    .m_axi_src_arlen(arlen), .m_axi_src_arsize(arsize), .m_axi_src_arburst(arburst),
    .m_axi_src_arlock(arlock), .m_axi_src_arcache(arcache), .m_axi_src_arprot(arprot),
    .m_axi_src_arqos(arqos), .m_axi_src_arregion(arregion), .m_axi_src_aruser(aruser),
    .m_axi_src_arvalid(arvalid), .m_axi_src_rready(rready)
  );
`else
  write_count_seed dut (
    .clock(clock), .reset(reset), .start_port(start_port), .dst(dst), .n(n), .seed(seed),
    .cache_reset(cache_reset),
    .m_axi_output_awready(awready), .m_axi_output_wready(wready),
    .m_axi_output_bid(bid), .m_axi_output_bresp(bresp), .m_axi_output_buser(1'b0),
    .m_axi_output_bvalid(bvalid), .m_axi_output_arready(arready),
    .m_axi_output_rid(rid), .m_axi_output_rdata(rdata), .m_axi_output_rresp(rresp),
    .m_axi_output_rlast(rlast), .m_axi_output_ruser(1'b0), .m_axi_output_rvalid(rvalid),
    .done_port(done_port), .idle_port(idle_port),
    .m_axi_output_awid(awid), .m_axi_output_awaddr(awaddr), .m_axi_output_awlen(awlen),
    .m_axi_output_awsize(awsize), .m_axi_output_awburst(awburst), .m_axi_output_awlock(awlock),
    .m_axi_output_awcache(awcache), .m_axi_output_awprot(awprot), .m_axi_output_awqos(awqos),
    .m_axi_output_awregion(awregion), .m_axi_output_awuser(awuser), .m_axi_output_awvalid(awvalid),
    .m_axi_output_wdata(wdata), .m_axi_output_wstrb(wstrb), .m_axi_output_wlast(wlast),
    .m_axi_output_wuser(wuser), .m_axi_output_wvalid(wvalid), .m_axi_output_bready(bready),
    .m_axi_output_arid(arid), .m_axi_output_araddr(araddr), .m_axi_output_arlen(arlen),
    .m_axi_output_arsize(arsize), .m_axi_output_arburst(arburst), .m_axi_output_arlock(arlock),
    .m_axi_output_arcache(arcache), .m_axi_output_arprot(arprot), .m_axi_output_arqos(arqos),
    .m_axi_output_arregion(arregion), .m_axi_output_aruser(aruser),
    .m_axi_output_arvalid(arvalid), .m_axi_output_rready(rready)
  );
  assign return_port = '0;
`endif

  TestbenchMEMAXI #(.READ_DELAY(1), .WRITE_DELAY(1)) model (
    .clock(clock), .reset(reset),
    .arid(arid[0]), .araddr(araddr), .arlen(arlen), .arsize(arsize), .arburst(arburst),
    .arlock(arlock), .arcache(arcache), .arprot(arprot), .arqos(arqos),
    .arregion(arregion), .aruser(aruser), .arvalid(arvalid), .arready(arready),
    .rid(model_rid), .rdata(rdata), .rresp(rresp), .ruser(), .rlast(rlast),
    .rvalid(rvalid), .rready(rready),
    .awid(awid[0]), .awaddr(awaddr), .awlen(awlen), .awsize(awsize), .awburst(awburst),
    .awlock(awlock), .awcache(awcache), .awprot(awprot), .awqos(awqos),
    .awregion(awregion), .awuser(awuser), .awvalid(awvalid), .awready(awready),
    .wdata(wdata), .wstrb(wstrb), .wlast(wlast), .wvalid(wvalid), .wready(wready),
    .wuser(wuser), .bid(model_bid), .bresp(bresp), .buser(), .bvalid(bvalid), .bready(bready)
  );

  always @(posedge clock)
  begin
    if (!reset)
    begin
      done_pulses <= 0;
      read_fault_handshakes <= 0;
      write_fault_handshakes <= 0;
      read_okay_handshakes <= 0;
      write_okay_handshakes <= 0;
    end
    else
    begin
      if (done_port)
        done_pulses <= done_pulses + 1;
      if (rvalid && rready)
      begin
        if (rresp != 0)
          read_fault_handshakes <= read_fault_handshakes + 1;
        else
          read_okay_handshakes <= read_okay_handshakes + 1;
      end
      if (bvalid && bready)
      begin
        if (bresp != 0)
          write_fault_handshakes <= write_fault_handshakes + 1;
        else
          write_okay_handshakes <= write_okay_handshakes + 1;
      end
    end
  end

  initial
  begin : run_case
    integer cycles;
    integer calls_before_recovery;
    bit fault_mode;
    fault_mode = $test$plusargs("FAULT");
    repeat (4) @(negedge clock);
    reset = 1'b1;
    repeat (2) @(negedge clock);

`ifdef RESPONSE_READ
    if (fault_mode)
      model.inject_next_read_response(2'b10);
    @(negedge clock);
    start_port = 1'b1;
    @(negedge clock);
    start_port = 1'b0;
    if (fault_mode)
    begin
      cycles = 0;
      while (read_fault_handshakes == 0 && cycles < 500)
      begin
        @(negedge clock);
        cycles = cycles + 1;
      end
      if (read_fault_handshakes != 1 || done_pulses != 0)
        $fatal(1, "generated read wrapper did not contain injected RRESP: handshakes=%0d done_pulses=%0d",
               read_fault_handshakes, done_pulses);
      repeat (4) @(negedge clock);
      if (done_pulses != 0)
        $fatal(1, "generated read wrapper reported success after injected RRESP");
      $display("GENERATED_MEMAXI_RESPONSE_CONTAINED mode=read response=SLVERR handshakes=1 done=0");

      calls_before_recovery = model.m_utils.read_calls;
      reset = 1'b0;
      start_port = 1'b0;
      repeat (3) @(negedge clock);
      if (done_port || !idle_port)
        $fatal(1, "generated read wrapper did not return idle during recovery reset");
      reset = 1'b1;
      repeat (2) @(negedge clock);
      if (read_fault_handshakes != 0 || read_okay_handshakes != 0 || done_pulses != 0)
        $fatal(1, "generated read wrapper monitor did not clear on recovery reset");
      start_port = 1'b1;
      @(negedge clock);
      start_port = 1'b0;
      cycles = 0;
      while (!done_port && cycles < 500)
      begin
        @(negedge clock);
        cycles = cycles + 1;
      end
      if (!done_port || return_port !== 32'b0 || read_okay_handshakes != 1 ||
          model.m_utils.read_calls != calls_before_recovery + 1 ||
          model.m_utils.read_addr[calls_before_recovery] != 32'h0000_1000)
        $fatal(1, "generated read wrapper did not recover after reset: done=%b result=%08x okay=%0d reads=%0d",
               done_port, return_port, read_okay_handshakes,
               model.m_utils.read_calls - calls_before_recovery);
      $display("GENERATED_MEMAXI_RESPONSE_RECOVERY mode=read reset=1 reinvocation=1 handshakes=%0d result=%08x",
               read_okay_handshakes, return_port);
    end
    else
    begin
      cycles = 0;
      while (!done_port && cycles < 500)
      begin
        @(negedge clock);
        cycles = cycles + 1;
      end
      if (!done_port || return_port !== 32'b0)
        $fatal(1, "generated read wrapper OKAY control failed: done=%b result=%08x",
               done_port, return_port);
      $display("GENERATED_MEMAXI_RESPONSE_CONTROL mode=read done=1 result=%08x", return_port);
    end
`else
    if (fault_mode)
      model.inject_next_write_response(2'b10);
    @(negedge clock);
    start_port = 1'b1;
    @(negedge clock);
    start_port = 1'b0;
    if (fault_mode)
    begin
      cycles = 0;
      while (write_fault_handshakes == 0 && cycles < 500)
      begin
        @(negedge clock);
        cycles = cycles + 1;
      end
      if (write_fault_handshakes != 1 || done_pulses != 0)
        $fatal(1, "generated write wrapper did not contain injected BRESP: handshakes=%0d done_pulses=%0d",
               write_fault_handshakes, done_pulses);
      repeat (4) @(negedge clock);
      if (done_pulses != 0)
        $fatal(1, "generated write wrapper reported success after injected BRESP");
      $display("GENERATED_MEMAXI_RESPONSE_CONTAINED mode=write response=SLVERR handshakes=1 done=0");

      calls_before_recovery = model.m_utils.write_calls;
      reset = 1'b0;
      start_port = 1'b0;
      repeat (3) @(negedge clock);
      if (done_port || !idle_port)
        $fatal(1, "generated write wrapper did not return idle during recovery reset");
      reset = 1'b1;
      repeat (2) @(negedge clock);
      if (write_fault_handshakes != 0 || write_okay_handshakes != 0 || done_pulses != 0)
        $fatal(1, "generated write wrapper monitor did not clear on recovery reset");
      start_port = 1'b1;
      @(negedge clock);
      start_port = 1'b0;
      cycles = 0;
      while (!done_port && cycles < 500)
      begin
        @(negedge clock);
        cycles = cycles + 1;
      end
      if (!done_port || write_okay_handshakes != 1 ||
          model.m_utils.write_calls != calls_before_recovery + 1 ||
          model.m_utils.write_addr[calls_before_recovery] != 32'h0000_2000 ||
          model.m_utils.write_data[calls_before_recovery] != seed)
        $fatal(1, "generated write wrapper did not recover after reset: done=%b okay=%0d commits=%0d",
               done_port, write_okay_handshakes,
               model.m_utils.write_calls - calls_before_recovery);
      $display("GENERATED_MEMAXI_RESPONSE_RECOVERY mode=write reset=1 reinvocation=1 handshakes=%0d commits=%0d",
               write_okay_handshakes, model.m_utils.write_calls - calls_before_recovery);
    end
    else
    begin
      cycles = 0;
      while (!done_port && cycles < 500)
      begin
        @(negedge clock);
        cycles = cycles + 1;
      end
      if (!done_port || model.m_utils.write_calls != 1)
        $fatal(1, "generated write wrapper OKAY control failed: done=%b writes=%0d",
               done_port, model.m_utils.write_calls);
      $display("GENERATED_MEMAXI_RESPONSE_CONTROL mode=write done=1 commits=%0d", model.m_utils.write_calls);
    end
`endif
    $finish;
  end

  initial
  begin
    #100000;
    $fatal(1, "generated wrapper response test watchdog expired");
  end
endmodule
