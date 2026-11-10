// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0

#define N 20

#if DEDICATED_AXI == 1
#pragma HLS interface port = a mode = m_axi offset = direct
#pragma HLS interface port = b mode = m_axi offset = direct
#pragma HLS interface port = c mode = m_axi offset = direct
#elif DEDICATED_AXI == 2
#pragma HLS interface port = a mode = bus
#pragma HLS interface port = b mode = m_axi offset = direct bundle = dedicated
#pragma HLS interface port = c mode = m_axi offset = direct bundle = dedicated
#pragma HLS bus mode = m_axi
#else
#pragma HLS interface port = a mode = bus
#pragma HLS interface port = b mode = bus
#pragma HLS interface port = c mode = bus
#if BANKED && USE_AXI
#pragma HLS bus bank_number = 4 chunk_size = 8 mode = m_axi
#elif BANKED
#pragma HLS bus bank_number = 4 chunk_size = 8 mode = minimal
#elif USE_AXI
#pragma HLS bus mode = m_axi
#else
#pragma HLS bus mode = minimal
#endif
#endif

void ramulator_kernel(const int a[N], const int b[N], int c[N])
{
   unsigned int i;
#if BANKED
#pragma omp parallel num_threads(4)
#endif
   {
#if BANKED
#pragma omp for private(i)
#endif
      for(i = 0; i < N; ++i)
      {
#if TEST_KERNEL == 0
         c[i] = a[i] + b[i];
#elif TEST_KERNEL == 1
         c[i] += 3 * a[i] - b[i];
#else
         if(a[i] & 1)
         {
            c[i] = a[i] + b[i];
         }
         else
         {
            c[i] = a[i] - b[i];
         }
#endif
      }
   }
}
