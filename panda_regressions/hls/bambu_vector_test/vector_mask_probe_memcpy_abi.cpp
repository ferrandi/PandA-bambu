// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Copy the entire frontend comparison vector into equally sized storage.
// This ABI probe is separate from packed LLVM <N x i1> store/load.
#include "ac_channel.h"
#include <cstring>

typedef unsigned int payload_t __attribute__((vector_size(16)));

#pragma HLS interface port = input mode = fifo depth = 8
#pragma HLS interface port = output mode = fifo depth = 4
extern "C" void __attribute__((noinline)) probe(ac_channel<unsigned>& input, ac_channel<unsigned>& output)
{
   payload_t a = {}, b = {};
#pragma nounroll
   for(unsigned i = 0; i < 4; ++i)
   {
      a[i] = input.read();
      b[i] = input.read();
   }
   const auto mask = (a == b);
   unsigned copied[4] = {};
   static_assert(sizeof(copied) == sizeof(mask), "Frontend vector ABI size changed");
   std::memcpy(copied, &mask, sizeof(mask));
#pragma nounroll
   for(unsigned i = 0; i < 4; ++i)
   {
      output.write(copied[i]);
   }
}

int main()
{
   ac_channel<unsigned> input, output;
   const unsigned a[4] = {1, 2, 3, 4};
   const unsigned b[4] = {1, 9, 3, 9};
   const unsigned expected[4] = {~0u, 0, ~0u, 0};
   for(unsigned i = 0; i < 4; ++i)
   {
      input.write(a[i]);
      input.write(b[i]);
   }
   probe(input, output);
   for(unsigned i = 0; i < 4; ++i)
   {
      if(output.read() != expected[i])
      {
         return 1;
      }
   }
   return 0;
}
