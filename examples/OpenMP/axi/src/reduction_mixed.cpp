#ifndef NTHREADS
#define NTHREADS 4
#endif

#pragma HLS interface port = a mode = bus
#pragma HLS interface port = b mode = bus
#pragma HLS bus mode = m_axi
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
