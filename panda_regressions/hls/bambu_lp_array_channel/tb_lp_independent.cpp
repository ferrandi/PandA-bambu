// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "ac_channel.h"

#include <iostream>

int independent_read_write(int (&a)[16], ac_channel<int>& out);

int main()
{
   int a[16];
   ac_channel<int> out;
   int expected_sum = 0;

   for(int i = 0; i < 16; ++i)
   {
      a[i] = (i - 7) * 19;
      expected_sum += a[i];
   }

   const int actual_sum = independent_read_write(a, out);
   if(actual_sum != expected_sum)
   {
      std::cerr << "return value = " << actual_sum << ", expected " << expected_sum << '\n';
      return 1;
   }

   for(int i = 0; i < 16; ++i)
   {
      if(!out.available(1))
      {
         std::cerr << "missing output[" << i << "]\n";
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
      std::cerr << "independent_read_write produced more than 16 outputs\n";
      return 1;
   }

   return 0;
}
