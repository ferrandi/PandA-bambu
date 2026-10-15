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
#include <cassert>
#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

static int expected_nested(int value)
{
   return 2 * value + 15 + (value & 3) + ((value + 1) & 3);
}

int main()
{
   int data[size];
   int expected[size];
   for(unsigned i = 0; i < size; ++i)
   {
      data[i] = static_cast<int>(i);
      const auto first = expected_nested(data[i]);
      expected[i] = first + expected_nested(first + 2);
   }
#ifdef __BAMBU_SIM__
   m_param_alloc(0, sizeof(data));
#endif
   fn(data);
   for(unsigned i = 0; i < size; ++i)
   {
      assert(data[i] == expected[i]);
   }
   return 0;
}
