// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "ac_channel.h"

void div_stage(unsigned int (&numerator)[8], unsigned int (&denominator)[8], ac_channel<int>& out)
{
   for(int i = 0; i < 8; ++i)
      out.write(static_cast<int>(numerator[i] / denominator[i]));
}
