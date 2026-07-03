#include <cstdio>
#include <ac_channel.h>
#include <ac_fixed.h>
#include <ac_int.h>
#include <ac_math.h>

struct pair_obj
{
   ac_fixed<72, 16, false> refF;
   ac_fixed<72, 16, false> compareF;
};


extern "C" void  __attribute__((noinline)) test(ac_channel<pair_obj>& in1, ac_channel<ac_fixed<72, 16, false>>& res0)
{
  ac_fixed<72, 16, false> ref1;
  ac_fixed<72, 16, false> compare1;
  ac_fixed<72, 16, false> res1;
  pair_obj inObj;
  
  inObj = in1.read();
  ref1 = inObj.refF;
  compare1 = inObj.compareF;
  if(ref1 > compare1)
   res1 = compare1;
  else
   res1 = ref1;
  res0.write(res1);
}

int main()
{
   ac_channel<pair_obj> in0;
   ac_channel<ac_fixed<72, 16, false>> res;
   ac_fixed<72, 16, false> ref0 = 65535.984375;
   ac_fixed<72, 16, false> compare0 = 0;
   ac_fixed<72, 16, false> res0 = 1;
   pair_obj inObj;
   inObj.refF=ref0;
   inObj.compareF=compare0;
   in0.write(inObj);
   test(in0,res);
   res0 = res.read();
   printf("%f\n", res0.to_double());
   return 0;
}
