#include <hls_stream.h>

int shared[1] = {0};

void produce_shared(int values[1], hls::stream<int>& input, hls::stream<int>& output)
{
   values[0] = input.read();
   output.write(values[0]);
}

void consume_shared(int values[1], hls::stream<int>& input, hls::stream<int>& output)
{
   // A ding-dong reader may observe the previous buffer iteration.  The stream carries the synchronized token.
   if(values[0] == -1)
   {
      output.write(-1);
   }
   else
   {
      output.write(input.read());
   }
}

void dataflow_ding_dong(hls::stream<int>& input, hls::stream<int>& output)
{
#pragma HLS interface port = input mode = axis
#pragma HLS interface port = output mode = axis
#pragma HLS DATAFLOW
   hls::stream<int> produced;
   produce_shared(shared, input, produced);
   consume_shared(shared, produced, output);
}
