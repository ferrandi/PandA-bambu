// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <cstdint>

extern "C" std::uint32_t mask_store_load16(std::uint32_t a, std::uint32_t b);

int main()
{
   return mask_store_load16(5, 5) == 0xa5dbU && mask_store_load16(9, 5) == 0xa55bU &&
                  mask_store_load16(5, 9) == 0xa4dbU && mask_store_load16(9, 9) == 0xa45bU ?
              0 :
              1;
}
