// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "ac_channel.h"

#include <iostream>

void dual_read_acc(int (&a)[16], int (&b)[16], ac_channel<int>& out);

int main()
{
   int a[16];
   int b[16];
   ac_channel<int> out;

   for(int i = 0; i < 16; ++i)
   {
      a[i] = 3 * i - 17;
      b[i] = 41 - 2 * i;
   }

   dual_read_acc(a, b, out);

   int expected = 0;
   for(int i = 0; i < 16; ++i)
   {
      if(!out.available(1))
      {
         std::cerr << "missing output[" << i << "]\n";
         return 1;
      }

      expected += a[i] + b[i];
      const int actual = out.read();
      if(actual != expected)
      {
         std::cerr << "output[" << i << "] = " << actual << ", expected " << expected << '\n';
         return 1;
      }
   }

   if(out.available(1))
   {
      std::cerr << "dual_read_acc produced more than 16 outputs\n";
      return 1;
   }

   return 0;
}
