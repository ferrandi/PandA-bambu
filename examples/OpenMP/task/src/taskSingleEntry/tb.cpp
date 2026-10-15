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
int main()
{
   int data[size] = {};
#ifdef __BAMBU_SIM__
   m_param_alloc(0, sizeof(data));
#endif
   fn(data);
   for(unsigned i = 0; i < size; ++i)
   {
      assert(data[i] == static_cast<int>(i) + 1);
   }
   return 0;
}
