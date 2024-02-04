#include "ac_channel.h"

#pragma HLS interface port=a mode=fifo depth=8
#pragma HLS interface port=b mode=fifo depth=8
#pragma HLS interface port=c mode=fifo depth=8
#pragma HLS interface port=d mode=fifo depth=8
void sum3numbers(ac_channel<float>& a, ac_channel<float>& b, ac_channel<float>& c, ac_channel<float>& d)
{
   int i;
#pragma nounroll
   for(i = 0; i < 8; ++i)
      d.write(a.read() + b.read() + c.read());
}
