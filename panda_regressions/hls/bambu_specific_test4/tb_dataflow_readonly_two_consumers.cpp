#include <hls_stream.h>

extern void dataflow_readonly_two_consumers(hls::stream<int>& input, hls::stream<int>& output);

int main()
{
   hls::stream<int> input;
   hls::stream<int> output;
   input.write(5);
   dataflow_readonly_two_consumers(input, output);
   return output.read() == 36 ? 0 : 1;
}
