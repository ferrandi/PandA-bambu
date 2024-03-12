#ifndef _DATAFLOW_HPP
#define _DATAFLOW_HPP

#include <ac_channel.h>

#pragma HLS interface port = in mode = fifo depth = 1
#pragma HLS interface port = out mode = fifo depth = 1
void dataflow_top(ac_channel<int>& in, ac_channel<int>& out);

#endif