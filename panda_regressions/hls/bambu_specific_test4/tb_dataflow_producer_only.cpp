#include <hls_stream.h>

extern void dataflow_producer_only(hls::stream<int>& input, hls::stream<int>& output);

int main()
{
   hls::stream<int> input;
   hls::stream<int> output;
   input.write(5);
   dataflow_producer_only(input, output);
   return output.read() == 5 ? 0 : 1;
}
