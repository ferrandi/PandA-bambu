#include <cassert>
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
#include "kernel.hpp"

int main()
{
   int* data1 = (int*)malloc(sizeof(int) * size);
   int* data2 = (int*)malloc(sizeof(int) * size);
   for(unsigned i = 0; i < size; i++)
   {
      data1[i] = 0;
      data2[i] = 0;
   }

#ifdef __BAMBU_SIM__
   m_param_alloc(0, sizeof(int) * size);
   m_param_alloc(1, sizeof(int) * size);
#endif

   fn(data1, data2);

   for(unsigned i = 0; i < size; ++i)
   {
      assert(data1[i] == 2);
      assert(data2[i] == 6);
   }

   free(data1);
   free(data2);
   return 0;
}