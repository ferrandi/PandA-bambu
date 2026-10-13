// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <cstdint>

extern "C" std::uint32_t mask_extract16(std::uint32_t a, std::uint32_t b, std::uint32_t index);

int main()
{
   for(std::uint32_t index = 0; index < 16; ++index)
   {
      if(mask_extract16(5, 5, index) != 1U || mask_extract16(5, 9, index) != (index == 0 ? 0U : 1U))
      {
         return 1;
      }
   }
   return 0;
}
