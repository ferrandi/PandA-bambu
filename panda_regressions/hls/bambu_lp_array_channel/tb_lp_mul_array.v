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

`define DUT_DATAPATH dut.PBI_Z9mul_stageRA16_iS0_R10ac_channelIiE_i0.Datapath_i
`define PIPELINE_STALL dut.PBI_Z9mul_stageRA16_iS0_R10ac_channelIiE_i0.selector_stallable_fus

`ifdef LP_MUL_LAUNCH
`define MUL_LAUNCH `DUT_DATAPATH.`LP_MUL_LAUNCH
`define MUL_INPUT_A `DUT_DATAPATH.`LP_MUL_INSTANCE.`LP_MUL_PORT_A
`define MUL_INPUT_B `DUT_DATAPATH.`LP_MUL_INSTANCE.`LP_MUL_PORT_B
`endif

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
   integer count = 0, errors = 0, stalls = 0, started = 0, start_cycle = 0, pipeline_stall_cycles = 0;
   reg done_seen = 0;
`ifdef LP_ARRAY_INTERFACE
   integer a_read_requests = 0, b_read_requests = 0;
   integer array_ce_errors = 0, array_address_errors = 0, array_q_errors = 0;
   reg [DEPTH-1:0] a_read_seen = 0, b_read_seen = 0;
   reg array_stall_snapshot_valid = 0;
   reg [3:0] stall_a_address0, stall_a_address1, stall_b_address0, stall_b_address1;
   reg [31:0] stall_a_q0, stall_a_q1, stall_b_q0, stall_b_q1;
`endif
`ifdef LP_MUL_LAUNCH
   integer fu_cycle = 0, fu_launches = 0, fu_hold_checks = 0, fu_stability_errors = 0, fu_operand_changes = 0;
   integer fu_previous_launch = -1, fu_min_spacing = 999999, fu_spacing_errors = 0, fu_stalled_hold_checks = 0;
   reg fu_hold_valid = 0;
   reg [31:0] fu_sample_a, fu_sample_b;
`endif
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
      if(reset && started && `PIPELINE_STALL)
         pipeline_stall_cycles = pipeline_stall_cycles + 1;
