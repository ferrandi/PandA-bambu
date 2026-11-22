// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0
#ifndef THREADS
#define THREADS 4
#endif

extern "C" void omp_predication(const int* a, int* result, int enabled)
{
#pragma omp parallel for num_threads(THREADS)
   for(int i = 0; i < 32; ++i)
   {
      result[i] = a[i] + enabled;
   }

   int scale;
   if(enabled)
   {
      scale = a[0] + 3;
   }
   else
   {
      scale = a[1] + 5;
   }

#pragma omp parallel for num_threads(THREADS)
   for(int i = 0; i < 32; ++i)
   {
      result[i] = result[i] * scale + a[31 - i];
   }
   // Verify that both fork calls completed before consuming their results.
   int sum = 0;
   for(int i = 0; i < 32; ++i)
   {
      sum += result[i];
   }
   result[32] = sum;
}
