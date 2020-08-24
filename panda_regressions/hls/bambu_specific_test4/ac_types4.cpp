#include "ap_int.h"

#pragma HLS_interface d none
void mul2numbers(const ap_int<2048>& a, const ap_int<16>& b, ap_int<16>* d)
{
   *d = a * b;
}
