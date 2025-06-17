#define rank 32
#define tile_rank 2

/* AXI pragmas */
#pragma HLS interface port = a mode = m_axi offset = direct bundle = gmem0
#pragma HLS interface port = b mode = m_axi offset = direct bundle = gmem1
#pragma HLS interface port = c mode = m_axi offset = direct bundle = gmem2

void mmult(int* a, int* b, int* c)
{
   int running = 0;

   for(unsigned c_tile = 0; c_tile < tile_rank; c_tile++)
   {
      for(unsigned r_tile = 0; r_tile < tile_rank; r_tile++)
      {
         for(unsigned r = 0; r < rank / tile_rank; r++)
         {
            for(unsigned c = 0; c < rank / tile_rank; c++)
            {
               c[(r + r_tile * rank / tile_rank) * rank + (c + c_tile * rank / tile_rank)] = 0;
            }
         }
         for(unsigned i_tile = 0; i_tile < tile_rank; i_tile++)
         {
            for(unsigned c = 0; c < rank / tile_rank; c++)
            {
               for(unsigned r = 0; r < rank / tile_rank; r++)
               {
                  running = 0;
                  for(unsigned index = 0; index < rank / tile_rank; index++)
                  {
                     unsigned aIndex = (r + r_tile * rank / tile_rank) * rank + (index + i_tile * rank / tile_rank);
                     unsigned bIndex = (index + i_tile * rank / tile_rank) * rank + (c + c_tile * rank / tile_rank);
                     running += a[aIndex] * b[bIndex];
                  }
                  c[(r + r_tile * rank / tile_rank) * rank + (c + c_tile * rank / tile_rank)] += running;
               }
            }
         }
      }
   }
}