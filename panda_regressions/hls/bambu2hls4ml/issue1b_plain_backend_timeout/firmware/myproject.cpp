#include <iostream>

#include "myproject.h"
#include "parameters.h"


void myproject(
    input_t fc1_input[16],
    result_t layer8_out[5]
) {

    // hls-fpga-machine-learning insert IO
    #pragma HLS ARRAY_PARTITION variable=fc1_input complete dim=0
    #pragma HLS ARRAY_PARTITION variable=layer8_out complete dim=0
    //#pragma HLS PIPELINE

    // hls-fpga-machine-learning insert load weights
#ifndef __BAMBU__
    static bool loaded_weights = false;
    if (!loaded_weights) {
        nnet::load_weights_from_txt<fc1_weight_t, 1024>(w2, "w2.txt");
        nnet::load_weights_from_txt<fc1_bias_t, 64>(b2, "b2.txt");
        nnet::load_weights_from_txt<fc2_weight_t, 2048>(w4, "w4.txt");
        nnet::load_weights_from_txt<fc2_bias_t, 32>(b4, "b4.txt");
        nnet::load_weights_from_txt<fc3_weight_t, 1024>(w6, "w6.txt");
        nnet::load_weights_from_txt<fc3_bias_t, 32>(b6, "b6.txt");
        nnet::load_weights_from_txt<output_weight_t, 160>(w8, "w8.txt");
        nnet::load_weights_from_txt<output_bias_t, 5>(b8, "b8.txt");
        loaded_weights = true;    }
#endif
    // ****************************************
    // NETWORK INSTANTIATION
    // ****************************************

    // hls-fpga-machine-learning insert layers

    fc1_result_t layer2_out[64];
    #pragma HLS ARRAY_PARTITION variable=layer2_out complete dim=0

    layer3_t layer3_out[64];
    #pragma HLS ARRAY_PARTITION variable=layer3_out complete dim=0

    fc2_result_t layer4_out[32];
    #pragma HLS ARRAY_PARTITION variable=layer4_out complete dim=0

    layer5_t layer5_out[32];
    #pragma HLS ARRAY_PARTITION variable=layer5_out complete dim=0

    fc3_result_t layer6_out[32];
    #pragma HLS ARRAY_PARTITION variable=layer6_out complete dim=0

    layer7_t layer7_out[32];
    #pragma HLS ARRAY_PARTITION variable=layer7_out complete dim=0

    nnet::dense<input_t, fc1_result_t, config2>(fc1_input, layer2_out); // fc1

    nnet::relu<fc1_result_t, layer3_t, relu_config3>(layer2_out, layer3_out); // fc1_relu

    nnet::dense<layer3_t, fc2_result_t, config4>(layer3_out, layer4_out); // fc2

    nnet::relu<fc2_result_t, layer5_t, relu_config5>(layer4_out, layer5_out); // fc2_relu

    nnet::dense<layer5_t, fc3_result_t, config6>(layer5_out, layer6_out); // fc3

    nnet::relu<fc3_result_t, layer7_t, relu_config7>(layer6_out, layer7_out); // fc3_relu

    nnet::dense<layer7_t, result_t, config8>(layer7_out, layer8_out); // output

}
