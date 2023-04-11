/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (C) 2021-2023 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/**
 * @file tf_infinity.c
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "math_privatetf.h"

static unsigned long long
    __attribute__((always_inline)) inline __local_inf(unsigned char __exp_bits, unsigned char __frac_bits,
                                                      int __exp_bias, _Bool __rounding, _Bool __nan, _Bool __one,
                                                      _Bool __subnorm, signed char __sign)
{
   if(__nan)
   {
      return ((1ULL << __exp_bits) - 1) << __frac_bits;
   }
   else
   {
      if((__exp_bits + __frac_bits) == 64)
      {
         return -1LL;
      }
      else
      {
         return ((1ULL << (__exp_bits + __frac_bits)) - 1);
      }
   }
}

unsigned long long __inf(unsigned char __exp_bits, unsigned char __frac_bits, int __exp_bias, _Bool __rounding,
                         _Bool __nan, _Bool __one, _Bool __subnorm, signed char __sign)
{
   return __local_inf(__exp_bits, __frac_bits, __exp_bias, __rounding, __nan, __one, __subnorm, __sign);
}

unsigned long long __infinity(unsigned char __exp_bits, unsigned char __frac_bits, int __exp_bias, _Bool __rounding,
                              _Bool __nan, _Bool __one, _Bool __subnorm, signed char __sign)
{
   return __local_inf(__exp_bits, __frac_bits, __exp_bias, __rounding, __nan, __one, __subnorm, __sign);
}

unsigned long long __huge_val(unsigned char __exp_bits, unsigned char __frac_bits, int __exp_bias, _Bool __rounding,
                              _Bool __nan, _Bool __one, _Bool __subnorm, signed char __sign)
{
   return __local_inf(__exp_bits, __frac_bits, __exp_bias, __rounding, __nan, __one, __subnorm, __sign);
}