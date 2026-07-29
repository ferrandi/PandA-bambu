#define N 8

int __attribute__((noinline)) sparta_smoke(int values[N])
{
   unsigned i;
#pragma omp parallel num_threads(4)
   {
#pragma omp for private(i)
      for(i = 0; i < N; ++i)
      {
         values[i] = values[i] * 2 + (int)i;
      }
   }

   int checksum = 0;
   for(i = 0; i < N; ++i)
   {
      checksum += values[i];
   }
   if(checksum != 100)
   {
      __builtin_trap();
   }
   return checksum;
}
