#include "dataflow2.hpp"

#include "AddBlock.hpp"
#include "MulBlock.hpp"
#include "SumBlock.hpp"

using namespace test;

test::MulBlock::MulBlock()
{
}

void test::MulBlock::compute(ac_channel<int>& in, ac_channel<int>& out)
{
   int val = in.read();
   int i = 0;
   for(; i < 3; ++i)
      val += val;
   out.write(val);
}

static AddBlock<5> a5;
static AddBlock<7> a7;
static SumBlock<int> s;
static MulBlock m1;

static ac_channel<int> x, y, z;

void dataflow_top(ac_channel<int>& in1, ac_channel<int>& in2, ac_channel<int>& out)
{
#pragma HLS dataflow

   a5.compute(in1, x);
   m1.compute(x, y);

   a7.compute(in2, z);

   s.compute(y, z, out);
}
