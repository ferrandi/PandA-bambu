// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <cstdint>

extern "C" std::uint32_t mask_eq16(std::uint32_t a, std::uint32_t b);

int main()
{
   return mask_eq16(5, 5) == 0xffffU && mask_eq16(5, 9) == 0xfffeU ? 0 : 1;
}
