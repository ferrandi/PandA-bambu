// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "ac_channel.h"

#include <iostream>

int co_launch(int (&a)[16], int (&b)[16], ac_channel<int>& out);

int main()
{
   int a[16];
   int b[16];
   ac_channel<int> out;
   int expected_sum = 0;

   for(int i = 0; i < 16; ++i)
   {
      a[i] = 7 * i - 41;
      b[i] = 31 - 5 * i;
      expected_sum += a[i] * b[i];
   }

   const int actual_sum = co_launch(a, b, out);
   if(actual_sum != expected_sum)
   {
      std::cerr << "return value = " << actual_sum << ", expected " << expected_sum << '\n';
      return 1;
   }

   for(int i = 0; i < 16; ++i)
   {
      if(!out.available(1))
      {
         std::cerr << "co_launch produced only " << i << " outputs\n";
         return 1;
      }
      const int actual = out.read();
      if(actual != i)
      {
         std::cerr << "output[" << i << "] = " << actual << ", expected " << i << '\n';
         return 1;
      }
   }

   if(out.available(1))
   {
      std::cerr << "co_launch produced more than 16 outputs\n";
      return 1;
   }

   return 0;
}
