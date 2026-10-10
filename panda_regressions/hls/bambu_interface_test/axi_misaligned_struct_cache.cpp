struct __attribute__((packed)) MisalignedRecord
{
   unsigned char pad0;
   unsigned short a;
   unsigned int b;
   unsigned char c;
};

#pragma HLS interface port = records mode = m_axi offset = direct bundle = gmem0
#pragma HLS cache bundle = gmem0 line_count = 16 line_size = 16 bus_size = 64 ways = 1 num_write_outstanding = 2 rep_policy = \
    lru write_policy = wt
extern "C" int __attribute__((noinline)) misaligned_struct_axi(MisalignedRecord* records, int n)
{
   unsigned int sum = 0;

   for(int i = 0; i < n; ++i)
   {
      sum += records[i].a;
      sum += records[i].b;
      sum += records[i].c;
   }

   return (int)sum;
}

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

int main()
{
   const int len = 4;
   MisalignedRecord records[len];
   unsigned int expected = 0;

   for(int i = 0; i < len; ++i)
   {
      records[i].pad0 = (unsigned char)i;
      records[i].a = (unsigned short)(0x100u + i);
      records[i].b = 0xABC00000u + (unsigned int)i;
      records[i].c = (unsigned char)(0x20u + i);
      expected += records[i].a + records[i].b + records[i].c;
   }

#ifdef __BAMBU_SIM__
   m_param_alloc(0, sizeof(records));
#endif

   return (unsigned int)misaligned_struct_axi(records, len) != expected;
}
