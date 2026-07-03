#ifndef NTHREADS
#define NTHREADS 4
#endif

int __attribute__((noinline)) red(int* const a, int* b)
{
   int sum = 0;
   char pre = 0;
   char post = 0;
   int i;
#pragma omp parallel num_threads(NTHREADS)
   {
#pragma omp for private(i) reduction(+ : pre, sum, post)
      for(i = 0; i < 5; i++)
      {
         pre += a[i];
         sum += b[i];
         post += a[i];
      }
   }
   return pre + sum + post;
}
