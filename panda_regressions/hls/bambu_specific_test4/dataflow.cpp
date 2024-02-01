#include "dataflow.hpp"

#include "SimpleSystem.hpp"

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

void SimpleSystem::top(ac_channel<int>& in, ac_channel<int>& out)
{
#pragma HLS dataflow

   mul.compute(in, mid);
   add.compute(mid, out);
}

void dataflow_top(ac_channel<int>& in, ac_channel<int>& out)
{
#ifdef USE_CLASS
   static SimpleSystem sys;

   sys.top(in, out);
#else
#pragma HLS dataflow

   static test::AddBlock<10> add;
   static test::MulBlock mul;

   static ac_channel<int> mid;

   mul.compute(in, mid);
   add.compute(mid, out);
#endif
}
