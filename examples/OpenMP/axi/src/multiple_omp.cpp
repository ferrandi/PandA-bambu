void __attribute__((noinline))
#pragma HLS interface port = a mode = bus
#pragma HLS interface port = b mode = bus
#pragma HLS interface port = c mode = bus
#pragma HLS interface port = d mode = bus
#pragma HLS interface port = e mode = bus
#pragma HLS interface port = f mode = bus
#pragma HLS bus mode = m_axi
vector_add(float* const a, float* const b, float* const c, float* const d, float* const e, float* const f, char n)
{
   unsigned i;
#pragma omp parallel num_threads(4)
   {
#pragma omp for private(i)
      for(i = 0; i < n; i++)
      {
         c[i] += a[i] + b[i];
      }
   }

#pragma omp parallel num_threads(4)
   {
#pragma omp for private(i)
      for(i = 0; i < n; i++)
      {
         f[i] += d[i] + e[i];
      }
   }
}
