module tb_generated_memaxi_response;
  logic clock = 1'b0;
  always #5 clock = ~clock;

  logic reset = 1'b0;
  bit reset_active_high = 1'b0;
  bit reset_async_mode = 1'b0;
  bit reset_pending_mode = 1'b0;
  string reset_type_name = "sync";
  string reset_level_name = "low";
  wire dut_reset = reset_active_high ? ~reset : reset;
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
  wire model_rvalid;
  wire [5:0] rid;
  wire model_rid;
  wire [31:0] rdata;
  wire [1:0] rresp;
  wire rlast;
  wire rvalid;
  wire rready;
  wire model_bvalid;
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

  logic response_gate = 1'b1;
  integer ar_handshakes = 0;
  integer aw_handshakes = 0;
  integer w_handshakes = 0;
  integer r_handshakes = 0;
  integer rlast_handshakes = 0;
  integer b_handshakes = 0;
  reg [31:0] observed_araddr [0:31];
  reg [7:0] observed_arlen [0:31];
  reg [2:0] observed_arsize [0:31];
  reg [1:0] observed_arburst [0:31];
  reg [31:0] observed_awaddr [0:31];
  reg [7:0] observed_awlen [0:31];
  reg [2:0] observed_awsize [0:31];
  reg [1:0] observed_awburst [0:31];
  integer observed_wlast = 0;

  integer read_fault_handshakes = 0;
  integer write_fault_handshakes = 0;
  integer read_okay_handshakes = 0;
  integer write_okay_handshakes = 0;
  integer done_pulses = 0;
  assign rid = {5'b0, model_rid};
  assign bid = {5'b0, model_bid};
  assign rvalid = model_rvalid && response_gate;
  assign bvalid = model_bvalid && response_gate;

`ifdef RESPONSE_READ
  kernel dut (
    .clock(clock), .reset(dut_reset), .start_port(start_port), .src(src), .n(n),
    .cache_reset(cache_reset),
    .m_axi_src_awready(awready), .m_axi_src_wready(wready), .m_axi_src_bid(bid),
    .m_axi_src_bresp(bresp), .m_axi_src_buser(1'b0), .m_axi_src_bvalid(bvalid),
    .m_axi_src_arready(arready), .m_axi_src_rid(rid), .m_axi_src_rdata(rdata),
    .m_axi_src_rresp(rresp), .m_axi_src_rlast(rlast), .m_axi_src_ruser(1'b0),
    .m_axi_src_rvalid(rvalid), .done_port(done_port), .idle_port(idle_port),
    .return_port(return_port), .m_axi_src_awid(awid), .m_axi_src_awaddr(awaddr),
    .m_axi_src_awlen(awlen), .m_axi_src_awsize(awsize), .m_axi_src_awburst(awburst),
    .m_axi_src_awlock(awlock), .m_axi_src_awcache(awcache), .m_axi_src_awprot(awprot),
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
`ifdef RESPONSE_WRITE_MULTI
  write_count_seed_three
`elsif RESPONSE_WRITE_B1O3
  write_count_seed_single_beat
`else
  write_count_seed
`endif
  dut (
    .clock(clock), .reset(dut_reset), .start_port(start_port), .dst(dst), .n(n), .seed(seed),
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

  TestbenchMEMAXI #(.QUEUE_SIZE(8), .READ_DELAY(1), .WRITE_DELAY(1)) model (
    .clock(clock), .reset(reset),
    .arid(arid[0]), .araddr(araddr), .arlen(arlen), .arsize(arsize), .arburst(arburst),
    .arlock(arlock), .arcache(arcache), .arprot(arprot), .arqos(arqos),
    .arregion(arregion), .aruser(aruser), .arvalid(arvalid), .arready(arready),
    .rid(model_rid), .rdata(rdata), .rresp(rresp), .ruser(), .rlast(rlast),
    .rvalid(model_rvalid), .rready(rready && response_gate),
    .awid(awid[0]), .awaddr(awaddr), .awlen(awlen), .awsize(awsize), .awburst(awburst),
    .awlock(awlock), .awcache(awcache), .awprot(awprot), .awqos(awqos),
    .awregion(awregion), .awuser(awuser), .awvalid(awvalid), .awready(awready),
    .wdata(wdata), .wstrb(wstrb), .wlast(wlast), .wvalid(wvalid), .wready(wready),
    .wuser(wuser), .bid(model_bid), .bresp(bresp), .buser(), .bvalid(model_bvalid), .bready(bready && response_gate)
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
      ar_handshakes <= 0;
      aw_handshakes <= 0;
      w_handshakes <= 0;
      r_handshakes <= 0;
      rlast_handshakes <= 0;
      b_handshakes <= 0;
      observed_wlast <= 0;
    end
    else
    begin
      if (arvalid && arready)
      begin
        if (ar_handshakes < 32)
        begin
          observed_araddr[ar_handshakes] <= araddr;
          observed_arlen[ar_handshakes] <= arlen;
          observed_arsize[ar_handshakes] <= arsize;
          observed_arburst[ar_handshakes] <= arburst;
        end
        ar_handshakes <= ar_handshakes + 1;
      end
      if (awvalid && awready)
      begin
        if (aw_handshakes < 32)
        begin
          observed_awaddr[aw_handshakes] <= awaddr;
          observed_awlen[aw_handshakes] <= awlen;
          observed_awsize[aw_handshakes] <= awsize;
          observed_awburst[aw_handshakes] <= awburst;
        end
        aw_handshakes <= aw_handshakes + 1;
      end
      if (wvalid && wready)
      begin
        w_handshakes <= w_handshakes + 1;
        if (wlast)
          observed_wlast <= observed_wlast + 1;
      end
      if (done_port)
        done_pulses <= done_pulses + 1;
      if (rvalid && rready)
      begin
        r_handshakes <= r_handshakes + 1;
        if (rlast)
          rlast_handshakes <= rlast_handshakes + 1;
        if (rresp != 0)
          read_fault_handshakes <= read_fault_handshakes + 1;
        else
          read_okay_handshakes <= read_okay_handshakes + 1;
      end
      if (bvalid && bready)
      begin
        b_handshakes <= b_handshakes + 1;
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
    integer i;
    bit fault_mode;
    bit multi_mode;
    bit b1_o3_mode;
    reset_pending_mode = $test$plusargs("RESET_PENDING");
    reset_active_high = $test$plusargs("RESET_ACTIVE_HIGH");
    reset_async_mode = $test$plusargs("RESET_ASYNC");
    reset_type_name = reset_async_mode ? "async" : "sync";
    reset_level_name = reset_active_high ? "high" : "low";
    fault_mode = $test$plusargs("FAULT");
    multi_mode = $test$plusargs("MULTI");
    b1_o3_mode = $test$plusargs("B1O3");
    repeat (4) @(negedge clock);
    reset = 1'b1;
    repeat (2) @(negedge clock);

`ifdef RESPONSE_READ
    if (reset_pending_mode)
    begin
      n = 48;
      src = 32'h0000_3000;
      response_gate = 1'b0;
      for (i = 0; i < 48; i = i + 1)
        model.m_utils.set_word(src + i*4, i + 1);
    end
    if (b1_o3_mode)
    begin
      n = 17;
      response_gate = 1'b0;
      for (i = 0; i < 17; i = i + 1)
        model.m_utils.set_word(src + i*4, i + 1);
    end
    if (multi_mode)
      n = 112;
    if (fault_mode)
      model.inject_next_read_response(2'b10);
    if (multi_mode)
      response_gate = 1'b0;
    @(negedge clock);
    start_port = 1'b1;
    @(negedge clock);
    start_port = 1'b0;
    if (reset_pending_mode)
    begin
      cycles = 0;
      while ((ar_handshakes < 3 || !model_rvalid) && cycles < 1000)
      begin
        @(negedge clock);
        cycles = cycles + 1;
      end
      if (ar_handshakes != 3 || r_handshakes != 0 || !model_rvalid || !rready || done_port || done_pulses != 0)
        $fatal(1, "pending-reset read did not reach three queued requests: ar=%0d r=%0d model_rvalid=%b rready=%b done=%b",
               ar_handshakes, r_handshakes, model_rvalid, rready, done_port);
      for (i = 0; i < 3; i = i + 1)
        if (observed_araddr[i] !== src + i*64 || observed_arlen[i] !== 8'd15 ||
            observed_arsize[i] !== 3'd2 || observed_arburst[i] !== 2'b01)
          $fatal(1, "pending-reset read descriptor mismatch index=%0d addr=%h len=%0d size=%0d burst=%b",
                 i, observed_araddr[i], observed_arlen[i], observed_arsize[i], observed_arburst[i]);
      $display("GENERATED_MEMAXI_RESPONSE_RESET_PENDING engine=read reset_type=%s reset_level=%s ar=%0d r=%0d model_pending=1 done=0",
               reset_type_name, reset_level_name, ar_handshakes, r_handshakes);
      reset = 1'b0;
      start_port = 1'b0;
      if (reset_async_mode)
      begin
        #1;
        if (!idle_port || done_port || rready)
          $fatal(1, "async pending-reset read did not clear controller/engine before the next active edge: idle=%b rready=%b done=%b",
                 idle_port, rready, done_port);
      end
      else
      begin
        #1;
        if (!rready)
          $fatal(1, "sync pending-reset read cleared engine before the next active edge");
      end
      @(posedge clock);
      #1;
      if (rready)
        $fatal(1, "pending-reset read engine did not clear on the reset clock edge");
      repeat (2) @(negedge clock);
      if (done_port || !idle_port || model_rvalid)
        $fatal(1, "pending-reset read did not clear DUT/model state: idle=%b done=%b model_rvalid=%b",
               idle_port, done_port, model_rvalid);
      model.m_utils.reset_counters();
      src = 32'h0000_4000;
      n = 17;
      for (i = 0; i < 17; i = i + 1)
        model.m_utils.set_word(src + i*4, i + 1);
      response_gate = 1'b1;
      reset = 1'b1;
      repeat (2) @(negedge clock);
      start_port = 1'b1;
      @(negedge clock);
      start_port = 1'b0;
      cycles = 0;
      while (!done_port && cycles < 1000)
      begin
        @(negedge clock);
        cycles = cycles + 1;
      end
      if (!done_port)
        $fatal(1, "pending-reset read recovery did not complete within the cycle limit");
      @(posedge clock);
      @(negedge clock);
      if (return_port !== 32'd153 || ar_handshakes != 2 ||
          r_handshakes != 17 || rlast_handshakes != 2 || model.m_utils.read_calls != 17 || done_pulses != 1)
        $fatal(1, "pending-reset read recovery mismatch done=%b result=%0d ar=%0d r=%0d last=%0d reads=%0d pulses=%0d",
               done_port, return_port, ar_handshakes, r_handshakes, rlast_handshakes,
               model.m_utils.read_calls, done_pulses);
      for (i = 0; i < 17; i = i + 1)
        if (model.m_utils.read_addr[i] !== src + i*4 || model.m_utils.read_data[i] !== i + 1)
          $fatal(1, "pending-reset read recovery data mismatch index=%0d addr=%h data=%0d",
                 i, model.m_utils.read_addr[i], model.m_utils.read_data[i]);
      repeat (8) @(negedge clock);
      if (ar_handshakes != 2 || r_handshakes != 17 || done_pulses != 1)
        $fatal(1, "pending-reset read recovery had extra work: ar=%0d r=%0d done=%0d",
               ar_handshakes, r_handshakes, done_pulses);
      $display("GENERATED_MEMAXI_RESPONSE_RESET_RECOVERY engine=read reset_type=%s reset_level=%s reinvocation=1 ar=%0d r=%0d rlast=%0d sum=%0d backend=%0d done=1",
               reset_type_name, reset_level_name, ar_handshakes, r_handshakes,
               rlast_handshakes, return_port, model.m_utils.read_calls);
    end
    else if (b1_o3_mode)
    begin
      cycles = 0;
      while (ar_handshakes < 3 && cycles < 500)
      begin
        @(negedge clock);
        cycles = cycles + 1;
      end
      if (ar_handshakes != 3 || r_handshakes != 0)
        $fatal(1, "B1/O3 read did not queue three requests before responses: ar=%0d r=%0d",
               ar_handshakes, r_handshakes);
      for (i = 0; i < 3; i = i + 1)
        if (observed_araddr[i] !== src + i*4 || observed_arlen[i] !== 0 ||
            observed_arsize[i] !== 3'd2 || observed_arburst[i] !== 2'b01)
          $fatal(1, "B1/O3 read descriptor mismatch index=%0d addr=%h len=%0d size=%0d burst=%b",
                 i, observed_araddr[i], observed_arlen[i], observed_arsize[i], observed_arburst[i]);
      response_gate = 1'b1;
      cycles = 0;
      while (!done_port && cycles < 1000)
      begin
        @(negedge clock);
        cycles = cycles + 1;
      end
      if (!done_port)
        $fatal(1, "B1/O3 read did not complete within the cycle limit");
      @(posedge clock);
      @(negedge clock);
      if (return_port !== 32'd153 || ar_handshakes != 17 ||
          r_handshakes != 17 || rlast_handshakes != 17 || model.m_utils.read_calls != 17 || done_pulses != 1)
        $fatal(1, "B1/O3 read totals mismatch done=%b result=%0d ar=%0d r=%0d last=%0d reads=%0d done_pulses=%0d",
               done_port, return_port, ar_handshakes, r_handshakes, rlast_handshakes,
               model.m_utils.read_calls, done_pulses);
      for (i = 0; i < 17; i = i + 1)
        if (observed_araddr[i] !== src + i*4 || observed_arlen[i] !== 0 ||
            observed_arsize[i] !== 3'd2 || observed_arburst[i] !== 2'b01 ||
            model.m_utils.read_addr[i] !== src + i*4 || model.m_utils.read_data[i] !== i + 1)
          $fatal(1, "B1/O3 read transfer mismatch index=%0d araddr=%h backend_addr=%h data=%h",
                 i, observed_araddr[i], model.m_utils.read_addr[i], model.m_utils.read_data[i]);
      repeat (8) @(negedge clock);
      if (ar_handshakes != 17 || r_handshakes != 17 || done_pulses != 1)
        $fatal(1, "B1/O3 read had extra work after completion: ar=%0d r=%0d done=%0d",
               ar_handshakes, r_handshakes, done_pulses);
      $display("GENERATED_MEMAXI_RESPONSE_B1O3 mode=read n=17 outstanding_before_response=3 ar=17 r=17 rlast=17 sum=153 backend=17 done=1");
    end
    else if (fault_mode)
    begin
      if (multi_mode)
      begin
        cycles = 0;
        while (ar_handshakes < 5 && cycles < 500)
        begin
          @(negedge clock);
          cycles = cycles + 1;
        end
        if (ar_handshakes != 5 || r_handshakes != 0)
          $fatal(1, "multi-read did not queue five descriptors before responses: ar=%0d r=%0d",
                 ar_handshakes, r_handshakes);
        for (i = 0; i < 5; i = i + 1)
          if (observed_araddr[i] !== 32'h0000_1000 + i*64 || observed_arlen[i] !== 8'd15 ||
              observed_arsize[i] !== 3'd2)
            $fatal(1, "multi-read descriptor mismatch index=%0d addr=%h len=%0d size=%0d",
                   i, observed_araddr[i], observed_arlen[i], observed_arsize[i]);
        response_gate = 1'b1;
        cycles = 0;
        while (r_handshakes < 80 && cycles < 1000)
        begin
          @(negedge clock);
          cycles = cycles + 1;
        end
        if (r_handshakes != 80 || read_fault_handshakes != 16 || read_okay_handshakes != 64 ||
            rlast_handshakes != 5 || ar_handshakes != 5 || model.m_utils.read_calls != 80 || done_pulses != 0)
          $fatal(1, "multi-read drain mismatch ar=%0d r=%0d fault=%0d okay=%0d last=%0d reads=%0d done=%0d",
                 ar_handshakes, r_handshakes, read_fault_handshakes, read_okay_handshakes,
                 rlast_handshakes, model.m_utils.read_calls, done_pulses);
        repeat (8) @(negedge clock);
        if (ar_handshakes != 5 || done_pulses != 0)
          $fatal(1, "multi-read issued work or completed after fault: ar=%0d done=%0d", ar_handshakes, done_pulses);
        $display("GENERATED_MEMAXI_RESPONSE_DRAIN mode=read descriptors=5 beats=80 fault_beats=16 okay_beats=64 rlast=5 done=0");
      end
      else
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
      end

      calls_before_recovery = model.m_utils.read_calls;
      n = 1;
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
    if (reset_pending_mode)
    begin
      n = 48;
      dst = 32'h0000_3000;
      seed = 0;
      response_gate = 1'b0;
    end
    if (b1_o3_mode)
    begin
      n = 17;
      seed = 100;
      response_gate = 1'b0;
    end
    if (multi_mode)
      n = 80;
    if (fault_mode)
      model.inject_next_write_response(2'b10);
    if (multi_mode)
      response_gate = 1'b0;
    @(negedge clock);
    start_port = 1'b1;
    @(negedge clock);
    start_port = 1'b0;
    if (reset_pending_mode)
    begin
      cycles = 0;
      while ((aw_handshakes < 3 || w_handshakes < 48 || !model_bvalid) && cycles < 1500)
      begin
        @(negedge clock);
        cycles = cycles + 1;
      end
      if (aw_handshakes != 3 || w_handshakes != 48 || observed_wlast != 3 ||
          model.m_utils.write_calls != 48 || b_handshakes != 0 || !model_bvalid || !bready || done_port || done_pulses != 0)
        $fatal(1, "pending-reset write did not reach three queued responses: aw=%0d w=%0d wlast=%0d commits=%0d b=%0d model_bvalid=%b bready=%b done=%b",
               aw_handshakes, w_handshakes, observed_wlast, model.m_utils.write_calls,
               b_handshakes, model_bvalid, bready, done_port);
      for (i = 0; i < 3; i = i + 1)
        if (observed_awaddr[i] !== dst + i*64 || observed_awlen[i] !== 8'd15 ||
            observed_awsize[i] !== 3'd2 || observed_awburst[i] !== 2'b01)
          $fatal(1, "pending-reset write descriptor mismatch index=%0d addr=%h len=%0d size=%0d burst=%b",
                 i, observed_awaddr[i], observed_awlen[i], observed_awsize[i], observed_awburst[i]);
      for (i = 0; i < 48; i = i + 1)
        if (model.m_utils.write_addr[i] !== dst + i*4 || model.m_utils.write_data[i] !== seed + i ||
            model.m_utils.write_mask[i] !== 4'hf)
          $fatal(1, "pending-reset write commit mismatch index=%0d addr=%h data=%h strobe=%h",
                 i, model.m_utils.write_addr[i], model.m_utils.write_data[i], model.m_utils.write_mask[i]);
      $display("GENERATED_MEMAXI_RESPONSE_RESET_PENDING engine=write reset_type=%s reset_level=%s aw=%0d w=%0d wlast=%0d backend=%0d b=%0d model_pending=1 done=0",
               reset_type_name, reset_level_name, aw_handshakes, w_handshakes,
               observed_wlast, model.m_utils.write_calls, b_handshakes);
      reset = 1'b0;
      start_port = 1'b0;
      if (reset_async_mode)
      begin
        #1;
        if (!idle_port || done_port || bready)
          $fatal(1, "async pending-reset write did not clear controller/engine before the next active edge: idle=%b bready=%b done=%b",
                 idle_port, bready, done_port);
      end
      else
      begin
        #1;
        if (!bready)
          $fatal(1, "sync pending-reset write cleared engine before the next active edge");
      end
      @(posedge clock);
      #1;
      if (bready)
        $fatal(1, "pending-reset write engine did not clear on the reset clock edge");
      repeat (2) @(negedge clock);
      if (done_port || !idle_port || model_bvalid)
        $fatal(1, "pending-reset write did not clear DUT/model state: idle=%b done=%b model_bvalid=%b",
               idle_port, done_port, model_bvalid);
      model.m_utils.reset_counters();
      dst = 32'h0000_4000;
      n = 17;
      seed = 100;
      response_gate = 1'b1;
      reset = 1'b1;
      repeat (2) @(negedge clock);
      start_port = 1'b1;
      @(negedge clock);
      start_port = 1'b0;
      cycles = 0;
      while (!done_port && cycles < 1000)
      begin
        @(negedge clock);
        cycles = cycles + 1;
      end
      if (!done_port)
        $fatal(1, "pending-reset write recovery did not complete within the cycle limit");
      @(posedge clock);
      @(negedge clock);
      if (aw_handshakes != 2 || w_handshakes != 17 || observed_wlast != 2 ||
          b_handshakes != 2 || model.m_utils.write_calls != 17 || done_pulses != 1)
        $fatal(1, "pending-reset write recovery mismatch done=%b aw=%0d w=%0d wlast=%0d b=%0d commits=%0d pulses=%0d",
               done_port, aw_handshakes, w_handshakes, observed_wlast, b_handshakes,
               model.m_utils.write_calls, done_pulses);
      for (i = 0; i < 17; i = i + 1)
        if (model.m_utils.write_addr[i] !== dst + i*4 || model.m_utils.write_data[i] !== seed + i ||
            model.m_utils.write_mask[i] !== 4'hf)
          $fatal(1, "pending-reset write recovery mismatch index=%0d addr=%h data=%h strobe=%h",
                 i, model.m_utils.write_addr[i], model.m_utils.write_data[i], model.m_utils.write_mask[i]);
      if (observed_awaddr[0] !== dst || observed_awlen[0] !== 8'd15 ||
          observed_awaddr[1] !== dst + 64 || observed_awlen[1] !== 0)
        $fatal(1, "pending-reset write recovery descriptor mismatch aw0=%h/%0d aw1=%h/%0d",
               observed_awaddr[0], observed_awlen[0], observed_awaddr[1], observed_awlen[1]);
      repeat (8) @(negedge clock);
      if (aw_handshakes != 2 || w_handshakes != 17 || b_handshakes != 2 || done_pulses != 1)
        $fatal(1, "pending-reset write recovery had extra work: aw=%0d w=%0d b=%0d done=%0d",
               aw_handshakes, w_handshakes, b_handshakes, done_pulses);
      $display("GENERATED_MEMAXI_RESPONSE_RESET_RECOVERY engine=write reset_type=%s reset_level=%s reinvocation=1 aw=%0d w=%0d wlast=%0d b=%0d backend=%0d strobes=ffff done=1",
               reset_type_name, reset_level_name, aw_handshakes, w_handshakes,
               observed_wlast, b_handshakes, model.m_utils.write_calls);
    end
    else if (b1_o3_mode)
    begin
      cycles = 0;
      while ((aw_handshakes < 3 || w_handshakes < 3) && cycles < 500)
      begin
        @(negedge clock);
        cycles = cycles + 1;
      end
      if (aw_handshakes < 3 || w_handshakes < 3 || b_handshakes != 0)
        $fatal(1, "B1/O3 write did not queue three requests before responses: aw=%0d w=%0d b=%0d",
               aw_handshakes, w_handshakes, b_handshakes);
      for (i = 0; i < 3; i = i + 1)
        if (observed_awaddr[i] !== dst + i*4 || observed_awlen[i] !== 0 ||
            observed_awsize[i] !== 3'd2 || observed_awburst[i] !== 2'b01)
          $fatal(1, "B1/O3 write descriptor mismatch index=%0d addr=%h len=%0d size=%0d burst=%b",
                 i, observed_awaddr[i], observed_awlen[i], observed_awsize[i], observed_awburst[i]);
      response_gate = 1'b1;
      cycles = 0;
      while (!done_port && cycles < 1000)
      begin
        @(negedge clock);
        cycles = cycles + 1;
      end
      if (!done_port)
        $fatal(1, "B1/O3 write did not complete within the cycle limit");
      @(posedge clock);
      @(negedge clock);
      if (aw_handshakes != 17 || w_handshakes != 17 || observed_wlast != 17 ||
          b_handshakes != 17 || model.m_utils.write_calls != 17 || done_pulses != 1)
        $fatal(1, "B1/O3 write totals mismatch done=%b aw=%0d w=%0d wlast=%0d b=%0d commits=%0d done_pulses=%0d",
               done_port, aw_handshakes, w_handshakes, observed_wlast, b_handshakes,
               model.m_utils.write_calls, done_pulses);
      for (i = 0; i < 17; i = i + 1)
        if (observed_awaddr[i] !== dst + i*4 || observed_awlen[i] !== 0 ||
            observed_awsize[i] !== 3'd2 || observed_awburst[i] !== 2'b01 ||
            model.m_utils.write_addr[i] !== dst + i*4 || model.m_utils.write_data[i] !== seed + i ||
            model.m_utils.write_mask[i] !== 4'hf)
          $fatal(1, "B1/O3 write transfer mismatch index=%0d awaddr=%h addr=%h data=%h strobe=%h",
                 i, observed_awaddr[i], model.m_utils.write_addr[i], model.m_utils.write_data[i],
                 model.m_utils.write_mask[i]);
      repeat (8) @(negedge clock);
      if (aw_handshakes != 17 || w_handshakes != 17 || b_handshakes != 17 || done_pulses != 1)
        $fatal(1, "B1/O3 write had extra work after completion: aw=%0d w=%0d b=%0d done=%0d",
               aw_handshakes, w_handshakes, b_handshakes, done_pulses);
      $display("GENERATED_MEMAXI_RESPONSE_B1O3 mode=write n=17 outstanding_before_response=3 aw=17 w=17 b=17 wlast=17 backend=17 done=1");
    end
    else if (fault_mode)
    begin
      if (multi_mode)
      begin
        cycles = 0;
        while ((aw_handshakes < 3 || w_handshakes < 48) && cycles < 1000)
        begin
          @(negedge clock);
          cycles = cycles + 1;
        end
        if (aw_handshakes != 3 || w_handshakes != 48 || observed_wlast != 3 || b_handshakes != 0)
          $fatal(1, "multi-write did not queue three complete descriptors before responses: aw=%0d w=%0d wlast=%0d b=%0d",
                 aw_handshakes, w_handshakes, observed_wlast, b_handshakes);
        for (i = 0; i < 3; i = i + 1)
          if (observed_awaddr[i] !== 32'h0000_2000 + i*64 || observed_awlen[i] !== 8'd15 ||
              observed_awsize[i] !== 3'd2)
            $fatal(1, "multi-write descriptor mismatch index=%0d addr=%h len=%0d size=%0d",
                   i, observed_awaddr[i], observed_awlen[i], observed_awsize[i]);
        if (model.m_utils.write_calls != 48)
          $fatal(1, "multi-write backend did not commit all exposed data before B: commits=%0d",
                 model.m_utils.write_calls);
        for (i = 0; i < 48; i = i + 1)
          if (model.m_utils.write_addr[i] !== 32'h0000_2000 + i*4 ||
              model.m_utils.write_data[i] !== seed + i || model.m_utils.write_mask[i] !== 4'hf)
            $fatal(1, "multi-write backend commit mismatch index=%0d addr=%h data=%h strobe=%h",
                   i, model.m_utils.write_addr[i], model.m_utils.write_data[i], model.m_utils.write_mask[i]);
        response_gate = 1'b1;
        cycles = 0;
        while (b_handshakes < 3 && cycles < 500)
        begin
          @(negedge clock);
          cycles = cycles + 1;
        end
        if (b_handshakes != 3 || write_fault_handshakes != 1 || write_okay_handshakes != 2 ||
            aw_handshakes != 3 || w_handshakes != 48 || model.m_utils.write_calls != 48 || done_pulses != 0)
          $fatal(1, "multi-write drain mismatch aw=%0d w=%0d b=%0d fault=%0d okay=%0d commits=%0d done=%0d",
                 aw_handshakes, w_handshakes, b_handshakes, write_fault_handshakes,
                 write_okay_handshakes, model.m_utils.write_calls, done_pulses);
        repeat (8) @(negedge clock);
        if (aw_handshakes != 3 || w_handshakes != 48 || done_pulses != 0)
          $fatal(1, "multi-write issued work or completed after fault: aw=%0d w=%0d done=%0d",
                 aw_handshakes, w_handshakes, done_pulses);
        $display("GENERATED_MEMAXI_RESPONSE_DRAIN mode=write descriptors=3 beats=48 fault_responses=1 okay_responses=2 commits=48 done=0");
      end
      else
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
      end

      calls_before_recovery = model.m_utils.write_calls;
      n = 1;
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
