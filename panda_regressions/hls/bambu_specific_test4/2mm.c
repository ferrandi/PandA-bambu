#ifndef DATA_TYPE
#define DATA_TYPE int
#endif

#ifndef m_rank
#define m_rank 32
#endif

#ifndef m_tile_rank
#define m_tile_rank 2
#endif

#pragma HLS_interface a m_axi direct bundle = gmem0
#pragma HLS_interface b m_axi direct bundle = gmem1
#pragma HLS_interface output m_axi direct bundle = gmem2

#if defined(BIG_BUFFER)
#pragma HLS_cache bundle = gmem0 way_size = 16 line_size = 16 bus_size = 32 n_ways = 1 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS_cache bundle = gmem1 way_size = 16 line_size = 16 bus_size = 32 n_ways = 1 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS_cache bundle = gmem2 way_size = 16 line_size = 16 bus_size = 32 n_ways = 1 buffer_size = 16 rep_policy = \
    lru write_policy = wt
#elif defined(BIG_BUS)
#pragma HLS_cache bundle = gmem0 way_size = 16 line_size = 16 bus_size = 128 n_ways = 1 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS_cache bundle = gmem1 way_size = 16 line_size = 16 bus_size = 64 n_ways = 1 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS_cache bundle = gmem2 way_size = 8 line_size = 16 bus_size = 256 n_ways = 2 buffer_size = 4 rep_policy = \
    lru write_policy = wb
#elif defined(BIG)
#pragma HLS_cache bundle = gmem0 way_size = 32 line_size = 32 bus_size = 32 n_ways = 1 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS_cache bundle = gmem1 way_size = 16 line_size = 64 bus_size = 32 n_ways = 1 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS_cache bundle = gmem2 way_size = 16 line_size = 32 bus_size = 32 n_ways = 2 buffer_size = 8 rep_policy = \
    tree write_policy = wt
#elif defined(BIG_LINE)
#pragma HLS_cache bundle = gmem0 way_size = 2 line_size = 128 bus_size = 32 n_ways = 1 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS_cache bundle = gmem1 way_size = 2 line_size = 64 bus_size = 32 n_ways = 2 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS_cache bundle = gmem2 way_size = 4 line_size = 64 bus_size = 32 n_ways = 1 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#elif defined(FULLY_ASS)
#pragma HLS_cache bundle = gmem0 way_size = 1 line_size = 32 bus_size = 32 n_ways = 8 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS_cache bundle = gmem1 way_size = 1 line_size = 8 bus_size = 32 n_ways = 32 buffer_size = 2 rep_policy = \
    tree write_policy = wt
#pragma HLS_cache bundle = gmem2 way_size = 1 line_size = 16 bus_size = 32 n_ways = 16 buffer_size = 4 rep_policy = \
    tree write_policy = wb
#elif defined(SET_ASS)
#pragma HLS_cache bundle = gmem0 way_size = 8 line_size = 16 bus_size = 32 n_ways = 2 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS_cache bundle = gmem1 way_size = 4 line_size = 16 bus_size = 32 n_ways = 4 buffer_size = 2 rep_policy = \
    tree write_policy = wt
#pragma HLS_cache bundle = gmem2 way_size = 2 line_size = 16 bus_size = 32 n_ways = 8 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#elif defined(SIMPLE)
#pragma HLS_cache bundle = gmem0 way_size = 16 line_size = 16 bus_size = 32 n_ways = 1 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS_cache bundle = gmem1 way_size = 16 line_size = 16 bus_size = 32 n_ways = 1 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS_cache bundle = gmem2 way_size = 16 line_size = 16 bus_size = 32 n_ways = 1 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#elif defined(SMALL_LINE)
#pragma HLS_cache bundle = gmem0 way_size = 32 line_size = 8 bus_size = 32 n_ways = 1 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS_cache bundle = gmem1 way_size = 32 line_size = 4 bus_size = 32 n_ways = 2 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS_cache bundle = gmem2 way_size = 16 line_size = 8 bus_size = 32 n_ways = 2 buffer_size = 2 rep_policy = \
    lru write_policy = wb
