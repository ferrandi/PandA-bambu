#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define size 1024
#define twoPI 6.28318530717959

void fft(double real[size], double img[size], double real_twid[size], double img_twid[size]);


////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct bench_args_t {
        double real[size];
        double img[size];
        double real_twid[size];
        double img_twid[size];
};
int INPUT_SIZE = sizeof(struct bench_args_t);

void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;
  fft(args->real, args->img, args->real_twid, args->img_twid );
}

////////////////////////////////////////////////////////////////////////////////


