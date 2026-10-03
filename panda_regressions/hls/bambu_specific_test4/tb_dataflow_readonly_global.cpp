#include <hls_stream.h>

extern void dataflow_readonly_global(hls::stream<int>& input, hls::stream<int>& output);

int main()
{
   hls::stream<int> input;
   hls::stream<int> output;
   input.write(5);
   dataflow_readonly_global(input, output);
   return output.read() == 12 ? 0 : 1;
}
