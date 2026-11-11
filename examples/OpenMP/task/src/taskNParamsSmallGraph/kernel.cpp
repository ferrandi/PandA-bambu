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
#pragma HLS interface port = data2 mode = bus
#pragma HLS interface port = data3 mode = bus
#pragma HLS interface port = data4 mode = bus
__attribute__((noinline)) void fn(int* data1, int* data2, int* data3, int* data4)
{
#pragma omp parallel default(none) shared(data1, data2, data3, data4) num_threads(1)
   {
#pragma omp single nowait
      {
         for(unsigned i = 0; i < size; i++)
         {
#pragma omp task firstprivate(i) depend(inout : data1[i], data2[i], data3[i], data4[i])
            {
               data1[i] += 1;
               data2[i] += 2;
               data3[i] += 3;
               data4[i] += 4;
            } // end task
#pragma omp task firstprivate(i) depend(inout : data1[i], data2[i])
            {
               data1[i] += 2;
               data2[i] += 3;
            } // end task
#pragma omp task firstprivate(i) depend(inout : data3[i], data4[i])
            {
               data3[i] += 4;
               data4[i] += 5;
            } // end task
#pragma omp task firstprivate(i) depend(inout : data1[i], data2[i], data3[i], data4[i])
            {
               data1[i] *= 2;
               data2[i] *= 2;
               data3[i] *= 2;
               data4[i] *= 2;
            } // end task
         }    // end for
      }       // end single
   }          // end parallel
}
