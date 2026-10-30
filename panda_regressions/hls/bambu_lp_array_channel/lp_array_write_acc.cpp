// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "ac_channel.h"

// Same as lp_dual_array_read with a loop-carried accumulator: the value must
// stay valid also in the states following the write.
void dual_read_acc(int (&a)[16], int (&b)[16], ac_channel<int>& out)
{
   int s = 0;
   for(int i = 0; i < 16; i++)
   {
      int v = a[i] + b[i];
      s += v;
      out.write(s);
   }
}
