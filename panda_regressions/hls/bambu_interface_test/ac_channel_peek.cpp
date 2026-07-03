#include "ac_channel.h"
#include "ap_int.h"

#pragma HLS interface port = a mode = fifo depth = 8
#pragma HLS interface port = d mode = fifo depth = 8
void peek_sum(ac_channel<ap_uint<16>>& a, ac_channel<ap_uint<16>>& d)
{
   int i;
#pragma nounroll
   for(i = 0; i < 8; ++i)
   {
      const ap_uint<16> p = a.peek();
      const ap_uint<16> r = a.read();
      d.write(p + r);
   }
}
