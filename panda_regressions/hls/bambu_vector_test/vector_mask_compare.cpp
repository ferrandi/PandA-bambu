// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Mixed mask lanes: lane 0 compares equal and lane 1 compares differently.
#include "ac_channel.h"
#include <cstring>

#ifndef MASK_OP
#define MASK_OP 0
#endif

#ifndef MASK_SIGNED
#define MASK_SIGNED 0
#endif

#if MASK_SIGNED
typedef int element_t;
#else
typedef unsigned int element_t;
#endif
typedef element_t payload_t __attribute__((vector_size(8)));

#pragma HLS interface port = input mode = fifo depth = 4
#pragma HLS interface port = output mode = fifo depth = 4
extern "C" void __attribute__((noinline))
vector_mask_compare(ac_channel<payload_t>& input, ac_channel<payload_t>& output)
{
   const payload_t a = input.read();
   const payload_t b = input.read();
#if MASK_OP == 0
   // Keep the early eq guard in the form measured on both simulators.
   output.write((a[0] == b[0] && (a == b)[1]) ? a : payload_t{0, 0});
#elif MASK_OP == 1
   output.write((a != b)[1] ? a : b);
#elif MASK_OP == 2
   output.write((a < b)[1] ? a : b);
#elif MASK_OP == 3
   output.write((a > b)[1] ? a : b);
#elif MASK_OP == 4
   output.write((a <= b)[1] ? a : b);
#elif MASK_OP == 5
   output.write((a >= b)[1] ? a : b);
#else
#error Unsupported MASK_OP
#endif
}

int main()
{
   ac_channel<payload_t> input, output;
   const payload_t a = {1, static_cast<element_t>(0xffffffffU)};
   const payload_t b = {1, 1};
   input.write(a);
   input.write(b);
   vector_mask_compare(input, output);
   const payload_t got = output.read();

   bool select_a = false;
#if MASK_OP == 1
   select_a = true;
#elif MASK_OP == 2 || MASK_OP == 4
   select_a = MASK_SIGNED;
#elif MASK_OP == 3 || MASK_OP == 5
   select_a = !MASK_SIGNED;
#endif
   const payload_t expected = MASK_OP == 0 ? payload_t{0, 0} : (select_a ? a : b);
   return std::memcmp(&got, &expected, sizeof(got)) == 0 ? 0 : 1;
}
