// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Repeated selectors and non-contiguous physical banks, with shared partial chunks.
#include <cstdlib>

#ifdef INVALID_CHUNK
#pragma HLS bus bank_number = 8 chunk_size = 3
#else
#pragma HLS bus bank_number = 8 chunk_size = 8
#endif
#if defined(INVALID_DUPLICATE_BANK)
#pragma HLS interface port = a mode = bus bank_allocation = 0, 0
#pragma HLS interface port = b mode = bus bank_allocation = 2, 5
#pragma HLS interface port = c mode = bus bank_allocation = 1, 3, 4, 7
#elif defined(INVALID_BANK_ID)
#pragma HLS interface port = a mode = bus bank_allocation = 0, 8
#pragma HLS interface port = b mode = bus bank_allocation = 2, 5
#pragma HLS interface port = c mode = bus bank_allocation = 1, 3, 4, 7
#elif defined(INVALID_BANK_COUNT)
#pragma HLS interface port = a mode = bus bank_allocation = 0, 1, 2, 3, 4, 5, 6, 7, 0
#pragma HLS interface port = b mode = bus bank_allocation = 2, 5
#pragma HLS interface port = c mode = bus bank_allocation = 1, 3, 4, 7
#elif defined(POWER_OF_TWO)
#pragma HLS interface port = a mode = bus bank_allocation = 2, 5
#pragma HLS interface port = b mode = bus bank_allocation = 2, 5
#pragma HLS interface port = c mode = bus bank_allocation = 1, 3, 4, 7
#else
#pragma HLS interface port = a mode = bus bank_allocation = 0, 2, 5
#pragma HLS interface port = b mode = bus bank_allocation = 0, 2, 5
#pragma HLS interface port = c mode = bus bank_allocation = 0, 1, 3, 4, 7
#endif
extern "C" void __attribute__((noinline)) banked_copy(int* a, const int* b, int* c)
{
   for(unsigned i = 0; i < 35; ++i)
   {
      c[i] = a[i] + b[i];
      a[i] ^= 0x12345678;
   }
}

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

int main()
{
   int a[39], b[37], c[35];
   for(unsigned i = 0; i < 39; ++i)
   {
      a[i] = static_cast<int>(i) * 7 - 90;
   }
   for(unsigned i = 0; i < 37; ++i)
   {
      b[i] = static_cast<int>(i) * 11 + 3;
   }
   for(auto& value : c)
   {
      value = 0;
   }
#ifdef __BAMBU_SIM__
   m_param_alloc(0, sizeof(a));
   m_param_alloc(1, sizeof(b));
   m_param_alloc(2, sizeof(c));
#endif
   banked_copy(a, b, c);
   for(unsigned i = 0; i < 39; ++i)
   {
      const int initial = static_cast<int>(i) * 7 - 90;
      if(a[i] != (i < 35 ? (initial ^ 0x12345678) : initial))
      {
         std::abort();
      }
   }
   for(unsigned i = 0; i < 37; ++i)
   {
      if(b[i] != static_cast<int>(i) * 11 + 3 || (i < 35 && c[i] != static_cast<int>(i) * 18 - 87))
      {
         std::abort();
      }
   }
   return 0;
}
