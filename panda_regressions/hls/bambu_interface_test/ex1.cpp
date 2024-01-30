#include <ap_int.h>

void test_func(ap_int<1> bit_stream[20], ap_int<20>* a_out)
{
#pragma HLS interface port=a_out mode=none register
   int i;
   ap_int<20> a_out_temp;
#pragma unroll
   for(i = 0; i < 20; i++)
   {
      a_out_temp[i] = bit_stream[i];
   }
   *a_out = a_out_temp;
}
