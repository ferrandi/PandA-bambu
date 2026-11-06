// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "ac_channel.h"

int lp_shared_fu_wait(int (&a)[16], int (&b)[16], int (&c)[16], int (&d)[16], ac_channel<int>& out)
{
   int sum = 0;
   for(int i = 0; i < 16; ++i)
   {
      int first = a[i] * b[i];
      out.write(i);
      int second = c[i] * d[i];
      sum += first + second;
   }
   return sum;
}
