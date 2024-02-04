#ifndef _SUM_BLOCK_HPP
#define _SUM_BLOCK_HPP

#include <ac_channel.h>

namespace test
{
   template <typename T>
   class SumBlock
   {
    public:
      SumBlock()
      {
      }

      void compute(ac_channel<T>& in1, ac_channel<T>& in2, ac_channel<T>& out)
      {
         T val = in1.read() + in2.read();
         out.write(val);
      }
   };
} // namespace test

#endif