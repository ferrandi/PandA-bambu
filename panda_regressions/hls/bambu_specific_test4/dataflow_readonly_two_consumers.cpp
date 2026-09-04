#include <hls_stream.h>

int weights[1] = {13};

void read_left(int values[1], hls::stream<int>& input, hls::stream<int>& output)
{
   output.write(values[0] + input.read());
}

void read_right(int values[1], hls::stream<int>& input, hls::stream<int>& output)
{
   output.write(values[0] + input.read());
}

void split_input(hls::stream<int>& input, hls::stream<int>& left, hls::stream<int>& right)
{
   const int value = input.read();
   left.write(value);
   right.write(value);
}

void join_results(hls::stream<int>& left, hls::stream<int>& right, hls::stream<int>& output)
{
   output.write(left.read() + right.read());
}

void dataflow_readonly_two_consumers(hls::stream<int>& input, hls::stream<int>& output)
{
#pragma HLS interface port = input mode = axis
#pragma HLS interface port = output mode = axis
#pragma HLS DATAFLOW
   hls::stream<int> left_input;
   hls::stream<int> right_input;
   hls::stream<int> left;
   hls::stream<int> right;
   split_input(input, left_input, right_input);
   read_left(weights, left_input, left);
   read_right(weights, right_input, right);
   join_results(left, right, output);
}
