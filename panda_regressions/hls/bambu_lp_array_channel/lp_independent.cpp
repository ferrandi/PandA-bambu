// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "ac_channel.h"

// The channel write is independent of the array read; the returned sum keeps
// the read result observable and checked by the testbench.
int independent_read_write(int (&a)[16], ac_channel<int>& out)
{
   int sum = 0;
   for(int i = 0; i < 16; i++)
   {
      sum += a[i];
      out.write(i);
   }
   return sum;
}
