#include <hls_stream.h>

extern void dataflow_readonly_local(hls::stream<int>& input, hls::stream<int>& output);

int main()
{
   hls::stream<int> input;
   hls::stream<int> output;
   input.write(5);
   dataflow_readonly_local(input, output);
   return output.read() == 16 ? 0 : 1;
}
