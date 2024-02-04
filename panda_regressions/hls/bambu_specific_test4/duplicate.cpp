#include "dataflow.hpp"

#include "AddBlock.hpp"

static test::AddBlock<10> a1, a2, a3;
static ac_channel<int> x, y;

void dataflow_top(ac_channel<int>& in, ac_channel<int>& out)
{
#pragma HLS dataflow

   a1.compute(in, x);
   a2.compute(x, y);
   a3.compute(y, out);
}
