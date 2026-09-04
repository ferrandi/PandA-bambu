#include <hls_stream.h>

extern void dataflow_readonly_repeated_consumer(hls::stream<int>& input, hls::stream<int>& output);

int main()
{
   hls::stream<int> input;
   hls::stream<int> output;
   input.write(5);
   dataflow_readonly_repeated_consumer(input, output);
   return output.read() == 44 ? 0 : 1;
}
