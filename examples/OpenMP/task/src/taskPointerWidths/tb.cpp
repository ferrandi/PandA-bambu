/* SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 * Copyright (C) 2026 Politecnico di Milano
 */
#include "kernel.hpp"
#include <cassert>
#include <cstdlib>

int main()
{
   auto* input = static_cast<std::uint64_t*>(std::malloc(size * sizeof(std::uint64_t)));
   auto* output = static_cast<std::uint64_t*>(std::calloc(size, sizeof(std::uint64_t)));
   assert(input && output);
   for(unsigned i = 0; i < size; ++i)
   {
      input[i] = UINT64_C(0xfedcba9876543200) + i;
   }
#ifdef __BAMBU_SIM__
   m_param_alloc(0, size * sizeof(std::uint64_t));
   m_param_alloc(1, size * sizeof(std::uint64_t));
#endif
   fn(input, output);
   for(unsigned i = 0; i < size; ++i)
   {
      assert(output[i] == ((input[i] ^ UINT64_C(0x123456789abcdef0)) + UINT64_C(0x100000001)));
   }
   std::free(input);
   std::free(output);
}
