// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
`timescale 1 ns / 1 ps

module tb_lp_co_launch;
   parameter BP_MODE = 0;
   parameter MAX_CYCLE = 4000;
   reg clock = 0, reset = 0, start_port = 0, out_full_n = 1;
   reg [31:0] a_q0 = 0, a_q1 = 0, b_q0 = 0, b_q1 = 0;
   wire done_port, idle_port, a_ce0, a_ce1, b_ce0, b_ce1, out_write;
   wire [3:0] a_address0, a_address1, b_address0, b_address1;
   wire [31:0] out_din, return_port;
   reg [31:0] memA [0:15], memB [0:15];
   integer k;
   initial for(k = 0; k < 16; k = k + 1) begin
      memA[k] = 5 + k;
      memB[k] = 3 + 2 * k;
   end
   always @(posedge clock) begin
      if(a_ce0) a_q0 <= memA[a_address0];
      if(a_ce1) a_q1 <= memA[a_address1];
      if(b_ce0) b_q0 <= memB[b_address0];
      if(b_ce1) b_q1 <= memB[b_address1];
   end

   p_Z9co_launchRA16_iS0_R10ac_channelIiE dut (
      .clock(clock), .reset(reset), .start_port(start_port),
      .a_q0(a_q0), .a_q1(a_q1), .b_q0(b_q0), .b_q1(b_q1),
      .out_full_n(out_full_n), .done_port(done_port), .idle_port(idle_port),
      .a_address0(a_address0), .a_address1(a_address1), .a_ce0(a_ce0), .a_ce1(a_ce1),
      .b_address0(b_address0), .b_address1(b_address1), .b_ce0(b_ce0), .b_ce1(b_ce1),
      .out_din(out_din), .out_write(out_write), .return_port(return_port));

`ifdef MUT
   initial #1 memA[2] = memA[2] + 7;
