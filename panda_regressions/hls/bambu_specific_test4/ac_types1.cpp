#include "ap_int.h"

#pragma HLS_interface d none
void sum3numbers(ap_int<16> a, ap_int<16> b, ap_int<16> c, ap_int<18>* d)
{
   *d = a + b + c;
}
