#ifndef MYPROJECT_FLOAT_H_
#define MYPROJECT_FLOAT_H_

#include "ac_int.h"
// hls-fpga-machine-learning insert float-includes
#include "hls_stream.h"

// hls-fpga-machine-learning insert definitions
#define IN_CONTAINER_WIDTH  16
#define OUT_CONTAINER_WIDTH 64
typedef ac_int<IN_CONTAINER_WIDTH,  false> in_container_t;
typedef ac_int<OUT_CONTAINER_WIDTH, false> out_container_t;
static const unsigned N_IN  = 2;
static const unsigned N_OUT = 2;

// hls-fpga-machine-learning insert float-signature
void myproject_float(hls::stream<in_container_t> &input_stream, hls::stream<out_container_t> &output_stream);

#endif
