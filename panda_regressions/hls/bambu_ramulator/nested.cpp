// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0
#pragma HLS interface port = a mode = bus
#pragma HLS interface port = b mode = bus
#pragma HLS interface port = c mode = bus
#if USE_AXI
#pragma HLS bus mode = m_axi
#else
#pragma HLS bus mode = minimal
#endif
void ramulator_kernel(const int a[20], const int b[20], int c[20])
{
#pragma omp parallel num_threads(4)
   {
#pragma omp for
      for(int i = 0; i < 4; ++i)
      {
#pragma omp parallel num_threads(2)
         {
#pragma omp for
            for(int j = 0; j < 5; ++j)
            {
               const int index = i * 5 + j;
               c[index] = a[index] + b[index];
            }
         }
#pragma omp parallel num_threads(2)
         {
#pragma omp for
            for(int j = 0; j < 5; ++j)
            {
               const int index = i * 5 + j;
               c[index] += a[index] * b[index];
            }
         }
      }
   }
}
