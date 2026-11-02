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

   always #5 clock = ~clock;
   integer cycle = 0, count = 0, errors = 0, stalls = 0, started = 0, start_cycle = 0;
   integer stall_cycles = 0;
   integer first_output_cycle = -1;
   wire stall = dut.PBI_Z9co_launchRA16_iS0_R10ac_channelIiE_i0.selector_stallable_fus;
   always @(negedge clock) begin
      if(!reset || BP_MODE == 0) out_full_n = 1;
      else if(BP_MODE == 1) out_full_n = (cycle % 5) < 2;
      else out_full_n = !((first_output_cycle >= 0) && (cycle < first_output_cycle + 120));
   end
   always @(posedge clock) begin
      cycle = cycle + 1;
      if(reset && started && !out_full_n && !done_port) stalls = stalls + 1;
      if(reset && started && !out_full_n && stall === 1'b1) stall_cycles = stall_cycles + 1;
      if(reset && started && out_write && out_full_n) begin
         if(count >= 16 || out_din !== count) begin
            errors = errors + 1;
            $display("MISMATCH index=%0d got=%0d expected=%0d", count, out_din, count);
         end
         count = count + 1;
         if(first_output_cycle < 0) first_output_cycle = cycle;
      end
   end
   initial begin
      repeat(4) @(posedge clock); reset <= 1;
      repeat(3) @(posedge clock);
      while(!idle_port) @(posedge clock);
      start_port <= 1; @(posedge clock); start_port <= 0;
      started = 1; start_cycle = cycle;
      while(!done_port && cycle - start_cycle < MAX_CYCLE) @(posedge clock);
      if(!done_port || count != 16 || return_port !== 32'd4280) errors = errors + 1;
      if(errors) $display("FAIL [BP=%0d] errors=%0d count=%0d return=%0d", BP_MODE, errors, count, return_port);
      else if(BP_MODE && !stalls) $display("INCONCLUSIVE [BP=%0d] no stall", BP_MODE);
      else $display("PASS [BP=%0d] writes=%0d return=%0d latency=%0d stalls=%0d", BP_MODE, count,
                    return_port, cycle - start_cycle, stalls);
      if(BP_MODE != 0) $display("STALLABLE_STALL_ORACLE stall_cycles=%0d", stall_cycles);
      $finish;
   end
   initial begin #(MAX_CYCLE * 20); $display("FAIL [BP=%0d] watchdog", BP_MODE); $finish; end
endmodule
