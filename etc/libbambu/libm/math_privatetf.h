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
 * @file math_privatetf.h
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#define IEEE32_EXP_BITS 8
#define IEEE32_FRAC_BITS 23
#define IEEE32_EXP_BIAS -127
#define IEEE64_EXP_BITS 11
#define IEEE64_FRAC_BITS 52
#define IEEE64_EXP_BIAS -1023
#define IEEE_RND 1
#define IEEE_NAN 1
#define IEEE_ONE 1
#define IEEE_SUBNORM 0
#define IEEE_SIGN -1

#define IEEE32_SPEC \
   IEEE32_EXP_BITS, IEEE32_FRAC_BITS, IEEE32_EXP_BIAS, IEEE_RND, IEEE_NAN, IEEE_ONE, IEEE_SUBNORM, IEEE_SIGN
#define IEEE64_SPEC \
   IEEE64_EXP_BITS, IEEE64_FRAC_BITS, IEEE64_EXP_BIAS, IEEE_RND, IEEE_NAN, IEEE_ONE, IEEE_SUBNORM, IEEE_SIGN

#define FP_NAN 0
#define FP_INFINITE 1
#define FP_ZERO 2
#define FP_SUBNORMAL 3
#define FP_NORMAL 4

extern unsigned long long __copysign(unsigned long long, unsigned long long, unsigned char __exp_bits,
                                     unsigned char __frac_bits, int __exp_bias, _Bool __rounding, _Bool __nan,
                                     _Bool __one, _Bool __subnorm, signed char __sign);
extern unsigned long long __fabs(unsigned long long, unsigned char __exp_bits, unsigned char __frac_bits,
                                 int __exp_bias, _Bool __rounding, _Bool __nan, _Bool __one, _Bool __subnorm,
                                 signed char __sign);
extern unsigned long long __nan(const char*, unsigned char __exp_bits, unsigned char __frac_bits, int __exp_bias,
                                _Bool __rounding, _Bool __nan, _Bool __one, _Bool __subnorm, signed char __sign);
extern unsigned long long __nans(const char*, unsigned char __exp_bits, unsigned char __frac_bits, int __exp_bias,
                                 _Bool __rounding, _Bool __nan, _Bool __one, _Bool __subnorm, signed char __sign);
extern unsigned long long __inf(unsigned char __exp_bits, unsigned char __frac_bits, int __exp_bias, _Bool __rounding,
                                _Bool __nan, _Bool __one, _Bool __subnorm, signed char __sign);
extern unsigned long long __infinity(unsigned char __exp_bits, unsigned char __frac_bits, int __exp_bias,
                                     _Bool __rounding, _Bool __nan, _Bool __one, _Bool __subnorm, signed char __sign);
extern unsigned long long __huge_val(unsigned char __exp_bits, unsigned char __frac_bits, int __exp_bias,
                                     _Bool __rounding, _Bool __nan, _Bool __one, _Bool __subnorm, signed char __sign);

extern int __fpclassify(unsigned long long, unsigned char __exp_bits, unsigned char __frac_bits, int __exp_bias,
                        _Bool __rounding, _Bool __nan, _Bool __one, _Bool __subnorm, signed char __sign);
extern int __isfinite(unsigned long long, unsigned char __exp_bits, unsigned char __frac_bits, int __exp_bias,
                      _Bool __rounding, _Bool __nan, _Bool __one, _Bool __subnorm, signed char __sign);
extern int __finite(unsigned long long, unsigned char __exp_bits, unsigned char __frac_bits, int __exp_bias,
                    _Bool __rounding, _Bool __nan, _Bool __one, _Bool __subnorm, signed char __sign);
extern int __isinf(unsigned long long, unsigned char __exp_bits, unsigned char __frac_bits, int __exp_bias,
                   _Bool __rounding, _Bool __nan, _Bool __one, _Bool __subnorm, signed char __sign);
extern int __isinf_sign(unsigned long long, unsigned char __exp_bits, unsigned char __frac_bits, int __exp_bias,
                        _Bool __rounding, _Bool __nan, _Bool __one, _Bool __subnorm, signed char __sign);
extern int __isnan(unsigned long long, unsigned char __exp_bits, unsigned char __frac_bits, int __exp_bias,
                   _Bool __rounding, _Bool __nan, _Bool __one, _Bool __subnorm, signed char __sign);
extern int __isnormal(unsigned long long, unsigned char __exp_bits, unsigned char __frac_bits, int __exp_bias,
                      _Bool __rounding, _Bool __nan, _Bool __one, _Bool __subnorm, signed char __sign);
extern int __signbit(unsigned long long, unsigned char __exp_bits, unsigned char __frac_bits, int __exp_bias,
                     _Bool __rounding, _Bool __nan, _Bool __one, _Bool __subnorm, signed char __sign);
