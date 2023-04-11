#include <algorithm>
#include "ap_int.h"
#define SIMULATION 1
#pragma HLS_interface x_fifo fifo
#pragma HLS_interface y_fifo fifo
#pragma HLS_interface res fifo
void gcd(ap_int<11>* x_fifo, ap_int<11>* y_fifo, ap_int<11>* res)
{
  do
  {
     ap_int<11> x = *x_fifo, y = *y_fifo;
     if( x < y )
          std::swap( x, y );

     while( y > 0 )
     {
          int f = x % y;
          x = y;
          y = f;
     }
     *res = x;
  } while(!SIMULATION);
}