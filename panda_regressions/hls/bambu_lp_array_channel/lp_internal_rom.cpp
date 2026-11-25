// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "ac_channel.h"

static const int rom[16] = {17, -23, 41, 5, -9, 73, 12, -31, 58, 4, -6, 29, 88, -14, 3, 67};

void internal_rom(unsigned int start, ac_channel<int>& out)
{
   for(unsigned int i = 0; i < 16; ++i)
      out.write(rom[(i + start) & 15]);
}
