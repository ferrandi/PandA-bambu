// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <mdpi/mdpi.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if(!(condition)) { \
   fprintf(stderr, "Wide MDPI contract failed at line %d\n", __LINE__); exit(1); } } while(0)

static unsigned long long cycle;
static unsigned refused;
static svLogicVecVal data[128], expected[128];

static void tick(void)
{
   CHECK(++cycle < 200000);
   m_memory_tick(cycle);
}

static void read_bytes(ptr_t addr, unsigned bytes, int written)
{
   int tag;
   do
   {
      tag = m_read_request(0, bytes * 8, addr, 7);
      CHECK(tag >= 0);
      if(!tag)
      {
         ++refused;
         // A partially submitted read must not publish a logical completion.
         CHECK(m_read_complete_ch(0, 7, data) == 0);
         tick();
      }
   } while(!tag);
   int result;
   do
   {
      tick();
      result = m_read_complete_ch(0, 7, data);
   } while(!result);
   CHECK(result == tag);
   for(unsigned i = 0; i < bytes; ++i)
   {
      const unsigned actual = (data[i / 4].aval >> (8 * (i % 4))) & 255;
      const unsigned offset = (unsigned)(addr - 0x40000000) + i;
      const unsigned value = written ? (expected[i / 4].aval >> (8 * (i % 4))) & 255 :
          (((offset / 4) ^ 0x13579bdfU) >> (8 * (offset % 4))) & 255;
      CHECK(actual == value);
   }
   CHECK(m_read_complete_ch(0, 7, data) == 0);
}

int main(void)
{
   CHECK((m_next(1) & 255) == 2);
   // Native HBM2 transactions are 32 bytes. Include a small crossing access
   // and a maximum MDPI transfer that cannot fit in the single-entry queues.
   const unsigned lengths[] = {8, 64, 512, 512};
   const unsigned offsets[] = {0, 1024, 2048, 4095};
   for(unsigned test = 0; test < 4; ++test)
   {
      const ptr_t addr = 0x40000000 + offsets[test];
      const unsigned bytes = lengths[test];
      read_bytes(addr, bytes, 0);
      for(unsigned i = 0; i < 128; ++i)
      {
         expected[i].aval = 0xfedcba98U ^ (i * 0x01020304U);
         expected[i].bval = 0;
      }
      int result;
      do
      {
         result = m_write_request(0, expected, bytes * 8, addr, 0);
         CHECK(result >= 0);
         if(!result) { ++refused; tick(); }
      } while(!result);
      read_bytes(addr, bytes, 1);
   }
   // One byte on either side of the native boundary.
   read_bytes(0x4000201f, 2, 0);
   // Identical writes from separate TB channels have separate ownership.
   int accepted[2] = {0, 0};
   while(!accepted[0] || !accepted[1])
   {
      for(unsigned channel = 0; channel < 2; ++channel)
         if(!accepted[channel])
         {
            const int result = m_write_request_ch(0, expected, 4096, 0x40003000, 0, channel + 3);
            CHECK(result >= 0);
            if(channel == 1 && !accepted[0]) CHECK(result == 0);
            accepted[channel] = result > 0;
         }
      tick();
   }
   read_bytes(0x40003000, 512, 1);
   // Interleave partial reads from two independent channels.
   int tags[2] = {0, 0}, done[2] = {0, 0};
   while(!done[0] || !done[1])
   {
      for(unsigned channel = 0; channel < 2; ++channel)
      {
         if(!tags[channel])
         {
            tags[channel] = m_read_request(0, 4096, 0x40003000, channel + 10);
            CHECK(tags[channel] >= 0);
         }
         const int result = m_read_complete_ch(0, channel + 10, data);
         if(result)
         {
            CHECK(tags[channel] > 0 && result == tags[channel] && !done[channel]);
            done[channel] = 1;
            for(unsigned i = 0; i < 128; ++i) CHECK(data[i].aval == expected[i].aval);
         }
      }
      tick();
   }
   CHECK(tags[0] != tags[1]);
   CHECK(refused > 0);
   // Drain writes and detect duplicate logical read completions.
   for(unsigned i = 0; i < 1000; ++i)
   {
      tick();
      CHECK(m_read_complete_ch(0, 7, data) == 0);
   }
   printf("PASS wide MDPI transfers, partial retries=%u\n", refused);
   return 0;
}
