#include <hls_stream.h>

int weights[1] = {7};

void read_weights(int values[1], hls::stream<int>& input, hls::stream<int>& output)
{
   output.write(values[0] + input.read());
}

void write_result(hls::stream<int>& input, hls::stream<int>& output)
{
   output.write(input.read());
}

void dataflow_readonly_global(hls::stream<int>& input, hls::stream<int>& output)
{
#pragma HLS interface port = input mode = axis
#pragma HLS interface port = output mode = axis
#pragma HLS DATAFLOW
   hls::stream<int> middle;
   read_weights(weights, input, middle);
   write_result(middle, output);
}
