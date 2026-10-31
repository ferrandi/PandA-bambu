// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
`timescale 1 ns / 1 ps

module tb_lp_internal_rom;

   parameter BP_MODE = 0;
   parameter MAX_CYCLE = 1000;

   reg clock = 1'b0;
   reg reset = 1'b0;
   reg start_port = 1'b0;
   reg [31:0] start = 32'd3;
   reg out_full_n = 1'b1;
   wire done_port, idle_port, out_write;
   wire [31:0] out_din;
   integer expected[0:15];
   integer accepted = 0, cycles = 0, low_ready_cycles = 0, errors = 0;
   integer bp2_cycles = 0;
   integer started = 0, start_cycle = 0, done_cycle = 0;
   integer wait_cycles = 0;
   reg bp2_stalled = 1'b0;
   integer signed observed;

   p_Z12internal_romjR10ac_channelIiE dut (
      .clock(clock), .reset(reset), .start_port(start_port), .start(start),
      .out_full_n(out_full_n), .done_port(done_port), .idle_port(idle_port),
      .out_din(out_din), .out_write(out_write));

   always #5 clock = ~clock;

   initial
   begin
      expected[0] = 5; expected[1] = -9; expected[2] = 73; expected[3] = 12;
      expected[4] = -31; expected[5] = 58; expected[6] = 4; expected[7] = -6;
      expected[8] = 29; expected[9] = 88; expected[10] = -14; expected[11] = 3;
      expected[12] = 67; expected[13] = 17; expected[14] = -23; expected[15] = 41;

      repeat (4) @(negedge clock);
      reset = 1'b1;
      wait_cycles = 0;
      while (!idle_port && wait_cycles < 50)
      begin
         @(negedge clock);
         wait_cycles = wait_cycles + 1;
      end
      if (!idle_port)
      begin
         errors = errors + 1;
         $display("  ERROR: DUT did not become idle");
      end
      start_port = 1'b1;
      @(negedge clock);
      start_port = 1'b0;
      started = 1;
      start_cycle = cycles;
   end

   // Set ready before the active edge where the DUT samples the transfer.
   always @(negedge clock)
      if (reset && started)
      begin
         if (BP_MODE == 0)
            out_full_n = 1'b1;
         else if (BP_MODE == 1)
            out_full_n = ((cycles % 5) < 2);
         else if (BP_MODE == 2 && accepted >= 1 && !bp2_stalled)
         begin
            out_full_n = 1'b0;
            bp2_stalled = 1'b1;
            bp2_cycles = 200;
         end
         else if (bp2_cycles > 0)
         begin
            bp2_cycles = bp2_cycles - 1;
            out_full_n = (bp2_cycles == 0);
         end
      end

   always @(posedge clock)
      if (reset && started)
      begin
         cycles = cycles + 1;
         if (cycles - start_cycle > MAX_CYCLE)
         begin
            errors = errors + 1;
            $display("  ERROR: TIMEOUT, accepted %0d outputs", accepted);
            $display("FAIL [BP=%0d] %0d errors", BP_MODE, errors);
            $finish;
         end

         if (!out_full_n && !done_port && accepted < 16)
            low_ready_cycles = low_ready_cycles + 1;

         if (out_write && out_full_n)
         begin
            if (accepted >= 16)
            begin
               errors = errors + 1;
               $display("  ERROR: extra output");
            end
            else
            begin
               observed = $signed(out_din);
`ifdef MUT
               if (accepted == 3) observed = observed + 1;
`endif
               if (observed !== expected[accepted])
               begin
                  errors = errors + 1;
                  $display("  MISMATCH item=%0d got=%0d expected=%0d",
                           accepted, observed, expected[accepted]);
               end
               accepted = accepted + 1;
            end
         end

         if (done_port)
         begin
            done_cycle = cycles - start_cycle;
            if (accepted != 16)
            begin
               errors = errors + 1;
               $display("  ERROR: done with %0d outputs, 16 expected", accepted);
            end
            $display("  outputs checked = %0d, cycles = %0d, low-ready cycles = %0d",
                     accepted, done_cycle, low_ready_cycles);
            if (errors > 0)
               $display("FAIL [BP=%0d] %0d errors", BP_MODE, errors);
            else if (BP_MODE != 0 && low_ready_cycles == 0)
               $display("INCONCLUSIVE [BP=%0d] no low-ready cycles", BP_MODE);
            else
               $display("PASS [BP=%0d] %0d correct outputs in %0d cycles (%0d stalled)",
                        BP_MODE, accepted, done_cycle, low_ready_cycles);
            $finish;
         end
      end

   initial
   begin
      #(MAX_CYCLE * 20);
      $display("FAIL [BP=%0d] global watchdog", BP_MODE);
      $finish;
   end

endmodule
