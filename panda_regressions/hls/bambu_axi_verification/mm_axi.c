#ifndef rank
#define rank 32
#endif
#ifndef tile_rank
#define tile_rank 2
#endif

#pragma HLS interface port=a mode=m_axi offset=direct bundle=gmem0
#pragma HLS interface port=b mode=m_axi offset=direct bundle=gmem0
#pragma HLS interface port=res mode=m_axi offset=direct bundle=gmem0
#ifdef MEM_CACHE
#pragma HLS cache bundle = gmem0 line_count = 16 line_size = 16 bus_size = 32 ways = 1 num_write_outstanding = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS cache bundle = gmem1 line_count = 32 line_size = 16 bus_size = 32 ways = 1 num_write_outstanding = 2 rep_policy = \
    tree write_policy = wt
#pragma HLS cache bundle = gmem2 line_count = 16 line_size = 16 bus_size = 32 ways = 1 num_write_outstanding = 4 rep_policy = \
    tree write_policy = wb
#endif
void mmult(int* a, int* b, int* res)
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
               res[(r + r_tile * rank / tile_rank) * rank + (c + c_tile * rank / tile_rank)] = 0;
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
                  res[(r + r_tile * rank / tile_rank) * rank + (c + c_tile * rank / tile_rank)] += running;
               }
            }
         }
      }
   }

   return;
}