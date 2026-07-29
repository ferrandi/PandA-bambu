#include "myproject_float.h"
#include "myproject.h"
#include "parameters.h"

static void ingest_fc1_input(hls::stream<in_container_t> &input_stream, hls::stream<input_t> &fc1_input) {
    for (int i = 0; i < 1; i++) {
        input_t pack;
        #pragma clang loop unroll(full)
        for (int j = 0; j < input_t::size; j++) {
            in_container_t raw = input_stream.read();
            pack[j].set_slc(0, raw.slc<16>(0));
        }
        fc1_input.write(pack);
    }
}

static void egress_layer5_out(hls::stream<result_t> &layer5_out, hls::stream<out_container_t> &output_stream) {
    for (int i = 0; i < 1; i++) {
        result_t pack = layer5_out.read();
        #pragma clang loop unroll(full)
        for (int j = 0; j < result_t::size; j++) {
            out_container_t raw = 0;
            raw.set_slc(0, pack[j].slc<35>(0));
            output_stream.write(raw);
        }
    }
}

#pragma HLS interface mode=axis port=input_stream
#pragma HLS interface mode=axis port=output_stream
void myproject_float(hls::stream<in_container_t> &input_stream, hls::stream<out_container_t> &output_stream)
{
    #pragma HLS DATAFLOW
    hls::stream<input_t> fc1_input("fc1_input");
    hls::stream<result_t> layer5_out("layer5_out");
    hls::stream<fc1_result_t> layer2_out("layer2_out");
    //#pragma HLS STREAM variable=layer2_out depth=1

    hls::stream<layer4_t> layer4_out("layer4_out");
    //#pragma HLS STREAM variable=layer4_out depth=1

    ingest_fc1_input(input_stream, fc1_input);
#ifndef __BAMBU__
    static bool loaded_weights = false;
    if (!loaded_weights) {
        nnet::load_weights_from_txt<fc1_weight_t, 8>(w2, "w2.txt");
        nnet::load_weights_from_txt<fc1_bias_t, 4>(b2, "b2.txt");
        nnet::load_weights_from_txt<out_weight_t, 8>(w5, "w5.txt");
        nnet::load_weights_from_txt<out_bias_t, 2>(b5, "b5.txt");
        loaded_weights = true;    }
#endif
    nnet::dense<input_t, fc1_result_t, config2>(fc1_input, layer2_out); // fc1

    nnet::relu<fc1_result_t, layer4_t, relu_config4>(layer2_out, layer4_out); // relu1

    nnet::dense<layer4_t, result_t, config5>(layer4_out, layer5_out); // out

    egress_layer5_out(layer5_out, output_stream);
}
