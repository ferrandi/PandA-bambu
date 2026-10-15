module tb_generated_write_kernel;
  logic clock = 1'b0;
  always #5 clock = ~clock;

  logic reset = 1'b0;
  logic start_port = 1'b0;
  logic [31:0] dst = '0;
  logic [31:0] n = '0;
  logic [31:0] seed = '0;
  logic [31:0] unused = 32'h0000_d000;
  logic awready = 1'b1;
  logic wready = 1'b1;
  logic bvalid = 1'b0;
  logic [1:0] bresp = 2'b0;
  logic [5:0] bid = 6'b0;
  logic release_final_b = 1'b0;
  wire done_port;
  wire idle_port;
  wire [5:0] awid;
  wire [31:0] awaddr;
  wire [7:0] awlen;
  wire [2:0] awsize;
  wire [1:0] awburst;
  wire awvalid;
  wire [31:0] wdata;
  wire [3:0] wstrb;
  wire wlast;
  wire wvalid;
  wire bready;
  wire arvalid;
  wire rready;

  logic [31:0] memory [0:131071];
  logic [31:0] aw_addr_q [0:255];
  integer aw_len_q [0:255];
  logic [31:0] w_data_q [0:4095];
  logic w_last_q [0:4095];
  integer aw_wr = 0, aw_rd = 0, w_wr = 0, w_rd = 0;
  integer responses_pending = 0;
  integer response_count = 0;
  integer committed_words = 0;
  integer committed_bursts = 0;
  integer expected_aw_words = 0;
  integer expected_w_words = 0;
  logic [31:0] case_base = 0;
  integer case_count = 0;
  logic case_reset = 1'b0;
  integer k;

  `ifdef GENERATED_TOP
  `GENERATED_TOP dut (
  `else
  write_count_seed dut (
  `endif
    .clock(clock), .reset(reset), .start_port(start_port), .dst(dst), .n(n),
    `ifdef UNUSED_BUNDLE_CASE
    `ifdef GENERATED_UNUSED_PORT
    .unused(unused),
    `endif
    `else
    .seed(seed),
    `endif
    .m_axi_output_awready(awready), .m_axi_output_wready(wready),
    .m_axi_output_bid(bid), .m_axi_output_bresp(bresp), .m_axi_output_buser(1'b0),
    .m_axi_output_bvalid(bvalid), .m_axi_output_arready(1'b0),
    .m_axi_output_rid(6'b0), .m_axi_output_rdata(32'b0),
    .m_axi_output_rresp(2'b0), .m_axi_output_rlast(1'b0),
    .m_axi_output_ruser(1'b0), .m_axi_output_rvalid(1'b0),
    .done_port(done_port), .idle_port(idle_port),
    .m_axi_output_awid(awid), .m_axi_output_awaddr(awaddr),
    .m_axi_output_awlen(awlen), .m_axi_output_awsize(awsize),
    .m_axi_output_awburst(awburst), .m_axi_output_awvalid(awvalid),
    .m_axi_output_wdata(wdata), .m_axi_output_wstrb(wstrb),
    .m_axi_output_wlast(wlast), .m_axi_output_wvalid(wvalid),
    .m_axi_output_bready(bready), .m_axi_output_arvalid(arvalid),
    .m_axi_output_rready(rready)
  );

  function automatic integer burst_beats(input integer offset);
    integer remain;
    integer page;
    begin
      remain = case_count - offset;
      page = (4096 - ((case_base + offset*4) & 32'hfff)) / 4;
      burst_beats = remain < 16 ? remain : 16;
      if (page < burst_beats) burst_beats = page;
    end
  endfunction

  task automatic fail(input string message);
    $display("GENERATED_WRITE_KERNEL_G2_FAIL: %s", message);
    $fatal(1, "%s", message);
  endtask

  always @(posedge clock) begin : axi_slave
    integer beats;
    integer j;
    integer index;
    if (!reset || case_reset) begin
      aw_wr = 0; aw_rd = 0; w_wr = 0; w_rd = 0;
      responses_pending = 0; response_count = 0;
      committed_words = 0; committed_bursts = 0;
      expected_aw_words = 0; expected_w_words = 0;
      bvalid = 1'b0;
    end else begin
      if (arvalid || rready)
        fail("write-only generated bundle drove read-side valid/ready");
      if (awvalid && awready) begin
        if (aw_wr >= 256) fail("AW queue overflow");
        if (awaddr !== case_base + expected_aw_words*4)
          fail($sformatf("AW address mismatch got=%08x expected=%08x", awaddr,
                         case_base + expected_aw_words*4));
        beats = burst_beats(expected_aw_words);
        if (beats <= 0 || awlen !== beats-1)
          fail($sformatf("AWLEN mismatch offset=%0d got=%0d expected=%0d",
                         expected_aw_words, awlen, beats-1));
        if (awsize !== 3'd2 || awburst !== 2'b01 || awid !== 0)
          fail("AW sideband is not 32-bit AXI INCR with ID zero");
        if (awaddr[1:0] != 0 || (awaddr[11:0] + ((awlen+1)*4)) > 4096)
          fail("unaligned or 4-KiB-crossing write burst");
        aw_addr_q[aw_wr] = awaddr;
        aw_len_q[aw_wr] = beats;
        aw_wr = aw_wr + 1;
        expected_aw_words = expected_aw_words + beats;
      end
      if (wvalid && wready) begin
        if (w_wr >= 4096) fail("W queue overflow");
        if (wstrb !== 4'hf) fail("WSTRB is not full-word");
        if (wdata !== seed + expected_w_words)
          fail($sformatf("W data mismatch beat=%0d got=%08x expected=%08x",
                         expected_w_words, wdata, seed + expected_w_words));
        w_data_q[w_wr] = wdata;
        w_last_q[w_wr] = wlast;
        w_wr = w_wr + 1;
        expected_w_words = expected_w_words + 1;
      end
      while (aw_rd < aw_wr && w_wr-w_rd >= aw_len_q[aw_rd]) begin
        beats = aw_len_q[aw_rd];
        for (j = 0; j < beats; j = j + 1) begin
          if (w_last_q[w_rd+j] !== (j == beats-1))
            fail($sformatf("WLAST mismatch burst=%0d beat=%0d", aw_rd, j));
          index = (aw_addr_q[aw_rd] >> 2) + j;
          memory[index] = w_data_q[w_rd+j];
        end
        w_rd = w_rd + beats;
        committed_words = committed_words + beats;
        committed_bursts = committed_bursts + 1;
        responses_pending = responses_pending + 1;
        aw_rd = aw_rd + 1;
      end
      if (bvalid && bready) begin
        bvalid = 1'b0;
        responses_pending = responses_pending - 1;
        response_count = response_count + 1;
      end
      if (!bvalid && responses_pending > 0 &&
          (committed_words < case_count || release_final_b))
        bvalid = 1'b1;
      if (aw_wr - response_count > 1)
        fail("more than one write response outstanding");
    end
  end

  task automatic launch_case(input integer count, input logic [31:0] base,
                             input logic [31:0] data_seed);
    integer cycles;
    integer before_words;
    integer before_bursts;
    integer before_responses;
    integer expected_bursts;
    integer offset;
    integer beats;
    logic seen_done;
    begin
      @(negedge clock);
      case_reset = 1'b1;
      case_count = count;
      case_base = base;
      dst = base;
      n = count;
      seed = data_seed;
      release_final_b = 1'b0;
      start_port = 1'b0;
      @(negedge clock);
      case_reset = 1'b0;
      @(negedge clock);
      start_port = 1'b1;
      @(posedge clock); #1;
      seen_done = done_port;
      @(negedge clock);
      start_port = 1'b0;

      cycles = 0;
      if (count == 0) begin
        while (!seen_done && cycles < 2000) begin
          @(posedge clock); #1;
          seen_done = done_port;
          cycles = cycles + 1;
        end
        if (!seen_done) fail("zero-count invocation did not complete");
        if (aw_wr != 0 || w_wr != 0 || committed_words != 0 || arvalid || rready)
          fail("zero-count invocation emitted a bus transaction");
      end else begin
        if (seen_done) fail("nonzero invocation completed on its start edge");
        while (committed_words < count && cycles < 10000) begin
          @(posedge clock); #1;
          if (done_port) fail("done asserted before final B response");
          cycles = cycles + 1;
        end
        if (committed_words != count || committed_bursts == 0)
          fail("write data did not commit before final B was withheld");
        if (bvalid) fail("final B response was not withheld after final data commit");
        repeat (3) begin
          @(posedge clock); #1;
          if (done_port) fail("done asserted while final B response was withheld");
        end
        release_final_b = 1'b1;
        cycles = 0;
        seen_done = 1'b0;
        while (!seen_done && cycles < 10000) begin
          @(posedge clock); #1;
          seen_done = done_port;
          cycles = cycles + 1;
        end
        if (!seen_done) fail("kernel did not complete after final B response");
      end

      expected_bursts = 0;
      offset = 0;
      while (offset < count) begin
        beats = burst_beats(offset);
        expected_bursts = expected_bursts + 1;
        offset = offset + beats;
      end
      if (aw_wr != expected_bursts || committed_bursts != expected_bursts ||
          response_count != expected_bursts || committed_words != count)
        fail($sformatf("transaction counts mismatch n=%0d AW=%0d commits=%0d B=%0d",
                       count, aw_wr, committed_bursts, response_count));
      for (k = 0; k < count; k = k + 1)
        if (memory[(base >> 2)+k] !== data_seed + k)
          fail($sformatf("external memory observation mismatch address=%08x", base+k*4));
      if ((base >> 2) > 0 && memory[(base >> 2)-1] !== 32'hcafe_0001)
        fail("leading canary changed");
      if (memory[(base >> 2)+count] !== 32'hcafe_0002)
        fail("trailing canary changed");
      if (memory[(unused >> 2)] !== 32'hcafe_5151)
        fail("unused-pointer sentinel canary changed");
      $display("GENERATED_WRITE_KERNEL_CASE_PASS n=%0d base=%08x seed=%08x bursts=%0d beats=%0d",
               count, base, data_seed, expected_bursts, count);
    end
  endtask

  initial begin
    integer index;
    for (index = 0; index < 131072; index = index + 1) memory[index] = 32'hcafe_cafe;
    memory[(32'h0000_1000 >> 2)-1] = 32'hcafe_0001;
    memory[(32'h0000_1000 >> 2)+0] = 32'hcafe_0002;
    memory[(32'h0000_2000 >> 2)-1] = 32'hcafe_0001;
    memory[(32'h0000_2000 >> 2)+1] = 32'hcafe_0002;
    memory[(32'h0000_3000 >> 2)-1] = 32'hcafe_0001;
    memory[(32'h0000_3000 >> 2)+15] = 32'hcafe_0002;
    memory[(32'h0000_4000 >> 2)-1] = 32'hcafe_0001;
    memory[(32'h0000_4000 >> 2)+16] = 32'hcafe_0002;
    memory[(32'h0000_5000 >> 2)-1] = 32'hcafe_0001;
    memory[(32'h0000_5000 >> 2)+17] = 32'hcafe_0002;
    memory[(32'h0000_6000 >> 2)-1] = 32'hcafe_0001;
    memory[(32'h0000_6000 >> 2)+38] = 32'hcafe_0002;
    memory[(32'h0000_8ff8 >> 2)-1] = 32'hcafe_0001;
    memory[(32'h0000_8ff8 >> 2)+38] = 32'hcafe_0002;

    repeat (3) @(negedge clock);
    reset = 1'b1;
    repeat (2) @(negedge clock);
    if (!idle_port) fail("kernel not idle after reset");
    memory[(unused >> 2)] = 32'hcafe_5151;
    `ifdef UNUSED_BUNDLE_CASE
    launch_case(0, 32'h0000_1000, 32'd7);
    launch_case(1, 32'h0000_2000, 32'd7);
    launch_case(15, 32'h0000_3000, 32'd7);
    launch_case(16, 32'h0000_4000, 32'd7);
    launch_case(17, 32'h0000_5000, 32'd7);
    launch_case(38, 32'h0000_6000, 32'd7);
    launch_case(38, 32'h0000_6000, 32'd7);
    launch_case(38, 32'h0000_8ff8, 32'd7);
    `else
    launch_case(0, 32'h0000_1000, 32'd0);
    launch_case(1, 32'h0000_2000, 32'd0);
    launch_case(15, 32'h0000_3000, 32'd0);
    launch_case(16, 32'h0000_4000, 32'd0);
    launch_case(17, 32'h0000_5000, 32'd0);
    launch_case(38, 32'h0000_6000, 32'd0);
    launch_case(38, 32'h0000_6000, 32'd4096);
    launch_case(38, 32'h0000_8ff8, 32'd0);
    `endif
    $display("GENERATED_WRITE_KERNEL_G2_PASS cases=8 reinvocation=1");
    $finish;
  end
endmodule
