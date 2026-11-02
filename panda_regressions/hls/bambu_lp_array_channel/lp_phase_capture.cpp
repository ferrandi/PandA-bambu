// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "ac_channel.h"

int phase_capture(int (&a)[16], int (&b)[16], int (&c)[16], int (&d)[16], ac_channel<int>& out)
{
   int sum = 0;
   for(int i = 0; i < 16; ++i)
   {
      int product = a[i] * b[i];
      int tag = (c[i] + d[i]) + 1;
      out.write(tag);
      sum += product;
   }
   return sum;
}
