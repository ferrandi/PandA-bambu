#include <iostream>

#include "myproject.h"
#include "parameters.h"


void myproject(
    hls::stream<input_t> &max_pooling1d_1_input,
    hls::stream<result_t> &layer2_out
) {

    // hls-fpga-machine-learning insert IO
    #pragma HLS interface mode=axis port=max_pooling1d_1_input
    #pragma HLS interface mode=axis port=layer2_out
    #pragma HLS DATAFLOW

    // hls-fpga-machine-learning insert load weights
#ifndef __BAMBU__
    static bool loaded_weights = false;
    if (!loaded_weights) {
        loaded_weights = true;    }
#endif
    // ****************************************
    // NETWORK INSTANTIATION
    // ****************************************

    // hls-fpga-machine-learning insert layers

    nnet::pooling1d_cl<input_t, result_t, config2>(max_pooling1d_1_input, layer2_out); // max_pooling1d_1

}

