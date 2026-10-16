// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <cstdint>

extern "C" std::uint32_t mask_store_load4(std::uint32_t a, std::uint32_t b);

int main()
{
   return mask_store_load4(5, 5) == 0x0bU && mask_store_load4(5, 9) == 0x0aU ? 0 : 1;
}
