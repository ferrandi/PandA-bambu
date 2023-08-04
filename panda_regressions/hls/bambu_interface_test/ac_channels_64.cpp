#include "ac_channel.h"
#include "ap_int.h"

void sum3numbers(ac_channel<ap_uint<64>>& a, ac_channel<ap_uint<64>>& b, ac_channel<ap_uint<64>>& c,
                 ac_channel<ap_uint<64>>& d)
{
   int i;
#pragma nounroll
   for(i = 0; i < 8; ++i)
      d.write(a.read() + b.read() + c.read());
}
