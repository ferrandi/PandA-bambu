/* SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 * Copyright (C) 2026 Politecnico di Milano
 */
#include "kernel.hpp"
#include <cassert>
#include <cstdlib>
int main()
{
   auto* data = static_cast<int*>(std::calloc(size, sizeof(int)));
   assert(data);
#ifdef __BAMBU_SIM__
   m_param_alloc(0, size * sizeof(int));
#endif
   fn(data);
   for(unsigned i = 0; i < size; ++i)
   {
      assert(data[i] == 3 * static_cast<int>(i));
   }
   std::free(data);
}
