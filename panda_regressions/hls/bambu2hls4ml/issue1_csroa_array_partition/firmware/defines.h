#ifndef DEFINES_H_
#define DEFINES_H_

#include "ap_fixed.h"
#include "ap_int.h"
#include "nnet_utils/nnet_types.h"
#include <array>
#include <cstddef>
#include <cstdio>
#include <tuple>
#include <tuple>


// hls-fpga-machine-learning insert numbers

// hls-fpga-machine-learning insert layer-precision
typedef ap_fixed<16,6> input_t;
typedef ap_fixed<37,17> fc1_accum_t;
typedef ap_fixed<37,17> fc1_result_t;
typedef ap_fixed<16,6> fc1_weight_t;
typedef ap_fixed<16,6> fc1_bias_t;
typedef ap_uint<1> layer2_index;
typedef ap_fixed<16,6> result_t;
typedef ap_fixed<18,8> relu1_table_t;

// hls-fpga-machine-learning insert emulator-defines


#endif
