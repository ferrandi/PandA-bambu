#ifndef DATA_TYPE
#define DATA_TYPE int
#endif

#ifndef elem_number
#define elem_number 20
#endif

#pragma HLS interface port = a mode = m_axi offset = direct bundle = gmem0
#pragma HLS interface port = b mode = m_axi offset = direct bundle = gmem1

#if defined(BIG_BUS)
#pragma HLS cache bundle = gmem0 line_count = 16 line_size = 16 bus_size = 128 ways = 1 num_write_outstanding = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS cache bundle = gmem1 line_count = 16 line_size = 16 bus_size = 128 ways = 1 num_write_outstanding = 2 rep_policy = \
    lru write_policy = wt
#else
#pragma HLS cache bundle = gmem0 line_count = 16 line_size = 16 bus_size = 128 ways = 1 num_write_outstanding = 2 rep_policy = \
    lru write_policy = wt
#pragma HLS cache bundle = gmem1 line_count = 16 line_size = 16 bus_size = 128 ways = 1 num_write_outstanding = 2 rep_policy = \
    lru write_policy = wt
#endif
int __attribute__((noinline)) vector_copy_plus(int* a, int* b)
{
   for(int i = 0; i < elem_number; i++)
   {
      b[i] = a[i] + 1;
   }
   return b[0];
}

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

int main()
{
   DATA_TYPE A[elem_number];
   DATA_TYPE B[elem_number];

   for(int i = 0; i < elem_number; i++)
   {
      A[i] = (DATA_TYPE)i;
      B[i] = (DATA_TYPE)3 * i;
   }

#ifdef __BAMBU_SIM__
   m_param_alloc(0, sizeof(A));
   m_param_alloc(1, sizeof(B));
#endif
   vector_copy_plus(A, B);

   return 0;
}