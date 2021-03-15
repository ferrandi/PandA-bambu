#include "ap_int.h"

//__attribute__((no_inline))
void mac(ap_int<67> a[8], ap_int<67> b[8], ap_int<67> c[8], ap_int<68> temp[8])
{
   unsigned i;
   for(i = 0; i < 8; ++i)
      temp[i] = a[i] + b[i] * c[i];
}

void sum3numbers(ap_int<67> a[8], ap_int<67> b[8], ap_int<67> c[8], ap_int<68> d[8])
{
   ap_int<68> temp[8];
   unsigned i;
   mac(a, b, c, temp);
   for(i = 0; i < 8; ++i)
      d[i] = d[i] + temp[i] + (i < 7 ? a[i + 1] : a[0]);
}
