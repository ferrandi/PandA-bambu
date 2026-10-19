#ifndef PARAMETERS_H_
#define PARAMETERS_H_

#include "ap_fixed.h"
#include "ap_int.h"

#include "nnet_utils/nnet_code_gen.h"
#include "nnet_utils/nnet_helpers.h"
// hls-fpga-machine-learning insert includes
#include "nnet_utils/nnet_pooling.h"
#include "nnet_utils/nnet_pooling_stream.h"

// hls-fpga-machine-learning insert weights


// hls-fpga-machine-learning insert layer-config
// max_pooling1d_1
struct config2 : nnet::pooling1d_config {
    static const unsigned n_in = 34;
    static const unsigned n_out = 17;
    static const unsigned n_filt = 5;
    static const unsigned pool_width = 2;

    static const unsigned filt_width = pool_width;
    static const unsigned n_chan = n_filt;

    static const unsigned pad_left = 0;
    static const unsigned pad_right = 0;
    static const bool count_pad = false;
    static const unsigned stride_width = 2;
    static const nnet::Pool_Op pool_op = nnet::Max;
    static const nnet::conv_implementation implementation = nnet::conv_implementation::linebuffer;
    static const unsigned reuse_factor = 1;
    typedef model_default_t accum_t;
};



#endif
