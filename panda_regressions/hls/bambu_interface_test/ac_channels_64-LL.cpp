#include "ac_channel.h"

void sum3numbers(ac_channel<unsigned long long>& a, ac_channel<unsigned long long>& b, ac_channel<unsigned long long>& c,
                 ac_channel<unsigned long long>& d)
{
   int i;
#pragma nounroll
   for(i = 0; i < 8; ++i)
      d.write(a.read() + b.read() + c.read());
}
