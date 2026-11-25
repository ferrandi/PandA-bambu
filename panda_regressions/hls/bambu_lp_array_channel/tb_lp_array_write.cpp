// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "ac_channel.h"

#include <iostream>

void store_stage(int (&arr)[16], ac_channel<int>& out);

int main()
{
   int arr[16];
   ac_channel<int> out;

   for(int i = 0; i < 16; ++i)
      arr[i] = (i - 7) * 19;

   store_stage(arr, out);

   for(int i = 0; i < 16; ++i)
   {
      if(!out.available(1))
      {
         std::cerr << "missing output[" << i << "]\n";
         return 1;
      }

      const int expected = arr[i];
      const int actual = out.read();
      if(actual != expected)
      {
         std::cerr << "output[" << i << "] = " << actual << ", expected " << expected << '\n';
         return 1;
      }
   }

   if(out.available(1))
   {
      std::cerr << "store_stage produced more than 16 outputs\n";
      return 1;
   }

   return 0;
}
