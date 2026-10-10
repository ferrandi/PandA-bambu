#ifndef MYPROJECT_FLOAT_H_
#define MYPROJECT_FLOAT_H_

#include "ac_int.h"
// hls-fpga-machine-learning insert float-includes

// hls-fpga-machine-learning insert definitions
#define IN_CONTAINER_WIDTH  16
#define OUT_CONTAINER_WIDTH 16
typedef ac_int<IN_CONTAINER_WIDTH,  false> in_container_t;
typedef ac_int<OUT_CONTAINER_WIDTH, false> out_container_t;
static const unsigned N_IN  = 16;
static const unsigned N_OUT = 16;

// hls-fpga-machine-learning insert float-signature
void myproject_float(in_container_t input[N_IN], out_container_t output[N_OUT]);

#endif
