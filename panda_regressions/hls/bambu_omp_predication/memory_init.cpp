// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0
#ifndef THREADS
#define THREADS 4
#endif

extern "C" void omp_predication(const int* a, int* result, int enabled)
{
#pragma omp parallel num_threads(THREADS)
   {
      static const volatile int constants[3] = {0x11223344, 0x22334455, 0x33445566};
      volatile int values[3] = {17, 35, 93};
#pragma omp for
      for(int i = 0; i < 32; ++i)
      {
         const int index = (a[i] + enabled) % 3;
         const auto* bytes = reinterpret_cast<const volatile unsigned char*>(constants);
         result[i] = values[index] + constants[index] + bytes[(a[i] + enabled) % 12];
      }
   }
}
