void __attribute__((noinline))
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
