// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0
#include "Vrtl_test.h"
#include "verilated.h"

#include <iostream>
#include <stdexcept>

static void check(bool condition, const char* message)
{
   if(!condition)
   {
      throw std::runtime_error(message);
   }
}

int main(int argc, char** argv)
{
   Verilated::commandArgs(argc, argv);
   Vrtl_test top;
   const auto settle = [&]() { top.eval(); };
   const auto tick = [&]() {
      top.eval();
      top.clock = 1;
      top.eval();
      top.clock = 0;
      top.eval();
   };
   const auto effects = [&](bool expected) {
      check(top.acquire == expected && top.release_lock == expected && top.barrier_we == expected &&
                top.wait_ld == expected && top.cs_ld == expected && top.reduce_we == expected &&
                top.reduce_ld == expected,
            "side effects");
   };
   try
   {
      top.clock = 0;
      top.start_port = top.predicate = top.ack = top.rd = top.selector_register_file = 0;
      top.reset = ACTIVE_LOW ? 0 : 1;
      settle();
      tick();
      top.reset = ACTIVE_LOW ? 1 : 0;
      settle();
      check(!top.lock_done && !top.wait_done && !top.cs_done, "idle completion");
      // Skipped starts complete even if no controller acknowledges them.
      for(unsigned int i = 0; i < 3; ++i)
      {
         top.start_port = 1;
         top.predicate = 0;
         settle();
         effects(false);
         check(top.lock_done && top.release_done && top.barrier_done && top.wait_done && top.cs_done &&
                   top.reduce_set_done && top.reduce_get_done,
               "skip completion");
         tick();
         top.start_port = 0;
         top.predicate = 1;
         settle();
         effects(false);
         check(!top.wait_done && !top.cs_done && !top.lock_done, "skipped operation left pending state");
         tick();
      }
      // Accept a true operation and change the predicate while waiting.
      top.start_port = top.predicate = 1;
      settle();
      effects(true);
      check(!top.lock_done && !top.wait_done && !top.cs_done && top.release_done && top.barrier_done,
            "delayed completion");
      check(top.barrier_tid == 1, "barrier operand");
      check(top.reduce_set_tid == 3 && top.reduce_get_tid == 3 && top.reduce_set_val == 0x12345678 &&
                top.reduce_get_val == 0x87654321,
            "reduction operands");
      check(!top.reduce_set_done && !top.reduce_get_done, "reduction completed before acknowledgment");
      tick();
      top.start_port = top.predicate = 0;
      settle();
      effects(false);
      check(!top.wait_done && !top.cs_done, "predicate changed accepted operation");
      tick();
      top.ack = top.rd = 1;
      settle();
      check(top.lock_done && top.wait_done && top.cs_done && top.reduce_set_done && top.reduce_get_done,
            "delayed acknowledgment");
      tick();
      top.ack = top.rd = 0;
      tick();
      check(!top.wait_done && !top.cs_done, "completion did not clear");
      // Immediate acknowledgments and back-to-back true/false operations.
      top.start_port = top.predicate = top.ack = top.rd = 1;
      settle();
      effects(true);
      check(top.lock_done && top.wait_done && top.cs_done && top.reduce_set_done && top.reduce_get_done,
            "immediate acknowledgment");
      tick();
      top.predicate = top.ack = top.rd = 0;
      settle();
      effects(false);
      check(top.lock_done && top.wait_done && top.cs_done, "back-to-back skip");
      tick();
      top.start_port = 0;
      tick();
      // Context zero waits, context one skips; the skip must not clear zero's wait.
      top.selector_register_file = 0;
      top.start_port = top.predicate = 1;
      settle();
      tick();
      top.selector_register_file = 1;
      top.predicate = 0;
      settle();
      check(top.cs_done && !top.cs_ld, "context one skip");
      tick();
      top.start_port = top.selector_register_file = 0;
      settle();
      check(!top.cs_done, "context zero completed early");
      top.rd = 1;
      settle();
      check(top.cs_done, "context zero lost pending operation");
      tick();
      top.rd = 0;
      tick();
      check(!top.cs_done, "context zero completion stale");
      top.selector_register_file = 1;
      settle();
      check(!top.cs_done, "skipped context left pending state");
      // Two real waits in different contexts must survive independently.
      top.selector_register_file = 0;
      top.start_port = top.predicate = 1;
      tick();
      top.selector_register_file = 1;
      tick();
      top.start_port = top.predicate = 0;
      settle();
      top.rd = 1;
      settle();
      check(top.cs_done, "second context acknowledgment");
      tick();
      top.rd = top.selector_register_file = 0;
      settle();
      check(!top.cs_done, "acknowledgment leaked between contexts");
      top.rd = 1;
      settle();
      check(top.cs_done, "first context lost wait");
      tick();
      top.rd = 0;
      tick();
      // Reset must discard pending waits, including unselected contexts.
      top.start_port = top.predicate = 1;
      tick();
      top.selector_register_file = 1;
      tick();
      top.start_port = 0;
      top.reset = ACTIVE_LOW ? 0 : 1;
      settle();
      tick();
      top.reset = ACTIVE_LOW ? 1 : 0;
      settle();
      check(!top.wait_done && !top.cs_done, "reset selected context");
      top.selector_register_file = 0;
      settle();
      check(!top.cs_done, "reset unselected context");
      top.final();
      std::cout << "PASS OpenMP primitive predication\n";
   }
   catch(const std::exception& error)
   {
      std::cerr << "FAIL OpenMP primitive predication: " << error.what() << '\n';
      return 1;
   }
   return 0;
}
