#include <hls_stream.h>

int weights[1] = {31};

extern void dataflow_readonly_uninitialized(hls::stream<int>& input, hls::stream<int>& output);

int main()
{
   hls::stream<int> input;
   hls::stream<int> output;
   input.write(5);
   dataflow_readonly_uninitialized(input, output);
   return output.read() == 36 ? 0 : 1;
}
