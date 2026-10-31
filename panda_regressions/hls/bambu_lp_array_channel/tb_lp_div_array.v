// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
`timescale 1 ns / 1 ps

module tb_lp_div_array;
   parameter BP_MODE = 0;
   parameter MAX_CYCLE = 1000;
   reg clock = 0, reset = 0, start_port = 0, out_full_n = 1;
   reg [31:0] numerator_q0 = 0, numerator_q1 = 0, denominator_q0 = 0, denominator_q1 = 0;
   wire done_port, idle_port, numerator_ce0, numerator_ce1, denominator_ce0, denominator_ce1, out_write;
   wire [2:0] numerator_address0, numerator_address1, denominator_address0, denominator_address1;
   wire [31:0] out_din;
   reg [31:0] numerator_mem [0:7], denominator_mem [0:7];
   integer k;

   initial begin
      for(k = 0; k < 8; k = k + 1) begin
         numerator_mem[k] = 1000 + 137*k;
         denominator_mem[k] = 3 + 2*k;
      end
   end
   always @(posedge clock) begin
      if(numerator_ce0) numerator_q0 <= numerator_mem[numerator_address0];
      if(numerator_ce1) numerator_q1 <= numerator_mem[numerator_address1];
      if(denominator_ce0) denominator_q0 <= denominator_mem[denominator_address0];
      if(denominator_ce1) denominator_q1 <= denominator_mem[denominator_address1];
   end

   p_Z9div_stageRA8_jS0_R10ac_channelIiE dut (
      .clock(clock), .reset(reset), .start_port(start_port),
      .denominator_q0(denominator_q0), .denominator_q1(denominator_q1),
      .numerator_q0(numerator_q0), .numerator_q1(numerator_q1),
      .out_full_n(out_full_n), .done_port(done_port), .idle_port(idle_port),
      .denominator_address0(denominator_address0), .denominator_address1(denominator_address1),
      .denominator_ce0(denominator_ce0), .denominator_ce1(denominator_ce1),
      .numerator_address0(numerator_address0), .numerator_address1(numerator_address1),
      .numerator_ce0(numerator_ce0), .numerator_ce1(numerator_ce1),
      .out_din(out_din), .out_write(out_write));

`ifdef MUT
   initial #1 numerator_mem[3] = numerator_mem[3] + 1000;
`endif

   always #5 clock = ~clock;
   integer cycle = 0, count = 0, errors = 0, low_ready_cycles = 0;
   integer first_output_cycle = -1, start_cycle = -1, completion_cycle = -1;
   always @(negedge clock) begin
      if(!reset || BP_MODE == 0) out_full_n = 1;
      else if(BP_MODE == 1) out_full_n = (cycle % 7) < 3;
      else out_full_n = !((first_output_cycle >= 0) && (cycle < first_output_cycle + 100));
   end
   always @(posedge clock) begin
      cycle = cycle + 1;
      if(reset && start_cycle >= 0 && !out_full_n)
         low_ready_cycles = low_ready_cycles + 1;
      if(reset && start_cycle >= 0 && out_write && out_full_n) begin
         if(count >= 8) begin
            errors = errors + 1;
            $display("MISMATCH unexpected output index=%0d", count);
         end else if(out_din !== ((1000 + 137*count) / (3 + 2*count))) begin
            errors = errors + 1;
            $display("MISMATCH index=%0d got=%0d expected=%0d", count, out_din,
                     (1000 + 137*count) / (3 + 2*count));
         end
         count = count + 1;
         if(count == 1) first_output_cycle = cycle;
      end
      if(reset && start_cycle >= 0 && done_port && completion_cycle < 0)
         completion_cycle = cycle;
   end

   initial begin
      repeat(4) @(posedge clock); reset <= 1;
      repeat(3) @(posedge clock);
      while(!idle_port) @(posedge clock);
      start_port <= 1; @(posedge clock); start_port <= 0; start_cycle = cycle;
      while(completion_cycle < 0 && cycle - start_cycle < MAX_CYCLE) @(posedge clock);
      if(completion_cycle < 0 || count != 8) errors = errors + 1;
      if(errors) $display("FAIL [BP=%0d] errors=%0d count=%0d", BP_MODE, errors, count);
      else if(BP_MODE && !low_ready_cycles) $display("INCONCLUSIVE [BP=%0d] no stall", BP_MODE);
      else $display("PASS [BP=%0d] count=%0d latency=%0d stalls=%0d", BP_MODE, count,
                    completion_cycle - start_cycle, low_ready_cycles);
      $finish;
   end
   initial begin #(MAX_CYCLE * 10); $display("FAIL [BP=%0d] watchdog", BP_MODE); $finish; end
endmodule
