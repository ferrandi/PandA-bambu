// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <mdpi/mdpi.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
   if(getenv("MDPI_TEST_DELAY_READY"))
   {
      sleep(1);
   }
   svLogicVecVal data[128] = {{0}};
   assert((m_next(1) & 255) == 2);
   int tag = m_read_request(0, 32, 0x40000000, 0);
   assert(tag > 0);
   unsigned completed_cycle = 0;
   const unsigned step = getenv("MDPI_TEST_CLOCK_JUMPS") ? 7 : 1;
   for(unsigned cycle = 1; cycle <= 2000; cycle += step)
   {
      m_memory_tick(cycle);
      m_memory_tick(cycle); // Repeated targets must not tick any backend twice.
      if(!completed_cycle)
      {
         const int completed = m_read_complete(0, data);
         assert(completed >= 0);
         if(completed)
         {
            assert(completed == tag && data[0].aval == 0x13579bdf);
            completed_cycle = cycle;
         }
      }
   }
   m_memory_tick(2000);
   m_memory_tick(2000);
   assert(completed_cycle);
   if(getenv("MDPI_TEST_READ_CYCLE"))
   {
      assert(completed_cycle == strtoul(getenv("MDPI_TEST_READ_CYCLE"), NULL, 10));
   }
   printf("CLOCK_READ_CYCLE=%u\n", completed_cycle);
   puts("PASS MDPI central clock and configured Ramulator model");
   return 0;
}
