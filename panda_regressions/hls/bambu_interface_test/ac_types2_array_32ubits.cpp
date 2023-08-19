#include "ap_int.h"

void sum3numbers(ap_uint<32> a[8], ap_uint<32> b[8], ap_uint<32> c[8], ap_uint<32> d[8])
{
   unsigned i;
   for(i = 0; i < 8; ++i)
      d[i] = a[i] + b[i] + c[i];
}
