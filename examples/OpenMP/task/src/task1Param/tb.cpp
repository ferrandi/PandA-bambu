/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
#include <cassert>

#include "kernel.hpp"

int main()
{
   int* data1 = (int*)malloc(sizeof(int) * size);
   for(unsigned i = 0; i < size; i++)
   {
      data1[i] = 0;
   }

#ifdef __BAMBU_SIM__
   m_param_alloc(0, sizeof(int) * size);
#endif

   fn(data1);

   for(unsigned i = 0; i < size; ++i)
   {
      assert(data1[i] == 2 * static_cast<int>(i));
   }

   free(data1);
   return 0;
}
