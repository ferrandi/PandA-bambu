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
 *              Copyright (C) 2021-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */

#define _GNU_SOURCE
#include <math.h>
#include <stdlib.h>

#if !defined(TEST_FLOAT) && !defined(TEST_DOUBLE)
#error Floating-point type test should be defined
#endif

#ifdef TEST_FLOAT
#define F_TEST_F(func)         \
   FLOAT func##f_test(FLOAT a) \
   {                           \
      return func##f(a);       \
   }

#define I_TEST_F(func)       \
   int func##f_test(FLOAT a) \
   {                         \
      return func##f(a);     \
   }

#define F_TEST_FF(func)                 \
   FLOAT func##f_test(FLOAT a, FLOAT b) \
   {                                    \
      return func##f(a, b);             \
   }

#define F_TEST_FI(func)               \
   FLOAT func##f_test(FLOAT a, int b) \
   {                                  \
      return func##f(a, b);           \
   }

#define F_TEST_FIP(func)                \
   FLOAT func##f_test(FLOAT a, int* pb) \
   {                                    \
      return func##f(a, pb);            \
   }
#endif

#ifdef TEST_DOUBLE
#define F_TEST_F(func)        \
   FLOAT func##_test(FLOAT a) \
   {                          \
      return func(a);         \
   }

#define I_TEST_F(func)      \
   int func##_test(FLOAT a) \
   {                        \
      return func(a);       \
   }

#define F_TEST_FF(func)                \
   FLOAT func##_test(FLOAT a, FLOAT b) \
   {                                   \
      return func(a, b);               \
   }

#define F_TEST_FI(func)              \
   FLOAT func##_test(FLOAT a, int b) \
   {                                 \
      return func(a, b);             \
   }

#define F_TEST_FIP(func)               \
   FLOAT func##_test(FLOAT a, int* pb) \
   {                                   \
      return func(a, pb);              \
   }
#endif

#define I_MULTITEST_F(func) \
   int func##_test(FLOAT a) \
   {                        \
      return func(a);       \
   }

#define B_MULTITEST_F(func)   \
   _Bool func##_test(FLOAT a) \
   {                          \
      return func(a);         \
   }

F_TEST_F(acos)
F_TEST_F(acosh)
F_TEST_F(asin)
F_TEST_F(asinh)
F_TEST_F(atan)
F_TEST_FF(atan2)
F_TEST_F(atanh)
F_TEST_F(cbrt)
F_TEST_F(ceil)
F_TEST_FF(copysign)
F_TEST_F(cos)
F_TEST_F(cosh)
F_TEST_F(erf)
F_TEST_F(erfc)
F_TEST_F(exp)
F_TEST_F(expm1)
F_TEST_F(fabs)
F_TEST_F(floor)
F_TEST_FF(fmod)
I_MULTITEST_F(fpclassify)
F_TEST_FIP(frexp)

F_TEST_FF(hypot)
I_TEST_F(ilogb)
I_MULTITEST_F(isfinite)
I_MULTITEST_F(isnormal)
F_TEST_FI(ldexp)

FLOAT
#if TEST_FLOAT
lgammaf_r_test
#else
lgamma_r_test
#endif
    (FLOAT a, int* pb)
{
   return
#if TEST_FLOAT
       lgammaf_r
#else
       lgamma_r
#endif
       (a, pb);
}

F_TEST_F(log)
F_TEST_F(log1p)
F_TEST_F(log10)
F_TEST_F(logb)

FLOAT
#if TEST_FLOAT
modff_test
#else
modf_test
#endif
    (FLOAT a, FLOAT* pb)
{
   return FUNC(modf)(a, pb);
}

F_TEST_FF(pow)
F_TEST_FF(remainder)
F_TEST_F(rint)
F_TEST_FF(scalb)
F_TEST_FI(scalbn)
B_MULTITEST_F(signbit)
F_TEST_F(significand)
F_TEST_F(sin)

void
#if TEST_FLOAT
sincosf_test
#else
sincos_test
#endif
    (FLOAT a, FLOAT* pb, FLOAT* pc)
{
   return FUNC(sincos)(a, pb, pc);
}

F_TEST_F(sinh)
F_TEST_F(sqrt)
F_TEST_F(tan)
F_TEST_F(tanh)
F_TEST_F(tgamma)
