#include "ac_channel.h"
#include "ap_int.h"

#pragma HLS interface port = a mode = fifo depth = 8
#pragma HLS interface port = d mode = fifo depth = 8
void nb_peek_sum(ac_channel<ap_uint<16>>& a, ac_channel<ap_uint<16>>& d)
{
   int i;
#pragma nounroll
   for(i = 0; i < 8; ++i)
   {
      ap_uint<16> p = 0;
      const bool valid = a.nb_peek(p);
      const ap_uint<16> r = a.read();
      if(valid)
      {
         d.write(p + r);
      }
      else
      {
         d.write(0);
      }
   }
}
