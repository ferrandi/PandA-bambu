#define NUM_ACCELS 5
#define ARRAY_SIZE 10000
#define OPS_PER_ACCEL ARRAY_SIZE/NUM_ACCELS

#include <stdio.h>
#include "add.h"

int output[NUM_ACCELS];

__attribute__((noinline))
void add (int accelnum)
{
  int startidx = accelnum * OPS_PER_ACCEL;
  int endidx = (accelnum+1) * OPS_PER_ACCEL;
    int sum=0;
    int i;
    for (i=startidx; i<endidx; i++)
    {
      sum += array[i];
    }
    output[accelnum] = sum;
}

__attribute__((noinline))
void add_for(int start, int end)
{
  int i;
  #pragma omp parallel for num_threads(NUM_ACCELS) private(i)
  for (i=start; i < end; i++) {
    add(i);
  }
}

int
main ()
{
    int sum=0;
    int i;

    add_for(0, NUM_ACCELS);
     
    //combine results
    for (i=0; i<NUM_ACCELS; i++) {
        sum += output[i];
    }

    return sum;
}
