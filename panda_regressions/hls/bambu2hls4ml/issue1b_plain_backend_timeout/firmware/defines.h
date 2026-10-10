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
typedef ap_fixed<16,6> layer3_t;
typedef ap_fixed<18,8> fc1_relu_table_t;
typedef ap_fixed<39,19> fc2_accum_t;
typedef ap_fixed<39,19> fc2_result_t;
typedef ap_fixed<16,6> fc2_weight_t;
typedef ap_fixed<16,6> fc2_bias_t;
typedef ap_uint<1> layer4_index;
typedef ap_fixed<16,6> layer5_t;
typedef ap_fixed<18,8> fc2_relu_table_t;
typedef ap_fixed<38,18> fc3_accum_t;
typedef ap_fixed<38,18> fc3_result_t;
typedef ap_fixed<16,6> fc3_weight_t;
typedef ap_fixed<16,6> fc3_bias_t;
typedef ap_uint<1> layer6_index;
typedef ap_fixed<16,6> layer7_t;
typedef ap_fixed<18,8> fc3_relu_table_t;
typedef ap_fixed<38,18> output_accum_t;
typedef ap_fixed<38,18> result_t;
typedef ap_fixed<16,6> output_weight_t;
typedef ap_fixed<16,6> output_bias_t;
typedef ap_uint<1> layer8_index;

// hls-fpga-machine-learning insert emulator-defines


#endif
