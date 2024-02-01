#ifndef _MUL_BLOCK_HPP
#define _MUL_BLOCK_HPP

#include <ac_channel.h>

namespace test
{
   class MulBlock
   {
    public:
      MulBlock();

      void compute(ac_channel<int>& in, ac_channel<int>& out);
   };
} // namespace test

#endif