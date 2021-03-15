#include "ap_int.h"

#pragma HLS_interface d none
void sum3numbers(ap_int<67> a, ap_int<67> b, ap_int<67> c, ap_int<68>* d)
{
   *d = a + b + c;
}