#elif defined(BIG_BUFFER_WB)
#pragma HLS_cache bundle = gmem0 way_size = 16 line_size = 16 bus_size = 32 n_ways = 1 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS_cache bundle = gmem1 way_size = 16 line_size = 16 bus_size = 32 n_ways = 1 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS_cache bundle = gmem2 way_size = 16 line_size = 16 bus_size = 32 n_ways = 1 buffer_size = 16 rep_policy = \
    lru write_policy = wb
#elif defined(WRITE_BACK)
#pragma HLS_cache bundle = gmem0 way_size = 16 line_size = 16 bus_size = 32 n_ways = 1 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS_cache bundle = gmem1 way_size = 16 line_size = 16 bus_size = 32 n_ways = 1 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS_cache bundle = gmem2 way_size = 16 line_size = 16 bus_size = 32 n_ways = 1 buffer_size = 2 rep_policy = \
    lru write_policy = wb
#elif defined(TILED)
#pragma HLS_cache bundle = gmem0 way_size = 16 line_size = 16 bus_size = 32 n_ways = 1 buffer_size = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS_cache bundle = gmem1 way_size = 32 line_size = 16 bus_size = 32 n_ways = 1 buffer_size = 2 rep_policy = \
    tree write_policy = wt
#pragma HLS_cache bundle = gmem2 way_size = 16 line_size = 16 bus_size = 32 n_ways = 1 buffer_size = 4 rep_policy = \
    tree write_policy = wb
#endif

void __attribute__((noinline)) mmult(int a[m_rank][m_rank], int b[m_rank][m_rank], int output[m_rank][m_rank])
{
   for(unsigned int c = 0; c < m_rank; c++)
   {
      for(unsigned int r = 0; r < m_rank; r++)
      {
         int running = 0;
         for(int index = 0; index < m_rank; index++)
         {
            running += a[r][index] * b[index][c];
         }
         output[r][c] = running;
      }
   }
}

void __attribute__((noinline)) mmult_tile(int a[m_rank][m_rank], int b[m_rank][m_rank], int output[m_rank][m_rank])
{
   for(unsigned c_tile = 0; c_tile < m_tile_rank; c_tile++)
   {
      for(unsigned r_tile = 0; r_tile < m_tile_rank; r_tile++)
      {
         for(unsigned r = 0; r < m_rank / m_tile_rank; r++)
         {
            for(unsigned c = 0; c < m_rank / m_tile_rank; c++)
            {
               output[r + r_tile * m_rank / m_tile_rank][c + c_tile * m_rank / m_tile_rank] = 0;
            }
         }
         for(unsigned i_tile = 0; i_tile < m_tile_rank; i_tile++)
         {
            for(unsigned c = 0; c < m_rank / m_tile_rank; c++)
            {
               for(unsigned r = 0; r < m_rank / m_tile_rank; r++)
               {
                  int running = 0;
                  for(unsigned index = 0; index < m_rank / m_tile_rank; index++)
                  {
                     running += a[r + r_tile * m_rank / m_tile_rank][index + i_tile * m_rank / m_tile_rank] *
                                b[index + i_tile * m_rank / m_tile_rank][c + c_tile * m_rank / m_tile_rank];
                  }
                  output[r + r_tile * m_rank / m_tile_rank][c + c_tile * m_rank / m_tile_rank] += running;
               }
            }
         }
      }
   }
}

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

int main()
{
   unsigned i, j;
   DATA_TYPE A[m_rank][m_rank];
   DATA_TYPE B[m_rank][m_rank];
   DATA_TYPE C[m_rank][m_rank];

   for(i = 0; i < m_rank; i++)
      for(j = 0; j < m_rank; j++)
         A[i][j] = (DATA_TYPE)((i * j + 1) % m_rank) / m_rank;
   for(i = 0; i < m_rank; i++)
      for(j = 0; j < m_rank; j++)
         B[i][j] = (DATA_TYPE)((i * j + 1) % m_rank) / m_rank;

#ifdef __BAMBU_SIM__
   m_param_alloc(0, sizeof(A));
   m_param_alloc(1, sizeof(B));
   m_param_alloc(2, sizeof(C));
#endif
#ifdef TILED
   mmult_tile(A, B, C);
#else
   mmult(A, B, C);
#endif

   return 0;
}