`endif

   integer cycle = 0, fu_cycle = 0, count = 0, errors = 0, stalls = 0, started = 0, start_cycle = 0;
   integer fu_launches = 0, fu_hold_checks = 0, fu_stability_errors = 0, fu_operand_changes = 0;
   integer fu_previous_launch = -1, last_fu_launch_cycle = -1, fu_min_spacing = 999999, fu_spacing_errors = 0;
   integer fu_stalled_hold_checks = 0;
   integer bp2_stall_start_cycle = -1;
   integer bp2_stalled_ub_logged = 0, bp2_delayed_ub_logged = 0;
   integer ub_launches = 0, inflight_launches = 0, done_same_cycle = 0, done_later = 0;
   integer done_before_ii = 0, ub_pending = 0, ub_handshake_errors = 0;
   integer ub_pending_fu_cycle = -1, ub_pending_inflight = 0, accepted_writes = 0;
   reg fu_hold_valid = 0;
   reg [31:0] fu_sample_a, fu_sample_b;
`define DUT_CONTROLLER dut.PBI_Z9co_launchRA16_iS0_R10ac_channelIiE_i0
`define DUT_DATAPATH `DUT_CONTROLLER.Datapath_i
`define FU_LAUNCH `DUT_CONTROLLER.`LP_MUL_LAUNCH
`define FU_INPUT_A `DUT_DATAPATH.`LP_MUL_INSTANCE.`LP_MUL_PORT_A
`define FU_INPUT_B `DUT_DATAPATH.`LP_MUL_INSTANCE.`LP_MUL_PORT_B
   wire fu_launch = `FU_LAUNCH;
   wire unbounded_launch = `DUT_CONTROLLER.`LP_UNBOUNDED_LAUNCH;
   wire unbounded_done = `DUT_CONTROLLER.`LP_UNBOUNDED_DONE;

   always @(posedge clock) begin
      fu_cycle = fu_cycle + 1;
      if(reset && ub_pending) begin
         if(unbounded_done === 1'b1) begin
            if(!(out_write && out_full_n)) ub_handshake_errors = ub_handshake_errors + 1;
            done_later = done_later + 1;
            if(ub_pending_inflight && fu_cycle - ub_pending_fu_cycle < 2)
               done_before_ii = done_before_ii + 1;
            if(BP_MODE == 2 && !bp2_delayed_ub_logged) begin
               bp2_delayed_ub_logged = 1;
               $display("BP2_UB_RELEASED cycle=%0d ready=%0d out_write=%0d", cycle, out_full_n, out_write);
            end
            ub_pending = 0;
            ub_pending_inflight = 0;
         end
      end
      if(reset && fu_launch === 1'b1) begin
         if(fu_previous_launch >= 0) begin
            if(fu_cycle - fu_previous_launch < 2) fu_spacing_errors = fu_spacing_errors + 1;
            if(fu_cycle - fu_previous_launch < fu_min_spacing)
               fu_min_spacing = fu_cycle - fu_previous_launch;
         end
         fu_previous_launch = fu_cycle;
         last_fu_launch_cycle = fu_cycle;
         fu_launches = fu_launches + 1;
         if(BP_MODE == 2 && fu_launches == 1)
            $display("BP2_FIRST_FU cycle=%0d ready=%0d", cycle, out_full_n);
         if(fu_hold_valid &&
            (`FU_INPUT_A !== fu_sample_a ||
             `FU_INPUT_B !== fu_sample_b))
            fu_operand_changes = fu_operand_changes + 1;
         fu_sample_a = `FU_INPUT_A;
         fu_sample_b = `FU_INPUT_B;
         fu_hold_valid = 1;
      end
      else if(reset && fu_hold_valid) begin
         fu_hold_checks = fu_hold_checks + 1;
         if(!out_full_n) fu_stalled_hold_checks = fu_stalled_hold_checks + 1;
         if(`FU_INPUT_A !== fu_sample_a ||
            `FU_INPUT_B !== fu_sample_b) begin
            fu_stability_errors = fu_stability_errors + 1;
            $display("FU_INPUT_CHANGED launch=%0d old=(%0d,%0d) now=(%0d,%0d)", fu_launches, fu_sample_a, fu_sample_b,
                     `FU_INPUT_A,
                     `FU_INPUT_B);
         end
      end
      if(reset && unbounded_launch === 1'b1) begin
         if(ub_pending) ub_handshake_errors = ub_handshake_errors + 1;
         ub_launches = ub_launches + 1;
         ub_pending_fu_cycle = last_fu_launch_cycle;
         ub_pending_inflight = last_fu_launch_cycle >= 0 && fu_cycle > last_fu_launch_cycle &&
                               fu_cycle - last_fu_launch_cycle < 2;
         if(ub_pending_inflight) inflight_launches = inflight_launches + 1;
         if(BP_MODE == 2 && !out_full_n && !bp2_stalled_ub_logged) begin
            bp2_stalled_ub_logged = 1;
            $display("BP2_STALLED_UB cycle=%0d ready=%0d out_write=%0d done_now=%0d", cycle, out_full_n,
                     out_write, unbounded_done);
         end
         if(unbounded_done === 1'b1) begin
            if(!(out_write && out_full_n)) ub_handshake_errors = ub_handshake_errors + 1;
            done_same_cycle = done_same_cycle + 1;
            if(ub_pending_inflight && fu_cycle - ub_pending_fu_cycle < 2)
               done_before_ii = done_before_ii + 1;
            ub_pending_inflight = 0;
         end
         else begin
            ub_pending = 1;
         end
      end
   end

   always @(negedge clock) begin
      #1;
      if(reset && fu_hold_valid && `FU_LAUNCH !== 1'b1) begin
         if(`FU_INPUT_A !== fu_sample_a ||
            `FU_INPUT_B !== fu_sample_b) begin
            fu_stability_errors = fu_stability_errors + 1;
            $display("FU_INPUT_CHANGED_BETWEEN_EDGES launch=%0d", fu_launches);
         end
      end
   end

`ifdef HOLD_MUT
   initial begin
      wait(reset && `FU_LAUNCH === 1'b1);
      @(posedge clock);
      @(negedge clock);
      #0.1 force `FU_INPUT_A = 32'hdeadbeef;
      #1.2 release `FU_INPUT_A;
   end
`endif

   always #5 clock = ~clock;
   always @(negedge clock) begin
      if(BP_MODE == 0) out_full_n = 1;
      else if(BP_MODE == 1) begin
         if(!reset) out_full_n = 1;
         else out_full_n = (cycle % 5) < 2;
      end
      else if(bp2_stall_start_cycle < 0 && started && fu_launches > 0) begin
         bp2_stall_start_cycle = cycle;
         out_full_n = 0;
         $display("BP2_STALL_BEGIN cycle=%0d fu_launches=%0d", cycle, fu_launches);
      end
      else if(bp2_stall_start_cycle >= 0 && cycle - bp2_stall_start_cycle < 120)
         out_full_n = 0;
      else begin
         if(bp2_stall_start_cycle >= 0 && cycle - bp2_stall_start_cycle == 120)
            $display("BP2_STALL_END cycle=%0d done_later=%0d", cycle, done_later);
         out_full_n = 1;
      end
   end
   always @(posedge clock) begin
      cycle = cycle + 1;
      if(BP_MODE == 2 && start_port)
         $display("BP2_START cycle=%0d start_port=%0d ready=%0d", cycle, start_port, out_full_n);
      if(reset && started && !out_full_n && !done_port) stalls = stalls + 1;
      if(reset && started && out_write && out_full_n) begin
         accepted_writes = accepted_writes + 1;
         if(count >= 16 || out_din !== count) begin
            errors = errors + 1;
            $display("MISMATCH index=%0d got=%0d expected=%0d", count, out_din, count);
         end
         count = count + 1;
      end
   end

   initial begin
      repeat(4) @(posedge clock); reset <= 1;
      repeat(3) @(posedge clock);
      while(!idle_port) @(posedge clock);
      start_port <= 1; @(posedge clock); start_port <= 0;
      started = 1; start_cycle = cycle;
      while(!done_port && cycle - start_cycle < MAX_CYCLE) @(posedge clock);
      if(!done_port || count != 16 || accepted_writes != ub_launches || ub_launches != 16 ||
         return_port !== 32'd4280 || fu_launches < 2 || fu_hold_checks == 0 ||
         fu_stability_errors || fu_operand_changes == 0 || fu_spacing_errors || fu_min_spacing < 2 ||
         inflight_launches == 0 || (BP_MODE == 0 && done_before_ii == 0) || ub_pending ||
         ub_handshake_errors || done_same_cycle + done_later != ub_launches ||
         (BP_MODE == 0 && done_later != 0) ||
         (BP_MODE != 0 && (!stalls || !done_later || !fu_stalled_hold_checks))) errors = errors + 1;
`ifdef HOLD_MUT
      if(fu_stability_errors && count == 16 && return_port === 32'd4280)
         $display("HOLD_MUT_DETECTED return=%0d", return_port);
`endif
      if(errors) $display("FAIL [BP=%0d] errors=%0d count=%0d return=%0d", BP_MODE, errors, count, return_port);
      else if(BP_MODE && !stalls) $display("INCONCLUSIVE [BP=%0d] no stall", BP_MODE);
      else $display("PASS [BP=%0d] writes=%0d return=%0d latency=%0d stalls=%0d", BP_MODE, count,
                    return_port, cycle - start_cycle, stalls);
      $display("FU_II2_MONITOR launches=%0d hold_checks=%0d stable_errors=%0d operand_changes=%0d spacing_min=%0d spacing_errors=%0d stalled_hold_checks=%0d",
               fu_launches, fu_hold_checks, fu_stability_errors, fu_operand_changes, fu_min_spacing, fu_spacing_errors,
               fu_stalled_hold_checks);
      $display("UNBOUNDED_HANDSHAKE launches=%0d inflight_launches=%0d done_same_cycle=%0d done_later=%0d done_before_ii=%0d pending=%0d handshake_errors=%0d accepted_writes=%0d",
               ub_launches, inflight_launches, done_same_cycle, done_later, done_before_ii, ub_pending,
               ub_handshake_errors, accepted_writes);
      $finish;
   end
   initial begin #(MAX_CYCLE * 20); $display("FAIL [BP=%0d] watchdog", BP_MODE); $finish; end
endmodule
