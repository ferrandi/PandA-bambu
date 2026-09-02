#include <hls_stream.h>

void read_local(const int values[], hls::stream<int>& input, hls::stream<int>& output)
{
   output.write(values[0] + input.read());
}

void write_local_result(hls::stream<int>& input, hls::stream<int>& output)
{
   output.write(input.read());
}

void dataflow_readonly_local(hls::stream<int>& input, hls::stream<int>& output)
{
#pragma HLS interface port = input mode = axis
#pragma HLS interface port = output mode = axis
#pragma HLS DATAFLOW
   const int local_value[] = {11};
   hls::stream<int> middle;
   read_local(local_value, input, middle);
   write_local_result(middle, output);
}
