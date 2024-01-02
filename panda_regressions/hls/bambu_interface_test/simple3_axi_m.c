#pragma HLS interface port = a mode = m_axi offset = direct bundle = gmem0
#pragma HLS interface port = n_ptr mode = m_axi offset = direct bundle = gmem1
short maxNumbers(short* a, unsigned short* n_ptr)
{
   unsigned i;
   short res;
   unsigned short n = *n_ptr;
   if(n == 0)
      return (short)(1u << 15);
   res = a[0];
   for(i = 1; i < n; ++i)
      res = res < a[i] ? a[i] : res;
   return res;
}
