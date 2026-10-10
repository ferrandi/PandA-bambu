#ifndef MYPROJECT_H_
#define MYPROJECT_H_

#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_stream.h"

#include "defines.h"


// Prototype of top level function for C-synthesis
void myproject(
    hls::stream<input_t> &max_pooling1d_1_input,
    hls::stream<result_t> &layer2_out
);

// hls-fpga-machine-learning insert emulator-defines


#endif
