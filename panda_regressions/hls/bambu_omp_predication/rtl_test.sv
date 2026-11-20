// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: MIT
module rtl_test(
  input wire clock, reset, start_port, predicate, ack, rd, selector_register_file,
  output wire lock_done, release_done, barrier_done, wait_done, cs_done,
  output wire acquire, release_lock, barrier_we, wait_ld, cs_ld,
  output wire [31:0] barrier_tid
);
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
endmodule
