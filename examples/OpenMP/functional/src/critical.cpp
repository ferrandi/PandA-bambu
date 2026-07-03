void __attribute__((noinline)) critical(int* const a, int* const b, char n)
{
   unsigned int i;
   int total = 0;
#pragma omp parallel num_threads(4)
   {
#pragma omp for private(i)
      for(i = 0; i < n; i++)
      {
#pragma omp critical
         {
            total += a[i];
         }
      }
   }
   b[0] = total;
}