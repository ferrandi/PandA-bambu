//    Copyright (C) 2024-2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//    This file is part of the PandA/Bambu libsoftfloat IP Library.
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//
// Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef _SOFTFLOAT_BUILTINS_H
#define _SOFTFLOAT_BUILTINS_H

#include "softfloat_features.h"
#include <bambu_config.h>

static __FORCE_INLINE int __kernel_fpclassify(__tfloat_t x, __uint8_t __exp_bits, __uint8_t __frac_bits,
                                              __int32_t __exp_bias, __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one,
                                              bool __subnorm, __int8_t __sign)
{
   __uint64_t xSig, xExp;
   bool expNull, expMax, sigNull;
   __uint64_t __exp_max = ((1ULL << __exp_bits) - 1ULL);

   xSig = x & ((1ULL << __frac_bits) - 1ULL);
   xExp = (x >> __frac_bits) & ((1ULL << __exp_bits) - 1ULL);

   expNull = xExp == 0ULL;
   sigNull = xSig == 0ULL;
   expMax = (__exc == FLOAT_EXC_STD) & (xExp == __exp_max);

   return ((int)(__one & !expNull & !expMax) << 2) | ((int)expNull << 1) |
          ((__subnorm & !sigNull & expNull) || (sigNull & expMax));
}

// Signaling NaN
#define __FPCLASS_SNAN 0x0001
// Quiet NaN
#define __FPCLASS_QNAN 0x0002 
// Negative infinity   
#define __FPCLASS_NEGINF 0x0004
// Negative normal
#define      __FPCLASS_NEGNORMAL 0x0008
// Negative subnormal
#define __FPCLASS_NEGSUBNORMAL 0x0010
// Negative zero
#define __FPCLASS_NEGZERO 0x0020
// Positive zero
#define __FPCLASS_POSZERO 0x0040
// Positive subnormal
#define __FPCLASS_POSSUBNORMAL 0x0080
// Positive normal
#define __FPCLASS_POSNORMAL 0x0100
// Positive infinity
#define __FPCLASS_POSINF 0x0200

static __FORCE_INLINE int __llvm_kernel_fpclassify(__tfloat_t x, int mask, __uint8_t __exp_bits, __uint8_t __frac_bits,
                                              __int32_t __exp_bias, __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one,
                                              bool __subnorm, __int8_t __sign)
{
   __uint64_t xSig, xExp;
   bool signNeg, expZero, expMax, fracZero, quietBit;
   bool isZero, isSub, isNorm, isInf, isNaN, isQNaN, isSNaN;
   
   __uint64_t __exp_max = ((1ULL << __exp_bits) - 1ULL);

   xSig = x & ((1ULL << __frac_bits) - 1ULL);
   xExp = (x >> __frac_bits) & ((1ULL << __exp_bits) - 1ULL);

   signNeg  = __sign != 0;
   expZero = xExp == 0ULL;
   fracZero = xSig == 0ULL;
   expMax = (__exc == FLOAT_EXC_STD) & (xExp == __exp_max);

   quietBit = (xSig & ((1ULL << (__frac_bits-1)) - 1ULL)) != 0;

   isZero = expZero && fracZero;
   isSub  = expZero && !fracZero;
   isNorm = !expZero && !expMax;
   isInf  = expMax && fracZero;
   isNaN  = expMax && !fracZero;
   isQNaN = isNaN && quietBit;
   isSNaN = isNaN && !quietBit;

    return
        ((mask & __FPCLASS_SNAN)          && isSNaN)
     || ((mask & __FPCLASS_QNAN)          && isQNaN)
     || ((mask & __FPCLASS_NEGINF)        && signNeg  && isInf)
     || ((mask & __FPCLASS_NEGNORMAL)     && signNeg  && isNorm)
     || ((mask & __FPCLASS_NEGSUBNORMAL)  && signNeg  && isSub)
     || ((mask & __FPCLASS_NEGZERO)       && signNeg  && isZero)
     || ((mask & __FPCLASS_POSZERO)       && !signNeg && isZero)
     || ((mask & __FPCLASS_POSSUBNORMAL)  && !signNeg && isSub)
     || ((mask & __FPCLASS_POSNORMAL)     && !signNeg && isNorm)
     || ((mask & __FPCLASS_POSINF)        && !signNeg && isInf);
}


static __FORCE_INLINE __tfloat_t __kernel_inf(__uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                                              __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm,
                                              __int8_t __sign)
{
   if(__exc == FLOAT_EXC_STD)
   {
      return ((1ULL << __exp_bits) - 1) << __frac_bits;
   }
   if((__exp_bits + __frac_bits) == 64)
   {
      return -1LL;
   }
   return ((1ULL << (__exp_bits + __frac_bits)) - 1);
}

