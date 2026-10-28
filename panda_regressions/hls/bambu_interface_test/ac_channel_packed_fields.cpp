// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Packed channel write: the aggregate reaches the interface as separate
// fragments, and the first one is narrower than the port with its MSB set.
// Zero-extension of the fragment must keep the following field intact.
#include "ac_channel.h"
#include <cstdlib>
#include <cstring>

struct payload
{
   float a;
   unsigned b;
};

#pragma HLS interface port = input mode = fifo depth = 4
#pragma HLS interface port = output mode = fifo depth = 4
extern "C" void __attribute__((noinline)) packed_fields(ac_channel<payload>& input, ac_channel<payload>& output)
{
   for(unsigned i = 0; i < 4; ++i)
   {
      const payload value = input.read();
      payload result;
      result.a = value.a;
      result.b = value.b + 1u;
      output.write(result);
   }
}

int main()
{
   // -1.0f is 0xBF800000: the fragment MSB is set, so a sign-extended fragment
   // would fill the bits of the following field with ones.
   const float fa[4] = {-1.0f, -2.5f, 3.5f, -0.75f};
   const unsigned fb[4] = {0x11111111U, 0x22222222U, 0x33333333U, 0x44444444U};

   ac_channel<payload> input, output;
   for(unsigned i = 0; i < 4; ++i)
   {
      payload value;
      value.a = fa[i];
      value.b = fb[i];
      input.write(value);
   }
   packed_fields(input, output);
   for(unsigned i = 0; i < 4; ++i)
   {
      const payload value = output.read();
      unsigned got_a = 0;
      unsigned exp_a = 0;
      std::memcpy(&got_a, &value.a, sizeof(got_a));
      std::memcpy(&exp_a, &fa[i], sizeof(exp_a));
      if(got_a != exp_a || value.b != fb[i] + 1u)
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
