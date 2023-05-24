#pragma HLS_interface v m_axi direct
#pragma HLS_interface n m_axi direct

int sum(int* v, unsigned* n)
{
   int sum = 0;

   for(unsigned i = 0; i < *(n); i++)
   {
      sum += v[i];
   }

   return sum;
}