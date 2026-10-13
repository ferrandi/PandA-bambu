// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Multi-byte mask case, shaped like ac_channel_nb_vector.cpp: the vectors come
// from channel payloads (so the values reach the comparison through the packed
// interface path), and the mask lanes are read as booleans.
#include "ac_channel.h"

typedef unsigned int payload_t __attribute__((vector_size(64))); // <16 x i32>

#pragma HLS interface port = input mode = fifo depth = 4
#pragma HLS interface port = output mode = fifo depth = 4
extern "C" void __attribute__((noinline)) probe(ac_channel<payload_t>& input, ac_channel<unsigned>& output)
{
   const payload_t a = input.read();
   const payload_t b = input.read();
   const auto hi = (a == b)[15];
   const auto lo = (a == b)[1];
   output.write(hi ? 1u : 0u);
   output.write(lo ? 1u : 0u);
}

int main()
{
   ac_channel<payload_t> input;
   ac_channel<unsigned> output;
   payload_t a = {};
   payload_t b = {};
   for(unsigned i = 0; i < 16; ++i)
   {
      a[i] = 100 + i;
      b[i] = (i == 15) ? (100 + i) : (7 + i);
   }
   input.write(a);
   input.write(b);
   probe(input, output);
   const unsigned first = output.read();
   const unsigned second = output.read();
   if(first != 1 || second != 0)
   {
      return 100 + first * 4 + second;
   }
   return 0;
}
