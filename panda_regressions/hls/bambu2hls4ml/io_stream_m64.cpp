#include "ac_channel.h"
#include "ac_fixed.h"

typedef ac_fixed<16, 6> stream_data_t;

#pragma HLS interface port = input mode = axis
#pragma HLS interface port = output mode = axis
void io_stream_m64(ac_channel<stream_data_t>& input, ac_channel<stream_data_t>& output)
{
   output.write(input.read());
}
