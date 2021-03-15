#include "ap_int.h"
extern "C"
{
#pragma HLS_interface d none
   void sum3numbers(const ap_int<16>& a, const ap_int<16>& b, const ap_int<16>& c, ap_int<18>& d)
   {
      d = a + b + c;
   }
}
