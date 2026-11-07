// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Self checking testbench with backpressure for the two designs that read from
// two arrays: two multi-cycle reads in flight at the same time feeding one
// unbounded write. Two modules are needed because the name of the generated
// component comes from the C++ signature and cannot be parameterized.
//
// BP_MODE: 0 no backpressure, 1 alternating, 2 not ready for 28 cycles. The
// expected value is a formula over the position, not a scheduling cost.

`timescale 1 ns / 1 ps

module tb_lp_dual_read;

   parameter DEPTH     = 16;
   parameter BP_MODE   = 0;
   parameter MAX_CYCLE = 4000;

   reg         clock = 1'b0;
   reg         reset = 1'b0;
   reg         start_port = 1'b0;
   reg  [31:0] a_q0 = 32'b0;
   reg  [31:0] a_q1 = 32'b0;
   reg  [31:0] b_q0 = 32'b0;
   reg  [31:0] b_q1 = 32'b0;
   reg         out_full_n = 1'b1;
   wire        done_port;
   wire        idle_port;
   wire [3:0]  a_address0;
   wire [3:0]  a_address1;
   wire        a_ce0;
   wire        a_ce1;
   wire [3:0]  b_address0;
   wire [3:0]  b_address1;
   wire        b_ce0;
   wire        b_ce1;
   wire [31:0] out_din;
   wire        out_write;

   reg [31:0] memA [0:DEPTH-1];
   reg [31:0] memB [0:DEPTH-1];
   integer k;
   initial
      for (k = 0; k < DEPTH; k = k + 1)
      begin
         memA[k] = 32'd700000 + k;
         memB[k] = 32'd900000 + 2 * k;
      end

   always @(posedge clock)
      if (a_ce0) a_q0 <= memA[a_address0];
   always @(posedge clock)
      if (a_ce1) a_q1 <= memA[a_address1];
   always @(posedge clock)
      if (b_ce0) b_q0 <= memB[b_address0];
   always @(posedge clock)
      if (b_ce1) b_q1 <= memB[b_address1];

   p_Z9dual_readRA16_iS0_R10ac_channelIiE dut (
      .clock(clock),
      .reset(reset),
      .start_port(start_port),
      .a_q0(a_q0),
      .a_q1(a_q1),
      .b_q0(b_q0),
      .b_q1(b_q1),
      .out_full_n(out_full_n),
      .done_port(done_port),
      .idle_port(idle_port),
      .a_address0(a_address0),
      .a_address1(a_address1),
      .a_ce0(a_ce0),
      .a_ce1(a_ce1),
      .b_address0(b_address0),
      .b_address1(b_address1),
      .b_ce0(b_ce0),
      .b_ce1(b_ce1),
      .out_din(out_din),
      .out_write(out_write));

`ifdef MUT
   // Sensitivity: a corrupted word of the memory A model must show up as a
   // MISMATCH against the formula based expectation.
   initial
      #1 memA[3] = memA[3] + 32'd5;
