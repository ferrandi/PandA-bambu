#pragma HLS interface port = a mode = bus
#pragma HLS interface port = b mode = bus
#pragma HLS interface port = c mode = bus
#pragma HLS interface port = d mode = bus
#pragma HLS bus mode = m_axi
void __attribute__((noinline)) vector_add(int* const a, int* const b, int* const c, int* const d, char n)
{
#pragma omp parallel num_threads(2)
   {
#pragma omp sections
      {
#pragma omp section
         for(int i = 0; i < n; i++)
         {
            c[i] += a[i] + b[i];
         }
#pragma omp section
         for(int j = 0; j < n; j++)
         {
            d[j] += a[j] + b[j];
         }
      }
   }
}
