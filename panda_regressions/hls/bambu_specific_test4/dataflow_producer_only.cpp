#include <hls_stream.h>

int weights[1] = {23};

void produce_weights(int values[1], hls::stream<int>& input, hls::stream<int>& output)
{
   values[0] = input.read();
   output.write(values[0]);
}

void write_result(hls::stream<int>& input, hls::stream<int>& output)
{
   output.write(input.read());
}

void dataflow_producer_only(hls::stream<int>& input, hls::stream<int>& output)
{
#pragma HLS interface port = input mode = axis
#pragma HLS interface port = output mode = axis
#pragma HLS DATAFLOW
   hls::stream<int> middle;
   produce_weights(weights, input, middle);
   write_result(middle, output);
}