`endif

   always #5 clock = ~clock;

   integer cycle = 0;
   always @(posedge clock)
   begin
      cycle = cycle + 1;
      if (!reset) out_full_n <= 1'b1;
      else if (BP_MODE == 0) out_full_n <= 1'b1;
      else if (BP_MODE == 1) out_full_n <= ((cycle % 5) < 2);
      else out_full_n <= !((cycle >= 14) && (cycle < 42));
   end

   integer n_checked = 0;
   integer errors = 0;
   integer first_bad_cycle = -1;
   integer pending_cycles = 0;
   integer started = 0;
   reg [31:0] exp;

   always @(posedge clock)
   begin
      if (reset && started && !done_port && !out_full_n && !out_write && n_checked < DEPTH)
         pending_cycles = pending_cycles + 1;

      if (reset && started && out_write && out_full_n)
      begin
         exp = (32'd700000 + n_checked) + (32'd900000 + 2 * n_checked);
         if (out_din !== exp)
         begin
            errors = errors + 1;
            if (first_bad_cycle < 0) first_bad_cycle = cycle;
            $display("  MISMATCH pos=%0d cycle=%0d: received %0d expected %0d",
                     n_checked, cycle, out_din, exp);
         end
         n_checked = n_checked + 1;
      end
   end

   integer wait_t = 0;
   integer start_cycle = 0;
   integer done_cycle = 0;

   initial
   begin
      $display("== tb_lp_dual_read: BP_MODE=%0d ==", BP_MODE);
      repeat (4) @(posedge clock);
      reset <= 1'b1;
      repeat (3) @(posedge clock);
      wait_t = 0;
      while (!idle_port && wait_t < 50)
      begin
         @(posedge clock);
         wait_t = wait_t + 1;
      end
      start_port <= 1'b1;
      @(posedge clock);
      start_port <= 1'b0;
      started = 1;
      start_cycle = cycle;

      while (!done_port && (cycle - start_cycle) < MAX_CYCLE) @(posedge clock);
      done_cycle = cycle;
      // a controller that never completes must not pass, even with every word
      // delivered correctly
      if (!done_port)
      begin
         errors = errors + 1;
         $display("  ERROR: TIMEOUT, done_port never asserted");
      end

      // the distance, not the difference: more writes than DEPTH are an error too
      if (n_checked != DEPTH)
      begin
         errors = errors + ((n_checked > DEPTH) ? (n_checked - DEPTH) : (DEPTH - n_checked));
         $display("  ERROR: %0d writes delivered, %0d expected", n_checked, DEPTH);
      end
      $display("  writes checked = %0d, cycles = %0d, stalling = %0d",
               n_checked, done_cycle - start_cycle, pending_cycles);
      if (errors > 0)
      begin
         if (first_bad_cycle >= 0)
            $display("FAIL [BP=%0d] %0d errors, first bad data at cycle %0d", BP_MODE, errors, first_bad_cycle);
         else
            $display("FAIL [BP=%0d] %0d errors, no data mismatch", BP_MODE, errors);
      end
      else if ((BP_MODE != 0) && (pending_cycles == 0))
         $display("INCONCLUSIVE [BP=%0d] no stalling cycle with a pending write", BP_MODE);
      else
         $display("PASS [BP=%0d] %0d correct writes in %0d cycles (%0d stalling)",
                  BP_MODE, n_checked, done_cycle - start_cycle, pending_cycles);
      $finish;
   end

   initial
   begin
      #(MAX_CYCLE * 20);
      $display("FAIL [BP=%0d] global watchdog", BP_MODE);
      $finish;
   end

endmodule

// Loop-carried accumulator: the expected value is the running partial sum.
module tb_lp_dual_read_acc;

   parameter DEPTH     = 16;
   parameter BP_MODE   = 0;
   parameter MAX_CYCLE = 4000;

   reg         clock = 1'b0;
   reg         reset = 1'b0;
   reg         start_port = 1'b0;
   reg  [31:0] a_q0 = 32'b0;
   reg  [31:0] a_q1 = 32'b0;
   reg  [31:0] b_q0 = 32'b0;
   reg  [31:0] b_q1 = 32'b0;
   reg         out_full_n = 1'b1;
   wire        done_port;
   wire        idle_port;
   wire [3:0]  a_address0;
   wire [3:0]  a_address1;
   wire        a_ce0;
   wire        a_ce1;
   wire [3:0]  b_address0;
   wire [3:0]  b_address1;
   wire        b_ce0;
   wire        b_ce1;
   wire [31:0] out_din;
   wire        out_write;

   reg [31:0] memA [0:DEPTH-1];
   reg [31:0] memB [0:DEPTH-1];
   integer k;
   initial
      for (k = 0; k < DEPTH; k = k + 1)
      begin
         memA[k] = 32'd700000 + k;
         memB[k] = 32'd900000 + 2 * k;
      end

   always @(posedge clock)
      if (a_ce0) a_q0 <= memA[a_address0];
   always @(posedge clock)
      if (a_ce1) a_q1 <= memA[a_address1];
   always @(posedge clock)
      if (b_ce0) b_q0 <= memB[b_address0];
   always @(posedge clock)
      if (b_ce1) b_q1 <= memB[b_address1];

   p_Z13dual_read_accRA16_iS0_R10ac_channelIiE dut (
      .clock(clock),
      .reset(reset),
      .start_port(start_port),
      .a_q0(a_q0),
      .a_q1(a_q1),
      .b_q0(b_q0),
      .b_q1(b_q1),
      .out_full_n(out_full_n),
      .done_port(done_port),
      .idle_port(idle_port),
      .a_address0(a_address0),
      .a_address1(a_address1),
      .a_ce0(a_ce0),
      .a_ce1(a_ce1),
      .b_address0(b_address0),
      .b_address1(b_address1),
      .b_ce0(b_ce0),
      .b_ce1(b_ce1),
      .out_din(out_din),
      .out_write(out_write));

`ifdef MUT
   // Sensitivity: a corrupted word of the memory A model must show up as a
   // MISMATCH against the formula based expectation.
   initial
      #1 memA[3] = memA[3] + 32'd5;
