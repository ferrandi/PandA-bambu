#ifndef PARAMETERS_H_
#define PARAMETERS_H_

#include "ap_fixed.h"
#include "ap_int.h"

#include "nnet_utils/nnet_code_gen.h"
#include "nnet_utils/nnet_helpers.h"
// hls-fpga-machine-learning insert includes
#include "nnet_utils/nnet_conv1d.h"
#include "nnet_utils/nnet_conv1d_stream.h"
#include "nnet_utils/nnet_padding.h"
#include "nnet_utils/nnet_padding_stream.h"

// hls-fpga-machine-learning insert weights
#include "weights/w2.h"
#include "weights/b2.h"


// hls-fpga-machine-learning insert layer-config
// zp1d_conv1d
struct config4 : nnet::padding1d_config {
    static const unsigned in_width = 16;
    static const unsigned n_chan = 4;
    static const unsigned out_width = 18;
    static const unsigned pad_left = 1;
    static const unsigned pad_right = 1;
};

// conv1d
struct config2_mult : nnet::dense_config {
    static const unsigned n_in = 12;
    static const unsigned n_out = 1;
    static const unsigned reuse_factor = 1;
    static const unsigned strategy = nnet::resource;
    static const unsigned n_zeros = 0;
    static const unsigned multiplier_limit = DIV_ROUNDUP(n_in * n_out, reuse_factor) - n_zeros / reuse_factor;
    typedef model_default_t accum_t;
    typedef bias2_t bias_t;
    typedef model_default_t weight_t;
    template<class data_T, class res_T, class CONFIG_T>
    using kernel = nnet::DenseResource_rf_leq_nin<data_T, res_T, CONFIG_T>;
    template<class x_T, class y_T>
    using product = nnet::product::mult<x_T, y_T>;
};

struct config2 : nnet::conv1d_config {
    static const unsigned pad_left = 0;
    static const unsigned pad_right = 0;
    static const unsigned in_width = 18;
    static const unsigned n_chan = 4;
    static const unsigned filt_width = 3;
    static const unsigned kernel_size = filt_width;
    static const unsigned n_filt = 1;
    static const unsigned stride_width = 1;
    static const unsigned dilation = 1;
    static const unsigned out_width = 16;
    static const unsigned reuse_factor = 1;
    static const unsigned n_zeros = 0;
    static const unsigned multiplier_limit =
        DIV_ROUNDUP(kernel_size * n_chan * n_filt, reuse_factor) - n_zeros / reuse_factor;
    static const bool store_weights_in_bram = false;
    static const unsigned strategy = nnet::resource;
    static const nnet::conv_implementation implementation = nnet::conv_implementation::linebuffer;
    static const unsigned min_width = 18;
    static const ap_uint<filt_width> pixels[min_width];
    static const unsigned n_partitions = 16;
    static const unsigned n_pixels = out_width / n_partitions;
    template<class data_T, class CONFIG_T>
    using fill_buffer = nnet::FillConv1DBuffer<data_T, CONFIG_T>;
    typedef model_default_t accum_t;
    typedef bias2_t bias_t;
    typedef model_default_t weight_t;
    typedef config2_mult mult_config;
    template<unsigned K, unsigned S, unsigned W>
    using scale_index = nnet::scale_index_regular<K, S, W>;
    template<class data_T, class res_T, class CONFIG_T>
    using conv_kernel = nnet::Conv1DResource<data_T, res_T, CONFIG_T>;
};
const ap_uint<config2::filt_width> config2::pixels[] = {ap_uint<config2::filt_width>(0),ap_uint<config2::filt_width>(0),ap_uint<config2::filt_width>(0),ap_uint<config2::filt_width>(0),ap_uint<config2::filt_width>(0),ap_uint<config2::filt_width>(0),ap_uint<config2::filt_width>(0),ap_uint<config2::filt_width>(0),ap_uint<config2::filt_width>(0),ap_uint<config2::filt_width>(0),ap_uint<config2::filt_width>(0),ap_uint<config2::filt_width>(0),ap_uint<config2::filt_width>(0),ap_uint<config2::filt_width>(0),ap_uint<config2::filt_width>(0),ap_uint<config2::filt_width>(0),ap_uint<config2::filt_width>(0),ap_uint<config2::filt_width>(0)};



#endif
