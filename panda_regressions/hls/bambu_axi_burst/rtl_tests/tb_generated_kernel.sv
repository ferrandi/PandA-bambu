// Whole-top RTL qualification for the actual Bambu-generated kernel.
// The AXI slave is deterministic but independently stalls AR and inserts R
// gaps. The hierarchical consume check intentionally fails closed if the
// generated datapath/resource path changes.
module tb_generated_kernel;
  logic clock = 1'b0;
  always #5 clock = ~clock;

  logic reset = 1'b0;
  logic start_port = 1'b0;
  logic [31:0] src = 32'b0;
  logic [31:0] n = 32'b0;
  logic cache_reset = 1'b0;
  logic arready = 1'b0;
  logic [5:0] rid = 6'b0;
  logic [31:0] rdata = 32'b0;
  logic [1:0] rresp = 2'b0;
  logic rlast = 1'b0;
  logic rvalid = 1'b0;
  wire done_port;
  wire idle_port;
  wire [31:0] return_port;

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
  wire [31:0] wdata;
  wire [3:0] wstrb;
  wire wlast;
  wire wuser;
  wire wvalid;
  wire bready;
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
  wire rready;

  kernel dut (
    .clock(clock), .reset(reset), .start_port(start_port),
    .src(src), .n(n), .cache_reset(cache_reset),
    .m_axi_src_awready(1'b0), .m_axi_src_wready(1'b0),
    .m_axi_src_bid(6'b0), .m_axi_src_bresp(2'b0),
    .m_axi_src_buser(1'b0), .m_axi_src_bvalid(1'b0),
    .m_axi_src_arready(arready), .m_axi_src_rid(rid),
    .m_axi_src_rdata(rdata), .m_axi_src_rresp(rresp),
    .m_axi_src_rlast(rlast), .m_axi_src_ruser(1'b0),
    .m_axi_src_rvalid(rvalid),
    .done_port(done_port), .idle_port(idle_port), .return_port(return_port),
    .m_axi_src_awid(awid), .m_axi_src_awaddr(awaddr), .m_axi_src_awlen(awlen),
    .m_axi_src_awsize(awsize), .m_axi_src_awburst(awburst),
    .m_axi_src_awlock(awlock), .m_axi_src_awcache(awcache),
    .m_axi_src_awprot(awprot), .m_axi_src_awqos(awqos),
    .m_axi_src_awregion(awregion), .m_axi_src_awuser(awuser),
    .m_axi_src_awvalid(awvalid), .m_axi_src_wdata(wdata),
    .m_axi_src_wstrb(wstrb), .m_axi_src_wlast(wlast),
    .m_axi_src_wuser(wuser), .m_axi_src_wvalid(wvalid),
    .m_axi_src_bready(bready), .m_axi_src_arid(arid),
    .m_axi_src_araddr(araddr), .m_axi_src_arlen(arlen),
    .m_axi_src_arsize(arsize), .m_axi_src_arburst(arburst),
    .m_axi_src_arlock(arlock), .m_axi_src_arcache(arcache),
    .m_axi_src_arprot(arprot), .m_axi_src_arqos(arqos),
    .m_axi_src_arregion(arregion), .m_axi_src_aruser(aruser),
    .m_axi_src_arvalid(arvalid), .m_axi_src_rready(rready)
  );

  function automatic logic [31:0] data_word(input logic [31:0] index);
    logic [31:0] residue;
    logic signed [31:0] bounded;
    begin
      // Cycle through signed values [-15, 15] in a permuted order. Keeping
      // each value bounded guarantees every cumulative sum for n <= 4096
      // remains in signed int32 range, while retaining non-monotonic data.
      residue = (index * 32'd17 + 32'd3) % 32'd31;
      bounded = $signed(residue) - 32'sd15;
      data_word = bounded;
    end
  endfunction

  function automatic logic [31:0] expected_sum(input logic [31:0] count);
    logic [31:0] total;
    integer k;
    begin
      total = 0;
      for (k = 0; k < count; k = k + 1)
        total = total + data_word(k);
      expected_sum = total;
    end
  endfunction

  task automatic fail(input string message);
    begin
      $display("GENERATED_KERNEL_FAIL: %s", message);
      $fatal(1, "%s", message);
    end
  endtask

  task automatic run_case(input logic [31:0] count,
                          input logic [31:0] base,
                          input integer stall_mode,
                          input bit perturb_slave_data);
    integer cycle;
    integer ar_count;
    integer r_count;
    integer consumed_count;
    integer expected_burst_beats;
    integer remaining;
    integer boundary_beats;
    integer beat_index;
    integer response_left;
    integer consume_index;
    integer timeout;
    logic [31:0] issued_beats;
    logic [31:0] response_addr;
    logic response_active;
    logic ar_hs;
    logic r_hs;
    logic [31:0] expected_result;
    logic [63:0] last_byte;
    bit finished;
    bit early_done;
    begin
      reset = 1'b0;
      start_port = 1'b0;
      src = base;
      n = count;
      arready = 1'b0;
      rvalid = 1'b0;
      rdata = 0;
      rlast = 1'b0;
      response_active = 1'b0;
      issued_beats = 0;
      ar_count = 0;
      r_count = 0;
      consumed_count = 0;
      cycle = 0;
      timeout = 0;
      ar_hs = 1'b0;
      r_hs = 1'b0;
      finished = 1'b0;
      early_done = 1'b0;
      repeat (3) @(posedge clock);
      @(negedge clock);
      reset = 1'b1;
      repeat (2) @(negedge clock);
      if (!idle_port)
        fail("generated kernel did not enter idle after reset");
      expected_result = expected_sum(count);
      last_byte = {32'b0, base} + ({32'b0, count} << 2);
      start_port = 1'b1;
      @(posedge clock);
      #1;
      early_done = done_port;
      @(negedge clock);
      if (done_port)
        early_done = 1'b1;
      start_port = 1'b0;
      if (early_done) begin
        if (count != 0 || ar_count != 0 || r_count != 0 || consumed_count != 0 ||
            arvalid || rready || awvalid || wvalid || return_port !== expected_result)
          fail("early done did not represent a drained zero-length no-op");
        $display("KERNEL_CASE PASS n=0 base=%08x bursts=0 beats=0 consumed=0 cycles=1 mode=%0d", base, stall_mode);
        finished = 1'b1;
      end

      while (timeout < 200000 && !finished) begin
        @(negedge clock);
        cycle = cycle + 1;

        // Complete AXI slave bookkeeping from handshakes sampled at the
        // preceding rising edge; keep RVALID/data stable under RREADY stalls.
        if (ar_hs) begin
          response_active = 1'b1;
          response_addr = araddr;
          response_left = integer'(arlen) + 1;
          beat_index = 0;
          rvalid = 1'b0;
          ar_hs = 1'b0;
        end
        if (r_hs) begin
          rvalid = 1'b0;
          if (response_left == 1) begin
            response_active = 1'b0;
            response_left = 0;
          end else begin
            response_left = response_left - 1;
            beat_index = beat_index + 1;
          end
          r_hs = 1'b0;
        end

        case (stall_mode)
          0: arready = 1'b1;
          1: arready = (cycle % 4) != 0;
          default: arready = (cycle % 7) < 3;
        endcase

        if (response_active && !rvalid) begin
          if ((stall_mode == 2 && (cycle % 6) == 1) ||
              (stall_mode != 2 && (cycle % 5) == 1)) begin
            rvalid = 1'b0;
          end else begin
            rvalid = 1'b1;
            rdata = data_word((response_addr - base) / 4 + beat_index);
            if (perturb_slave_data && ar_count == 1 && beat_index == 0)
              rdata = rdata ^ 32'd1;
          end
        end
        rlast = response_active && (response_left == 1);

        if (awvalid || wvalid)
          fail("read-only kernel emitted an AXI write");

        @(posedge clock);
        timeout = timeout + 1;
        ar_hs = arvalid && arready;
        r_hs = rvalid && rready;
        if ($test$plusargs("DEBUG") && cycle < 30)
          $display("DBG n=%0d cyc=%0d idle=%b done=%b start=%b state=%0d cfgstart=%b op=%0d cfgseen=%b active=%b count=%0d ar=%b/%b r=%b/%b pop=%b issued=%0d returned=%0d consumed=%0d",
                   count, cycle, idle_port, done_port, start_port,
                   dut.PBI_kernel_i0.Controller_i.present_state,
                   dut.PBI_kernel_i0.Datapath_i.src_bambu_artificial_ParmMgr_modgen_16_i0.start_port,
                   dut.PBI_kernel_i0.Datapath_i.src_bambu_artificial_ParmMgr_modgen_16_i0.in1,
                   dut.PBI_kernel_i0.Datapath_i.src_bambu_artificial_ParmMgr_modgen_16_i0.burst_engine.cfg_seen,
                   dut.PBI_kernel_i0.Datapath_i.src_bambu_artificial_ParmMgr_modgen_16_i0.burst_engine.region_active,
                   dut.PBI_kernel_i0.Datapath_i.src_bambu_artificial_ParmMgr_modgen_16_i0.burst_engine.region_count,
                   arvalid, arready, rvalid, rready,
                   dut.PBI_kernel_i0.Datapath_i.src_bambu_artificial_ParmMgr_modgen_16_i0.burst_engine.fifo_load_pop,
                   dut.PBI_kernel_i0.Datapath_i.src_bambu_artificial_ParmMgr_modgen_16_i0.burst_engine.issued,
                   dut.PBI_kernel_i0.Datapath_i.src_bambu_artificial_ParmMgr_modgen_16_i0.burst_engine.returned,
                   dut.PBI_kernel_i0.Datapath_i.src_bambu_artificial_ParmMgr_modgen_16_i0.burst_engine.consumed);

        if (ar_hs) begin
          if (response_active)
            fail("a second AR was accepted while a burst response remained active");
          if (issued_beats >= count)
            fail("AXI read request exceeded the runtime count");
          if (araddr !== base + (issued_beats << 2))
            fail("accepted AR address was not the next sequential word");
          remaining = integer'(count - issued_beats);
          boundary_beats = (4096 - integer'(araddr[11:0])) / 4;
          expected_burst_beats = 16;
          if (remaining < expected_burst_beats)
            expected_burst_beats = remaining;
          if (boundary_beats < expected_burst_beats)
            expected_burst_beats = boundary_beats;
          if (integer'(arlen) + 1 != expected_burst_beats)
            fail("accepted ARLEN did not match B_MAX/count/4KiB boundary");
          if (arid !== 0 || arsize !== 3'd2 || arburst !== 2'b01)
            fail("accepted AR descriptor has wrong ID, size, or burst type");
          if (araddr[11:0] + ((integer'(arlen) + 1) * 4) > 4096)
            fail("accepted AXI burst crossed a 4KiB boundary");
          if ({32'b0, araddr} < {32'b0, base} ||
              ({32'b0, araddr} + ((integer'(arlen) + 1) * 4)) > last_byte)
            fail("accepted AXI burst fell outside the runtime byte range");
          issued_beats = issued_beats + expected_burst_beats;
          ar_count = ar_count + 1;
        end

        if (r_hs) begin
          if (!response_active || response_left <= 0)
            fail("AXI response beat arrived without an accepted request");
          if (rdata !== data_word((response_addr - base) / 4 + beat_index))
            fail("AXI response data order/value mismatch");
          if (rid !== 0 || rresp !== 0 || rlast !== (response_left == 1))
            fail("AXI response ID/RESP/RLAST mismatch");
          r_count = r_count + 1;
        end

        // This checks data when the generated engine actually pops its FIFO
        // for the top-level load, not merely what the AXI slave returned.
        if (dut.PBI_kernel_i0.Datapath_i.src_bambu_artificial_ParmMgr_modgen_16_i0.burst_engine.fifo_load_pop) begin
          consume_index = consumed_count;
          if (consume_index >= count)
            fail("generated AXI engine consumed more words than requested");
          if (dut.PBI_kernel_i0.Datapath_i.src_bambu_artificial_ParmMgr_modgen_16_i0.burst_engine.out1_reg !== data_word(consume_index))
            fail("generated AXI engine FIFO consume/pop data order/value mismatch");
          consumed_count = consumed_count + 1;
        end

        if (done_port) begin
          if (issued_beats != count || r_count != count || consumed_count != count)
            fail("done asserted before all requested beats were issued, returned, and consumed");
          if (response_active || rvalid || rready)
            fail("done asserted before AXI response path drained");
          if (return_port !== expected_result)
            fail("top-level return value differs from deterministic reference sum");
          $display("KERNEL_CASE PASS n=%0d base=%08x bursts=%0d beats=%0d consumed=%0d cycles=%0d mode=%0d",
                   count, base, ar_count, r_count, consumed_count, timeout, stall_mode);
          finished = 1'b1;
        end
      end
      if (!finished)
        fail("watchdog expired waiting for exact burst drain and top-level done");
    end
  endtask

  initial begin
    bit negative_sensitivity;
    negative_sensitivity = $test$plusargs("NEGATIVE_SENSITIVITY");
    if (negative_sensitivity) begin
      run_case(32'd1, 32'h1000_0000, 0, 1'b1);
      fail("negative sensitivity case unexpectedly passed");
    end

    if ($test$plusargs("ONLY_ONE")) begin
      run_case(32'd1, 32'h1000_0000, 1, 1'b0);
      $display("GENERATED_KERNEL_ONLY_ONE PASS");
      $finish;
    end
    if ($test$plusargs("ONLY_ZERO")) begin
      run_case(32'd0, 32'h1000_0000, 0, 1'b0);
      $display("GENERATED_KERNEL_ONLY_ZERO PASS");
      $finish;
    end

    run_case(32'd0,    32'h1000_0000, 0, 1'b0);
    run_case(32'd1,    32'h1000_0000, 1, 1'b0);
    run_case(32'd15,   32'h1000_0000, 2, 1'b0);
    run_case(32'd16,   32'h1000_0000, 1, 1'b0);
    run_case(32'd17,   32'h1000_0000, 2, 1'b0);
    run_case(32'd38,   32'h1000_0ff0, 1, 1'b0);
    run_case(32'd1024, 32'h1000_0000, 2, 1'b0);
    run_case(32'd2048, 32'h1000_0000, 1, 1'b0);
    run_case(32'd4096, 32'h1000_0000, 2, 1'b0);
    $display("GENERATED_KERNEL_ALL_CASES PASS");
    $finish;
  end

  initial begin
    #2000000;
    $fatal(1, "global watchdog expired");
  end
endmodule
