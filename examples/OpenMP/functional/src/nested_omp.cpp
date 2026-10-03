void __attribute__((noinline)) vector_op(float* const a, float* const b, float* const c, char n)
{
   unsigned i, j;
#pragma omp parallel num_threads(4)
   {
#pragma omp for private(i)
      for(i = 0; i < n; i++)
      {
#pragma omp parallel num_threads(2)
         {
#pragma omp for private(j)
            for(j = 0; j < n; j++)
            {
               c[i * n + j] = a[i * n + j] + b[i * n + j];
            }
         }
#pragma omp parallel num_threads(2)
         {
#pragma omp for private(j)
            for(j = 0; j < n; j++)
            {
               c[i * n + j] += a[i * n + j] * b[i * n + j];
            }
         }
      }
   }
}
