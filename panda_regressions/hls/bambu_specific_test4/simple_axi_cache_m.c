#pragma HLS_interface a m_axi direct
#pragma HLS_interface n_ptr m_axi direct
#pragma HLS_interface res m_axi direct

int minMaxNumbers(int* a, unsigned int* n_ptr, int* res)

{
   unsigned i;
   int min, max;
   unsigned int n = *n_ptr;

   if(n == 0)
   {
      *res = (int)(1 << 31);
      return (int)((1 << 31) - 1);
   }
   min = max = a[0];
   for(i = 1; i < n; ++i)
      max = max < a[i] ? a[i] : max;
   *res = max;
   for(i = 1; i < n; ++i)
      min = min > a[i] ? a[i] : min;
   return min;
}