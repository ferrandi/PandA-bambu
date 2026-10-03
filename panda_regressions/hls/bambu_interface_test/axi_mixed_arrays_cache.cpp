#pragma HLS interface port = bytes mode = m_axi offset = direct bundle = gmem0
#pragma HLS interface port = values mode = m_axi offset = direct bundle = gmem0
#if defined(USE_CACHE)
#pragma HLS cache bundle = gmem0 line_count = 16 line_size = 16 bus_size = 64 ways = 1 num_write_outstanding = 2 rep_policy = \
    lru write_policy = wt
#endif
extern "C" int __attribute__((noinline)) mixed_arrays_axi(char* bytes, int* values, int n_bytes, int n_values)
{
   int sum = 0;

   for(int i = 0; i < n_bytes; ++i)
   {
      sum += bytes[i];
   }

   for(int i = 0; i < n_values; ++i)
   {
      sum += values[i];
   }

   return sum;
}

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

int main()
{
   const int byte_len = 9;
   const int value_len = 5;
   char bytes[byte_len];
   int values[value_len];
   int expected = 0;

   for(int i = 0; i < byte_len; ++i)
   {
      bytes[i] = (char)(i + 1);
      expected += bytes[i];
   }

   for(int i = 0; i < value_len; ++i)
   {
      values[i] = 0x1000 + i;
      expected += values[i];
   }

#ifdef __BAMBU_SIM__
   m_param_alloc(0, sizeof(bytes));
   m_param_alloc(1, sizeof(values));
#endif

   return mixed_arrays_axi(bytes, values, byte_len, value_len) != expected;
}
