extern "C" int kernel(const int* din, int* dout, int n);

#pragma HLS interface port = din mode = m_axi offset = direct bundle = gmem0
#pragma HLS interface port = dout mode = m_axi offset = direct bundle = gmem1
#pragma HLS cache bundle = gmem0 line_count = 1 line_size = 8 bus_size = 256 ways = 1 num_write_outstanding = 1 rep_policy = \
    lru write_policy = wb
#pragma HLS cache bundle = gmem1 line_count = 1 line_size = 8 bus_size = 256 ways = 1 num_write_outstanding = 1 rep_policy = \
    lru write_policy = wb
extern "C" int __attribute__((noinline)) kernel(const int* din, int* dout, int n)
{
   for(int i = 0; i < n; ++i)
   {
      dout[i] = din[i] + 1;
   }
   return dout[0];
}

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

int main()
{
   const int len = 16;
   int din[len];
   int dout[len];

   for(int i = 0; i < len; ++i)
   {
      din[i] = i;
      dout[i] = 0;
   }

#ifdef __BAMBU_SIM__
   m_param_alloc(0, sizeof(din));
   m_param_alloc(1, sizeof(dout));
#endif

   kernel(din, dout, len);
   return 0;
}
