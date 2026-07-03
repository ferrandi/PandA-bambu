#ifndef NTHREADS
#define NTHREADS 4
#endif

#ifndef MAX_ITERS
#define MAX_ITERS 4
#endif

float __attribute__((noinline)) nested_red(float* const a, float* b, int n)
{
   float sum = 0;
   int i;
   for(int j = 0; j < MAX_ITERS; j = j + 1)
   {
#pragma omp parallel num_threads(NTHREADS)
      {
#pragma omp for private(i) reduction(+ : sum)
         for(i = 0; i < n; i++)
         {
            sum += a[i];
         }
      }
   }
   return sum;
}
