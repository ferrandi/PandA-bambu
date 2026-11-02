// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "ac_channel.h"

#include <iostream>

int phase_capture(int (&a)[16], int (&b)[16], int (&c)[16], int (&d)[16], ac_channel<int>& out);

int main()
{
   int a[16];
   int b[16];
   int c[16];
   int d[16];
   ac_channel<int> out;
   int expected_sum = 0;

   for(int i = 0; i < 16; ++i)
   {
      a[i] = i + 3;
      b[i] = 2 * i + 5;
      c[i] = 4 * i + 7;
      d[i] = 3 * i + 11;
      expected_sum += a[i] * b[i];
   }

   const int actual_sum = phase_capture(a, b, c, d, out);
   if(actual_sum != expected_sum)
   {
      std::cerr << "return value = " << actual_sum << ", expected " << expected_sum << '\n';
      return 1;
   }

   for(int i = 0; i < 16; ++i)
   {
      if(!out.available(1))
      {
         std::cerr << "phase_capture produced only " << i << " outputs\n";
         return 1;
      }
      const int expected = (c[i] + d[i]) + 1;
      const int actual = out.read();
      if(actual != expected)
      {
         std::cerr << "output[" << i << "] = " << actual << ", expected " << expected << '\n';
         return 1;
      }
   }

   if(out.available(1))
   {
      std::cerr << "phase_capture produced more than 16 outputs\n";
      return 1;
   }

   return 0;
}
