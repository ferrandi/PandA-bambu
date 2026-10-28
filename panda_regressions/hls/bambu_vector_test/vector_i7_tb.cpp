// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Testbench for the <4 x i7> vector: four 7-bit lanes packed in a 28-bit word.
#include <cstdint>

extern "C" std::uint32_t vec_i7_add(std::uint32_t a, std::uint32_t b);

int main()
{
   const std::uint32_t patterns[] = {0x0000000U, 0x7f7f7f7fU, 0x01020304U, 0x3f004001U, 0x55aaaa55U, 0x2aa551ffU};
   for(const std::uint32_t a : patterns)
   {
      for(const std::uint32_t b : patterns)
      {
         std::uint32_t expected = 0;
         for(unsigned lane = 0; lane < 4; ++lane)
         {
            const auto sum = (((a >> (7 * lane)) & 0x7fU) + ((b >> (7 * lane)) & 0x7fU)) & 0x7fU;
            expected |= sum << (7 * lane);
         }
         if(vec_i7_add(a, b) != expected)
         {
            return 1;
         }
      }
   }
   return 0;
}
