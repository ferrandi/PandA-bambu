// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "ac_channel.h"

#include <iostream>

void div_stage(unsigned int (&numerator)[8], unsigned int (&denominator)[8], ac_channel<int>& out);

int main()
{
   unsigned int numerator[8] = {0, 1, 10, 17, 100, 255, 1024, 999};
   unsigned int denominator[8] = {3, 1, 3, 5, 9, 16, 17, 100};
   ac_channel<int> out;

   div_stage(numerator, denominator, out);

   for(int i = 0; i < 8; ++i)
   {
      if(!out.available(1))
      {
         std::cerr << "missing output[" << i << "]\n";
         return 1;
      }

      const int expected = static_cast<int>(numerator[i] / denominator[i]);
      const int actual = out.read();
      if(actual != expected)
      {
         std::cerr << "output[" << i << "] = " << actual << ", expected " << expected << '\n';
         return 1;
      }
   }

   if(out.available(1))
   {
      std::cerr << "div_stage produced more than 8 outputs\n";
      return 1;
   }

   return 0;
}
