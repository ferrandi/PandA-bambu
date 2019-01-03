#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#define RUN 32

#include "black_scholes.h"
#include "fixedptc.h"

fixedpt asset_path_test(int seed)
{
  int n = 100;
  fixedpt s0 = 13107200; // fixedpt_rconst(200.0);
  fixedpt mu = 16384; // fixedpt_rconst(0.25);
  fixedpt sigma = 4391; //fixedpt_rconst(0.067);
  fixedpt t1 = 131072; // fixedpt_rconst(2.0);
  return asset_path_fixed_simplified(s0, mu, sigma, t1, n, &seed);
}

fixedpt black_scholes()
{
  int run = RUN;
  fixedpt sum = 0;
  const fixedpt seeds[RUN] = {
    35645,
    20955,
    3231,
    55398,
    11191,
    7730,
    41421,
    52573,
    46468,
    4465,
    20791,
    64790,
    59818,
    42394,
    12807,
    36702,
    28255,
    17195,
    64055,
    15204,
    12488,
    45439,
    10215,
    52185,
    12464,
    30711,
    11892,
    54818,
    28299,
    29363,
    22002,
    49795,
  };
#pragma omp parallel for reduction(+:sum)
  for (int i = 0; i < run; i++) {
    fixedpt u = asset_path_test(seeds[i]);
    sum += (u >> FIXEDPT_BITS - FIXEDPT_WBITS);
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

