#include "ac_channel.h"

#pragma HLS interface port = a mode = fifo depth = 8
#pragma HLS interface port = d mode = fifo depth = 9
void nb_read_peek64(ac_channel<unsigned long long>& a, ac_channel<unsigned long long>& d)
{
#pragma nounroll
   for(unsigned int i = 0; i < 8; ++i)
   {
      unsigned long long peeked = 0;
      unsigned long long value = 0;
      // Both flags must survive the packed return ABI. Peeking must leave the
      // value available to the subsequent read, including its most significant bit.
      while(!a.nb_peek(peeked))
      {
      }
      while(!a.nb_read(value))
      {
      }
      d.write(peeked == value ? value : 0);
   }

   // On an empty channel both operations must return false and preserve the value.
   const unsigned long long sentinel = 0xfedcba9876543210ULL;
   unsigned long long peeked = sentinel;
   unsigned long long value = sentinel;
   const bool peek_valid = a.nb_peek(peeked);
   const bool read_valid = a.nb_read(value);
   d.write(!peek_valid && !read_valid && peeked == sentinel && value == sentinel ? sentinel : 0);
}
