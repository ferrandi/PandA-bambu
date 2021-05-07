#include "ap_fixed.h"

void sum3numbers(ap_fixed<16, 6> a, ap_fixed<16, 6> b, ap_fixed<16, 6> c, ap_fixed<18, 8>* d)
{
   *d = a + b + c;
}
