/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
#include "kernel.hpp"

// Each outlined task must own an independent instance of this complete call graph.
__attribute__((noinline)) static int leaf(int value)
{
   volatile int scratch[4] = {value, value + 1, value + 2, value + 3};
   return scratch[static_cast<unsigned>(value) & 3U] + 7;
}

__attribute__((noinline)) static int nested(int value)
{
   return leaf(value) + leaf(value + 1);
}

#ifdef BANK_NUMBER
#if BANK_NUMBER == 8
#pragma HLS bus bank_number = 8 chunk_size = 4
#endif
#endif
#pragma HLS interface port = data mode = bus
__attribute__((noinline)) void fn(int* data)
{
#pragma omp parallel default(none) shared(data) num_threads(2)
   {
#pragma omp single nowait
      {
         for(unsigned i = 0; i < size; ++i)
         {
#pragma omp task firstprivate(i) depend(inout : data[i])
            { data[i] = nested(data[i]); }
#pragma omp task firstprivate(i) depend(inout : data[i])
            { data[i] += nested(data[i] + 2); }
         }
      }
   }
}
