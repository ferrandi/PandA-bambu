#include <hls_stream.h>

extern void dataflow_ding_dong(hls::stream<int>& input, hls::stream<int>& output);

int main()
{
   hls::stream<int> input;
   hls::stream<int> output;
   input.write(5);
   dataflow_ding_dong(input, output);
   return output.read() == 5 ? 0 : 1;
}
