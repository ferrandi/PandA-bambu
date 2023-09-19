#pragma HLS_interface a m_axi direct
#pragma HLS_interface n_ptr m_axi direct
#pragma HLS_interface res m_axi direct

void maxNumbers(int* a, unsigned int* n_ptr, int* res)

{
   unsigned i;
   int result;
   unsigned int n = *n_ptr;

   if(n == 0)
   {
      *res = (int)(1 << 31);
      return;
   }
   result = a[0];
   for(i = 1; i < n; ++i)
      result = result < a[i] ? a[i] : result;
   *res = result;
}