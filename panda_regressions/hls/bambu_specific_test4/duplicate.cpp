#include "AddBlock.hpp"

void dataflow_top(ac_channel<int>& in, ac_channel<int>& out)
{
#pragma HLS interface port=in mode=fifo depth=1
#pragma HLS interface port=out mode=fifo depth=1

#pragma HLS dataflow

   static test::AddBlock<10> a1, a2, a3;

   static ac_channel<int> x, y;

   a1.compute(in, x);
   a2.compute(x, y);
   a3.compute(y, out);
}
