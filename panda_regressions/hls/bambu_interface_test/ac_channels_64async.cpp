#include "ac_channel.h"
#include "ap_int.h"

#pragma HLS interface port=a mode=fifo depth=8
#pragma HLS interface port=d mode=fifo depth=7
void count_async(ac_channel<ap_uint<64>>& a, ac_channel<ap_uint<64>>& b, ac_channel<ap_uint<64>>& d,
                 ac_channel<ap_uint<64>>& e)
{
   int c;
   ap_uint<64> _a;
   do
   {
      c = 0;
      while(!a.nb_read(_a))
      {
         c++;
      }
      _a += c + b.read();
      e.write(c);
   } while(d.nb_write(_a));
}
