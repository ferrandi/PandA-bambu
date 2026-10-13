// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Completeness probe for packed <N x i1> masks, mirroring the shape of
// ac_channel_nb_vector.cpp (vector payload, mask lane used as a boolean).
// VMODE selects eq, ne, vector select, mask and, max, or eq-lane select.
#include "ac_channel.h"
#include <cstring>

#ifndef VMODE
#define VMODE 0
#endif

typedef unsigned int payload_t __attribute__((vector_size(8)));

#pragma HLS interface port = input mode = fifo depth = 4
#pragma HLS interface port = output mode = fifo depth = 4
extern "C" void __attribute__((noinline)) probe(ac_channel<payload_t>& input, ac_channel<payload_t>& output)
{
   const payload_t a = input.read();
   const payload_t b = input.read();
#if VMODE == 0
   output.write((a[0] == b[0] && (a == b)[1]) ? a : payload_t{0, 0});
#elif VMODE == 1
   output.write((a[0] != b[0] || (a != b)[1]) ? a : payload_t{0, 0});
#elif VMODE == 2
   output.write((a == b) ? a : b);
#elif VMODE == 3
   output.write(((a == b) & (a == b))[1] ? a : b);
#elif VMODE == 4
   output.write((a > b) ? a : b);
#elif VMODE == 5
   output.write((a != b) ? a : b);
#elif VMODE == 6
   // lane 0 differ, lane 1 equal: the SELECTED mask bit is 1
   output.write((a == b)[1] ? a : b);
#endif
}

int main()
{
   ac_channel<payload_t> input, output;
#if VMODE == 6
   input.write(payload_t{1, 2}); // lane 0 differs, lane 1 equal
   input.write(payload_t{9, 2});
#else
   input.write(payload_t{1, 2}); // lane 0 equal, lane 1 different
   input.write(payload_t{1, 3});
#endif
   probe(input, output);
   const payload_t got = output.read();

   payload_t expected = {};
#if VMODE == 0
   expected = payload_t{0, 0}; // 1 && 0
#elif VMODE == 1
   expected = payload_t{1, 2}; // 0 || 1 -> a
#elif VMODE == 2
   expected = payload_t{1, 3}; // lane0 from a, lane1 from b
#elif VMODE == 3
   expected = payload_t{1, 3}; // lane1 of mask is false -> b
#elif VMODE == 4
   expected = payload_t{1, 3}; // lane0: 1>1 false -> b[0]=1 ; lane1: 2>3 false -> b[1]=3
#elif VMODE == 5
   expected = payload_t{1, 2}; // lane0 equal -> a[0]=1 ; lane1 differ -> a[1]=2
#elif VMODE == 6
   expected = payload_t{1, 2}; // mask lane 1 is true -> a
#endif
   if(std::memcmp(&got, &expected, sizeof(expected)))
   {
      return 100 + got[0] * 2 + got[1];
   }
   return 0;
}
