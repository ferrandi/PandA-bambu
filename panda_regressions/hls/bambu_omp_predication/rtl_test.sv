// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: MIT
module rtl_test;
  reg clock = 0;
  always #5 clock = !clock;
  reg reset = 0;
  reg start_port = 0, predicate = 0, ack = 0, rd = 0, selector_register_file = 0;
  wire lock_done, release_done, barrier_done, wait_done, cs_done;
  wire acquire, release_lock, barrier_we, wait_ld, cs_ld;
  wire [31:0] barrier_tid;
  KMP_BAMBU_CRITICAL_FU lock_fu(.clock(clock), .reset(reset), .start_port(start_port),
    .predicate(predicate), .loc(1'b0), .done_port(lock_done), .kmp_lock_acquired(acquire),
    .kmp_lock_acquired_ack(ack));
  KMP_BAMBU_END_CRITICAL_FU release_fu(.clock(clock), .reset(reset), .start_port(start_port),
    .predicate(predicate), .loc(1'b0), .done_port(release_done), .kmp_lock_release(release_lock));
  KMP_BAMBU_BARRIER_REACHED_FU barrier_fu(.start_port(start_port), .predicate(predicate),
    .in1(1'b1), .done_port(barrier_done), .kmp_bambu_barrier_reached_tid(barrier_tid),
    .kmp_bambu_barrier_reached_we(barrier_we));
  KMP_BAMBU_WAIT_ALL_THREADS_FU wait_fu(.clock(clock), .reset(reset), .start_port(start_port),
    .predicate(predicate), .done_port(wait_done), .kmp_bambu_wait_all_threads_ld(wait_ld),
    .kmp_bambu_wait_all_threads_rd(rd));
  KMP_BAMBU_CS_WAIT_ALL_THREADS_FU cs_fu(.clock(clock), .reset(reset), .start_port(start_port),
    .predicate(predicate), .done_port(cs_done), .kmp_bambu_wait_all_threads_ld(cs_ld),
    .kmp_bambu_wait_all_threads_rd(rd), .selector_register_file(selector_register_file));
  task check(input bit condition, input string message);
    if (!condition) $fatal(1, "%s", message);
  endtask
  task tick;
    @(posedge clock); #1; @(negedge clock); #1;
  endtask
  task effects(input bit expected);
    check({acquire, release_lock, barrier_we, wait_ld, cs_ld} == {5{expected}}, "side effects");
  endtask
  initial begin
    // ACTIVE_LOW is supplied by the extraction runner.
    reset = `ACTIVE_LOW ? 0 : 1;
    tick(); reset = `ACTIVE_LOW ? 1 : 0; #1;
    check(!lock_done && !wait_done && !cs_done, "idle completion");
    // Skipped starts complete even if no controller acknowledges them.
    repeat (3) begin
      start_port = 1; predicate = 0; #1;
      effects(0);
      check(lock_done && release_done && barrier_done && wait_done && cs_done, "skip completion");
      tick(); start_port = 0; predicate = 1; #1;
      effects(0); check(!wait_done && !cs_done && !lock_done, "skipped operation left pending state");
      tick();
    end
    // Accept a true operation and change the predicate while waiting.
    start_port = 1; predicate = 1; #1; effects(1);
    check(!lock_done && !wait_done && !cs_done && release_done && barrier_done, "delayed completion");
    check(barrier_tid == 1, "barrier operand");
    tick(); start_port = 0; predicate = 0; #1; effects(0);
    check(!wait_done && !cs_done, "predicate changed accepted operation");
    tick(); ack = 1; rd = 1; #1;
    check(lock_done && wait_done && cs_done, "delayed acknowledgment");
    tick(); ack = 0; rd = 0; tick();
    check(!wait_done && !cs_done, "completion did not clear");
    // Immediate acknowledgments and back-to-back true/false operations.
    start_port = 1; predicate = 1; ack = 1; rd = 1; #1;
    effects(1); check(lock_done && wait_done && cs_done, "immediate acknowledgment");
    tick(); predicate = 0; ack = 0; rd = 0; #1;
    effects(0); check(lock_done && wait_done && cs_done, "back-to-back skip");
    tick(); start_port = 0; tick();
    // Context zero waits, context one skips; the skip must not clear zero's wait.
    selector_register_file = 0; start_port = 1; predicate = 1; #1; tick();
    selector_register_file = 1; predicate = 0; #1;
    check(cs_done && !cs_ld, "context one skip"); tick();
    start_port = 0; selector_register_file = 0; #1;
    check(!cs_done, "context zero completed early");
    rd = 1; #1; check(cs_done, "context zero lost pending operation"); tick();
    rd = 0; tick(); check(!cs_done, "context zero completion stale");
    selector_register_file = 1; #1; check(!cs_done, "skipped context left pending state");
    // Two real waits in different contexts must survive independently.
    selector_register_file = 0; start_port = 1; predicate = 1; tick();
    selector_register_file = 1; tick(); start_port = 0; predicate = 0; #1;
    rd = 1; #1; check(cs_done, "second context acknowledgment"); tick();
    rd = 0; selector_register_file = 0; #1;
    check(!cs_done, "acknowledgment leaked between contexts");
    rd = 1; #1; check(cs_done, "first context lost wait"); tick();
    rd = 0; tick();
    // Reset must discard pending waits, including contexts not currently selected.
    start_port = 1; predicate = 1; tick();
    selector_register_file = 1; tick(); start_port = 0;
    reset = `ACTIVE_LOW ? 0 : 1; tick(); reset = `ACTIVE_LOW ? 1 : 0; #1;
    check(!wait_done && !cs_done, "reset selected context");
    selector_register_file = 0; #1; check(!cs_done, "reset unselected context");
    $display("PASS OpenMP primitive predication");
    $finish;
  end
  initial begin #5000; $fatal(1, "timeout"); end
endmodule
