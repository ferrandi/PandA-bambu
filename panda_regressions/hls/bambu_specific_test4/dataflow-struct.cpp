#include <ac_channel.h>

struct pair_obj
{
   bool cond1;
   bool cond2;
   unsigned short payload1;
   unsigned int payload2;
   unsigned long long payload3;
};

class ProdObj
{
 public:
   ProdObj()
   {
   }

   void compute(ac_channel<int>& in1, ac_channel<int>& in2, ac_channel<pair_obj>& out)
   {
      int val1 = in1.read() + in2.read();
      pair_obj res;
      res.cond1 = val1 > 0;
      res.cond2 = val1 == 0;
      res.payload1 = (unsigned short)val1;
      res.payload2 = (unsigned int)val1;
      res.payload3 = (unsigned long long)val1;
      out.write(res);
   }
};

class ConsObj
{
 public:
   ConsObj()
   {
   }

   void compute(ac_channel<pair_obj>& in, ac_channel<int>& out)
   {
      pair_obj val = in.read();
      int res;
      if(!val.cond1 && !val.cond2)
         res = (-val.payload1) * (-val.payload2);
      else
         res = val.payload2 * val.payload3;
      out.write(res);
   }
};

class SimpleSystem
{
   ProdObj p;
   ConsObj c;

   ac_channel<pair_obj> x;

 public:
   void top(ac_channel<int>& in1, ac_channel<int>& in2, ac_channel<int>& out)
   {
#pragma HLS dataflow
      p.compute(in1, in2, x);
      c.compute(x, out);
   }
};

void dataflow_top(ac_channel<int>& in1, ac_channel<int>& in2, ac_channel<int>& out)
{
   SimpleSystem sys;
   sys.top(in1, in2, out);
}
