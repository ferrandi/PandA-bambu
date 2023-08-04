#include "ac_channel.h"

void count_async(ac_channel<unsigned long long>& a, ac_channel<unsigned long long>& b, ac_channel<unsigned long long>& d,
                 ac_channel<unsigned long long>& e)
{
   int c;
   unsigned long long _a;
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
