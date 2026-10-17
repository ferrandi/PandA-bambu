module tb_generated_duplex_kernel #(
  parameter integer READ_B_MAX = 16,
  parameter integer WRITE_B_MAX = 16,
  parameter integer READ_OUTSTANDING = 1,
  parameter integer WRITE_OUTSTANDING = 1
);
  localparam integer WORDS = 32768;
  localparam integer OP_COUNT = 64;

  logic clock = 1'b0;
  always #5 clock = ~clock;
  logic reset = 1'b0;
  logic start_port = 1'b0;
  logic [31:0] data = 0;
  logic [31:0] n = 0;
  logic cache_reset = 1'b0;
  wire done_port;
  wire idle_port;

  wire [5:0] awid, arid;
  wire [31:0] awaddr, araddr, wdata;
  wire [7:0] awlen, arlen;
  wire [2:0] awsize, arsize;
  wire [1:0] awburst, arburst;
  wire [3:0] wstrb;
  wire wlast, awvalid, wvalid, bready, arvalid, rready;
  logic awready, wready, bvalid, arready, rvalid;
  logic [5:0] bid = 0, rid = 0;
  logic [1:0] bresp = 0, rresp = 0;
  logic [31:0] rdata;
  logic rlast;

  kernel dut (
    .clock(clock), .reset(reset), .start_port(start_port), .data(data), .n(n),
    .cache_reset(cache_reset), .m_axi_shared_awready(awready),
    .m_axi_shared_wready(wready), .m_axi_shared_bid(bid),
    .m_axi_shared_bresp(bresp), .m_axi_shared_buser(1'b0),
    .m_axi_shared_bvalid(bvalid), .m_axi_shared_arready(arready),
    .m_axi_shared_rid(rid), .m_axi_shared_rdata(rdata),
    .m_axi_shared_rresp(rresp), .m_axi_shared_rlast(rlast),
    .m_axi_shared_ruser(1'b0), .m_axi_shared_rvalid(rvalid),
    .done_port(done_port), .idle_port(idle_port),
    .m_axi_shared_awid(awid), .m_axi_shared_awaddr(awaddr),
    .m_axi_shared_awlen(awlen), .m_axi_shared_awsize(awsize),
    .m_axi_shared_awburst(awburst), .m_axi_shared_awlock(),
    .m_axi_shared_awcache(), .m_axi_shared_awprot(), .m_axi_shared_awqos(),
    .m_axi_shared_awregion(), .m_axi_shared_awuser(),
    .m_axi_shared_awvalid(awvalid), .m_axi_shared_wdata(wdata),
    .m_axi_shared_wstrb(wstrb), .m_axi_shared_wlast(wlast),
    .m_axi_shared_wuser(), .m_axi_shared_wvalid(wvalid),
    .m_axi_shared_bready(bready), .m_axi_shared_arid(arid),
    .m_axi_shared_araddr(araddr), .m_axi_shared_arlen(arlen),
    .m_axi_shared_arsize(arsize), .m_axi_shared_arburst(arburst),
    .m_axi_shared_arlock(), .m_axi_shared_arcache(), .m_axi_shared_arprot(),
    .m_axi_shared_arqos(), .m_axi_shared_arregion(), .m_axi_shared_aruser(),
    .m_axi_shared_arvalid(arvalid), .m_axi_shared_rready(rready)
  );

  logic [31:0] memory [0:WORDS-1];
  logic [31:0] expected [0:WORDS-1];
  logic [31:0] snapshot [0:WORDS-1];
  logic [31:0] ar_addr_q [0:OP_COUNT-1];
  integer ar_left_q [0:OP_COUNT-1];
  logic [31:0] aw_addr_q [0:OP_COUNT-1];
  integer aw_left_q [0:OP_COUNT-1];
  logic [31:0] w_data_q [0:4095];
  logic w_last_q [0:4095];
  integer ar_wr = 0, ar_rd = 0, aw_wr = 0, aw_rd = 0;
  integer w_wr = 0, w_rd = 0, b_pending = 0;
  integer ar_words = 0, aw_words = 0, w_words = 0, committed_words = 0;
  integer ar_count = 0, aw_count = 0, b_count = 0;
  integer cycle_count = 0;
  integer case_count = 0;
  logic [31:0] case_base = 0;
  logic case_begin = 0;
  logic release_final_b = 0;
  integer fault_mode = 0;
  logic fault_injected = 0;
  logic fault_accepted = 0;
  integer w_burst_progress = 0;

  function automatic integer mem_index(input logic [31:0] address);
    mem_index = integer'(address[16:2]);
  endfunction

  function automatic integer burst_beats(input logic [31:0] base,
                                          input integer count,
                                          input integer offset,
                                          input integer limit);
    integer remain;
    integer page;
    begin
      remain = count - offset;
      page = (4096 - ((base + offset * 4) & 32'hfff)) / 4;
      burst_beats = remain < limit ? remain : limit;
      if (page < burst_beats) burst_beats = page;
    end
  endfunction

  function automatic integer expected_burst_count(input logic [31:0] base,
                                                    input integer count,
                                                    input integer limit);
    integer offset;
    integer beats;
    begin
      offset = 0;
      expected_burst_count = 0;
      while (offset < count) begin
        beats = burst_beats(base, count, offset, limit);
        if (beats <= 0) begin
          expected_burst_count = -1;
          offset = count;
        end else begin
          offset = offset + beats;
          expected_burst_count = expected_burst_count + 1;
        end
      end
    end
  endfunction

  always_comb begin
    awready = reset && ((cycle_count % 5) != 0);
    wready = reset && ((cycle_count % 7) != 0);
    arready = reset && ((cycle_count % 4) != 0);
    rvalid = reset && (ar_rd < ar_wr) && ((cycle_count % 3) != 0);
    rdata = rvalid ? memory[mem_index(ar_addr_q[ar_rd])] : 32'b0;
    rlast = rvalid && (ar_left_q[ar_rd] == 1);
    rresp = (fault_mode == 1 && !fault_injected) ? 2'b10 : 2'b00;
    bvalid = reset && (b_pending > 0) &&
             ((committed_words < case_count) || release_final_b) &&
             ((cycle_count % 5) != 0);
    bresp = (fault_mode == 2 && !fault_injected) ? 2'b10 : 2'b00;
  end

  always @(posedge clock) begin : axi_memory
    integer beats;
    integer j;
    integer index;
    integer expected_beats;
    if (!reset) begin
      ar_wr = 0; ar_rd = 0; aw_wr = 0; aw_rd = 0; w_wr = 0; w_rd = 0;
      b_pending = 0; ar_words = 0; aw_words = 0; w_words = 0;
      committed_words = 0; ar_count = 0; aw_count = 0; b_count = 0;
      w_burst_progress = 0;
    end else begin
      cycle_count = cycle_count + 1;
      if (case_begin) begin
        ar_wr = 0; ar_rd = 0; aw_wr = 0; aw_rd = 0; w_wr = 0; w_rd = 0;
        b_pending = 0; ar_words = 0; aw_words = 0; w_words = 0;
        committed_words = 0; ar_count = 0; aw_count = 0; b_count = 0;
        w_burst_progress = 0;
      end
      if (arvalid && arready) begin
        if (ar_wr >= OP_COUNT) $fatal(1, "DUPLEX_KERNEL_AR_QUEUE_OVERFLOW");
        expected_beats = burst_beats(case_base, case_count, ar_words, READ_B_MAX);
        if (araddr !== case_base + ar_words * 4 || arlen !== expected_beats - 1)
          $fatal(1, "DUPLEX_KERNEL_AR_ADDRESS_OR_LENGTH_MISMATCH");
        if (arsize !== 3'd2 || arburst !== 2'b01 || arid !== 0)
          $fatal(1, "DUPLEX_KERNEL_AR_PROFILE_INVALID");
        if ((araddr & 32'hfff) + expected_beats * 4 > 4096)
          $fatal(1, "DUPLEX_KERNEL_AR_CROSSES_4K");
        ar_addr_q[ar_wr] = araddr;
        ar_left_q[ar_wr] = expected_beats;
        ar_wr = ar_wr + 1;
        ar_words = ar_words + expected_beats;
        ar_count = ar_count + 1;
      end
      if (rvalid && rready) begin
        index = mem_index(ar_addr_q[ar_rd]);
        if (rid !== 0) $fatal(1, "DUPLEX_KERNEL_RID_INVALID");
        if (rlast !== (ar_left_q[ar_rd] == 1))
          $fatal(1, "DUPLEX_KERNEL_RLAST_INVALID");
        if (rresp != 2'b00) begin
          if (fault_mode != 1 || fault_injected)
            $fatal(1, "DUPLEX_KERNEL_UNEXPECTED_RRESP");
          fault_injected = 1;
          fault_accepted = 1;
        end
        if (rdata !== snapshot[index])
          $fatal(1, "DUPLEX_KERNEL_READ_DATA_MISMATCH");
        if (ar_left_q[ar_rd] == 1) ar_rd = ar_rd + 1;
        else begin
          ar_left_q[ar_rd] = ar_left_q[ar_rd] - 1;
          ar_addr_q[ar_rd] = ar_addr_q[ar_rd] + 4;
        end
      end
      if (awvalid && awready) begin
        if (aw_wr >= OP_COUNT) $fatal(1, "DUPLEX_KERNEL_AW_QUEUE_OVERFLOW");
        expected_beats = burst_beats(case_base, case_count, aw_words, WRITE_B_MAX);
        if (awaddr !== case_base + aw_words * 4 || awlen !== expected_beats - 1)
          $fatal(1, "DUPLEX_KERNEL_AW_ADDRESS_OR_LENGTH_MISMATCH");
        if (awsize !== 3'd2 || awburst !== 2'b01 || awid !== 0)
          $fatal(1, "DUPLEX_KERNEL_AW_PROFILE_INVALID");
        if ((awaddr & 32'hfff) + expected_beats * 4 > 4096)
          $fatal(1, "DUPLEX_KERNEL_AW_CROSSES_4K");
        aw_addr_q[aw_wr] = awaddr;
        aw_left_q[aw_wr] = expected_beats;
        aw_wr = aw_wr + 1;
        aw_words = aw_words + expected_beats;
        aw_count = aw_count + 1;
      end
      if (wvalid && wready) begin
        if (w_wr >= 4096 || w_words >= case_count)
          $fatal(1, "DUPLEX_KERNEL_UNEXPECTED_W_BEAT");
        expected_beats = burst_beats(case_base, case_count, w_words - w_burst_progress,
                                     WRITE_B_MAX);
        if (wstrb !== 4'hf) $fatal(1, "DUPLEX_KERNEL_WSTRB_INVALID");
        index = mem_index(case_base + w_words * 4);
        if (wdata !== expected[index])
          $fatal(1, "DUPLEX_KERNEL_WRITE_DATA_MISMATCH");
        if (wlast !== (w_burst_progress == expected_beats - 1))
          $fatal(1, "DUPLEX_KERNEL_WLAST_INVALID");
        w_data_q[w_wr] = wdata;
        w_last_q[w_wr] = wlast;
        w_wr = w_wr + 1;
        w_words = w_words + 1;
        w_burst_progress = wlast ? 0 : w_burst_progress + 1;
      end
      while (aw_rd < aw_wr && (w_wr - w_rd) >= aw_left_q[aw_rd]) begin
        beats = aw_left_q[aw_rd];
        for (j = 0; j < beats; j = j + 1) begin
          if (w_last_q[w_rd+j] !== (j == beats-1))
            $fatal(1, "DUPLEX_KERNEL_WLAST_DOES_NOT_MATCH_AW");
          index = mem_index(aw_addr_q[aw_rd] + j * 4);
          memory[index] = w_data_q[w_rd+j];
        end
        w_rd = w_rd + beats;
        committed_words = committed_words + beats;
        b_pending = b_pending + 1;
        aw_rd = aw_rd + 1;
      end
      if (bvalid && bready) begin
        if (bid !== 0) $fatal(1, "DUPLEX_KERNEL_BID_INVALID");
        if (bresp != 2'b00) begin
          if (fault_mode != 2 || fault_injected)
            $fatal(1, "DUPLEX_KERNEL_UNEXPECTED_BRESP");
          fault_injected = 1;
          fault_accepted = 1;
        end
        b_pending = b_pending - 1;
        b_count = b_count + 1;
      end
    end
  end

  task automatic fail(input string message);
    $display("DUPLEX_KERNEL_G2_FAIL: %s", message);
    $fatal(1, "%s", message);
  endtask

  task automatic prepare_case(input integer count, input logic [31:0] base);
    integer k;
    integer index;
    begin
      case_count = count;
      case_base = base;
      for (k = 0; k < WORDS; k = k + 1) begin
        expected[k] = memory[k];
        snapshot[k] = memory[k];
      end
      for (k = 0; k < count; k = k + 1) begin
        index = mem_index(base + k * 4);
        expected[index] = snapshot[index] * 3 + 1;
      end
      @(negedge clock);
      case_begin = 1;
      @(negedge clock);
      case_begin = 0;
    end
  endtask

  task automatic invoke(input integer count, input logic [31:0] base);
    integer cycles;
    integer k;
    logic seen_done;
    begin
      prepare_case(count, base);
      release_final_b = 0;
      @(negedge clock);
      data = base;
      n = count;
      start_port = 1;
      @(posedge clock); #1;
      @(negedge clock);
      start_port = 0;
      cycles = 0;
      if (count == 0) begin
        seen_done = done_port;
        while (!seen_done && cycles < 2000) begin
          @(posedge clock); #1;
          seen_done = done_port;
          cycles = cycles + 1;
        end
        if (!seen_done || ar_count != 0 || aw_count != 0 || w_words != 0)
          fail("zero-count call did not complete without bus traffic");
      end else begin
        while ((committed_words < count || ar_words < count) && cycles < 20000) begin
          @(posedge clock); #1;
          if (done_port) fail("done asserted before final B response");
          cycles = cycles + 1;
        end
        if (committed_words != count || ar_words != count || w_words != count)
          fail("read/write traffic did not cover the requested region");
        if (b_pending != 1 || bvalid)
          fail("final B response was not held after data commit");
        repeat (3) begin
          @(posedge clock); #1;
          if (done_port) fail("done asserted while final B response was withheld");
        end
        release_final_b = 1;
        cycles = 0;
        seen_done = 0;
        while (!seen_done && cycles < 10000) begin
          @(posedge clock); #1;
          seen_done = done_port;
          cycles = cycles + 1;
        end
        if (!seen_done) fail("kernel did not complete after final B response");
        if (ar_count != expected_burst_count(case_base, count, READ_B_MAX))
          fail("AR burst count does not match the read profile");
        if (aw_count != expected_burst_count(case_base, count, WRITE_B_MAX))
          fail("AW burst count does not match the write profile");
        if (b_count != aw_count || b_pending != 0)
          fail("B responses do not match completed AW bursts");
      end
      for (k = 0; k < WORDS; k = k + 1)
        if (memory[k] !== expected[k])
          fail($sformatf("memory/canary mismatch at word %0d got=%08x expected=%08x",
                         k, memory[k], expected[k]));
      if (fault_mode == 0 && fault_accepted)
        fail("unexpected fault response accepted");
    end
  endtask

  task automatic fault_reset_restart(input integer mode);
    integer cycles;
    integer observe_cycle;
    integer count;
    logic accepted;
    begin
      count = 17;
      prepare_case(count, 32'h00002000);
      release_final_b = 1;
      fault_mode = mode;
      fault_injected = 0;
      fault_accepted = 0;
      @(negedge clock);
      data = case_base;
      n = count;
      start_port = 1;
      @(posedge clock); #1;
      @(negedge clock);
      start_port = 0;
      cycles = 0;
      accepted = 0;
      while (!accepted && cycles < 10000) begin
        @(posedge clock); #1;
        accepted = fault_accepted;
        cycles = cycles + 1;
      end
      if (!accepted) fail("fault response was not accepted");
      if (done_port)
        fail(mode == 1 ? "done asserted on non-OKAY R response" :
                         "done asserted on non-OKAY B response");
      for (observe_cycle = 0; observe_cycle < 64; observe_cycle = observe_cycle + 1) begin
        @(posedge clock); #1;
        if (done_port)
          fail(mode == 1 ? "done asserted after non-OKAY R response" :
                           "done asserted after non-OKAY B response");
      end
      fault_mode = 0;
      @(negedge clock);
      reset = 0;
      repeat (3) @(negedge clock);
      reset = 1;
      fault_injected = 0;
      fault_accepted = 0;
      invoke(count, 32'h00002000);
    end
  endtask

  task automatic report_pass(input string case_name);
    begin
      if (WRITE_B_MAX == 16)
        $display("DUPLEX_KERNEL_G2 PASS CASE=%s PROFILE=symmetric", case_name);
      else
        $display("DUPLEX_KERNEL_G2 PASS CASE=%s PROFILE=asymmetric", case_name);
    end
  endtask

  string selected_case;
  integer i;
  initial begin
    if (READ_OUTSTANDING < 1 || WRITE_OUTSTANDING < 1)
      $fatal(1, "DUPLEX_KERNEL_INVALID_PARAMETERS");
    if (!$value$plusargs("CASE=%s", selected_case))
      $fatal(1, "DUPLEX_KERNEL_MISSING_CASE_SELECTOR");
    for (i = 0; i < WORDS; i = i + 1) begin
      memory[i] = 32'h13570000 ^ (i * 32'h00000103);
      expected[i] = memory[i];
      snapshot[i] = memory[i];
    end
    repeat (3) @(posedge clock);
    reset = 1;
    if (selected_case == "COUNTS_0_1_15_16_17") begin
      invoke(0, 32'h00002000);
      invoke(1, 32'h00002000);
      invoke(15, 32'h00002000);
      invoke(16, 32'h00002000);
      invoke(17, 32'h00002000);
    end else if (selected_case == "COUNT38_SPLITS") begin
      invoke(38, 32'h00002000);
    end else if (selected_case == "PAGE_BOUNDARY") begin
      invoke(17, 32'h00010ff0);
    end else if (selected_case == "REINVOKE_DEPENDENT") begin
      invoke(15, 32'h00003000);
      invoke(15, 32'h00003000);
    end else if (selected_case == "REINVOKE_BASE_COUNT") begin
      invoke(17, 32'h00005000);
      invoke(8, 32'h00005004);
    end else if (selected_case == "LAST_B_WAIT") begin
      invoke(1, 32'h00004000);
    end else if (selected_case == "RRESP_FAULT_RESET") begin
      fault_reset_restart(1);
    end else if (selected_case == "BRESP_FAULT_RESET") begin
      fault_reset_restart(2);
    end else begin
      $fatal(1, "DUPLEX_KERNEL_UNKNOWN_CASE_SELECTOR: %s", selected_case);
    end
    report_pass(selected_case);
    $finish;
  end
endmodule