`ifdef LP_ARRAY_INTERFACE
      if(reset && started)
      begin
         if(a_ce0 === 1'b1)
         begin
            if((^a_address0) === 1'bx || a_address0 >= DEPTH || a_read_seen[a_address0])
            begin
               array_address_errors = array_address_errors + 1;
               errors = errors + 1;
               $display("ARRAY_READ_ADDRESS_ERROR a0=%0d", a_address0);
            end
            else
            begin
               a_read_seen[a_address0] = 1'b1;
               a_read_requests = a_read_requests + 1;
            end
         end
         else if(a_ce0 !== 1'b0)
         begin
            array_ce_errors = array_ce_errors + 1;
            errors = errors + 1;
         end
         if(a_ce1 === 1'b1)
         begin
            if((^a_address1) === 1'bx || a_address1 >= DEPTH || a_read_seen[a_address1])
            begin
               array_address_errors = array_address_errors + 1;
               errors = errors + 1;
               $display("ARRAY_READ_ADDRESS_ERROR a1=%0d", a_address1);
            end
            else
            begin
               a_read_seen[a_address1] = 1'b1;
               a_read_requests = a_read_requests + 1;
            end
         end
         else if(a_ce1 !== 1'b0)
         begin
            array_ce_errors = array_ce_errors + 1;
            errors = errors + 1;
         end
         if(b_ce0 === 1'b1)
         begin
            if((^b_address0) === 1'bx || b_address0 >= DEPTH || b_read_seen[b_address0])
            begin
               array_address_errors = array_address_errors + 1;
               errors = errors + 1;
               $display("ARRAY_READ_ADDRESS_ERROR b0=%0d", b_address0);
            end
            else
            begin
               b_read_seen[b_address0] = 1'b1;
               b_read_requests = b_read_requests + 1;
            end
         end
         else if(b_ce0 !== 1'b0)
         begin
            array_ce_errors = array_ce_errors + 1;
            errors = errors + 1;
         end
         if(b_ce1 === 1'b1)
         begin
            if((^b_address1) === 1'bx || b_address1 >= DEPTH || b_read_seen[b_address1])
            begin
               array_address_errors = array_address_errors + 1;
               errors = errors + 1;
               $display("ARRAY_READ_ADDRESS_ERROR b1=%0d", b_address1);
            end
            else
            begin
               b_read_seen[b_address1] = 1'b1;
               b_read_requests = b_read_requests + 1;
            end
         end
         else if(b_ce1 !== 1'b0)
         begin
            array_ce_errors = array_ce_errors + 1;
            errors = errors + 1;
         end

         if(`PIPELINE_STALL)
         begin
            if(!array_stall_snapshot_valid)
            begin
               // The transition into the dummy wait can share the launch edge
               // with one final read. Account for that accepted request, then
               // require every later wait cycle to keep all array CEs low.
               stall_a_address0 = a_address0;
               stall_a_address1 = a_address1;
               stall_b_address0 = b_address0;
               stall_b_address1 = b_address1;
               stall_a_q0 = (a_ce0 === 1'b1) ? memA[a_address0] : a_q0;
               stall_a_q1 = (a_ce1 === 1'b1) ? memA[a_address1] : a_q1;
               stall_b_q0 = (b_ce0 === 1'b1) ? memB[b_address0] : b_q0;
               stall_b_q1 = (b_ce1 === 1'b1) ? memB[b_address1] : b_q1;
               array_stall_snapshot_valid = 1;
            end
            else
            begin
               if(a_ce0 !== 1'b0 || a_ce1 !== 1'b0 || b_ce0 !== 1'b0 || b_ce1 !== 1'b0)
               begin
                  array_ce_errors = array_ce_errors + 1;
                  errors = errors + 1;
                  $display("ARRAY_CE_DURING_STALL");
               end
               if(a_address0 !== stall_a_address0 || a_address1 !== stall_a_address1 ||
                  b_address0 !== stall_b_address0 || b_address1 !== stall_b_address1)
               begin
                  array_address_errors = array_address_errors + 1;
                  errors = errors + 1;
                  $display("ARRAY_ADDRESS_CHANGED_DURING_STALL");
               end
               if(a_q0 !== stall_a_q0 || a_q1 !== stall_a_q1 || b_q0 !== stall_b_q0 || b_q1 !== stall_b_q1)
               begin
                  array_q_errors = array_q_errors + 1;
                  errors = errors + 1;
                  $display("ARRAY_Q_CHANGED_WHILE_CE_LOW");
               end
            end
         end
         else
            array_stall_snapshot_valid = 0;
      end
`endif
   end

`ifdef LP_MUL_LAUNCH
   always @(posedge clock)
   begin
      fu_cycle = fu_cycle + 1;
      if(reset && `MUL_LAUNCH === 1'b1)
      begin
         if(fu_previous_launch >= 0)
         begin
            if(fu_cycle - fu_previous_launch < 2) fu_spacing_errors = fu_spacing_errors + 1;
            if(fu_cycle - fu_previous_launch < fu_min_spacing) fu_min_spacing = fu_cycle - fu_previous_launch;
         end
         fu_previous_launch = fu_cycle;
         fu_launches = fu_launches + 1;
         if(fu_hold_valid && ( `MUL_INPUT_A !== fu_sample_a || `MUL_INPUT_B !== fu_sample_b))
            fu_operand_changes = fu_operand_changes + 1;
         fu_sample_a = `MUL_INPUT_A;
         fu_sample_b = `MUL_INPUT_B;
         fu_hold_valid = 1;
      end
      else if(reset && fu_hold_valid)
      begin
         fu_hold_checks = fu_hold_checks + 1;
         if(`PIPELINE_STALL) fu_stalled_hold_checks = fu_stalled_hold_checks + 1;
         if(`MUL_INPUT_A !== fu_sample_a || `MUL_INPUT_B !== fu_sample_b)
         begin
            fu_stability_errors = fu_stability_errors + 1;
            $display("FU_INPUT_CHANGED launch=%0d", fu_launches);
         end
      end
   end

   always @(negedge clock)
   begin
      #1;
      if(reset && fu_hold_valid && `MUL_LAUNCH !== 1'b1 &&
         (`MUL_INPUT_A !== fu_sample_a || `MUL_INPUT_B !== fu_sample_b))
      begin
         fu_stability_errors = fu_stability_errors + 1;
         $display("FU_INPUT_CHANGED_BETWEEN_EDGES launch=%0d", fu_launches);
      end
   end

`ifdef HOLD_MUT
   initial
   begin
      wait(reset && `MUL_LAUNCH === 1'b1);
      @(posedge clock);
      @(negedge clock);
      #0.1 force `MUL_INPUT_A = 32'hdeadbeef;
      #1.2 release `MUL_INPUT_A;
   end
`endif
`endif
   initial
   begin
      repeat(4) @(posedge clock); reset <= 1;
      repeat(3) @(posedge clock);
      while(!idle_port) @(posedge clock);
      start_port <= 1; @(posedge clock); start_port <= 0;
      started = 1; start_cycle = cycle;
      while(!done_port && cycle - start_cycle < MAX_CYCLE) @(posedge clock);
      done_seen = (done_port === 1'b1);
      @(negedge clock);
      if(!done_seen || count != DEPTH) errors = errors + 1;
`ifdef LP_ARRAY_INTERFACE
      if(a_read_requests != DEPTH || b_read_requests != DEPTH ||
         a_read_seen !== {DEPTH{1'b1}} || b_read_seen !== {DEPTH{1'b1}})
      begin
         errors = errors + 1;
         $display("ARRAY_REQUEST_COUNT_ERROR a=%0d b=%0d", a_read_requests, b_read_requests);
      end
`endif
`ifdef LP_MUL_LAUNCH
      if(fu_launches != DEPTH || !fu_hold_checks || fu_stability_errors || fu_operand_changes == 0 ||
         fu_spacing_errors || fu_min_spacing < 2 || (BP_MODE != 0 && !fu_stalled_hold_checks)) errors = errors + 1;
`endif
      if(BP_MODE != 0 && !pipeline_stall_cycles) errors = errors + 1;
      if(errors) $display("FAIL [BP=%0d] %0d errors", BP_MODE, errors);
      else if(BP_MODE && !stalls) $display("INCONCLUSIVE [BP=%0d] no pending-write stall", BP_MODE);
      else
      begin
         $display("PASS [BP=%0d] %0d products (%0d stalled cycles)", BP_MODE, count, stalls);
      end
      $display("STALLABLE_STALL_ORACLE stall_cycles=%0d", pipeline_stall_cycles);
`ifdef LP_ARRAY_INTERFACE
      $display("ARRAY_INTERFACE_MONITOR a_reads=%0d b_reads=%0d ce_errors=%0d address_errors=%0d q_errors=%0d stall_cycles=%0d",
               a_read_requests, b_read_requests, array_ce_errors, array_address_errors, array_q_errors,
               pipeline_stall_cycles);
`endif
`ifdef LP_MUL_LAUNCH
      `ifdef HOLD_MUT
      if(fu_stability_errors) $display("STALLABLE_HOLD_MUT_DETECTED errors=%0d", fu_stability_errors);
      `endif
      $display("STALLABLE_II2_MONITOR launches=%0d hold_checks=%0d stable_errors=%0d operand_changes=%0d spacing_min=%0d spacing_errors=%0d stalled_hold_checks=%0d stall_cycles=%0d",
               fu_launches, fu_hold_checks, fu_stability_errors, fu_operand_changes, fu_min_spacing,
               fu_spacing_errors, fu_stalled_hold_checks, pipeline_stall_cycles);
`endif
      $finish;
   end
   initial begin #(MAX_CYCLE * 20); $display("FAIL [BP=%0d] global watchdog", BP_MODE); $finish; end
endmodule

`undef PIPELINE_STALL
`ifdef LP_MUL_LAUNCH
`undef MUL_LAUNCH
`undef MUL_INPUT_A
`undef MUL_INPUT_B
`endif
`undef DUT_DATAPATH
