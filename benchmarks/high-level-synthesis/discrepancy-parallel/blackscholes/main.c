#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#define RUN 32

#include "black_scholes.h"
#include "fixedptc.h"
#include "seed.h"

fixedpt asset_path_test(int seed)
{
  int n = 100;
  fixedpt s0 = 13107200; // fixedpt_rconst(200.0);
  fixedpt mu = 16384; // fixedpt_rconst(0.25);
  fixedpt sigma = 4391; //fixedpt_rconst(0.067);
  fixedpt t1 = 131072; // fixedpt_rconst(2.0);
  fixedpt s = asset_path_fixed_simplified(s0, mu, sigma, t1, n, &seed);
  return s;
}

fixedpt black_scholes()
{
  int run = RUN;
  fixedpt sum = 0;
  for (int i = 0; i < run; i++) {
    fixedpt u;
    fixedpt seed = seeds[i];
    u = asset_path_test (seed);
    result[i] = u;
    sum += (u >> (FIXEDPT_BITS - FIXEDPT_WBITS));
  }
  return sum;
}

int main ( void )
{

  int ret = 0;
  fixedpt sum = black_scholes();
  printf("sum = %d\n", sum); // the golden result will be the SUM of the prices
  if (sum == 10752) {
    printf("RESULT: PASS\n");
  } else {
    printf("RESULT: FAIL\n");
    ret = 1;
  }

  return ret;
}

