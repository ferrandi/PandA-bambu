#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#define RUN 32

#include "black_scholes.h"
#include "fixedptc.h"
#include "fixedptc.c"
#include "black_scholes.c"
#include "seed.h"

fixedpt asset_path_test ( int seed );

fixedpt black_scholes() {
    
    int run = RUN;
    //int dummySeed = 123;
    int i;
    fixedpt sum = 0;
    for (i =0; i<run; i++) {
      fixedpt u;
      //fixedpt seed = get_uniform_fixed(&dummySeed); 
      //printf("%d\n", seed);
      fixedpt seed = seeds[i];
      u = asset_path_test (seed);
      //printf("seed = %d, u = %d\n", seed, u >> (FIXEDPT_BITS - FIXEDPT_WBITS));
      result[i] = u;           
      sum += (u >> FIXEDPT_BITS - FIXEDPT_WBITS);
    }
    
    return sum;
}

int main ( void ){

    legup_start_counter(0);
    fixedpt sum=0;
    sum = black_scholes();

    printf("sum = %d\n", sum); // the golden result will be the SUM of the prices

    int perf_counter = legup_stop_counter(0);
    printf("perf_counter = %d\n", perf_counter);
    if (sum == 10752) {
      printf("RESULT: PASS\n");
    }
    else {
      printf("RESULT: FAIL\n");
    }

    return (int)sum;
}

fixedpt asset_path_test (int seed ){
    
    int n = 100;
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


