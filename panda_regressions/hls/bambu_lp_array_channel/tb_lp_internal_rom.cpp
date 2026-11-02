// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "ac_channel.h"

#include <iostream>

void internal_rom(unsigned int start, ac_channel<int>& out);

int main()
{
   ac_channel<int> out;
   const int expected[16] = {5, -9, 73, 12, -31, 58, 4, -6, 29, 88, -14, 3, 67, 17, -23, 41};

   internal_rom(3, out);

   for(int i = 0; i < 16; ++i)
   {
      if(!out.available(1))
      {
         std::cerr << "internal_rom produced only " << i << " outputs\n";
         return 1;
      }
      const int actual = out.read();
      if(actual != expected[i])
      {
         std::cerr << "output[" << i << "] = " << actual << ", expected " << expected[i] << '\n';
         return 1;
      }
   }

   if(out.available(1))
   {
      std::cerr << "internal_rom produced more than 16 outputs\n";
      return 1;
   }

   return 0;
}