static __FORCE_INLINE int __kernel_isfinite(__tfloat_t x, __uint8_t __exp_bits, __uint8_t __frac_bits,
                                            __int32_t __exp_bias, __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one,
                                            bool __subnorm, __int8_t __sign)
{
   __uint64_t exp = (x >> __frac_bits) & ((1ULL << __exp_bits) - 1);
   if(__exc == FLOAT_EXC_STD)
   {
      return exp != ((1ULL << __exp_bits) - 1);
   }
   return 1;
}

static __FORCE_INLINE int __kernel_isgreater(__tfloat_t x, __tfloat_t y, __uint8_t __exp_bits, __uint8_t __frac_bits,
                                             __int32_t __exp_bias, __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one,
                                             bool __subnorm, __int8_t __sign)
{
   bool xSign, ySign;
   __uint64_t xExp, xSig, yExp, ySig;
   __uint64_t __exp_max = ((1ULL << __exp_bits) - 1ULL);
   xSign = __sign == -1 ? ((x >> (__frac_bits + __exp_bits)) & 1) : __sign;
   ySign = __sign == -1 ? ((y >> (__frac_bits + __exp_bits)) & 1) : __sign;
   xExp = (x >> __frac_bits) & ((1ULL << __exp_bits) - 1ULL);
   yExp = (y >> __frac_bits) & ((1ULL << __exp_bits) - 1ULL);
   xSig = x & ((1ULL << __frac_bits) - 1ULL);
   ySig = y & ((1ULL << __frac_bits) - 1ULL);

   return !(__exc == FLOAT_EXC_STD && ((xExp == __exp_max && xSig) || (yExp == __exp_max && ySig))) &&
          (x ^ (1ULL << (__frac_bits + __exp_bits))) > (y ^ (1ULL << (__frac_bits + __exp_bits)));
}

static __FORCE_INLINE int __kernel_isgreaterequal(__tfloat_t x, __tfloat_t y, __uint8_t __exp_bits,
                                                  __uint8_t __frac_bits, __int32_t __exp_bias, __rnd_mode_t __rnd,
                                                  __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   __uint64_t xExp, xSig, yExp, ySig;
   __uint64_t __exp_max = ((1ULL << __exp_bits) - 1ULL);
   xExp = (x >> __frac_bits) & ((1ULL << __exp_bits) - 1ULL);
   yExp = (y >> __frac_bits) & ((1ULL << __exp_bits) - 1ULL);
   xSig = x & ((1ULL << __frac_bits) - 1ULL);
   ySig = y & ((1ULL << __frac_bits) - 1ULL);

   return !(__exc == FLOAT_EXC_STD && ((xExp == __exp_max && xSig) || (yExp == __exp_max && ySig))) &&
          (x ^ (1ULL << (__frac_bits + __exp_bits))) >= (y ^ (1ULL << (__frac_bits + __exp_bits)));
}

static __FORCE_INLINE int __kernel_isinf(__tfloat_t x, __uint8_t __exp_bits, __uint8_t __frac_bits,
                                         __int32_t __exp_bias, __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one,
                                         bool __subnorm, __int8_t __sign)
{
   if(__exc == FLOAT_EXC_STD)
   {
      return (x & ((1ULL << (__exp_bits + __frac_bits)) - 1)) == (((1ULL << __exp_bits) - 1) << __frac_bits);
   }
   return 0;
}

static __FORCE_INLINE int __kernel_isless(__tfloat_t x, __tfloat_t y, __uint8_t __exp_bits, __uint8_t __frac_bits,
                                          __int32_t __exp_bias, __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one,
                                          bool __subnorm, __int8_t __sign)
{
   __uint64_t xExp, xSig, yExp, ySig;
   __uint64_t __exp_max = ((1ULL << __exp_bits) - 1ULL);
   xExp = (x >> __frac_bits) & ((1ULL << __exp_bits) - 1ULL);
   yExp = (y >> __frac_bits) & ((1ULL << __exp_bits) - 1ULL);
   xSig = x & ((1ULL << __frac_bits) - 1ULL);
   ySig = y & ((1ULL << __frac_bits) - 1ULL);

   return !(__exc == FLOAT_EXC_STD && ((xExp == __exp_max && xSig) || (yExp == __exp_max && ySig))) &&
          (x ^ (1ULL << (__frac_bits + __exp_bits))) < (y ^ (1ULL << (__frac_bits + __exp_bits)));
}

