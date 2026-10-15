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
#include <cmath>
#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

int main()
{
   float data[size];
   float expected[size];
   for(unsigned i = 0; i < size; ++i)
   {
      data[i] = static_cast<float>(i) * 0.25f;
      const auto first = std::exp(data[i]) + std::sqrt(data[i] + 1.0f);
      expected[i] = std::exp(first + 1.0f) + std::sqrt(first + 2.0f);
   }
#ifdef __BAMBU_SIM__
   m_param_alloc(0, sizeof(data));
#endif
   fn(data);
   for(unsigned i = 0; i < size; ++i)
   {
      assert(std::abs(data[i] - expected[i]) <= 1.0e-5f * expected[i]);
   }
   return 0;
}
