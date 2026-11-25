// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "ac_channel.h"

// Two `array` reads, both multi-cycle, feeding one unbounded write: two
// operations in flight at once, so the fix cannot just unblock a single read.
void dual_read(int (&a)[16], int (&b)[16], ac_channel<int>& out)
{
   for(int i = 0; i < 16; i++)
      out.write(a[i] + b[i]);
}
