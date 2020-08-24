#include "ap_int.h"

void sum3numbers(ap_uint<64> a[8], ap_uint<64> b[8], ap_uint<64> c[8], ap_uint<64> d[8])
{
   unsigned i;
   for(i = 0; i < 8; ++i)
      d[i] = a[i] + b[i] + c[i];
}
