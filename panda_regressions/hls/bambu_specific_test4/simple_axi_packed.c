#pragma HLS_interface a m_axi direct
#pragma HLS_interface n_ptr m_axi direct
#pragma HLS_interface res m_axi direct

typedef struct __attribute__((packed, aligned(1))) s_int
{
   char padding;
   int data;
} s_int;

void maxNumbers(s_int* a, s_int* n_ptr, s_int* res)
{
   unsigned i;
   int result;
   unsigned int n = n_ptr->data;

   if(n == 0)
   {
      res->data = (int)(1 << 31);
      return;
   }
   result = a[0].data;
   for(i = 1; i < n; ++i)
      result = result < a[i].data ? a[i].data : result;
   res->data = result;
}
