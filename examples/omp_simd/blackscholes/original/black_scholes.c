# include "black_scholes.h"
# include "fixedptc.h"


fixedpt asset_path_fixed_simplified ( fixedpt s0, fixedpt mu, fixedpt sigma, fixedpt t1, int n, int *seed ){
    int i;
    fixedpt dt, stepnum, p;
    fixedpt gaussR1, gaussR2;

    //    stepnum = fixedpt_rconst(n);
    stepnum = n << FIXEDPT_FBITS; // ??? janders
    dt = fixedpt_div(t1, stepnum);

    fixedpt constA = fixedpt_mul(fixedpt_sub(mu, fixedpt_mul(sigma, sigma)), dt);
    fixedpt constB = fixedpt_mul(sigma, fixedpt_sqrt ( dt ));

    p = s0;
    for ( i = 1; i <= n; i++ )
    {  
      if (i & 1) // iteration is odd, generate two random Gaussian numbers (the Box-Muller transform gens 2 numbers)
	get_two_normal_fixed(seed, &gaussR1, &gaussR2);
      
      p = fixedpt_mul(p, fixedpt_exp (fixedpt_add(constA,
						  fixedpt_mul(constB, i & 1 ? gaussR1 : gaussR2))));
      

      //      fixedpt_print(p);
    }
    return p;
}

void get_two_normal_fixed(int *seed, fixedpt *n1, fixedpt *n2) 
{
  fixedpt r1, r2;

  fixedpt twoPI = 411775; // ??? janders -- hard-code 2PI in fixed point to avoid conversion from double
  // from 2 uniform random numbers r1 and r2, we will generate two Gaussian random numbers deposited into n1 and n2
  r1 = get_uniform_fixed (seed);
  r2 = get_uniform_fixed (seed);
  
  *n1 = fixedpt_mul(fixedpt_sqrt ( fixedpt_mul(-1*FIXEDPT_TWO , fixedpt_ln( r1 )) ), fixedpt_cos(fixedpt_mul( twoPI , r2)));
  *n2 = fixedpt_mul(fixedpt_sqrt ( fixedpt_mul(-1*FIXEDPT_TWO , fixedpt_ln( r1)) ), fixedpt_sin (fixedpt_mul( twoPI , r2)));
}

fixedpt get_uniform_fixed ( int *seed )

{
    int i4_huge = 2147483647;
    int k;
    fixedpt r;

    k = *seed / 127773;
    // printf("k = %i \n", k);
    
    *seed = 16807 * ( *seed - k * 127773 ) - k * 2836;
    //printf("*seed = %i \n", *seed);
    
    if ( *seed < 0 )
    {
      *seed = *seed + i4_huge;
    }

    r = *seed & 0x0000FFFF;

    return r;
}
/******************************************************************************/
