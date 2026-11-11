// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0

#pragma HLS interface port = a mode = bus
#pragma HLS interface port = b mode = bus
#pragma HLS interface port = c mode = bus
#if BANKED
#pragma HLS bus bank_number = 4 chunk_size = 8 mode = minimal
#elif USE_CACHE
#pragma HLS bus mode = minimal
#else
#pragma HLS bus mode = m_axi
#endif
#if USE_CACHE
#pragma HLS cache bundle = bus line_count = 16 line_size = 2 bus_size = 32 ways = 1 num_write_outstanding = 2 rep_policy = lru write_policy = wt
#endif

void bus_cache_kernel(const int a[20], const int b[20], int c[20])
{
   for(unsigned i = 0; i < 20; ++i)
      c[i] += 3 * a[i] - b[i];
}
