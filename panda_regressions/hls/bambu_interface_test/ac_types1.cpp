#include "ap_int.h"

#pragma HLS interface port = d mode = none
void sum3numbers(ap_int<16> a, ap_int<16> b, ap_int<16> c, ap_int<18>* d)
{
   *d = a + b + c;
}
