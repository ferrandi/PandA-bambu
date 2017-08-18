#define NUM_ACCELS 5
#define ARRAY_SIZE 10000
#define OPS_PER_ACCEL ARRAY_SIZE/NUM_ACCELS

#include <stdio.h>
#include "add.h"

int output[NUM_ACCELS];

void add (int accelnum, int startidx, int endidx)
{
   int sum = 0;
   int i;
   for (i = startidx; i < endidx; i++)
   {
      sum += array[i];
   }
   output[accelnum] = sum;
}


int
main ()
{
   int sum = 0;
   int i;

   #pragma omp parallel for num_threads(NUM_ACCELS) private(i)
   for (i = 0; i < NUM_ACCELS; i++)
   {
      add(i, i * OPS_PER_ACCEL, (i + 1)*OPS_PER_ACCEL);
   }

   //combine results
   for (i = 0; i < NUM_ACCELS; i++)
   {
      sum += output[i];
   }

   //check result
   printf ("Result: %d\n", sum);
   if (sum == 55000)
   {
      printf("RESULT: PASS\n");
   }
   else
   {
      printf("RESULT: FAIL\n");
   }

   return 0;
}
