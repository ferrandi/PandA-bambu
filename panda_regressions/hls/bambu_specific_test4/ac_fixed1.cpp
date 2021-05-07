#include "ap_fixed.h"

void sum3numbers(ap_fixed<16, 6> a[8], ap_fixed<16, 6> b[8], ap_fixed<16, 6> c[8], ap_fixed<18, 8> d[8])
{
   unsigned i;
   for(i = 0; i < 8; ++i)
      d[i] = a[i] + b[i] + c[i];
}
