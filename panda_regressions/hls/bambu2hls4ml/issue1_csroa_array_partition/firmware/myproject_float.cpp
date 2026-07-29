#include "myproject_float.h"
#include "myproject.h"
#include "parameters.h"

static void ingest_fc1_input(in_container_t input[N_IN], input_t fc1_input[16]) {
    #pragma clang loop unroll(full)
    for (int i = 0; i < 16; i++)
        fc1_input[i].set_slc(0, input[i].slc<16>(0));
}

static void egress_layer4_out(result_t layer4_out[16], out_container_t output[N_OUT]) {
    #pragma clang loop unroll(full)
    for (int i = 0; i < 16; i++) {
        output[i] = 0;
        output[i].set_slc(0, layer4_out[i].slc<16>(0));
    }
}

void myproject_float(in_container_t input[N_IN], out_container_t output[N_OUT])
{
    input_t fc1_input[16];
    result_t layer4_out[16];
    fc1_result_t layer2_out[16];
    #pragma HLS ARRAY_PARTITION variable=layer2_out complete dim=0

    ingest_fc1_input(input, fc1_input);
#ifndef __BAMBU__
    static bool loaded_weights = false;
    if (!loaded_weights) {
        nnet::load_weights_from_txt<fc1_weight_t, 256>(w2, "w2.txt");
        nnet::load_weights_from_txt<fc1_bias_t, 16>(b2, "b2.txt");
        loaded_weights = true;    }
#endif
    nnet::dense<input_t, fc1_result_t, config2>(fc1_input, layer2_out); // fc1

    nnet::relu<fc1_result_t, result_t, relu_config4>(layer2_out, layer4_out); // relu1

    egress_layer4_out(layer4_out, output);
}
