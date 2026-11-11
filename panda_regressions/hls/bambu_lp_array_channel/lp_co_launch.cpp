// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "ac_channel.h"

int co_launch(int (&a)[16], int (&b)[16], ac_channel<int>& out)
{
   int sum = 0;
   for(int i = 0; i < 16; ++i)
   {
      sum += a[i] * b[i];
      out.write(i);
   }
   return sum;
}
