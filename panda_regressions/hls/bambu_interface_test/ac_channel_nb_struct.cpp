// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Non-blocking read/peek of an aggregate payload. Pre-_BitInt compilers return
// the payload through an sret pointer and the valid flag by reference, which is
// the legacy signature whose packed return width must match the interface return
// type; recent compilers pack both in one _BitInt word. Both paths are checked,
// including the empty-channel case, where the seams must report invalid and
// leave their argument untouched.
#include "ac_channel.h"
#include <cstdlib>

struct payload
{
   unsigned lo;
   unsigned hi;
};

#pragma HLS interface port = input mode = fifo depth = 4
#pragma HLS interface port = output mode = fifo depth = 5
extern "C" void __attribute__((noinline)) nb_struct(ac_channel<payload>& input, ac_channel<payload>& output)
{
   for(unsigned i = 0; i < 4; ++i)
   {
      payload peeked = {};
      payload value = {};
      while(!input.nb_peek(peeked))
      {
      }
      while(!input.nb_read(value))
      {
      }
      payload out;
      out.lo = peeked.lo == value.lo && peeked.hi == value.hi ? value.lo + 1u : 0u;
      out.hi = value.hi ^ 0x80000000U;
      output.write(out);
   }
   // The empty-channel case: both seams must report invalid and leave the
   // argument untouched. This is where the hardware stops making progress.
   const payload sentinel = {0xfedcba98U, 0x80000001U};
   payload peeked = sentinel;
   payload value = sentinel;
   const bool peek_valid = input.nb_peek(peeked);
   const bool read_valid = input.nb_read(value);
   payload out;
   out.lo = !peek_valid && !read_valid && peeked.lo == sentinel.lo && value.lo == sentinel.lo ? sentinel.lo : 0U;
   out.hi = peeked.hi == sentinel.hi && value.hi == sentinel.hi ? sentinel.hi : 0U;
   output.write(out);
}

int main()
{
   ac_channel<payload> input, output;
   for(unsigned i = 0; i < 4; ++i)
   {
      payload value;
      value.lo = 0x1000U + i;
      value.hi = 0xf0000000U + i;
      input.write(value);
   }
   nb_struct(input, output);
   for(unsigned i = 0; i < 5; ++i)
   {
      const payload value = output.read();
      if(i < 4)
      {
         if(value.lo != 0x1001U + i || value.hi != ((0xf0000000U + i) ^ 0x80000000U))
         {
            std::abort();
         }
      }
      else if(value.lo != 0xfedcba98U || value.hi != 0x80000001U)
      {
         std::abort();
      }
   }
   if(input.available(1) || output.available(1))
   {
      std::abort();
   }
   return 0;
}
