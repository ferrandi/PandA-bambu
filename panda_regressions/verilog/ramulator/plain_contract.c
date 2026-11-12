// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <mdpi/mdpi.h>
#include <assert.h>
#include <stdio.h>
int main(void)
{
   svLogicVecVal data[128] = {{0}};
   assert((m_next(1) & 255) == 2);
   assert(m_read(0, data, 32, 0x40000000, 0) == 0 && data[0].aval == 0x13579bdf);
   data[0].aval = 0x12345678;
   assert(m_write(0, data, 32, 0x40000000, 0) == 0);
   assert(m_read(0, data, 32, 0x40000000, 0) == 0 && data[0].aval == 0x12345678);
   assert(m_read_request(0, 32, 0x40000000, 0) < 0);
   m_memory_tick(1000);
   puts("PASS plain MDPI memory remains untimed");
   return 0;
}
