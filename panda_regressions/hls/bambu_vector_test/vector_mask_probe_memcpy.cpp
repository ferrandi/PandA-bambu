// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// C++ vector comparison copied through the frontend vector ABI. This is
// distinct from an LLVM store of <N x i1>, whose lanes are bit packed.
// Legacy failing baseline only: sizeof(m) is 16 in the C++ frontend ABI,
// but copied has size 8. This source has undefined behavior natively and
// must not serve as a positive test or as an oracle. Use the separate
// vector_mask_probe_memcpy_abi.cpp for a valid full-width copy.
#include "ac_channel.h"
#include <cstring>

typedef unsigned int u4 __attribute__((vector_size(16))); // <4 x i32>

#pragma HLS interface port = input mode = fifo depth = 4
#pragma HLS interface port = output mode = fifo depth = 4
extern "C" void __attribute__((noinline)) probe(ac_channel<unsigned>& input, ac_channel<unsigned>& output)
{
   u4 a = {};
   u4 b = {};
#pragma nounroll
   for(unsigned i = 0; i < 4; ++i)
   {
      a[i] = input.read();
      b[i] = input.read();
   }
   const auto m = (a == b);
   unsigned long long copied = 0;
   std::memcpy(&copied, &m, sizeof(m));
   output.write(static_cast<unsigned>(copied));
}

int main()
{
   ac_channel<unsigned> input, output;
   const unsigned a[4] = {1, 2, 3, 4};
   const unsigned b[4] = {1, 9, 3, 9};
   for(unsigned i = 0; i < 4; ++i)
   {
      input.write(a[i]);
      input.write(b[i]);
   }
   probe(input, output);
   const unsigned got = output.read();
   const unsigned expected = 0x00010001u; // byte per element, little endian
   if(got != expected)
   {
      return 100 + (got == 5u);
   }
   return 0;
}