static __FORCE_INLINE int __kernel_islessequal(__tfloat_t x, __tfloat_t y, __uint8_t __exp_bits, __uint8_t __frac_bits,
                                               __int32_t __exp_bias, __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one,
                                               bool __subnorm, __int8_t __sign)
{
   __uint64_t xExp, xSig, yExp, ySig;
   __uint64_t __exp_max = ((1ULL << __exp_bits) - 1ULL);
   xExp = (x >> __frac_bits) & ((1ULL << __exp_bits) - 1ULL);
   yExp = (y >> __frac_bits) & ((1ULL << __exp_bits) - 1ULL);
   xSig = x & ((1ULL << __frac_bits) - 1ULL);
   ySig = y & ((1ULL << __frac_bits) - 1ULL);

   return !(__exc == FLOAT_EXC_STD && ((xExp == __exp_max && xSig) || (yExp == __exp_max && ySig))) &&
          (x ^ (1ULL << (__frac_bits + __exp_bits))) <= (y ^ (1ULL << (__frac_bits + __exp_bits)));
}

static __FORCE_INLINE int __kernel_islessgreater(__tfloat_t x, __tfloat_t y, __uint8_t __exp_bits,
                                                 __uint8_t __frac_bits, __int32_t __exp_bias, __rnd_mode_t __rnd,
                                                 __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   __uint64_t xExp, xSig, yExp, ySig;
   __uint64_t __exp_max = ((1ULL << __exp_bits) - 1ULL);
   xExp = (x >> __frac_bits) & ((1ULL << __exp_bits) - 1ULL);
   yExp = (y >> __frac_bits) & ((1ULL << __exp_bits) - 1ULL);
   xSig = x & ((1ULL << __frac_bits) - 1ULL);
   ySig = y & ((1ULL << __frac_bits) - 1ULL);

   return !(__exc == FLOAT_EXC_STD && ((xExp == __exp_max && xSig) || (yExp == __exp_max && ySig))) && x != y;
}

static __FORCE_INLINE int __kernel_isnan(__tfloat_t x, __uint8_t __exp_bits, __uint8_t __frac_bits,
                                         __int32_t __exp_bias, __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one,
                                         bool __subnorm, __int8_t __sign)
{
   if(__exc == FLOAT_EXC_STD)
   {
      return (x & ((1ULL << (__exp_bits + __frac_bits)) - 1)) > (((1ULL << __exp_bits) - 1) << __frac_bits);
   }
   return 0;
}

static __FORCE_INLINE int __kernel_isnormal(__tfloat_t x, __uint8_t __exp_bits, __uint8_t __frac_bits,
                                            __int32_t __exp_bias, __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one,
                                            bool __subnorm, __int8_t __sign)
{
   __uint64_t exp = (x >> __frac_bits) & ((1ULL << __exp_bits) - 1);
   bool expMax = __exc == FLOAT_EXC_STD && (exp == ((1ULL << __exp_bits) - 1));
   bool expNull = exp == 0;
   return !__one || !expMax && !expNull;
}

static __FORCE_INLINE __tfloat_t __kernel_nan(const char* __tagb, __uint8_t __exp_bits, __uint8_t __frac_bits,
                                              __int32_t __exp_bias, __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one,
                                              bool __subnorm, __int8_t __sign)
{
   if(__frac_bits > 0)
   {
      return ((1ULL << (__exp_bits + 1)) - 1) << (__frac_bits - 1);
   }
   return ((1ULL << __exp_bits) - 1);
}

static __FORCE_INLINE __tfloat_t __kernel_nans(const char* __tagb, __uint8_t __exp_bits, __uint8_t __frac_bits,
                                               __int32_t __exp_bias, __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one,
                                               bool __subnorm, __int8_t __sign)
{
   if(__frac_bits > 1)
   {
      return ((((1ULL << __exp_bits) - 1) << 2) | 1ULL) << (__frac_bits - 2);
   }
   if(__frac_bits == 1)
   {
      return ((1ULL << (__exp_bits + 1)) - 1);
   }
   return ((1ULL << __exp_bits) - 1);
}

static __FORCE_INLINE int __kernel_signbit(__tfloat_t x, __uint8_t __exp_bits, __uint8_t __frac_bits,
                                           __int32_t __exp_bias, __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one,
                                           bool __subnorm, __int8_t __sign)
{
   if(__sign == -1)
   {
      return (x >> (__exp_bits + __frac_bits)) & 1ULL;
   }
   return __sign;
}

#endif // _SOFTFLOAT_BUILTINS_H
