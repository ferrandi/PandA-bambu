#pragma HLS_interface a m_axi direct bundle=common
#pragma HLS_interface n_ptr m_axi direct bundle=common
short maxNumbers(short *a, unsigned short *n_ptr)
{
  unsigned i;
  short res;
  unsigned short n =*n_ptr;
  if(n==0) return (short)(1u << 15);
  res = a[0];
  for(i=1;i<n;++i)
    res = res < a[i] ? a[i] : res;
  return res;
}
