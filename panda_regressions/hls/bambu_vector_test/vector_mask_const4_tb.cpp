// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <cstdint>

extern "C" std::uint8_t mask_const4();

int main()
{
   return mask_const4() == 0x0bU ? 0 : 1;
}
