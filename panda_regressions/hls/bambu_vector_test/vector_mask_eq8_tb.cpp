// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <cstdint>

extern "C" std::uint8_t mask_eq8(std::uint64_t a, std::uint64_t b);

int main()
{
   if(mask_eq8(0, 0) != 0xffU)
   {
      return 1;
   }
   for(unsigned int lane = 0; lane < 8; ++lane)
   {
      const auto changed = std::uint64_t{1} << (8 * lane);
      if(mask_eq8(changed, 0) != (0xffU ^ (1U << lane)))
      {
         return 2 + lane;
      }
   }
   return mask_eq8(~std::uint64_t{0}, 0) == 0 ? 0 : 10;
}
