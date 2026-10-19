// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Nonblocking packed returns, vector constructors stored in memory, and empty preservation.
#include "ac_channel.h"
#include <cstdlib>
#include <cstring>

typedef unsigned int payload_t __attribute__((vector_size(8)));

#pragma HLS interface port = input mode = fifo depth = 4
#pragma HLS interface port = output mode = fifo depth = 5
extern "C" void __attribute__((noinline)) nb_vector(ac_channel<payload_t>& input, ac_channel<payload_t>& output)
{
   for(unsigned i = 0; i < 4; ++i)
   {
      payload_t peeked = {};
      payload_t value = {};
      while(!input.nb_peek(peeked))
      {
      }
      while(!input.nb_read(value))
      {
      }
      output.write(peeked[0] == value[0] && peeked[1] == value[1] ? value : payload_t{0, 0});
   }
   const payload_t sentinel = {0xfedcba98U, 0x80000001U};
   payload_t peeked = sentinel;
   payload_t value = sentinel;
   const bool peek_valid = input.nb_peek(peeked);
   const bool read_valid = input.nb_read(value);
   output.write(!peek_valid && !read_valid && peeked[0] == sentinel[0] && peeked[1] == sentinel[1] &&
                        value[0] == sentinel[0] && value[1] == sentinel[1] ?
                    sentinel :
                    payload_t{0, 0});
}

int main()
{
   ac_channel<payload_t> input, output;
   for(unsigned i = 0; i < 4; ++i)
   {
      input.write(payload_t{0x80000001U + i, 0xfffffffeU - i});
   }
   nb_vector(input, output);
   for(unsigned i = 0; i < 5; ++i)
   {
      const payload_t value = output.read();
      const payload_t expected =
          i < 4 ? payload_t{0x80000001U + i, 0xfffffffeU - i} : payload_t{0xfedcba98U, 0x80000001U};
      if(std::memcmp(&value, &expected, sizeof(value)))
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
