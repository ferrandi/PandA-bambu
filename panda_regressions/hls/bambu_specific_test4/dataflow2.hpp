#ifndef _DATAFLOW2_HPP
#define _DATAFLOW2_HPP

#include <ac_channel.h>

#pragma HLS interface port = in1 mode = fifo depth = 1
#pragma HLS interface port = in2 mode = fifo depth = 1
#pragma HLS interface port = out mode = fifo depth = 1
void dataflow_top(ac_channel<int>& in1, ac_channel<int>& in2, ac_channel<int>& out);

#endif