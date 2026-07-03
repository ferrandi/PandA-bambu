#include <cstdio>
#include <ac_channel.h>
#include <ac_fixed.h>
#include <ac_int.h>
#include <ac_math.h>

void  __attribute__((noinline)) test(ac_fixed<21, 21, true>& ref0, int shift0, ac_fixed<21, 21, true>& res0)
{
  res0 = ref0 >> shift0;
}

int main()
{
   ac_fixed<21, 21, true> ref = (1 << 4);
   int shift = 4;
   ac_fixed<21, 21, true> res = 1;
   test(ref,shift,res);
   printf("%f\n", res.to_double());
   return 0;
}
