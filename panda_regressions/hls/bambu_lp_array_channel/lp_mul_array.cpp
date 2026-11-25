// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "ac_channel.h"

// A multi-cycle pipelined multiply feeds an unbounded channel write.
void mul_stage(int (&a)[16], int (&b)[16], ac_channel<int>& out)
{
   for(int i = 0; i < 16; i++)
      out.write(a[i] * b[i]);
}
