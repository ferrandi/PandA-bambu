/* SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 * Copyright (C) 2026 Politecnico di Milano
 */
#include "kernel.hpp"
__attribute__((noinline)) void task_worker(int* data)
{
#pragma omp parallel default(none) shared(data) num_threads(1)
   {
#pragma omp single nowait
      {
         for(unsigned i = 0; i < size; ++i)
         {
#pragma omp task firstprivate(i) depend(inout : data[i])
            data[i] += i;
#pragma omp task firstprivate(i) depend(inout : data[i])
            data[i] *= 3;
         }
      }
   }
}
