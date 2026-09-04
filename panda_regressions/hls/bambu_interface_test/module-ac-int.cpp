#include <cstdio>
#include <ac_channel.h>
#include <ac_fixed.h>
#include <ac_int.h>
#include <ac_math.h>

void  __attribute__((noinline)) test(ac_int<16, false>& ref0, ac_int<16, false> compare0, ac_int<16, false>& res0)
{
  if(ref0 > compare0)
   res0 = compare0;
  else
   res0 = ref0;
}

int main()
{
   ac_int<16, false> ref = 65535;
   ac_int<16, false> compare = 0;
   ac_int<16, false> res = 1;
   test(ref,compare,res);
   printf("%d\n", res.to_double());
   return 0;
}
