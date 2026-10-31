// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
`timescale 1 ns / 1 ps

module tb_lp_mul_array;
   parameter DEPTH = 16;
   parameter BP_MODE = 0;
   parameter MAX_CYCLE = 4000;
   reg clock = 0, reset = 0, start_port = 0, out_full_n = 1;
   reg [31:0] a_q0 = 0, a_q1 = 0, b_q0 = 0, b_q1 = 0;
   wire done_port, idle_port, a_ce0, a_ce1, b_ce0, b_ce1, out_write;
   wire [3:0] a_address0, a_address1, b_address0, b_address1;
   wire [31:0] out_din;
   reg [31:0] memA [0:DEPTH-1], memB [0:DEPTH-1];
   integer k;
   initial for(k = 0; k < DEPTH; k = k + 1)
   begin
      memA[k] = 5 + k;
      memB[k] = 3 + 2 * k;
   end
   always @(posedge clock) if(a_ce0) a_q0 <= memA[a_address0];
   always @(posedge clock) if(a_ce1) a_q1 <= memA[a_address1];
   always @(posedge clock) if(b_ce0) b_q0 <= memB[b_address0];
   always @(posedge clock) if(b_ce1) b_q1 <= memB[b_address1];

   p_Z9mul_stageRA16_iS0_R10ac_channelIiE dut (
      .clock(clock), .reset(reset), .start_port(start_port),
      .a_q0(a_q0), .a_q1(a_q1), .b_q0(b_q0), .b_q1(b_q1),
      .out_full_n(out_full_n), .done_port(done_port), .idle_port(idle_port),
      .a_address0(a_address0), .a_address1(a_address1), .a_ce0(a_ce0), .a_ce1(a_ce1),
      .b_address0(b_address0), .b_address1(b_address1), .b_ce0(b_ce0), .b_ce1(b_ce1),
      .out_din(out_din), .out_write(out_write));

`ifdef MUT
   initial #1 memA[3] = memA[3] + 5;
`endif
   always #5 clock = ~clock;
   integer cycle = 0;
   always @(posedge clock)
   begin
      cycle = cycle + 1;
      if(!reset || BP_MODE == 0) out_full_n <= 1;
      else if(BP_MODE == 1) out_full_n <= (cycle % 5) < 2;
      else out_full_n <= !((cycle >= 14) && (cycle < 42));
   end
   integer count = 0, errors = 0, stalls = 0, started = 0, start_cycle = 0;
   always @(posedge clock)
   begin
      if(reset && started && !done_port && !out_full_n && !out_write && count < DEPTH)
         stalls = stalls + 1;
      if(reset && started && out_write && out_full_n)
      begin
         if(out_din !== (5 + count) * (3 + 2 * count))
         begin
            errors = errors + 1;
            $display("MISMATCH pos=%0d got=%0d expected=%0d", count, out_din, (5 + count) * (3 + 2 * count));
         end
         count = count + 1;
      end
   end
   initial
   begin
      repeat(4) @(posedge clock); reset <= 1;
      repeat(3) @(posedge clock);
      while(!idle_port) @(posedge clock);
      start_port <= 1; @(posedge clock); start_port <= 0;
      started = 1; start_cycle = cycle;
      while(!done_port && cycle - start_cycle < MAX_CYCLE) @(posedge clock);
      if(!done_port || count != DEPTH) errors = errors + 1;
      if(errors) $display("FAIL [BP=%0d] %0d errors", BP_MODE, errors);
      else if(BP_MODE && !stalls) $display("INCONCLUSIVE [BP=%0d] no pending-write stall", BP_MODE);
      else $display("PASS [BP=%0d] %0d products (%0d stalled cycles)", BP_MODE, count, stalls);
      $finish;
   end
   initial begin #(MAX_CYCLE * 20); $display("FAIL [BP=%0d] global watchdog", BP_MODE); $finish; end
endmodule
