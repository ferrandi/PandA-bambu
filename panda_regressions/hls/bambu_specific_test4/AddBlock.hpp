#ifndef _ADD_BLOCK_HPP
#define _ADD_BLOCK_HPP

#include <ac_channel.h>

namespace test
{
   template <int N>
   class AddBlock
   {
    public:
      AddBlock()
      {
      }

      void compute(ac_channel<int>& in, ac_channel<int>& out)
      {
         int val = in.read() + N;
         out.write(val);
      }
   };
} // namespace test

#endif