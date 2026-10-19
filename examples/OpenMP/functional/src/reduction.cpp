#ifndef NTHREADS
#define NTHREADS 4
#endif

float __attribute__((noinline)) red(float* const a, float* b, int n)
{
   float sum = b[0];
   int i;
#pragma omp parallel num_threads(NTHREADS)
   {
#pragma omp for private(i) reduction(+ : sum)
      for(i = 0; i < n; i++)
      {
         sum += a[i];
      }
   }
   return sum;
}
