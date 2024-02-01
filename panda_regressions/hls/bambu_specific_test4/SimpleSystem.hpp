#ifndef _SIMPLE_SYSTEM_HPP
#define _SIMPLE_SYSTEM_HPP

#include "AddBlock.hpp"
#include "MulBlock.hpp"

class SimpleSystem
{
   test::AddBlock<10> add;
   test::MulBlock mul;

   ac_channel<int> mid;

 public:
   void top(ac_channel<int>& in, ac_channel<int>& out);
};

#endif