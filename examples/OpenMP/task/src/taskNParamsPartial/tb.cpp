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
   int* data3 = (int*)malloc(sizeof(int) * size);
   int* data4 = (int*)malloc(sizeof(int) * size);
   for(unsigned i = 0; i < size; i++)
   {
      data1[i] = 0;
      data2[i] = 0;
      data3[i] = 0;
      data4[i] = 0;
   }

#ifdef __BAMBU_SIM__
   m_param_alloc(0, sizeof(int) * size);
   m_param_alloc(1, sizeof(int) * size);
   m_param_alloc(2, sizeof(int) * size);
   m_param_alloc(3, sizeof(int) * size);
#endif

   fn(data1, data2, data3, data4);

   for(unsigned i = 0; i < size; ++i)
   {
      assert(data1[i] == 4);
      assert(data2[i] == 10);
      assert(data3[i] == 3);
      assert(data4[i] == 4);
   }

   free(data1);
   free(data2);
   free(data3);
   free(data4);
   return 0;
}