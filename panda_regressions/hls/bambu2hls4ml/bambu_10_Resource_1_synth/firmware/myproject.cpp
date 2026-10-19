#include <iostream>

#include "myproject.h"
#include "parameters.h"


void myproject(
    hls::stream<input_t> &conv1d_input,
    hls::stream<result_t> &layer2_out
) {

    // hls-fpga-machine-learning insert IO
    #pragma HLS interface mode=axis port=conv1d_input,layer2_out 
    #pragma HLS DATAFLOW

    // hls-fpga-machine-learning insert load weights
#ifndef __BAMBU__
    static bool loaded_weights = false;
    if (!loaded_weights) {
        nnet::load_weights_from_txt<model_default_t, 12>(w2, "w2.txt");
        nnet::load_weights_from_txt<bias2_t, 1>(b2, "b2.txt");
        loaded_weights = true;    }
#endif
    // ****************************************
    // NETWORK INSTANTIATION
    // ****************************************

    // hls-fpga-machine-learning insert layers

    hls::stream<layer4_t> layer4_out("layer4_out");
    //#pragma HLS STREAM variable=layer4_out depth=18

    nnet::zeropad1d_cl<input_t, layer4_t, config4>(conv1d_input, layer4_out); // zp1d_conv1d

    nnet::conv_1d_cl<layer4_t, result_t, config2>(layer4_out, layer2_out, w2, b2); // conv1d

}
