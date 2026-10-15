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
#include <cmath>

__attribute__((noinline)) static float evaluate(float value)
{
   return std::exp(value) + std::sqrt(value + 1.0f);
}

#pragma HLS interface port = data mode = bus
__attribute__((noinline)) void fn(float* data)
{
#pragma omp parallel default(none) shared(data) num_threads(2)
   {
#pragma omp single nowait
      {
         for(unsigned i = 0; i < size; ++i)
         {
#pragma omp task firstprivate(i) depend(inout : data[i])
            { data[i] = evaluate(data[i]); }
#pragma omp task firstprivate(i) depend(inout : data[i])
            { data[i] = evaluate(data[i] + 1.0f); }
         }
      }
   }
}
