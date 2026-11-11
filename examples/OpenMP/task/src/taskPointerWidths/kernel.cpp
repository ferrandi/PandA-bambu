/* SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 * Copyright (C) 2026 Politecnico di Milano
 */
#include "kernel.hpp"

// These are user functions, not task graph entries or task-private entry points.
__attribute__((noinline)) static std::uint64_t build_task_graph(std::uint64_t value)
{
   volatile std::uint64_t local[2] = {value, value ^ UINT64_C(0x123456789abcdef0)};
   return local[1];
}
__attribute__((noinline)) static std::uint64_t user_omp_task_entry_helper(std::uint64_t value)
{
   return value + UINT64_C(0x100000001);
}

#pragma HLS interface port = input mode = bus
#pragma HLS interface port = output mode = bus
void fn(std::uint64_t* input, std::uint64_t* output)
{
#pragma omp parallel default(none) shared(input, output) num_threads(1)
   {
#pragma omp single nowait
      {
         for(unsigned i = 0; i < size; ++i)
         {
#pragma omp task firstprivate(i) depend(in : input[i]) depend(out : output[i])
            output[i] = build_task_graph(input[i]);
#pragma omp task firstprivate(i) depend(inout : output[i])
            output[i] = user_omp_task_entry_helper(output[i]);
         }
      }
   }
}
