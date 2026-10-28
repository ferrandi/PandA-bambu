#ifndef NTHREADS
#define NTHREADS 4
#endif

#pragma HLS interface port = a mode = bus
#pragma HLS interface port = b mode = bus
#pragma HLS bus mode = m_axi
long __attribute__((noinline)) red(int* a, int* b, int n)
{
   long sum = b[0];
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