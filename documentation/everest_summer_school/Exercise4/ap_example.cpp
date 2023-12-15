#include "ap_int.h"
#include <algorithm>
#define SIMULATION 1
#pragma HLS interface port = x_fifo mode = fifo
#pragma HLS interface port = y_fifo mode = fifo
#pragma HLS interface port = res mode = fifo
void gcd(ap_int<11>* x_fifo, ap_int<11>* y_fifo, ap_int<11>* res)
{
   do
   {
      ap_int<11> x = *x_fifo, y = *y_fifo;
      if(x < y)
         std::swap(x, y);

      while(y > 0)
      {
         int f = x % y;
         x = y;
         y = f;
      }
      *res = x;
   } while(!SIMULATION);
}