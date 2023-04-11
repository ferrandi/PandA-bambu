#define rank 32

#include <stdio.h>

#pragma HLS_interface a m_axi direct bundle = gmem0
#pragma HLS_interface b m_axi direct bundle = gmem1
#pragma HLS_interface output m_axi direct bundle = gmem2

#pragma HLS_cache bundle = gmem0 way_size = 16 line_size = 16 bus_size = 32 n_ways = 1 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS_cache bundle = gmem1 way_size = 16 line_size = 16 bus_size = 32 n_ways = 1 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS_cache bundle = gmem2 way_size = 16 line_size = 16 bus_size = 32 n_ways = 1 buffer_size = 16 rep_policy = \
    lru write_policy = wt
void mmult(int* a, int* b, int* output)
{
   int running = 0;

   for(unsigned int c = 0; c < rank; c++)
   {
      for(unsigned int r = 0; r < rank; r++)
      {
         running = 0;
         for(int index = 0; index < rank; index++)
         {
            unsigned aIndex = r * rank + index;
            unsigned bIndex = index * rank + c;
            running += a[aIndex] * b[bIndex];
         }
         output[r * rank + c] = running;
      }
   }
   return;
}