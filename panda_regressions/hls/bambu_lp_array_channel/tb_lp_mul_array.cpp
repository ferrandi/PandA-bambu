// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "ac_channel.h"

#include <iostream>

void mul_stage(int (&a)[16], int (&b)[16], ac_channel<int>& out);

int main()
{
   int a[16];
   int b[16];
   ac_channel<int> out;

   for(int i = 0; i < 16; ++i)
   {
      a[i] = i * 7 - 41;
      b[i] = 31 - i * 5;
   }

   mul_stage(a, b, out);

   for(int i = 0; i < 16; ++i)
   {
      const int expected = a[i] * b[i];
      const int actual = out.read();
      if(actual != expected)
      {
         std::cerr << "output[" << i << "] = " << actual << ", expected " << expected << '\n';
         return 1;
      }
   }

   if(out.available(1))
   {
      std::cerr << "mul_stage produced more than 16 outputs\n";
      return 1;
   }

   return 0;
}
