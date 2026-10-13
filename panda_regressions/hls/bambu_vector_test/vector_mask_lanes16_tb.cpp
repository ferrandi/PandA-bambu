// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <cstdint>

extern "C" std::uint32_t mask_lanes16(std::uint32_t active, std::uint32_t index);

int main()
{
   for(std::uint32_t active = 0; active < 16; ++active)
   {
      for(std::uint32_t index = 0; index < 16; ++index)
      {
         const auto expected = (1U << active) | (static_cast<std::uint32_t>(active == index) << 16);
         if(mask_lanes16(active, index) != expected)
         {
            return 1;
         }
      }
   }
   return 0;
}
