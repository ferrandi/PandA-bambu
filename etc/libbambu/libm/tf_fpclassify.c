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
 *              Copyright (C) 2021-2024 Politecnico di Milano
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
 * @file tf_fpclassify.c
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "math_privatetf.h"

int __fpclassify(unsigned long long x, unsigned char __exp_bits, unsigned char __frac_bits, int __exp_bias,
                 _Bool __rounding, _Bool __nan, _Bool __one, _Bool __subnorm, signed char __sign)
{
   unsigned long long w = x & (1ULL << (__exp_bits + __frac_bits));
   _Bool expNull, expMax, sigNull;
   expNull = (w >> __frac_bits) == 0ULL;
   expMax = __nan & ((w >> __frac_bits) == ((1ULL << __exp_bits) - 1));
   sigNull = (w & ((1ULL << __frac_bits) - 1)) == 0ULL;
   if(expNull && sigNull)
   {
      return FP_ZERO;
   }
   else if(!expNull && !expMax && !sigNull)
   {
      return __one ? FP_NORMAL : FP_SUBNORMAL;
   }
   else if(__one && expNull && !sigNull)
   {
      return FP_SUBNORMAL;
   }
   else if(expMax && sigNull)
   {
      return FP_INFINITE;
   }
   else
   {
      return FP_NAN;
   }
}