#define rank 32

#include <stdio.h>

#pragma HLS_interface a m_axi direct bundle = gmem0
#pragma HLS_interface b m_axi direct bundle = gmem1
#pragma HLS_interface output m_axi direct bundle = gmem2
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