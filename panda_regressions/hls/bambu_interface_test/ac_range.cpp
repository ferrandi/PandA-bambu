#include "ap_int.h"

#pragma HLS_interface c none
void check_range(short a, short b, ap_int<32>* c)
{
   ap_int<32> temp;
   temp.range(15 , 0 ) = a;
   temp.range(31 , 16 ) = b;
   *c = temp;
}
