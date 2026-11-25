// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
`timescale 1 ns / 1 ps

module tb_lp_phase_capture;
   parameter BP_MODE = 0;
   parameter MAX_CYCLE = 4000;
   reg clock = 0, reset = 0, start_port = 0, out_full_n = 1;
   reg [31:0] a_q0 = 0, a_q1 = 0, b_q0 = 0, b_q1 = 0;
   reg [31:0] c_q0 = 0, c_q1 = 0, d_q0 = 0, d_q1 = 0;
   wire done_port, idle_port, a_ce0, a_ce1, b_ce0, b_ce1, c_ce0, c_ce1, d_ce0, d_ce1, out_write;
   wire [3:0] a_address0, a_address1, b_address0, b_address1;
   wire [3:0] c_address0, c_address1, d_address0, d_address1;
   wire [31:0] out_din, return_port;
   reg [31:0] memA[0:15], memB[0:15], memC[0:15], memD[0:15];
   integer k;

   initial
      for(k = 0; k < 16; k = k + 1)
      begin
         memA[k] = k + 3;
         memB[k] = 2 * k + 5;
         memC[k] = 4 * k + 7;
         memD[k] = 3 * k + 11;
      end

   always @(posedge clock)
   begin
      if(a_ce0) a_q0 <= memA[a_address0];
      if(a_ce1) a_q1 <= memA[a_address1];
      if(b_ce0) b_q0 <= memB[b_address0];
      if(b_ce1) b_q1 <= memB[b_address1];
      if(c_ce0) c_q0 <= memC[c_address0];
      if(c_ce1) c_q1 <= memC[c_address1];
      if(d_ce0) d_q0 <= memD[d_address0];
      if(d_ce1) d_q1 <= memD[d_address1];
   end

   p_Z13phase_captureRA16_iS0_S0_S0_R10ac_channelIiE dut(
      .clock(clock), .reset(reset), .start_port(start_port),
      .a_q0(a_q0), .a_q1(a_q1), .b_q0(b_q0), .b_q1(b_q1),
      .c_q0(c_q0), .c_q1(c_q1), .d_q0(d_q0), .d_q1(d_q1),
      .out_full_n(out_full_n), .done_port(done_port), .idle_port(idle_port), .return_port(return_port),
      .a_address0(a_address0), .a_address1(a_address1), .a_ce0(a_ce0), .a_ce1(a_ce1),
      .b_address0(b_address0), .b_address1(b_address1), .b_ce0(b_ce0), .b_ce1(b_ce1),
      .c_address0(c_address0), .c_address1(c_address1), .c_ce0(c_ce0), .c_ce1(c_ce1),
      .d_address0(d_address0), .d_address1(d_address1), .d_ce0(d_ce0), .d_ce1(d_ce1),
      .out_din(out_din), .out_write(out_write));

`ifdef MUT
   initial #1 memA[2] = memA[2] + 9;
`endif

   always #5 clock = ~clock;

   integer cycle = 0, count = 0, errors = 0, stalls = 0;
   integer mul_launch_cycle = -1, overlap_launches = 0, phase_errors = 0, pending = 0;
   integer capture_during_wait = 0, consume_during_wait = 0, queue_peak = 0;
`define DUT_CONTROLLER dut.PBI_Z13phase_captureRA16_iS0_S0_S0_R10ac_channelIiE_i0
`define DUT_DATAPATH `DUT_CONTROLLER.Datapath_i
   wire mul_launch = `DUT_CONTROLLER.`LP_MUL_LAUNCH;
   wire unbounded_launch = `DUT_CONTROLLER.`LP_UNBOUNDED_LAUNCH;
   wire mul_consume = `DUT_CONTROLLER.`LP_MUL_CONSUME;
   wire [1:0] mul_queue_count = `DUT_DATAPATH.`LP_MUL_CAPTURE.result_queue.count;

   always @(posedge clock)
   begin
      cycle = cycle + 1;
      if(!reset || BP_MODE == 0) out_full_n <= 1;
      else if(BP_MODE == 1) out_full_n <= (cycle % 5) < 2;
      else out_full_n <= !((cycle >= 8) && (cycle < 48));

      if(reset && mul_launch) mul_launch_cycle = cycle;
      if(reset && unbounded_launch)
      begin
         if(mul_launch_cycle >= 0 && cycle - mul_launch_cycle == 1)
            overlap_launches = overlap_launches + 1;
         else
            phase_errors = phase_errors + 1;
         pending = 1;
      end

      if(reset && pending && !out_full_n)
      begin
         stalls = stalls + 1;
         if(mul_queue_count > queue_peak) queue_peak = mul_queue_count;
         if(mul_queue_count > 0) capture_during_wait = capture_during_wait + 1;
         if(mul_consume) consume_during_wait = consume_during_wait + 1;
      end

      if(reset && out_write && out_full_n)
      begin
         if(out_din !== expected_tag(count))
         begin
            errors = errors + 1;
            $display("OUTPUT mismatch pos=%0d got=%0d expected=%0d", count, out_din, expected_tag(count));
         end
         count = count + 1;
         pending = 0;
      end
   end

   initial
   begin
      repeat(4) @(posedge clock);
      reset <= 1;
      repeat(3) @(posedge clock);
      while(!idle_port) @(posedge clock);
      start_port <= 1;
      @(posedge clock);
      start_port <= 0;
      while(!done_port && cycle < MAX_CYCLE) @(posedge clock);
      if(!done_port || count != 16 || return_port !== expected_sum(0)) errors = errors + 1;
      if(overlap_launches != 16 || phase_errors != 0) errors = errors + 1;
      if(BP_MODE != 0 && (!stalls || !capture_during_wait || !queue_peak || consume_during_wait))
         errors = errors + 1;
      $display("PHASE_CAPTURE bp=%0d loop_ii=2 mul_phase=1 unbounded_phase=0 overlap=%0d capture_wait=%0d queue_peak=%0d consume_wait=%0d stalls=%0d",
               BP_MODE, overlap_launches, capture_during_wait, queue_peak, consume_during_wait, stalls);
      if(errors) $display("FAIL [BP=%0d] errors=%0d count=%0d return=%0d", BP_MODE, errors, count, return_port);
      else $display("PASS [BP=%0d] outputs=%0d return=%0d", BP_MODE, count, return_port);
      $finish;
   end

   function [31:0] expected_tag;
      input integer index;
      begin expected_tag = (4 * index + 7) + (3 * index + 11) + 1; end
   endfunction

   function [31:0] expected_sum;
      input integer unused;
      integer i;
      begin
         expected_sum = 0;
         for(i = 0; i < 16; i = i + 1) expected_sum = expected_sum + (i + 3) * (2 * i + 5);
      end
   endfunction

   initial
   begin
      #(MAX_CYCLE * 20);
      $display("FAIL [BP=%0d] watchdog", BP_MODE);
      $finish;
   end
endmodule
