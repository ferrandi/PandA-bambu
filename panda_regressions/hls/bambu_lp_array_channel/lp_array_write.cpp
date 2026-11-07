// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "ac_channel.h"

// Reads an `array` interface and writes a channel in the same basic block. The
// read takes two cycles with initiation time 1, so the value is still live in
// the next cycle, while the channel write is unbounded (issue #434).
void store_stage(int (&arr)[16], ac_channel<int>& out)
{
   for(int i = 0; i < 16; i++)
      out.write(arr[i]);
}
