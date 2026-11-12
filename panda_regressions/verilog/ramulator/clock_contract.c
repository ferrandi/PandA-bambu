// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <mdpi/mdpi.h>
#include <assert.h>
#include <stdio.h>
int main(void)
{
   svLogicVecVal data[128] = {{0}};
   assert((m_next(1) & 255) == 2);
   int tag = m_read_request(0, 32, 0x40000000, 0);
   assert(tag > 0);
   m_memory_tick(1000);
   m_memory_tick(1000); // Repeating a target cycle must not tick any backend twice.
   m_memory_tick(2000);
   assert(m_read_complete(0, data) == tag && data[0].aval == 0x13579bdf);
   puts("PASS MDPI central clock and configured Ramulator model");
   return 0;
}
