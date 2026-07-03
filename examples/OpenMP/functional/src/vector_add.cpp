#ifndef NTHREADS
#define NTHREADS 4
#endif

#define N 20
void __attribute__((noinline)) vector_add(float a[N], float b[N], float c[N])
{
   unsigned i;
#pragma omp parallel num_threads(NTHREADS)
   {
#pragma omp for private(i)
      for(i = 0; i < N; i++)
      {
         c[i] += a[i] + b[i];
      }
   }
}
