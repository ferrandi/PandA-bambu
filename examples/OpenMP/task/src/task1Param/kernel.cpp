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

#pragma HLS interface port = data1 mode = bus
__attribute__((noinline)) void fn(int* data1)
{
#pragma omp parallel default(none) shared(data1) num_threads(1)
   {
#pragma omp single nowait
      {
         for(unsigned i = 0; i < size; i++)
         {
#pragma omp task firstprivate(i) depend(inout : data1[i])
            {
               data1[i] += i;
            } // end task
#pragma omp task firstprivate(i) depend(inout : data1[i])
            {
               data1[i] *= 2;
            } // end task
         }
      } // end single
   }    // end parallel
}
