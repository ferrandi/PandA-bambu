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
__attribute__((noinline)) void fn(int* data1, int* data2)
{
#pragma omp parallel default(none) shared(data1, data2) num_threads(1)
   {
#pragma omp single nowait
      {
         for(unsigned i = 0; i < size; i++)
         {
            const int offset = i * size;
#pragma omp task firstprivate(offset) depend(inout : data1[offset], data2[offset])
            {
               if(data1[offset] != null)
               {
                  for(unsigned j = 0; j < size; j++)
                  {
                     data1[offset + j] += data1_amount;
                  }
               }
               if(data2[offset] != null)
               {
                  for(unsigned j = 0; j < size; j++)
                  {
                     data2[offset + j] += data2_amount;
                  }
               }
            } // end task
#pragma omp task firstprivate(offset) depend(inout : data1[offset], data2[offset])
            {
               if(data1[offset] != null)
               {
                  for(unsigned j = 0; j < size; j++)
                  {
                     data1[offset + j] *= 2;
                  }
               }
               if(data2[offset] != null)
               {
                  for(unsigned j = 0; j < size; j++)
                  {
                     data2[offset + j] *= 2;
                  }
               }
            } // end task
         }    // end for
      }       // end single
   }          // end parallel
}
