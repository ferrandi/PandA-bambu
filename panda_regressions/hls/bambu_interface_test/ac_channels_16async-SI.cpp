#include "ac_channel.h"

#pragma HLS interface port=a mode=fifo depth=8
#pragma HLS interface port=d mode=fifo depth=7
void count_async(ac_channel<unsigned short>& a, ac_channel<unsigned short>& b, ac_channel<unsigned short>& d,
                 ac_channel<unsigned short>& e)
{
   int c;
   unsigned short _a;
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
