#include "ap_int.h"

void sum3numbers(ap_int<7> a[8], ap_int<7> b[8], ap_int<7> c[8], ap_int<8> d[8])
{
   unsigned i;
   for(i = 0; i < 8; ++i)
      d[i] = a[i] + b[i] + c[i];
}
