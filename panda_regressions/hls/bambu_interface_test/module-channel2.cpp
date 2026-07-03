#include <cstdio>
#include <ac_channel.h>
#include <ac_fixed.h>
#include <ac_int.h>
#include <ac_math.h>

void  __attribute__((noinline)) test(ac_channel<ac_fixed<72, 16, false>>& ref0, ac_channel<ac_fixed<72, 16, false>>& compare0, ac_channel<ac_fixed<72, 16, false>>& res0)
{
  ac_fixed<72, 16, false> ref1;
  ac_fixed<72, 16, false> compare1;
  ac_fixed<72, 16, false> res1;
  
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
   ac_channel<ac_fixed<72, 16, false>> ref;
   ac_channel<ac_fixed<72, 16, false>> compare;
   ac_channel<ac_fixed<72, 16, false>> res;
   ac_fixed<72, 16, false> ref0 = 65535.984375;
   ac_fixed<72, 16, false> compare0 = 0;
   ac_fixed<72, 16, false> res0 = 1;
   ref.write(ref0);
   compare.write(compare0);   
   test(ref,compare,res);
   res0 = res.read();
   printf("%f\n", res0.to_double());
   exit(0);
}
