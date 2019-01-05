/**
 * Tiny MCML benchmark.
 * "Simulates light propagation from a point source in an infinite medium with isotropic scattering."
 * Original source: http://omic.ogi.edu/software/mc
 */
#define SHELL_MAX  101
#define NUMPHOTONS 16

#include <stdio.h>
#include "fixedptc.c"
#include "tiny_fixed.h"
#ifdef _OPENMP
#include <omp.h>
#endif

const fixedpt mu_a = 131072; // fixedpt_fromint(2);               /* Absorption Coefficient in 1/cm !!non-zero!! */
const fixedpt mu_s = 1310720; //fixedpt_fromint(20);               /* Reduced Scattering Coefficient in 1/cm */
const fixedpt microns_per_shell = 327680; //fixedpt_fromint(50); /* Thickness of spherical shells in microns */
//long   i;
const long photons = NUMPHOTONS;
const fixedpt albedo = 59578;
const fixedpt shells_per_mfp = 595782;
fixedpt heat[SHELL_MAX] = {0};

fixedpt get_uniform_fixed ( int *seed )
{
    int i;
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

    r = ((unsigned)(*seed & 0xFFFF));

    return r;
}

typedef struct
{
  fixedpt x, y, z, u, v, w, weight;

} photon;

void processPhoton(int seed) {

  photon p;

  p.x = 0; p.y = 0; p.z = 0;                    /*launch*/
  p.u = 0; p.v = 0; p.w = FIXEDPT_ONE;
  p.weight = FIXEDPT_ONE;
  fixedpt t, xi1, xi2;
  long shell;

  for (;;) {
    fixedpt r0 = get_uniform_fixed(&seed) + 1;
    t = -fixedpt_ln(r0);
    p.x += fixedpt_mul(t,p.u); //t * u;
    p.y += fixedpt_mul(t,p.v); //t * v;
    p.z += fixedpt_mul(t,p.w); //t * w;

    fixedpt jay = fixedpt_mul(fixedpt_sqrt(fixedpt_mul(p.x,p.x) + fixedpt_mul(p.y,p.y)+ fixedpt_mul(p.z,p.z)), shells_per_mfp);
    shell = jay >> FIXEDPT_FBITS;
    //        shell=(fixedpt_mul(fixedpt_sqrt(fixedpt_mul(x,x) + fixedpt_mul(y,y)+ fixedpt_mul(z,z)), shells_per_mfp)) >> FIXEDPT_FBITS; //sqrt(x*x+y*y+z*z)*shells_per_mfp;    /*absorb*/
    if (shell > SHELL_MAX-1) {
        shell = SHELL_MAX-1;
    }
#pragma omp atomic
    heat[shell] += fixedpt_mul(FIXEDPT_ONE-albedo, p.weight); //(1.0-albedo)*weight;
    p.weight = fixedpt_mul(p.weight, albedo);

    for(;;) {
      fixedpt r1, r2;/*new direction*/
      r1 = get_uniform_fixed(&seed);
      r2 = get_uniform_fixed(&seed);
      xi1 = (r1 << 1) - FIXEDPT_ONE;
      xi2 = (r2 << 1) - FIXEDPT_ONE;
      //if ((t=xi1*xi1+xi2*xi2)<=1) break;
      t = fixedpt_mul(xi1,xi1) + fixedpt_mul(xi2,xi2);
      if (t <= FIXEDPT_ONE)
    break;
    }
    if (t == 0)
      t = 1;
    p.u = (t << 1) - FIXEDPT_ONE; //2.0 * t - 1.0;
    fixedpt temp = fixedpt_sqrt(fixedpt_div(FIXEDPT_ONE-fixedpt_mul(p.u,p.u),t));
    p.v = fixedpt_mul(xi1, temp); // xi1 * sqrt((1-u*u)/t);
    p.w = fixedpt_mul(xi2, temp); // xi2 * sqrt((1-u*u)/t);

    if (p.weight < 66){  // 66 = 0.001 in fixedpt                     /*roulette*/
    if (get_uniform_fixed(&seed) > 6554) break;  // 6554 = 0.1 in fixedpt
      p.weight = fixedpt_div(p.weight, 6554); // /= 0.1;
    }
  }
}

void process() {
#pragma omp parallel
{
#ifdef _OPENMP
  const int n_threads = omp_get_num_threads();
  const int max = photons / n_threads;
  const int thread_id = omp_get_thread_num();
  const int offset = thread_id * max;
#else
  const int max = photons;
  const int offset = 0;
#endif
  int i;
  for (i = 0; i < max; i++)
  {
    processPhoton(seeds[i+offset]);
  }
}
}

int main ()
{
  int sum=0;
  process();
  for (int i=0;i<SHELL_MAX-1;i++) {
    //    fixedpt_print(heat[i]);
    sum ^= heat[i]; // janders -- check correctness by XOR'ing all the values
  }
  printf("Result: %d\n", sum);
  int res = 0;
  if (sum == 56060)
  {
    printf("RESULT: PASS\n");
  }
  else
  {
    printf("RESULT: FAIL\n");
    res = 1;
  }
  return res;
}
