// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0
#ifndef THREADS
#define THREADS 4
#endif

extern "C" void omp_predication(const int* a, int* result, int enabled)
{
   int sum = enabled;
   int sum1 = 2;
#pragma omp parallel num_threads(THREADS)
   {
      if(enabled)
      {
#pragma omp for reduction(+ : sum, sum1)
         for(int i = 0; i < 32; ++i)
         {
            sum += a[i];
            sum1 += 2 * a[i];
         }
      }
   }
   result[0] = sum;
   result[1] = sum1;
}
