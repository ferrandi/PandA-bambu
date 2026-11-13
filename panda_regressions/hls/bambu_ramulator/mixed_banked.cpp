// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0

#define N 20

#pragma HLS interface port = a mode = bus
#pragma HLS interface port = b mode = m_axi offset = direct bundle = dedicated
#pragma HLS interface port = c mode = bus
#if USE_AXI
#pragma HLS bus bank_number = 4 chunk_size = 8 mode = m_axi
#else
#pragma HLS bus bank_number = 4 chunk_size = 8 mode = minimal
#endif

void ramulator_kernel(const int a[N], const int b[N], int c[N])
{
   // Dedicated interfaces are used only by the sequential part of the kernel.
   int bias = 0;
   for(unsigned int j = 0; j < N; ++j)
   {
      bias += b[j];
   }

   unsigned int i;
#pragma omp parallel num_threads(4)
   {
#pragma omp for private(i)
      for(i = 0; i < N; ++i)
      {
         c[i] = a[(7 * i + 3) % N] + bias;
      }
   }
}
