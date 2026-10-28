#pragma HLS interface port = a mode = bus
#pragma HLS interface port = b mode = bus
#pragma HLS interface port = c mode = bus
#pragma HLS bus mode = m_axi
void __attribute__((noinline)) vector_add(int* const a, int* const b, int* const c, char n)
{
   unsigned i;
#pragma omp parallel num_threads(4)
   {
#pragma omp for private(i)
      for(i = 0; i < n; i++)
      {
         c[i] += a[i] + b[i];
      }
#pragma omp for private(i)
      for(i = 0; i < n; i++)
      {
         c[i] += a[i] + b[i];
      }
   }
}
