// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
`timescale 1 ns / 1 ps

module tb_lp_shared_fu_wait;
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

   p_Z17lp_shared_fu_waitRA16_iS0_S0_S0_R10ac_channelIiE dut(
      .clock(clock), .reset(reset), .start_port(start_port),
      .a_q0(a_q0), .a_q1(a_q1), .b_q0(b_q0), .b_q1(b_q1),
      .c_q0(c_q0), .c_q1(c_q1), .d_q0(d_q0), .d_q1(d_q1),
      .out_full_n(out_full_n), .done_port(done_port), .idle_port(idle_port), .return_port(return_port),
      .a_address0(a_address0), .a_address1(a_address1), .a_ce0(a_ce0), .a_ce1(a_ce1),
      .b_address0(b_address0), .b_address1(b_address1), .b_ce0(b_ce0), .b_ce1(b_ce1),
      .c_address0(c_address0), .c_address1(c_address1), .c_ce0(c_ce0), .c_ce1(c_ce1),
      .d_address0(d_address0), .d_address1(d_address1), .d_ce0(d_ce0), .d_ce1(d_ce1),
      .out_din(out_din), .out_write(out_write));

   always #5 clock = ~clock;

   integer cycle = 0, count = 0, errors = 0, stalls = 0;
   always @(posedge clock)
   begin
      cycle = cycle + 1;
      if(!reset || BP_MODE == 0) out_full_n <= 1;
      else if(BP_MODE == 1) out_full_n <= (cycle % 5) < 2;
      else out_full_n <= !((cycle >= 50) && (cycle < 75));

      if(reset && !out_full_n) stalls = stalls + 1;
      if(reset && out_write && out_full_n)
      begin
         if(out_din !== count)
         begin
            errors = errors + 1;
            $display("OUTPUT mismatch pos=%0d got=%0d expected=%0d", count, out_din, count);
         end
         count = count + 1;
      end
   end

   initial
   begin
      $dumpfile("shared_fu_wait.vcd");
      $dumpvars(0, dut);
      repeat(4) @(posedge clock);
      reset <= 1;
      repeat(3) @(posedge clock);
      while(!idle_port) @(posedge clock);
      start_port <= 1;
      @(posedge clock);
      start_port <= 0;
      while(!done_port && cycle < MAX_CYCLE) @(posedge clock);
      if(!done_port || count != 16 || return_port !== expected_sum(0)) errors = errors + 1;
      if(BP_MODE != 0 && stalls == 0) errors = errors + 1;
      $display("SHARED_FU_WAIT bp=%0d outputs=%0d stalls=%0d return=%0d", BP_MODE, count, stalls, return_port);
      if(errors) $display("FAIL [BP=%0d] errors=%0d", BP_MODE, errors);
      else $display("PASS [BP=%0d] outputs=%0d return=%0d", BP_MODE, count, return_port);
      $finish;
   end

   function [31:0] expected_sum;
      input integer unused;
      integer i;
      begin
         expected_sum = 0;
         for(i = 0; i < 16; i = i + 1)
            expected_sum = expected_sum + (i + 3) * (2 * i + 5) + (4 * i + 7) * (3 * i + 11);
      end
   endfunction

   initial
   begin
      #(MAX_CYCLE * 20);
      $display("FAIL [BP=%0d] watchdog", BP_MODE);
      $finish;
   end
endmodule
