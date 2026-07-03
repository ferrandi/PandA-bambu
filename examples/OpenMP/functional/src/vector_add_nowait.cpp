void __attribute__((noinline)) vector_add(int* const a, int* const b, int* const c, char n)
{
   unsigned i;
#pragma omp parallel num_threads(5)
   {
#pragma omp for nowait private(i)
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
