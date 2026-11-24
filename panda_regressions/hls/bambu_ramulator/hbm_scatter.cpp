// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0

#ifndef HBM_SIZE
#define HBM_SIZE 4096
#endif
#pragma HLS interface port = a mode = bus
#pragma HLS interface port = b mode = bus
#pragma HLS interface port = indices mode = bus
#pragma HLS interface port = c mode = bus
#if USE_AXI
#pragma HLS bus bank_number = 32 chunk_size = 8 mode = m_axi
#else
#pragma HLS bus bank_number = 32 chunk_size = 8 mode = minimal
#endif

void ramulator_kernel(const int a[HBM_SIZE], const int b[HBM_SIZE],
                      const unsigned indices[HBM_SIZE], int c[HBM_SIZE])
{
   unsigned i;
#pragma omp parallel num_threads(32)
   {
#pragma omp for private(i)
      for(i = 0; i < HBM_SIZE; ++i)
      {
         // indices is a permutation: each thread owns distinct output elements.
         const unsigned index = indices[i];
         c[index] = a[(29U * i + 7U) & (HBM_SIZE - 1U)] ^ b[index];
      }
   }
}