`endif

   always #5 clock = ~clock;

   integer cycle = 0;
   always @(posedge clock)
   begin
      cycle = cycle + 1;
      if (!reset) out_full_n <= 1'b1;
      else if (BP_MODE == 0) out_full_n <= 1'b1;
      else if (BP_MODE == 1) out_full_n <= ((cycle % 5) < 2);
      else out_full_n <= !((cycle >= 14) && (cycle < 42));
   end

   integer n_checked = 0;
   integer errors = 0;
   integer first_bad_cycle = -1;
   integer pending_cycles = 0;
   integer started = 0;
   reg [31:0] exp;
   reg [31:0] running;

   always @(posedge clock)
   begin
      if (reset && started && !done_port && !out_full_n && !out_write && n_checked < DEPTH)
         pending_cycles = pending_cycles + 1;

      if (reset && started && out_write && out_full_n)
      begin
         running = running + (32'd700000 + n_checked) + (32'd900000 + 2 * n_checked);
         exp = running;
         if (out_din !== exp)
         begin
            errors = errors + 1;
            if (first_bad_cycle < 0) first_bad_cycle = cycle;
            $display("  MISMATCH pos=%0d cycle=%0d: received %0d expected %0d",
                     n_checked, cycle, out_din, exp);
         end
         n_checked = n_checked + 1;
      end
   end

   integer wait_t = 0;
   integer start_cycle = 0;
   integer done_cycle = 0;

   initial
   begin
      running = 32'd0;
      $display("== tb_lp_dual_read_acc: BP_MODE=%0d ==", BP_MODE);
      repeat (4) @(posedge clock);
      reset <= 1'b1;
      repeat (3) @(posedge clock);
      wait_t = 0;
      while (!idle_port && wait_t < 50)
      begin
         @(posedge clock);
         wait_t = wait_t + 1;
      end
      start_port <= 1'b1;
      @(posedge clock);
      start_port <= 1'b0;
      started = 1;
      start_cycle = cycle;

      while (!done_port && (cycle - start_cycle) < MAX_CYCLE) @(posedge clock);
      done_cycle = cycle;
      // a controller that never completes must not pass, even with every word
      // delivered correctly
      if (!done_port)
      begin
         errors = errors + 1;
         $display("  ERROR: TIMEOUT, done_port never asserted");
      end

      // the distance, not the difference: more writes than DEPTH are an error too
      if (n_checked != DEPTH)
      begin
         errors = errors + ((n_checked > DEPTH) ? (n_checked - DEPTH) : (DEPTH - n_checked));
         $display("  ERROR: %0d writes delivered, %0d expected", n_checked, DEPTH);
      end
      $display("  writes checked = %0d, cycles = %0d, stalling = %0d",
               n_checked, done_cycle - start_cycle, pending_cycles);
      if (errors > 0)
      begin
         if (first_bad_cycle >= 0)
            $display("FAIL [BP=%0d] %0d errors, first bad data at cycle %0d", BP_MODE, errors, first_bad_cycle);
         else
            $display("FAIL [BP=%0d] %0d errors, no data mismatch", BP_MODE, errors);
      end
      else if ((BP_MODE != 0) && (pending_cycles == 0))
         $display("INCONCLUSIVE [BP=%0d] no stalling cycle with a pending write", BP_MODE);
      else
         $display("PASS [BP=%0d] %0d correct writes in %0d cycles (%0d stalling)",
                  BP_MODE, n_checked, done_cycle - start_cycle, pending_cycles);
      $finish;
   end

   initial
   begin
      #(MAX_CYCLE * 20);
      $display("FAIL [BP=%0d] global watchdog", BP_MODE);
      $finish;
   end

endmodule
