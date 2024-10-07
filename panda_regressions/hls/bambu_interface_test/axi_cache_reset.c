#ifndef DATA_TYPE
#define DATA_TYPE int
#endif

#pragma HLS interface port = a mode = m_axi offset = direct bundle = gmem0

#pragma HLS cache bundle = gmem0 line_count = 16 line_size = 16 bus_size = 128 ways = 1 num_write_outstanding = 2 rep_policy = \
    lru write_policy = wt
void __attribute__((noinline)) write_plus(int* a)
{
   for(int i = 0; i < 20; i++)
   {
      a[i] = a[i] + 1;
   }
}

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

int main()
{
   DATA_TYPE A[20];

   for(int i = 0; i < 20; i++)
   {
      A[i] = (DATA_TYPE)i;
   }

#ifdef __BAMBU_SIM__
   m_param_alloc(0, sizeof(A));
#endif
   write_plus(A);
   A[0] = 100;
   A[10] = 25;
   A[19] = 0;
   write_plus(A);
   return 0;
}