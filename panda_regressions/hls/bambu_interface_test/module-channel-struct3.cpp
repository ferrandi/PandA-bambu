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


extern "C" void  __attribute__((noinline)) test(ac_channel<pair_obj>& in1, ac_channel<pair_obj>& res0)
{
  ac_fixed<72, 16, false> ref1;
  ac_fixed<72, 16, false> compare1;
  ac_fixed<72, 16, false> res1;
  pair_obj inObj;
  pair_obj outObj;
  
  inObj = in1.read();
  ref1 = inObj.refF;
  compare1 = inObj.compareF;
  if(ref1 > compare1)
   res1 = compare1;
  else
   res1 = ref1;
  outObj.refF = ref1;
  outObj.compareF = res1;
  
  res0.write(outObj);
}

int main()
{
   ac_channel<pair_obj> in0;
   ac_channel<pair_obj> out0;
   ac_channel<ac_fixed<72, 16, false>> res;
   ac_fixed<72, 16, false> ref0 = 65535.984375;
   ac_fixed<72, 16, false> compare0 = 0;
   pair_obj inObj;
   pair_obj outObj;
   inObj.refF=ref0;
   inObj.compareF=compare0;
   in0.write(inObj);
   test(in0,out0);
   outObj = out0.read();
   printf("%f - %f\n", outObj.refF.to_double(), outObj.compareF.to_double());
   return 0;
}
