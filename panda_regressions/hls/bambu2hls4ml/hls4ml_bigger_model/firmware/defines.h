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
typedef nnet::array<ap_fixed<16,6>, 8*1> input_t;
typedef nnet::array<ap_fixed<16,6>, 8*1> layer4_t;
typedef ap_fixed<16,6> model_default_t;
typedef nnet::array<ap_fixed<16,6>, 8*1> result_t;

// hls-fpga-machine-learning insert emulator-defines


#endif
