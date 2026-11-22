#include <omp.h>

#define N 4

void kernel(int *a, int *b, int *c, int i, int j, int k)
{
   c[i * N + j] += a[i * N + k] * b[k * N + j];
}

__attribute__((noinline)) void kernel_wrapper(int *a, int *b, int *c, int i, int j, int k)
{
   kernel(a, b, c, i, j, k);
}

void gemm(int *a, int *b, int *c)
{
   int i = 0, j = 0, k = 0;
#pragma omp parallel for private(j, k)
   for (i = 0; i < N; i++)
   {
      for (j = 0; j < N; j++)
      {
         for (k = 0; k < N; k++)
         {
            kernel_wrapper(a, b, c, i, j, k);
         }
      }
   }
}
