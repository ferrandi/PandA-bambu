#include "ap_int.h"

void sum3numbers(const ap_int<67> a[4][2], const ap_int<67> b[4][2], const ap_int<67> c[4][2], ap_int<68> d[4][2])
{
   unsigned i,j;
   for(j = 0; j < 4; ++j)
      for(i = 0; i < 2; ++i)
         d[i][j] = a[i][j] + b[i][j] + c[i][j];
}
