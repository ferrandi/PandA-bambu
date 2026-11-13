// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <mdpi/mdpi.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
   assert((m_next(1) & 255) == 2);
   svLogicVecVal data[128] = {{0}};
   unsigned writes = 0, rejected = 0;
   data[0].aval = 0xdeadbeef;
   for(unsigned i = 0; i < 1024; ++i)
   {
      const int result = m_write_request(0, data, 32, 0x40000000 + i * 64, 0);
      assert(result >= 0);
      writes += result > 0;
      rejected += result == 0;
   }
   assert(writes > 0 && rejected > 0);
   const ptr_t address = 0x40040000;
   assert(m_write_request(0, data, 32, address, 0) == 0);
   unsigned long long cycle = 1;
   for(; cycle < 20000; ++cycle)
   {
      m_memory_tick(cycle);
   }
   int tag = m_read_request(0, 32, address, 0);
   assert(tag > 0);
   int completion = 0;
   for(unsigned i = 0; i < 20000 && !completion; ++i)
   {
      m_memory_tick(cycle++);
      completion = m_read_complete_ch(0, 0, data);
   }
   assert(completion == tag);
   assert(data[0].aval == ((0x40000 / 4) ^ 0x13579bdfU));
   // Fill reads without ticking: refused requests must not allocate tags or callbacks.
   unsigned reads = 0, read_rejected = 0;
   for(unsigned i = 0; i < 1024; ++i)
   {
      int result = m_read_request(0, 32, 0x40080000 + i * 64, 7);
      assert(result >= 0);
      reads += result > 0;
      read_rejected += result == 0;
   }
   assert(reads > 0 && read_rejected > 0);
   unsigned completed = 0;
   for(unsigned i = 0; i < 20000; ++i)
   {
      m_memory_tick(cycle++);
      completed += m_read_complete_ch(0, 7, data) > 0;
   }
   assert(completed == reads);
   printf("PASS MDPI acceptance writes=%u rejected=%u reads=%u rejected=%u completed=%u\n",
          writes, rejected, reads, read_rejected, completed);
   return 0;
}
