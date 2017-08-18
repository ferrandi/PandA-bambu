#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>

#define NUM_ACCEL 1
#define RUN 1
#define OPS_PER_ACCEL RUN/NUM_ACCEL

#include "black_scholes.h"
#include "fixedptc.h"
#include "fixedptc.c"
#include "black_scholes.c"
#include "seed.h"

fixedpt asset_path_test ( int seed );

//pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
//int dummySeed = 123;

struct thread_data
{
   int  startidx;
   int  maxidx;
};

inline fixedpt black_scholes(int startidx) __attribute__((always_inline));
fixedpt black_scholes(int startidx)
{

   int i;
   fixedpt sum = 0;
   fixedpt u;
   for(i=0; i < OPS_PER_ACCEL; i++)
   {
      fixedpt seed = seeds[i+startidx];
      u = asset_path_test (seed);
      result[i] = u;
      sum += (u >> FIXEDPT_BITS - FIXEDPT_WBITS);
   }
   return sum;
}

int main ( void )
{

   int run = RUN;
   int i;
   fixedpt sum = 0;
   //create the thread variables
   fixedpt result[NUM_ACCEL] = {0};

#pragma omp simd
   for (i = 0; i < NUM_ACCEL; i++)
   {
      result[i] = black_scholes(i * OPS_PER_ACCEL);
   }


   for (i = 0; i < NUM_ACCEL; i++)
   {
      sum += result[i];
   }

   printf("sum = %d\n", sum); // the golden result will be the SUM of the prices
   if (sum == 10752)
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

inline fixedpt asset_path_test (int seed)  __attribute__((always_inline));
fixedpt asset_path_test (int seed )
{

   int n = 1;
   fixedpt mu, s0, sigma, t1;
   fixedpt s;
   //int holdSeed = seed;

   s0 = 13107200; // fixedpt_rconst(200.0);
//        printf("%d\n", s0>> (FIXEDPT_BITS - FIXEDPT_WBITS));

   mu = 16384; // fixedpt_rconst(0.25);
//        printf("%d\n", mu>> (FIXEDPT_BITS - FIXEDPT_WBITS));

   sigma = 4391; //fixedpt_rconst(0.067);
//        printf("%d\n", sigma>> (FIXEDPT_BITS - FIXEDPT_WBITS));

   t1 = 131072; // fixedpt_rconst(2.0);
//        printf("%d\n", t1>> (FIXEDPT_BITS - FIXEDPT_WBITS));

   s = asset_path_fixed_simplified ( s0, mu, sigma, t1, n, &seed);

   return s;
}
/******************************************************************************/


