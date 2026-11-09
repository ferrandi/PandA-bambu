`timescale 1ns / 1ps
module tb_default_duplex_reduction #(
  parameter integer READ_B_MAX = 16,
  parameter integer WRITE_B_MAX = 16
);
  localparam integer WORDS = 32768;
  localparam integer QUEUE_DEPTH = 1024;
  localparam logic [31:0] SRC = 32'h00002000;
  localparam logic [31:0] DST = 32'h00010000;

  logic clock = 1'b0;
  always #5 clock = ~clock;
  logic reset = 1'b0;
  logic start_port = 1'b0;
  logic [31:0] src = 0, dst = 0, n = 0;
  wire done_port, idle_port;
  wire [31:0] return_port;

  wire [5:0] awid, arid;
  wire [31:0] awaddr, araddr, wdata;
  wire [7:0] awlen, arlen;
  wire [2:0] awsize, arsize;
  wire [1:0] awburst, arburst;
  wire [3:0] awcache, arcache, wstrb;
  wire wlast, awvalid, wvalid, bready, arvalid, rready;
  logic awready = 1'b1, arready = 1'b1;
  wire bvalid, rvalid, wready;
  logic [5:0] bid, rid;
  logic [31:0] rdata;
  logic [1:0] bresp = 0, rresp = 0;
  logic rlast;

  duplex_copy dut (
    .clock(clock), .reset(reset), .start_port(start_port), .src(src), .dst(dst), .n(n),
    .cache_reset(1'b0), .m_axi_shared_awready(awready), .m_axi_shared_wready(wready),
    .m_axi_shared_bid(bid), .m_axi_shared_bresp(bresp), .m_axi_shared_buser(1'b0),
    .m_axi_shared_bvalid(bvalid), .m_axi_shared_arready(arready), .m_axi_shared_rid(rid),
    .m_axi_shared_rdata(rdata), .m_axi_shared_rresp(rresp), .m_axi_shared_rlast(rlast),
    .m_axi_shared_ruser(1'b0), .m_axi_shared_rvalid(rvalid), .done_port(done_port),
    .idle_port(idle_port), .return_port(return_port), .m_axi_shared_awid(awid),
    .m_axi_shared_awaddr(awaddr), .m_axi_shared_awlen(awlen), .m_axi_shared_awsize(awsize),
    .m_axi_shared_awburst(awburst), .m_axi_shared_awlock(), .m_axi_shared_awcache(awcache),
    .m_axi_shared_awprot(), .m_axi_shared_awqos(), .m_axi_shared_awregion(), .m_axi_shared_awuser(),
    .m_axi_shared_awvalid(awvalid), .m_axi_shared_wdata(wdata), .m_axi_shared_wstrb(wstrb),
    .m_axi_shared_wlast(wlast), .m_axi_shared_wuser(), .m_axi_shared_wvalid(wvalid),
    .m_axi_shared_bready(bready), .m_axi_shared_arid(arid), .m_axi_shared_araddr(araddr),
    .m_axi_shared_arlen(arlen), .m_axi_shared_arsize(arsize), .m_axi_shared_arburst(arburst),
    .m_axi_shared_arlock(), .m_axi_shared_arcache(arcache), .m_axi_shared_arprot(),
    .m_axi_shared_arqos(), .m_axi_shared_arregion(), .m_axi_shared_aruser(),
    .m_axi_shared_arvalid(arvalid), .m_axi_shared_rready(rready)
  );

  logic [31:0] memory [0:WORDS-1];
  logic [31:0] expected [0:WORDS-1];
  logic [31:0] ar_addr_q [0:QUEUE_DEPTH-1];
  logic [7:0] ar_len_q [0:QUEUE_DEPTH-1];
  logic [5:0] ar_id_q [0:QUEUE_DEPTH-1];
  logic [31:0] aw_addr_q [0:QUEUE_DEPTH-1];
  logic [7:0] aw_len_q [0:QUEUE_DEPTH-1];
  logic [5:0] aw_id_q [0:QUEUE_DEPTH-1];
  integer ar_wr = 0, ar_rd = 0, aw_wr = 0, aw_rd = 0;
  integer ar_beat = 0, w_beat = 0, b_pending = 0;
  integer ar_words = 0, aw_words = 0, r_words = 0, w_words = 0;

  assign wready = aw_rd < aw_wr;
  assign rvalid = ar_rd < ar_wr;
  assign bvalid = b_pending > 0;

  function automatic integer mem_index(input logic [31:0] address);
    mem_index = address >> 2;
  endfunction

  function automatic integer next_burst(input logic [31:0] address, input integer remaining,
                                          input integer max_beats);
    integer page_words;
    begin
      page_words = (4096 - (address & 32'hfff)) / 4;
      next_burst = remaining < max_beats ? remaining : max_beats;
      if (next_burst > page_words) next_burst = page_words;
    end
  endfunction

  always_comb begin
    if (rvalid) begin
      rid = ar_id_q[ar_rd];
      rdata = memory[mem_index(ar_addr_q[ar_rd]) + ar_beat];
      rlast = ar_beat == ar_len_q[ar_rd];
    end else begin
      rid = 0;
      rdata = 0;
      rlast = 0;
    end
    bid = 0;
  end

  always @(posedge clock) begin
    integer beats, accepted, index;
    if (!reset) begin
      ar_wr = 0; ar_rd = 0; aw_wr = 0; aw_rd = 0;
      ar_beat = 0; w_beat = 0; b_pending = 0;
      ar_words = 0; aw_words = 0; r_words = 0; w_words = 0;
    end else begin
      if (arvalid && arready) begin
        beats = next_burst(araddr, n - ar_words, READ_B_MAX);
        accepted = arlen + 1;
        if (ar_wr >= QUEUE_DEPTH || araddr !== src + ar_words * 4 || accepted < 1 || accepted > beats ||
            arsize != 2 || arburst != 1 || arid != 0 || arcache != 4'b0010)
          $fatal(1, "DEFAULT_DUPLEX_AR_PROFILE_OR_RANGE addr=%08x len=%0d expected_addr=%08x max_len=%0d size=%0d burst=%0d id=%0d cache=%x",
                 araddr, arlen, src + ar_words * 4, beats - 1, arsize, arburst, arid, arcache);
        ar_addr_q[ar_wr] = araddr;
        ar_len_q[ar_wr] = arlen;
        ar_id_q[ar_wr] = arid;
        ar_wr = ar_wr + 1;
        ar_words = ar_words + accepted;
      end
      if (rvalid && rready) begin
        r_words = r_words + 1;
        if (rlast != (ar_beat == ar_len_q[ar_rd])) $fatal(1, "DEFAULT_DUPLEX_RLAST");
        if (rlast) begin ar_rd = ar_rd + 1; ar_beat = 0; end
        else ar_beat = ar_beat + 1;
      end
      if (awvalid && awready) begin
        beats = next_burst(awaddr, n - aw_words, WRITE_B_MAX);
        if (aw_wr >= QUEUE_DEPTH || awaddr !== dst + aw_words * 4 || awlen + 1 != beats ||
            awsize != 2 || awburst != 1 || awid != 0 || awcache != 4'b0010)
          $fatal(1, "DEFAULT_DUPLEX_AW_PROFILE_OR_RANGE addr=%08x len=%0d expected_addr=%08x expected_len=%0d size=%0d burst=%0d id=%0d cache=%x",
                 awaddr, awlen, dst + aw_words * 4, beats - 1, awsize, awburst, awid, awcache);
        aw_addr_q[aw_wr] = awaddr;
        aw_len_q[aw_wr] = awlen;
        aw_id_q[aw_wr] = awid;
        aw_wr = aw_wr + 1;
        aw_words = aw_words + beats;
      end
      if (wvalid && wready) begin
        if (wstrb != 4'hf || wlast != (w_beat == aw_len_q[aw_rd]))
          $fatal(1, "DEFAULT_DUPLEX_W_PROFILE");
        index = mem_index(aw_addr_q[aw_rd]) + w_beat;
        if (wdata !== expected[index]) $fatal(1, "DEFAULT_DUPLEX_WRITE_DATA");
        memory[index] = wdata;
        w_words = w_words + 1;
        if (wlast) begin aw_rd = aw_rd + 1; w_beat = 0; b_pending = b_pending + 1; end
        else w_beat = w_beat + 1;
      end
      if (bvalid && bready) b_pending = b_pending - 1;
    end
  end

  task automatic run_case(input integer count);
    integer k, cycles;
    logic [31:0] expected_sum;
    logic seen_done;
    begin
      ar_wr = 0; ar_rd = 0; aw_wr = 0; aw_rd = 0;
      ar_beat = 0; w_beat = 0; b_pending = 0;
      ar_words = 0; aw_words = 0; r_words = 0; w_words = 0;
      expected_sum = 0;
      for (k = 0; k < WORDS; k = k + 1) expected[k] = memory[k];
      for (k = 0; k < count; k = k + 1) begin
        expected_sum = expected_sum + memory[mem_index(SRC) + k];
        expected[mem_index(DST) + k] = memory[mem_index(SRC) + k];
      end
      @(negedge clock); src = SRC; dst = DST; n = count; start_port = 1;
      @(posedge clock); #1; seen_done = done_port;
      @(negedge clock); start_port = 0;
      cycles = 0;
      while (!seen_done && cycles < 50000) begin
        @(posedge clock); #1; seen_done = done_port; cycles = cycles + 1;
      end
      if (!seen_done)
        $fatal(1, "DEFAULT_DUPLEX_TIMEOUT n=%0d AR=%0d R=%0d AW=%0d W=%0d B=%0d",
               count, ar_words, r_words, aw_words, w_words, b_pending);
      if (ar_words != count || aw_words != count || r_words != count || w_words != count)
        $fatal(1, "DEFAULT_DUPLEX_TRAFFIC n=%0d AR=%0d R=%0d AW=%0d W=%0d",
               count, ar_words, r_words, aw_words, w_words);
      if (return_port !== expected_sum)
        $fatal(1, "DEFAULT_DUPLEX_RETURN n=%0d got=%0d expected=%0d", count, return_port, expected_sum);
      for (k = 0; k < WORDS; k = k + 1)
        if (memory[k] !== expected[k])
          $fatal(1, "DEFAULT_DUPLEX_CANARY n=%0d word=%0d got=%08x expected=%08x",
                 count, k, memory[k], expected[k]);
      $display("DEFAULT_DUPLEX_REDUCTION PASS n=%0d return=%0d", count, return_port);
      @(negedge clock); reset = 0;
      repeat (2) @(posedge clock);
      @(negedge clock); reset = 1;
      repeat (2) @(posedge clock);
    end
  endtask

  integer i;
  initial begin
    for (i = 0; i < WORDS; i = i + 1) memory[i] = (i % 97) + 1;
    repeat (3) @(posedge clock);
    reset = 1;
    repeat (3) @(posedge clock);
    @(negedge clock);
    run_case(0);
    run_case(1);
    run_case(2);
    run_case(17);
    run_case(33);
    run_case(4096);
    $display("DEFAULT_DUPLEX_REDUCTION ALL_CASES PASS");
    $finish;
  end
endmodule
