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
   int buff[(size * size) * 2] = {0};
   int* data1 = &(buff[0]);
   int* data2 = &(buff[size * size]);

#ifdef __BAMBU_SIM__
   m_param_alloc(0, sizeof(int) * size * size);
   m_param_alloc(1, sizeof(int) * size * size);
#endif

   // Fill matrix
   for(unsigned i = 0; i < 2; i++)
   {
      const int base = size * size * i;
      for(unsigned j = 0; j < size; j++)
      {
         if(j % 2 == 0 || j == 0)
         {
            for(unsigned k = 0; k < size; k++)
            {
               buff[base + (j * size) + k] = 0;
            }
         }
         else
         {
            for(unsigned k = 0; k < size; k++)
            {
               buff[base + (j * size) + k] = null;
            }
         }
      }
   }

   fn(data1, data2);

   for(unsigned row = 0; row < size; ++row)
   {
      for(unsigned col = 0; col < size; ++col)
      {
         assert(data1[row * size + col] == (row % 2U == 0U ? data1_amount * 2 : null));
         assert(data2[row * size + col] == (row % 2U == 0U ? data2_amount * 2 : null));
      }
   }

   return 0;
}
