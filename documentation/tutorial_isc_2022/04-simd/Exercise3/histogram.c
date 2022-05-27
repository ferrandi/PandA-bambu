#define NUM_ACCELS 8
#define MAX_NUM 100
#define NUM_BINS 5
#define BIN_MAX_NUM MAX_NUM/NUM_BINS
//#define ARRAY_SIZE 60
//#define ARRAY_SIZE 120
//#define ARRAY_SIZE 6000
#define ARRAY_SIZE 36000
#define OPS_PER_ACCEL ARRAY_SIZE/NUM_ACCELS

#include <stdio.h>
#include "histogram.h"

void histogram (int * input, int * output, int max_idx)
{
   int i, num;
   int temp0 = 0, temp1 = 0, temp2 = 0, temp3 = 0, temp4 = 0, temp5 = 0;
   //make local variables here instead of reading and writing to global memory each time???
   for (i = 0; i < max_idx; i++)
   {
//    printf ("%d\n", input[startidx]);
      num = input[i];
      if (num > 0 && num <= BIN_MAX_NUM)
      {
//       output[0]+=1;
         temp0 += 1;
      }
      else if (num > BIN_MAX_NUM && num <= (BIN_MAX_NUM * 2))
      {
//       output[1]+=1;
         temp1 += 1;
      }
      else if (num > (BIN_MAX_NUM * 2) && num <= (BIN_MAX_NUM * 3))
      {
//       output[2]+=1;
         temp2 += 1;
      }
      else if (num > (BIN_MAX_NUM * 3) && num <= (BIN_MAX_NUM * 4))
      {
//       output[3]+=1;
         temp3 += 1;
      }
      else
      {
//       output[4]+=1;
         temp4 += 1;
      }
   }
   printf("%d\n", temp0);
   output[0] += temp0;
   output[1] += temp1;
   output[2] += temp2;
   output[3] += temp3;
   output[4] += temp4;

}

int main ()
{

   int i, j;
   int main_result = 0;

   #pragma omp simd
   for (i = 0; i < NUM_ACCELS; i++)
   {
      histogram(input_array + OPS_PER_ACCEL * i, output_array_accel + i * NUM_BINS, OPS_PER_ACCEL);
   }

   //could have been done with locks inside accelerator
   //combine the results
   for (i = 0; i < NUM_BINS; i++)
   {
      for (j = 0; j < NUM_ACCELS; j++)
      {
         output_array[i] += output_array_accel[i + j * NUM_BINS];
      }
   }

   //check the results
   for (i = 0; i < NUM_BINS; i++)
   {
      main_result += (output_array[i] == expected_array[i]);
   }

   //check final result
   printf ("Result: %d\n", main_result);
   if (main_result == NUM_BINS)
   {
      printf("RESULT: PASS\n");
      return 0;
   }
   else
   {
      printf("RESULT: FAIL\n");
      return 1;
   }
}
