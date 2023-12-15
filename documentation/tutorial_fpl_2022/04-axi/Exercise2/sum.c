#pragma HLS interface port = v mode = m_axi offset = direct
#pragma HLS interface port = n mode = m_axi offset = direct

int sum(int* v, unsigned* n)
{
   int sum = 0;

   for(unsigned i = 0; i < *(n); i++)
   {
      sum += v[i];
   }

   return sum;
}