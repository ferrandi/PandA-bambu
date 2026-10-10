float __attribute__((noinline)) multiple_red(float* const a, float* b, unsigned int n)
{
   float sum = b[0];
   float sum1 = 2;
   int i;
#pragma omp parallel num_threads(4)
   {
#pragma omp for private(i) reduction(+ : sum, sum1)
      for(i = 0; i < n; i++)
      {
         sum += a[i];
         sum1 += a[i];
      }
   }
   return sum + sum1;
}