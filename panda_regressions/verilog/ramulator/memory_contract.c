// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <mdpi/mdpi.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned long long cycle = 1;
static svLogicVecVal data[128];

static void wait_read(int tag, unsigned char channel)
{
   int result = 0;
   for(unsigned i = 0; i < 20000 && !result; ++i)
   {
      m_memory_tick(cycle++);
      memset(data, 0xff, sizeof(data));
      result = m_read_complete_ch(0, channel, data);
   }
   assert(result == tag);
   for(unsigned i = 0; i < 128; ++i)
   {
      assert(data[i].bval == 0);
   }
}

int main(void)
{
   assert((m_next(1) & 255) == 2);
   const ptr_t address = 0x400a0000;
   const unsigned original = (0xa0000 / 4) ^ 0x13579bdfU;
   const int first = m_read_request(0, 32, address, 3);
   assert(first > 0);
   data[0].aval = 0x87654321;
   assert(m_write_request(0, data, 32, address, 0) > 0);
   wait_read(first, 3);
   assert(data[0].aval == original); // The write must not change the pending read.
   assert(data[1].aval == 0 && data[127].aval == 0);

   int tag = m_read_request(0, 8, address + 1, (signed char)255);
   assert(tag > 0);
   wait_read(tag, 255);
   assert(data[0].aval == 0x43 && data[1].aval == 0);
   data[0].aval = 7;
   assert(m_write_request(0, data, 3, address, 0) > 0);
   assert(m_read(0, data, 32, address, 0) == 0);
   assert(data[0].aval == 0x87654327);
   tag = m_read_request(0, 3, address, 0);
   assert(tag > 0);
   wait_read(tag, 0);
   assert(data[0].aval == 7);

   // Byte writes preserve the next argument; wide accesses translate both host buffers.
   data[0].aval = 0xdeadbeef;
   assert(m_write_request(0, data, 8, 0x400fffff, 0) > 0);
   assert(m_read(0, data, 32, 0x40100000, 0) == 0);
   assert(data[0].aval == 0x5a5a5a5a);
   tag = m_read_request(0, 32, 0x400fffff, 0);
   assert(tag > 0);
   wait_read(tag, 0);
   assert(data[0].aval == 0x5a5a5aef); // Wide read translates the next mapped region.
   data[0].aval = 0x11223344;
   assert(m_write_request(0, data, 32, 0x400fffff, 0) > 0);
   assert(m_read(0, data, 32, 0x40100000, 0) == 0 && data[0].aval == 0x5a112233);

   tag = m_read_request(0, 4096, 0x400c0000, 0);
   assert(tag > 0);
   wait_read(tag, 0);
   for(unsigned i = 0; i < 128; ++i)
   {
      assert(data[i].aval == (((0xc0000 / 4) + i) ^ 0x13579bdfU));
   }
   for(unsigned i = 0; i < 128; ++i)
   {
      data[i].aval = 0xfedc0000U + i;
   }
   assert(m_write_request(0, data, 4096, 0x400d0000, 0) > 0);
   tag = m_read_request(0, 4096, 0x400d0000, 0);
   assert(tag > 0);
   wait_read(tag, 0);
   for(unsigned i = 0; i < 128; ++i)
   {
      assert(data[i].aval == 0xfedc0000U + i);
   }
   // Unmapped padding on the last cache line is zero, never host memory.
   tag = m_read_request(0, 32, 0x4010007f, 0);
   assert(tag > 0);
   wait_read(tag, 0);
   assert(data[0].aval == 0x5a);
   data[0].aval = 0xdeadbeef;
   assert(m_write_request(0, data, 32, 0x4010007f, 0) > 0);
   tag = m_read_request(0, 32, 0x4010007f, 0);
   assert(tag > 0);
   wait_read(tag, 0);
   assert(data[0].aval == 0xef);
   assert(m_read_request(0, 32, 0, 0) < 0);
   assert(m_read_request(0, 32, 0x50000000, 0) < 0);
   assert(m_write_request(0, data, 32, 0x50000000, 0) < 0);
   assert(m_read_request(0, 0, address, 0) < 0);
   assert(m_read_request(0, 4097, address, 0) < 0);
   assert(m_write_request(0, data, 0, address, 0) < 0);
   assert(m_write_request(0, data, 4097, address, 0) < 0);
   assert(m_write_request(0, data, 32, address, 1) < 0);
   assert(m_write_request(0, NULL, 32, address, 0) < 0);
   assert(m_read_complete(0, NULL) < 0);
   assert(m_read_request(65000, 32, address, 0) < 0);
   memset(data, 0xff, sizeof(data));
   assert(m_read_complete_ch(0, 99, data) == 0);
   for(unsigned i = 0; i < 128; ++i)
   {
      assert(data[i].aval == 0 && data[i].bval == 0);
   }
   puts("PASS MDPI snapshot, byte/bit sizes, boundaries, channels and payload limits");
   return 0;
}
