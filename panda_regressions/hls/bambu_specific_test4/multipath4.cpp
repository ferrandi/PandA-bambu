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

class SubSystem
{
   AddBlock<5> a5;
   AddBlock<7> a7;
   SumBlock<int> s;
   MulBlock m1;

   ac_channel<int> x, y, z;

 public:
   void compute(ac_channel<int>& in1, ac_channel<int>& in2, ac_channel<int>& out)
   {
#pragma HLS dataflow
      a5.compute(in1, x);
      m1.compute(x, y);

      a7.compute(in2, z);

      s.compute(y, z, out);
   }
};


class SimpleSystem
{
   AddBlock<5> a5;
   AddBlock<7> a7;
   MulBlock m1;
   SubSystem s2;

   ac_channel<int> x, y, z;

 public:
   void top(ac_channel<int>& in1, ac_channel<int>& in2, ac_channel<int>& out)
   {
#pragma HLS dataflow
      a5.compute(in1, x);
      m1.compute(x, y);

      a7.compute(in2, z);

      s2.compute(y, z, out);
   }
};

void dataflow_top(ac_channel<int>& in1, ac_channel<int>& in2, ac_channel<int>& out)
{
   SimpleSystem sys;
   sys.top(in1, in2, out);
}
