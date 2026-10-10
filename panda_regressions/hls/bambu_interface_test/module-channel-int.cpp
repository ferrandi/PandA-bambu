#include <cstdio>
#include <ac_channel.h>
#include <ac_fixed.h>
#include <ac_int.h>
#include <ac_math.h>

void  __attribute__((noinline)) test(ac_channel<unsigned short>& ref0, ac_channel<unsigned short>& compare0, ac_channel<unsigned short>& res0)
{
  unsigned short ref1;
  unsigned short compare1;
  unsigned short res1;
  
  ref1 = ref0.read();
  compare1 = compare0.read();
  if(ref1 > compare1)
   res1 = compare1;
  else
   res1 = ref1;
  res0.write(res1);
}

int main()
{
   ac_channel<unsigned short> ref;
   ac_channel<unsigned short> compare;
   ac_channel<unsigned short> res;
   unsigned short ref0 = 65535;
   unsigned short compare0 = 0;
   unsigned short res0 = 1;
   ref.write(ref0);
   compare.write(compare0);   
   test(ref,compare,res);
   res0 = res.read();
   printf("%d\n", res0);
   return 0;
}
