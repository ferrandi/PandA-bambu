//    Copyright (C) 2013-2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//    This file is part of the PandA/Bambu IP Library.
//
//    author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
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
/**
 * The original version of the Softfloat library by John R. Hauser has been
 * heavily modified and almost completely rewritten, still the related
 * copyright notice has been preserved to pay credits to the original authors.
 */
/*============================================================================

This C source file is part of the SoftFloat IEC/IEEE Floating-point Arithmetic
Package, Release 2b.

Written by John R. Hauser.  This work was made possible in part by the
International Computer Science Institute, located at Suite 600, 1947 Center
Street, Berkeley, California 94704.  Funding was partially provided by the
National Science Foundation under grant MIP-9311980.  The original version
of this code was written as part of a project to build a fixed-point vector
processor in collaboration with the University of California at Berkeley,
overseen by Profs. Nelson Morgan and John Wawrzynek.  More information
is available through the Web page `http://www.cs.berkeley.edu/~jhauser/
arithmetic/SoftFloat.html'.

THIS SOFTWARE IS DISTRIBUTED AS IS, FOR FREE.  Although reasonable effort has
been made to avoid it, THIS SOFTWARE MAY CONTAIN FAULTS THAT WILL AT TIMES
RESULT IN INCORRECT BEHAVIOR.  USE OF THIS SOFTWARE IS RESTRICTED TO PERSONS
AND ORGANIZATIONS WHO CAN AND WILL TAKE FULL RESPONSIBILITY FOR ALL LOSSES,
COSTS, OR OTHER PROBLEMS THEY INCUR DUE TO THE SOFTWARE, AND WHO FURTHERMORE
EFFECTIVELY INDEMNIFY JOHN HAUSER AND THE INTERNATIONAL COMPUTER SCIENCE
INSTITUTE (possibly via similar legal warning) AGAINST ALL LOSSES, COSTS, OR
OTHER PROBLEMS INCURRED BY THEIR CUSTOMERS AND CLIENTS DUE TO THE SOFTWARE.

Derivative works are acceptable, even for commercial purposes, so long as
(1) the source code for the derivative work includes prominent notice that
the work is derivative, and (2) the source code includes prominent notice with
these four paragraphs for those parts of this code that are retained.

=============================================================================*/

#include "softfloat.h"

#include "div_utilities.h"

#include <bambu_macros.h>

/*----------------------------------------------------------------------------
| Floating-point rounding mode and exception flags.
*----------------------------------------------------------------------------*/
#ifndef NO_PARAMETRIC
__int8_t __float_rounding_mode = float_round_nearest_even;
__int8_t __float_exception_flags = 0;
#endif

/*----------------------------------------------------------------------------
| Underflow tininess-detection mode, statically initialized to default value.
| (The declaration in `softfloat.h' must match the `__int8_t' type here.)
*----------------------------------------------------------------------------*/
#ifndef NO_PARAMETRIC
__int8_t __float_detect_tininess = float_tininess_after_rounding;
#endif

/*----------------------------------------------------------------------------
| Packs the sign `zSign', exponent `zExp', and significand `zSig' into a
| single-precision floating-point value, returning the result.  After being
| shifted into the proper positions, the three fields are simply added
| together to form the result.  This means that any integer portion of `zSig'
| will be added into the exponent.  Since a properly normalized significand
| will have an integer portion equal to 1, the `zExp' input should be 1 less
| than the desired result exponent whenever `zSig' is a complete, normalized
| significand.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float32_t __packFloat32(bool zSign, __uint32_t zExp, __uint32_t zSig, __uint8_t __exp_bits,
                                                __uint8_t __frac_bits)
{
   return (((__uint32_t)zSign) << (__exp_bits + __frac_bits)) + (zExp << __frac_bits) + zSig;
}

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent `zExp',
| and significand `zSig', and returns the proper single-precision floating-
| point value corresponding to the abstract input.  Ordinarily, the abstract
| value is simply rounded and packed into the single-precision format, with
| the inexact exception raised if the abstract input cannot be represented
| exactly.  However, if the abstract value is too large, the overflow and
| inexact exceptions are raised and an infinity or maximal finite value is
| returned.  If the abstract value is too small, the input value is rounded to
| a subnormal number, and the underflow and inexact exceptions are raised if
| the abstract input cannot be represented exactly as a subnormal single-
| precision floating-point number.
|     The input significand `zSig' has its binary point between bits 30
| and 29, which is 7 bits to the left of the usual location.  This shifted
| significand must be normalized or smaller.  If `zSig' is not normalized,
| `zExp' must be 0; in that case, the result returned is a subnormal number,
| and it must not require rounding.  In the usual case that `zSig' is
| normalized, `zExp' must be 1 less than the ``true'' floating-point exponent.
| The handling of underflow and overflow follows the IEC/IEEE Standard for
| Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float32_t __roundAndPackFloat32(bool zSign, __int32_t zExp, __uint32_t zSig,
                                                        __uint8_t __exp_bits, __uint8_t __frac_bits, __exc_mode_t __exc,
                                                        bool __subnorm)
{
   __int8_t roundingMode;
   bool roundNearestEven;
   __int32_t roundIncrement, roundBits;
   bool isTiny;
   __uint8_t __ext_bits = (30 - __frac_bits);
   __int32_t __exp_max = ((1 << __exp_bits) - 1);

   roundingMode = __float_rounding_mode;
   roundNearestEven = (roundingMode == float_round_nearest_even);
   roundIncrement = 1 << (__ext_bits - 1);
#ifndef NO_PARAMETRIC
   if(!roundNearestEven)
   {
      if(roundingMode == float_round_to_zero)
      {
         roundIncrement = 0;
      }
      else
      {
         roundIncrement = ((1 << __ext_bits) - 1);
         if(zSign)
         {
            if(roundingMode == float_round_up)
               roundIncrement = 0;
         }
         else
         {
            if(roundingMode == float_round_down)
               roundIncrement = 0;
         }
      }
   }
#endif
   roundBits = zSig & ((1 << __ext_bits) - 1);
   if((__exp_max - ((__exc != FLOAT_EXC_STD) ? 2 : 1)) <= (__uint32_t)zExp)
   {
      if(((__exp_max - ((__exc != FLOAT_EXC_STD) ? 2 : 1)) < zExp) ||
         ((zExp == (__exp_max - ((__exc != FLOAT_EXC_STD) ? 2 : 1))) && ((__int32_t)(zSig + roundIncrement) < 0)))
      {
         __float_raise(float_flag_overflow | float_flag_inexact);
         return ((((__uint32_t)zSign) << (__exp_bits + __frac_bits)) | (__exp_max << __frac_bits) |
                 (((__uint32_t) !(__exc != FLOAT_EXC_STD) << __frac_bits) - !(__exc != FLOAT_EXC_STD))) -
                (roundIncrement == 0);
      }
      if(zExp < 0)
      {
         if(__subnorm)
         {
            isTiny = (__float_detect_tininess == float_tininess_before_rounding) || (zExp < -1) ||
                     (zSig + roundIncrement < 0x80000000);
            __shift32RightJamming(zSig, -zExp, &zSig);
            zExp = 0;
            roundBits = zSig & ((1 << __ext_bits) - 1);
            if(isTiny && roundBits)
               __float_raise(float_flag_underflow);
         }
         else
         {
            return ((__uint32_t)zSign) << (__exp_bits + __frac_bits); // __packFloat32(zSign, 0, 0, IEEE32_PACK);
         }
      }
   }
   if(roundBits)
   {
      __float_raise(float_flag_inexact);
   }
   zSig = (zSig + roundIncrement) >> __ext_bits;
   zSig &= ~(((roundBits ^ (1 << (__ext_bits - 1))) == 0) & roundNearestEven);
   if(zSig == 0)
      zExp = 0;
   return __packFloat32(zSign, zExp, zSig, __exp_bits, __frac_bits);
}

/*----------------------------------------------------------------------------
| Packs the sign `zSign', exponent `zExp', and significand `zSig' into a
| double-precision floating-point value, returning the result.  After being
| shifted into the proper positions, the three fields are simply added
| together to form the result.  This means that any integer portion of `zSig'
| will be added into the exponent.  Since a properly normalized significand
| will have an integer portion equal to 1, the `zExp' input should be 1 less
| than the desired result exponent whenever `zSig' is a complete, normalized
| significand.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float64_t __packFloat64(bool zSign, __uint64_t zExp, __uint64_t zSig, __uint8_t __exp_bits,
                                                __uint8_t __frac_bits)
{
   return (((__uint64_t)zSign) << (__exp_bits + __frac_bits)) + (zExp << __frac_bits) + zSig;
}

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent `zExp',
| and significand `zSig', and returns the proper double-precision floating-
| point value corresponding to the abstract input.  Ordinarily, the abstract
| value is simply rounded and packed into the double-precision format, with
| the inexact exception raised if the abstract input cannot be represented
| exactly.  However, if the abstract value is too large, the overflow and
| inexact exceptions are raised and an infinity or maximal finite value is
| returned.  If the abstract value is too small, the input value is rounded
| to a subnormal number, and the underflow and inexact exceptions are raised
| if the abstract input cannot be represented exactly as a subnormal double-
| precision floating-point number.
|     The input significand `zSig' has its binary point between bits 62
| and 61, which is 10 bits to the left of the usual location.  This shifted
| significand must be normalized or smaller.  If `zSig' is not normalized,
| `zExp' must be 0; in that case, the result returned is a subnormal number,
| and it must not require rounding.  In the usual case that `zSig' is
| normalized, `zExp' must be 1 less than the ``true'' floating-point exponent.
| The handling of underflow and overflow follows the IEC/IEEE Standard for
| Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float64_t __roundAndPackFloat64(bool zSign, __int32_t zExp, __uint64_t zSig,
                                                        __uint8_t __exp_bits, __uint8_t __frac_bits, __exc_mode_t __exc,
                                                        bool __subnorm)
{
   __int8_t roundingMode;
   bool roundNearestEven;
   __int64_t roundIncrement, roundBits;
   bool isTiny;
   __uint8_t __ext_bits = (62 - __frac_bits);
   __int32_t __exp_max = ((1LL << __exp_bits) - 1);

   roundingMode = __float_rounding_mode;
   roundNearestEven = (roundingMode == float_round_nearest_even);
   roundIncrement = 1ULL << (__ext_bits - 1);
#ifndef NO_PARAMETRIC
   if(!roundNearestEven)
   {
      if(roundingMode == float_round_to_zero)
      {
         roundIncrement = 0;
      }
      else
      {
         roundIncrement = (1ULL << __ext_bits) - 1;
         if(zSign)
         {
            if(roundingMode == float_round_up)
            {
               roundIncrement = 0;
            }
         }
         else
         {
            if(roundingMode == float_round_down)
            {
               roundIncrement = 0;
            }
         }
      }
   }
#endif
   roundBits = zSig & ((1ULL << __ext_bits) - 1);
   if((__exp_max - ((__exc == FLOAT_EXC_STD) ? 2 : 1)) <= (__uint32_t)zExp)
   {
      if(((__exp_max - ((__exc == FLOAT_EXC_STD) ? 2 : 1)) < zExp) ||
         ((zExp == (__exp_max - ((__exc == FLOAT_EXC_STD) ? 2 : 1))) && ((__int64_t)(zSig + roundIncrement) < 0)))
      {
         __float_raise(float_flag_overflow | float_flag_inexact);
         return ((((__uint64_t)zSign) << (__exp_bits + __frac_bits)) | (((__uint64_t)__exp_max) << __frac_bits) |
                 (((__uint64_t)(__exc != FLOAT_EXC_STD) << __frac_bits) - (__exc != FLOAT_EXC_STD))) -
                (roundIncrement == 0);
      }
      if(zExp < 0)
      {
         if(__subnorm)
         {
            isTiny = (__float_detect_tininess == float_tininess_before_rounding) || (zExp < -1) ||
                     (zSig + roundIncrement < 0x8000000000000000LL);
            __shift64RightJamming(zSig, -zExp, &zSig);
            zExp = 0;
            roundBits = zSig & ((1ULL << __ext_bits) - 1);
            if(isTiny && roundBits)
            {
               __float_raise(float_flag_underflow);
            }
         }
         else
         {
            return ((__uint64_t)zSign) << (__exp_bits + __frac_bits); // __packFloat64(zSign, 0, 0, IEEE32_PACK);
         }
      }
   }
   if(roundBits)
   {
      __float_raise(float_flag_inexact);
   }
   zSig = (zSig + roundIncrement) >> __ext_bits;
   zSig &= ~(((roundBits ^ (1ULL << (__ext_bits - 1))) == 0) & roundNearestEven);
   if(zSig == 0)
   {
      zExp = 0;
   }
   return __packFloat64(zSign, zExp, zSig, __exp_bits, __frac_bits);
}

static __FORCE_INLINE __tfloat_t __int_to_float_impl(__uint64_t aAbs, bool aSign, __uint8_t __a_bits,
                                                     __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                                                     __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm,
                                                     __int8_t __sign)
{
   __int8_t shiftCount;
   __uint64_t zSig, zSigShift;
   __int32_t __exp_int = (__a_bits - 2) - __exp_bias;

   if(aAbs == 0)
      return 0;
   if(__sign == 1 && !aSign)
   {
      return 0;
   }
   if(__sign == 0 && aSign)
   {
      return 0;
   }
   if(((1 << __exp_bits) - 1) < __exp_int)
   {
      // TODO: fix __exp_int and aAbs to comply with lower exponent
   }
   count_leading_zero_lshift_runtime_macro(__a_bits, aAbs, shiftCount, zSig);
   if(__frac_bits < (__a_bits - 1))
   {
      if(__a_bits >= 64)
      {
         zSigShift = (zSig >> (__a_bits - 63)) | ((zSig & ((1ULL << (__a_bits - 63)) - 1ULL)) != 0);
      }
      else
      {
         zSigShift = zSig << (63 - __a_bits);
      }
      return __roundAndPackFloat64(aSign, __exp_int - shiftCount, zSigShift, __exp_bits, __frac_bits, __exc, __subnorm);
   }
   // TODO: handle overflow if 2^__a_bits exceeds exponent range
   zSigShift = zSig << (__frac_bits - __a_bits + 1);
   return __packFloat64(aSign, __exp_int - shiftCount, zSigShift, __exp_bits, __frac_bits);
}

__tfloat_t __int_to_float(__int64_t a, __uint8_t __a_bits, __uint8_t __exp_bits, __uint8_t __frac_bits,
                          __int32_t __exp_bias, __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm,
                          __int8_t __sign)
{
   return __int_to_float_impl(a < 0 ? -a : a, a < 0, __a_bits, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one,
                              __subnorm, __sign);
}

__tfloat_t __uint_to_float(__uint64_t a, __uint8_t __a_bits, __uint8_t __exp_bits, __uint8_t __frac_bits,
                           __int32_t __exp_bias, __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm,
                           __int8_t __sign)
{
   return __int_to_float_impl(a, 0, __a_bits, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm,
                              __sign);
}

/*----------------------------------------------------------------------------
| Returns the result of converting the 64/32/16/8-bit two's complement integer
| `a' to the custom precision floating-point format.  The conversion is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__tfloat_t __int64_to_float(__int64_t a, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                            __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   return __int_to_float_impl(a < 0 ? -a : a, a < 0, 64, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one,
                              __subnorm, __sign);
}

__tfloat_t __int32_to_float(__int32_t a, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                            __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   return __int_to_float_impl(a < 0 ? -a : a, a < 0, 32, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one,
                              __subnorm, __sign);
}

__tfloat_t __int16_to_float(__int16_t a, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                            __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   return __int_to_float_impl(a < 0 ? -a : a, a < 0, 16, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one,
                              __subnorm, __sign);
}

__tfloat_t __int8_to_float(__int8_t a, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                           __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   return __int_to_float_impl(a < 0 ? -a : a, a < 0, 8, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one,
                              __subnorm, __sign);
}

/*----------------------------------------------------------------------------
| Returns the result of converting the 64/32/16/8-bit unsigned integer `a'
| to the custom precision floating-point format.  The conversion is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__tfloat_t __uint64_to_float(__uint64_t a, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                             __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   return __int_to_float_impl(a, 0, 64, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}

__tfloat_t __uint32_to_float(__uint32_t a, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                             __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   return __int_to_float_impl(a, 0, 32, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}

__tfloat_t __uint16_to_float(__uint16_t a, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                             __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   return __int_to_float_impl(a, 0, 16, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}

__tfloat_t __uint8_to_float(__uint8_t a, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                            __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   return __int_to_float_impl(a, 0, 8, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}

/*----------------------------------------------------------------------------
| Returns the result of converting the custom-precision floating-point value
| `a' to the r-bit two's complement integer format.  The conversion is
| performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic---which means in particular that the conversion is rounded
| according to the current rounding mode.  If `a' is a NaN, the largest
| positive integer is returned.  Otherwise, if the conversion overflows, the
| largest integer with the same sign as `a' is returned.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __int64_t __float_to_int_impl(__tfloat_t a, __uint8_t __r_bits, bool __r_sign,
                                                    __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                                                    __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm,
                                                    __int8_t __sign)
{
   bool aSign, roundNearestEven, notZero;
   __int8_t roundingMode;
   __int32_t aExp, shiftCount;
   __int32_t __exp_max = ((1LL << __exp_bits) - 1) + __exp_bias;
   __int32_t __exp_min = __exp_bias + 1;
   __int64_t aFixed;
   __uint64_t aSig, roundIncrement, roundBits;
   __uint64_t aSigShift;
   // ac_int<128, false> aSigShift;

   aSig = __extractFloatFrac(a, __frac_bits);
   aExp = __extractFloatExp(a, __exp_bits, __frac_bits);
   aSign = __extractFloatSign(a, __exp_bits, __frac_bits, __sign);
   notZero = aExp != 0;
#ifndef NO_PARAMETRIC
   roundingMode = __float_rounding_mode;
#else
   // Default rounding mode for float_to_int cast operations in GCC and Clang is round_to_zero, not
   // round_to_nearest_even as per all other floating-point operators
   roundingMode = float_round_to_zero;
#endif
   roundNearestEven = (roundingMode == float_round_nearest_even);
   roundIncrement = (1ULL << (__frac_bits - 1));
   if(!roundNearestEven)
   {
      if(roundingMode == float_round_to_zero)
      {
         roundIncrement = 0;
      }
      else
      {
         roundIncrement = (1ULL << __frac_bits) - 1ULL;
         if(aSign)
         {
            if(roundingMode == float_round_up)
               roundIncrement = 0;
         }
         else
         {
            if(roundingMode == float_round_down)
               roundIncrement = 0;
         }
      }
   }
   shiftCount = aExp + __exp_bias + 1;
   shiftCount = ((shiftCount << (31 - __exp_bits)) >> (31 - __exp_bits));
   if(__exp_max < 0 || shiftCount < 0)
   {
      __float_raise(float_flag_inexact);
      return 0;
   }
   if(__exp_min > (__r_bits - __r_sign) || shiftCount > (__r_bits - __r_sign) ||
      (!__r_sign && aSign && notZero && shiftCount != 0))
   {
      __float_raise(float_flag_invalid);
      return __r_sign ? (1ULL << (__r_bits - 1)) : ((1ULL << __r_bits) - 1ULL);
   }
   shiftCount &= 0x7F;
   aSigShift = aSig | (((__uint64_t)(notZero && __one)) << __frac_bits);
   // roundBits = aSigShift.to_uint64() & ((1ULL << (__frac_bits + 1)) - 1ULL);
   if(__rnd)
   {
      if((__frac_bits + __r_bits - __r_sign) < 64)
      {
         aSigShift <<= shiftCount;
         roundBits = aSigShift & ((1ULL << (__frac_bits + 1)) - 1ULL);
         aFixed = (((aSigShift >> 1) + roundIncrement) >> __frac_bits);
      }
      else
      {
         if((__r_bits - __r_sign) < 64)
         {
            roundBits = (aSigShift << shiftCount) & ((1ULL << (__frac_bits + 1)) - 1ULL);
            aFixed = (((aSigShift << (62 - __frac_bits)) >> (63 - shiftCount)));
         }
         else
         {
            roundBits =
                ((shiftCount >> 6) & 1) ? 0ULL : ((aSigShift << shiftCount) & ((1ULL << (__frac_bits + 1)) - 1ULL));
            aFixed = shiftCount != 0 ? (((aSigShift << (63 - __frac_bits)) >> (64 - shiftCount))) : 0ULL;
         }
         aFixed = aFixed + ((((roundBits >> 1) + roundIncrement) >> __frac_bits) & 1);
      }
      // aFixed = (((aSigShift >> 1) + roundIncrement) >> __frac_bits).to_int64();
      aFixed = aFixed & ~(((roundBits ^ (1ULL << __frac_bits)) == 0) & roundNearestEven);
   }
   else
   {
      roundBits = (aSigShift << shiftCount) & ((1ULL << (__frac_bits + 1)) - 1ULL);
      aFixed = (aSigShift >> (__frac_bits + 1));
      // aFixed = (aSigShift >> (__frac_bits + 1)).to_int64();
   }
   if(__r_bits < 64)
   {
      aFixed = aFixed & ((1ULL << __r_bits) - 1ULL) | (((aFixed >> __r_bits) & 1) != 0 ? -1LL : 0ULL);
   }
   if(roundBits)
   {
      __float_raise(float_flag_inexact);
   }
   return aSign ? -aFixed : aFixed;
}

__int64_t __float_to_int(__tfloat_t a, __uint8_t __r_bits, __uint8_t __exp_bits, __uint8_t __frac_bits,
                         __int32_t __exp_bias, __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm,
                         __int8_t __sign)
{
   return __float_to_int_impl(a, __r_bits, 1, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm,
                              __sign);
}

__uint64_t __float_to_uint(__tfloat_t a, __uint8_t __r_bits, __uint8_t __exp_bits, __uint8_t __frac_bits,
                           __int32_t __exp_bias, __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm,
                           __int8_t __sign)
{
   return __float_to_int_impl(a, __r_bits, 0, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm,
                              __sign);
}

__int32_t __float_to_int32(__tfloat_t a, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                           __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   return __float_to_int_impl(a, 32, 1, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}

__uint32_t __float_to_uint32(__tfloat_t a, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                             __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   return __float_to_int_impl(a, 32, 0, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}

__int64_t __float_to_int64(__tfloat_t a, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                           __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   return __float_to_int_impl(a, 64, 1, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}

__uint64_t __float_to_uint64(__tfloat_t a, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                             __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   return __float_to_int_impl(a, 64, 0, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}

/*----------------------------------------------------------------------------
| Returns the result of converting the single-precision floating-point value
| `a' to the double-precision floating-point format.  The conversion is
| performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

#define __normalizeFloat32Subnormal(aSig, zExp, zSig)   \
   {                                                    \
      __int8_t __shiftCount;                            \
      count_leading_zero_macro(24, aSig, __shiftCount); \
      zSig = aSig << __shiftCount;                      \
      zExp = 1 - __shiftCount;                          \
   }

__float64_t __float32_to_float64_ieee(__float32_t a, __exc_mode_t __exc, bool __subnorm)
{
   bool aSign;
   __int16_t aExp;
   __uint32_t aSig;

   aSig = __extractFloatFrac(a, IEEE32_EXTRACT_FRAC);
   aExp = __extractFloatExp(a, IEEE32_EXTRACT_EXP);
   aSign = __extractFloatSign(a, IEEE32_EXTRACT_SIGN);
   if(aExp == 0xFF && (__exc == FLOAT_EXC_STD))
   {
      return (((__uint64_t)aSign) << 63) | (((__uint64_t)(aSig != 0)) << (IEEE64_FRAC_BITS - 1)) | 0x7FF0000000000000;
   }
   if(aExp == 0)
   {
      if(__subnorm)
      {
         if(aSig == 0)
            return ((__uint64_t)aSign) << 63; // __packFloat64(aSign, 0, 0, IEEE64_PACK);
         __normalizeFloat32Subnormal(aSig, aExp, aSig);
         --aExp;
      }
      else
      {
         return ((__uint64_t)aSign) << 63; // __packFloat64(aSign, 0, 0, IEEE64_PACK);
      }
   }
   return __packFloat64(aSign, aExp + ((__int16_t)0x380), ((__uint64_t)aSig) << 29, IEEE64_PACK);
}

#define needed_bits(in, count)                        \
   {                                                  \
      __uint32_t i;                                   \
      __uint8_t lz;                                   \
      i = in > 0 ? in : -in;                          \
      count_leading_zero_runtime_macro(32, in, lz);   \
      lz = in > 0 ? lz : (lz + ((i & (i - 1)) != 0)); \
      count = 32 - lz;                                \
   }

#define MAX(a, b) (a > b ? a : b)

__tfloat_t __float_cast(__tfloat_t bits, __uint8_t __in_exp_bits, __uint8_t __in_frac_bits, __int32_t __in_exp_bias,
                        __rnd_mode_t __in_rnd, __exc_mode_t __in_exc, bool __in_has_one, bool __in_has_subnorm,
                        __int8_t __in_sign, __uint8_t __out_exp_bits, __uint8_t __out_frac_bits,
                        __int32_t __out_exp_bias, __rnd_mode_t __out_rnd, __exc_mode_t __out_exc, bool __out_has_one,
                        bool __out_has_subnorm, __int8_t __out_sign)
{
   __uint64_t Sign, Exp, Frac, FExp, SFrac, RExp, NFrac, RFrac, expOverflow, ExExp, FSign, out_val;
   __int32_t __biasDiff, __rangeDiff;
   __uint8_t __exp_bits_diff, __bits_diff;
   bool ExpOverflow, ExpUnderflow, ExpNull, FracNull, inputZero;
   bool GuardBit, LSB, RoundBit, Sticky, Round;
   bool in_nan, out_nan;

   Sign = bits >> (__in_exp_bits + __in_frac_bits);
   Exp = (bits >> (__in_frac_bits)) & ((1ULL << __in_exp_bits) - 1);
   Frac = bits & ((1ULL << __in_frac_bits) - 1);

   __exp_bits_diff =
       __in_exp_bits > __out_exp_bits ? (__in_exp_bits - __out_exp_bits) : (__out_exp_bits - __in_exp_bits);
   __uint8_t __nb_in_exp_bias, __nb_out_exp_bias, __exp_type_size;
   needed_bits(__in_exp_bias, __nb_in_exp_bias);
   needed_bits(__out_exp_bias, __nb_out_exp_bias);
   __exp_type_size =
       MAX((MAX(__in_exp_bits, __out_exp_bits) + (__exp_bits_diff == 1)), MAX(__nb_in_exp_bias, __nb_out_exp_bias));

   __biasDiff = __in_exp_bias - __out_exp_bias;
   __rangeDiff = ((1 << __out_exp_bits) - !__out_has_subnorm) - ((1 << __in_exp_bits) - !__in_has_subnorm);
   if((__in_exp_bits != __out_exp_bits) || (__in_exp_bias != __out_exp_bias))
   {
      FExp = Exp + ((__uint64_t)__biasDiff);
      if(__biasDiff < 0 || __biasDiff > __rangeDiff)
      {
         expOverflow = (FExp >> __out_exp_bits) & ((1ULL << (__exp_type_size - __out_exp_bits - 1)) - 1);
         ExpOverflow = expOverflow != 0ULL;
         ExpUnderflow = (FExp >> (__exp_type_size - 1)) & 1;
         if((ExpOverflow || ExpUnderflow) && bits != 0)
         {
            /// Invalid conversion
            return 0;
         }
         ExExp = ExpUnderflow ? 0ULL : ((1ULL << __out_exp_bits) - 1);
         FExp = FExp & ((1ULL << __out_exp_bits) - 1);
         FExp = ExpOverflow ? ExExp : FExp;
         Frac = ExpUnderflow ? 0 : Frac;
         ExpOverflow = ExpOverflow ^ ExpUnderflow;
      }
      else
      {
         ExpOverflow = 0;
         ExpUnderflow = 0;
      }

      FExp = FExp & ((1ULL << __out_exp_bits) - 1);
      ExpNull = Exp == 0;
      FracNull = Frac == 0;
      inputZero = ExpNull && FracNull;
      if(__biasDiff < 0 || __biasDiff > __rangeDiff)
      {
         inputZero = inputZero || ExpUnderflow;
      }
      FExp = inputZero ? 0ULL : FExp;
   }
   else
   {
      ExpOverflow = 0;
      ExpUnderflow = 0;
      if(__in_has_subnorm && !__out_has_subnorm)
      {
         ExpNull = Exp == 0;
         Frac = ExpNull ? 0ULL : Frac;
      }
      FExp = Exp;
   }

   if(__in_frac_bits > __out_frac_bits)
   {
      __bits_diff = __in_frac_bits - __out_frac_bits;

      SFrac = Frac >> __bits_diff;

      if(__out_rnd == FLOAT_RND_NEVN)
      {
         GuardBit = (Frac >> (__bits_diff - 1)) & 1;

         LSB = 0;
         if(__bits_diff > 1)
         {
            RoundBit = (Frac >> (__bits_diff - 2)) & 1;
            LSB = LSB | RoundBit;
         }

         if(__bits_diff > 2)
         {
            Sticky = (Frac & ((1ULL << (__bits_diff - 2)) - 1)) != 0;
            LSB = LSB | Sticky;
         }

         Round = GuardBit & LSB;
         SFrac = SFrac | ((__uint64_t)Round);
      }
   }
   else if(__in_frac_bits < __out_frac_bits)
   {
      __bits_diff = __out_frac_bits - __in_frac_bits;
      SFrac = Frac << __bits_diff;
   }
   else
   {
      SFrac = Frac;
   }

   out_nan = 0;
   if(__out_sign != -1 && __in_sign != __out_sign)
   {
      if(__in_sign == -1)
      {
         out_nan |= Sign != (__out_sign == 1 ? 1 : 0);
      }
      else
      {
         /// Invalid conversion
         return 0;
      }
   }

   if(__in_exc == FLOAT_EXC_STD)
   {
      out_nan |= Exp == ((1ULL << __in_exp_bits) - 1);
   }

   RExp = out_nan ? ((1ULL << __out_exp_bits) - 1) : FExp;
   RExp <<= __out_frac_bits;

   if(__biasDiff < 0 || __biasDiff > __rangeDiff)
   {
      out_nan |= ExpOverflow;
   }

   if(__out_exc == FLOAT_EXC_STD)
   {
      if(__in_exc == FLOAT_EXC_STD)
      {
         in_nan = (Exp == ((1ULL << __in_exp_bits) - 1)) && (Frac != 0);
         NFrac = in_nan ? ((1ULL << __out_frac_bits) - 1) : 0;
      }
      else
      {
         NFrac = 0;
      }
   }
   else
   {
      NFrac = ((1ULL << __out_frac_bits) - 1);
   }

   RFrac = out_nan ? NFrac : SFrac;

   out_val = RExp | RFrac;

   if(__out_sign == -1)
   {
      if(__in_sign != -1)
      {
         FSign = __in_sign == 1 ? (1ULL << (__out_exp_bits + __out_frac_bits)) : 0;
      }
      else
      {
         FSign = Sign << (__out_exp_bits + __out_frac_bits);
      }
      out_val |= FSign;
   }

   return out_val;
}

#define FP_CLS_ZERO 0U
#define FP_CLS_NORMAL 1U
#define FP_CLS_INF 2U
#define FP_CLS_NAN 3U

/*----------------------------------------------------------------------------
| Returns the result of converting the double-precision floating-point value
| `a' to the single-precision floating-point format.  The conversion is
| performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

__float32_t __float64_to_float32_ieee(__float64_t a, __exc_mode_t __exc, bool __subnorm)
{
   bool aSign;
   __int16_t aExp;
   __uint64_t aSig;
   __uint32_t zSig;

   aSig = __extractFloatFrac(a, IEEE64_EXTRACT_FRAC);
   aExp = __extractFloatExp(a, IEEE64_EXTRACT_EXP);
   aSign = __extractFloatSign(a, IEEE64_EXTRACT_SIGN);
   if(aExp == 0x7FF && (__exc == FLOAT_EXC_STD))
   {
      return (((__uint32_t)aSign) << (IEEE32_EXP_BITS + IEEE32_FRAC_BITS)) |
             (((__uint32_t)(aSig != 0)) << (IEEE32_FRAC_BITS - 1)) | 0x7F800000;
   }
   __shift64RightJamming(aSig, 22, &aSig);
   zSig = aSig;
   if(aExp || zSig)
   {
      zSig |= 0x40000000;
      aExp -= 0x381;
   }
   return __roundAndPackFloat32(aSign, aExp, zSig, IEEE32_PACK, __exc, __subnorm);
}

/*----------------------------------------------------------------------------
| Returns the result of adding the absolute values of the double-precision
| floating-point values `a' and `b'.  If `zSign' is 1, the sum is negated
| before being returned.  `zSign' is ignored if the result is a NaN.
| The addition is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

#define COND_EXPR_MACRO64(cond, a, b)                           \
   ((((__uint64_t)((((__int64_t)(cond)) << 63) >> 63)) & (a)) | \
    ((~((__uint64_t)((((__int64_t)(cond)) << 63) >> 63))) & (b)))

static __FORCE_INLINE __tfloat_t __float_addsub(__tfloat_t _a, __tfloat_t _b, bool sub, __uint8_t __exp_bits,
                                                __uint8_t __frac_bits, __int32_t __exp_bias, __rnd_mode_t __rnd,
                                                __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   __uint64_t a, b;
   __uint64_t aSig, bSig, shift_0;
   __uint64_t aExp, bExp, expDiff;
   __uint16_t nZeros;
   bool a_c_nan, b_c_nan, a_c_normal, b_c_normal, tmp_c_normal, swap, sAB, aSign, bSign;
   __uint64_t abs_a, abs_b;
   __uint64_t fA, fB, fB_shifted;
   __uint64_t fB_shifted_low, fBleft_shifted;
   bool LSB_bit, Guard_bit, Round_bit, Sticky_bit, round, sb;
   bool ge_frac_bits;
   // __uint64_t tmp_x;
   bool subnormal_exp_correction;
   __uint64_t fB_shifted1;
   __uint64_t fR0;
   bool R_c_zero;
   __uint64_t RExp0, RExp1;
   __uint64_t RSig0, RSig1, RSig2, RSig3;
   __uint64_t RExp0RSig1, Rrounded;
   bool overflow_to_infinite, saturation;
   bool aExpMax, bExpMax, aExp_null, bExp_null, aSig_not, bSig_not;
   __uint8_t __frac_shift, __frac_full, __frac_almost, __exp_shift, __nzeros_bits;

   __frac_shift = (__rnd == FLOAT_RND_NEVN) ? 2 : 0;
   __frac_almost = __frac_bits + __frac_shift + 1;
   __frac_full = __frac_almost + 1;

   abs_a = _a & ((1ULL << (__exp_bits + __frac_bits)) - 1);
   abs_b = _b & ((1ULL << (__exp_bits + __frac_bits)) - 1);
   swap = abs_a < abs_b;

   // Faster, but more area (~0.35 slice/bit more than COND_EXPR_MACRO64)
   // tmp_x = (_a ^ _b) & ((__uint64_t)((((__int64_t)(swap)) << 63) >> 63));
   // a = _a ^ tmp_x;
   // b = _b ^ tmp_x;
   // Slower, but less area
   a = COND_EXPR_MACRO64(swap, _b, _a);
   b = COND_EXPR_MACRO64(swap, _a, _b);

   aSign = __extractFloatSign(a, __exp_bits, __frac_bits, __sign) ^ (sub & swap);
   aSig = __extractFloatFrac(a, __frac_bits);
   aExp = __extractFloatExp(a, __exp_bits, __frac_bits);
   bSign = __extractFloatSign(b, __exp_bits, __frac_bits, __sign) ^ (sub & !swap);
   bSig = __extractFloatFrac(b, __frac_bits);
   bExp = __extractFloatExp(b, __exp_bits, __frac_bits);
   aExp_null = aExp == 0;
   bExp_null = bExp == 0;
   aSig_not = aSig != 0;
   bSig_not = bSig != 0;
   aExpMax = aExp == ((1ULL << __exp_bits) - 1);
   bExpMax = bExp == ((1ULL << __exp_bits) - 1);
   a_c_nan = (__exc == FLOAT_EXC_STD) & aExpMax & aSig_not;
   a_c_normal = __one & !aExp_null;
   b_c_nan = (__exc == FLOAT_EXC_STD) & bExpMax & bSig_not;
   b_c_normal = __one & !bExp_null;

   sAB = aSign ^ bSign;

   if(__subnorm)
   {
      subnormal_exp_correction = (aExp_null & aSig_not) ^ (bExp_null & bSig_not);
      expDiff = aExp - bExp - subnormal_exp_correction;
   }
   else
   {
      expDiff = aExp - bExp;
   }
   expDiff = expDiff & ((1ULL << __exp_bits) - 1);

   fA = (aSig | (((__uint64_t)a_c_normal) << __frac_bits)) << __frac_shift;
   fB = (bSig | (((__uint64_t)b_c_normal) << __frac_bits)) << __frac_shift;

   __exp_shift =
       (__frac_bits > 1 ?
            (__frac_bits > 2 ?
                 (__frac_bits > 4 ? (__frac_bits > 8 ? (__frac_bits > 16 ? (__frac_bits > 32 ? 6 : 5) : 4) : 3) : 2) :
                 1) :
            0);
   ge_frac_bits = (expDiff >> __exp_shift) != 0;
   expDiff = (expDiff | (__uint32_t)(((((__int32_t)ge_frac_bits) << 31) >> 31))) & ((1ULL << __exp_shift) - 1);

   if(__rnd == FLOAT_RND_NEVN)
   {
      fB_shifted_low = fB & (~((~(0ULL)) << expDiff));
      sb = fB_shifted_low != 0;
   }
   else
   {
      sb = 0;
   }
   fB_shifted = fB >> expDiff;
   fB_shifted = fB_shifted & ((1ULL << __frac_almost) - 1);

   fB_shifted1 = ((__uint64_t)((((__int64_t)sAB) << 63) >> 63)) ^ fB_shifted;
   fB_shifted1 = fB_shifted1 & ((1ULL << __frac_full) - 1);

   fR0 = fA + fB_shifted1 + (sAB && (!sb));

   fR0 = fR0 & ((1ULL << __frac_full) - 1);
   count_leading_zero_lshift64_runtime_macro(__frac_full, fR0, nZeros, shift_0);

   __nzeros_bits = RUNTIME_CEIL_LOG2(__frac_full);
   R_c_zero = nZeros == ((1ULL << __nzeros_bits) - 1);
   overflow_to_infinite =
       aExp == ((1ULL << __exp_bits) - ((__exc == FLOAT_EXC_STD) ? 2 : 1)) && (fR0 >> __frac_almost) & 1;
   overflow_to_infinite &= (__exc != FLOAT_EXC_OVF);

   if(__subnorm)
   {
      RExp0 = (R_c_zero || aExp < nZeros) ? (aExp_null && bExp_null && nZeros == 1) : (aExp - nZeros + 1);
      RSig0 = aExp < nZeros ? ((aExp_null && bExp_null) ? (fR0 << 1) : (fR0 << aExp)) : shift_0;
   }
   else
   {
      R_c_zero = R_c_zero || aExp < nZeros;
      RExp0 = R_c_zero ? 0 : (aExp - nZeros + 1);
      RSig0 = shift_0;
   }
   RExp0 = RExp0 & ((1ULL << __exp_bits) - 1);

   RSig1 = (RSig0 >> (__frac_shift + 1)) & ((1ULL << __frac_bits) - 1);

   RExp0RSig1 = (RExp0 << __frac_bits) | RSig1;

   if(__rnd == FLOAT_RND_NEVN)
   {
      LSB_bit = SELECT_BIT(RSig0, 3);
      Guard_bit = SELECT_BIT(RSig0, 2);
      Round_bit = SELECT_BIT(RSig0, 1);
      Sticky_bit = SELECT_BIT(RSig0, 0) | sb;
      round = Guard_bit & (LSB_bit | Round_bit | Sticky_bit);
      Rrounded = RExp0RSig1 + round;
   }
   else
   {
      Rrounded = RExp0RSig1;
   }

   if(__exc == FLOAT_EXC_STD)
   {
      RExp1 =
          aExpMax || bExpMax ? ((1ULL << __exp_bits) - 1) : ((Rrounded >> __frac_bits) & ((1ULL << __exp_bits) - 1));
      RSig2 = R_c_zero || aExpMax || bExpMax || overflow_to_infinite ? 0 : (Rrounded & ((1ULL << __frac_bits) - 1));
   }
   else
   {
      RExp1 =
          overflow_to_infinite ? ((1ULL << __exp_bits) - 1) : ((Rrounded >> __frac_bits) & ((1ULL << __exp_bits) - 1));
      RSig2 = (overflow_to_infinite ? (__uint64_t)((((__int64_t)!R_c_zero) << 63) >> 63) : Rrounded) &
              ((1ULL << __frac_bits) - 1);
   }

   aSign = aSign && (!R_c_zero || !sAB);

   if(__exc == FLOAT_EXC_STD)
   {
#ifdef NO_SIGNALLING
      RSig3 = (((__uint64_t)(a_c_nan || b_c_nan || (sAB && aExpMax && bExpMax))) << (__frac_bits - 1)) | RSig2;
#else
      RSig3 = a_c_nan || b_c_nan || (sAB && aExpMax && bExpMax) ?
                  ((1ULL << (__frac_bits - 1)) | (aSig & ((1ULL << (__frac_bits - 1)) - 1))) :
                  RSig2;
#endif
   }
   else
   {
      RSig3 = RSig2;
   }
   return (((__uint64_t)aSign) << (__exp_bits + __frac_bits)) | (RExp1 << __frac_bits) | RSig3;
}

/*----------------------------------------------------------------------------
| Returns the result of adding the double-precision floating-point values `a'
| and `b'.  The operation is performed according to the IEC/IEEE Standard for
| Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__tfloat_t __float_add(__tfloat_t a, __tfloat_t b, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                       __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   return __float_addsub(a, b, 0, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}

/*----------------------------------------------------------------------------
| Returns the result of subtracting the double-precision floating-point values
| `a' and `b'.  The operation is performed according to the IEC/IEEE Standard
| for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__tfloat_t __float_sub(__tfloat_t a, __tfloat_t b, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                       __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   return __float_addsub(a, b, 1, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}

/*----------------------------------------------------------------------------
| Returns the result of multiplying the double-precision floating-point values
| `a' and `b'.  The operation is performed according to the IEC/IEEE Standard
| for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__tfloat_t __float_mul(__tfloat_t a, __tfloat_t b, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                       __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   bool aSign, bSign, zSign;
   bool aExpMax, bExpMax;
   __uint32_t aExp, bExp;
   __uint64_t aSig, bSig;
   __uint8_t a_c, b_c, z_c;
   bool a_c_zero, b_c_zero, a_c_inf, b_c_inf, a_c_nan, b_c_nan, a_c_normal, b_c_normal;
   __uint32_t expSum, expPostNorm;
   bool norm, expSigOvf0, expSigOvf2;
   bool sticky, guard, round, expSigOvf1;
   __uint64_t expSig, expSigPostRound, zSig;
   __uint64_t excPostNorm;
   __uint64_t u1, u0, v1, v0, k0, k1, k2, k3, k4, k5, k6, k7, res_2K_0, res_full_2K;
   __uint64_t sigProd, sigProdExt, sigProdHigh, sigProdLow, sigProdExtHigh, sigProdExtLow;
   __uint8_t __frac_mul, __frac_full, __frac_almost, __k_bits;
   __frac_almost = __frac_bits + 1;
   __frac_full = __frac_almost + 1;
   __frac_mul = __frac_bits + __frac_bits + 1;
   __k_bits = __frac_almost > 38 ? (__frac_almost - 30) : (__frac_almost - 15);

   if(__sign == 1)
   {
      return (__exc == FLOAT_EXC_STD) ? __float_nan(__exp_bits, __frac_bits, __sign) : 0;
   }

   aSig = __extractFloatFrac(a, __frac_bits);
   aExp = __extractFloatExp(a, __exp_bits, __frac_bits);
   aSign = __extractFloatSign(a, __exp_bits, __frac_bits, __sign);
   bSig = __extractFloatFrac(b, __frac_bits);
   bExp = __extractFloatExp(b, __exp_bits, __frac_bits);
   bSign = __extractFloatSign(b, __exp_bits, __frac_bits, __sign);
   zSign = aSign ^ bSign;

   aExpMax = aExp == ((1ULL << __exp_bits) - 1);
   a_c_zero = (aExp == 0) & ((aSig == 0) | !__subnorm);
   a_c_inf = aExpMax && aSig == 0;
   a_c_inf = (__exc == FLOAT_EXC_STD) ? a_c_inf : 0;
   a_c_nan = aExpMax && aSig != 0;
   a_c_nan = (__exc == FLOAT_EXC_STD) ? a_c_nan : 0;
   a_c_normal = !a_c_zero && !aExpMax;
   a_c_normal = (__exc == FLOAT_EXC_STD) ? a_c_normal : !a_c_zero;
   a_c = /*((a_c_zero << 1 | a_c_zero) & FP_CLS_ZERO) |*/ ((a_c_normal << 1 | a_c_normal) & FP_CLS_NORMAL) |
         ((a_c_inf << 1 | a_c_inf) & FP_CLS_INF) | ((a_c_nan << 1 | a_c_nan) & FP_CLS_NAN);

   bExpMax = bExp == ((1ULL << __exp_bits) - 1);
   b_c_zero = (bExp == 0) & ((bSig == 0) | !__subnorm);
   b_c_inf = bExpMax && bSig == 0;
   b_c_inf = (__exc == FLOAT_EXC_STD) ? b_c_inf : 0;
   b_c_nan = bExpMax && bSig != 0;
   b_c_nan = (__exc == FLOAT_EXC_STD) ? b_c_nan : 0;
   b_c_normal = !b_c_zero && !bExpMax;
   b_c_normal = (__exc == FLOAT_EXC_STD) ? b_c_normal : !b_c_zero;
   b_c = /*((b_c_zero << 1 | b_c_zero) & FP_CLS_ZERO) |*/ ((b_c_normal << 1 | b_c_normal) & FP_CLS_NORMAL) |
         ((b_c_inf << 1 | b_c_inf) & FP_CLS_INF) | ((b_c_nan << 1 | b_c_nan) & FP_CLS_NAN);

   z_c = ((a_c >> 1 | b_c >> 1) << 1) |
         (((a_c >> 1) & (a_c & 1)) | ((b_c >> 1) & (b_c & 1)) | ((a_c & 1) & (b_c & 1)) |
          (1 & (~(a_c >> 1)) & ((~a_c) & 1) & (b_c >> 1)) | (1 & (~(b_c >> 1)) & ((~b_c) & 1) & (a_c >> 1)));
   if(__exc == FLOAT_EXC_OVF)
   {
      z_c = z_c & 1;
   }
   expSum = aExp + bExp + ((__uint32_t)__exp_bias);

   aSig = (aSig | (((__uint64_t)(__one & (a_c_normal | !__subnorm))) << __frac_bits));
   bSig = (bSig | (((__uint64_t)(__one & (b_c_normal | !__subnorm))) << __frac_bits));

   if(__frac_almost > 32)
   {
#if 0
      // start multi-part multiplication __frac_almostx__frac_almost=>2*__frac_almost
      // karatsuba
      // u = 2^K*u1+u0; //__frac_almost-bit -> u1=(__frac_almost-__k_bits)bit u0=__k_bits-bit
      // v = 2^K*v1+v0; //__frac_almost-bit -> v1=(__frac_almost-__k_bits)bit v0=__k_bits-bit
      // k0=u1*v1;      // k0=2*(__frac_almost-__k_bits)bit
      // k1=u0*v0;      // k1=2*__k_bits-bit
      // k3=u0+u1;      // k3=1+(__frac_almost-__k_bits)bit
      // k4=v0+v1;      // k4=1+(__frac_almost-__k_bits)bit
      // k5=k3*k4;      // k5=2*(1+(__frac_almost-__k_bits)))bit
      // k6=k5-k0-k1;   // k6=1+(2*(1+(__frac_almost-__k_bits))))bit

      u0 = aSig & ((1ULL << __k_bits) - 1);
      u1 = (aSig >> __k_bits) & ((1ULL << (__frac_almost - __k_bits)) - 1);
      v0 = bSig & ((1ULL << __k_bits) - 1);
      v1 = (bSig >> __k_bits) & ((1ULL << (__frac_almost - __k_bits)) - 1);
      k0 = u1 * v1;
      k1 = u0 * v0;
      k3 = u0 + u1;
      k4 = v0 + v1;
      k5 = k3 * k4;
      k6 = k5 - k0 - k1;
      k7 = (k1 >> __k_bits) + k6;
      res_2K_0 = (k1 & ((1ULL << __k_bits) - 1)) | ((k7 & ((1ULL << __k_bits) - 1)) << __k_bits);
      res_full_2K = (k7 >> __k_bits) + k0;

      if((2 * __k_bits) == __frac_full)
      {
         sigProdLow = res_2K_0;
         sigProdHigh = res_full_2K;
      }
      else if((2 * __k_bits) < __frac_full)
      {
         sigProdLow = ((res_full_2K & ((1ULL << (__frac_full - 2 * __k_bits)) - 1)) << (2 * __k_bits)) | res_2K_0;
         sigProdHigh = res_full_2K >> (__frac_full - 2 * __k_bits);
      }
      else
      {
         sigProdLow = res_2K_0 & ((1ULL << __frac_full) - 1);
         sigProdHigh = (res_full_2K << (2 * __k_bits - __frac_full)) | ((res_2K_0 >> __frac_full) & ((1ULL << (2 * __k_bits - __frac_full)) - 1));
      }
#else
      __uint64_t u0, u1, v1, v0, ts, ks, k, t, w1, w2, w3, w_0, w_1;
      __uint8_t __low_high = __frac_bits - (__frac_mul - 63);
      u0 = aSig >> 32;
      u1 = aSig & 0xFFFFFFFF;
      v0 = bSig >> 32;
      v1 = bSig & 0xFFFFFFFF;
      t = u1 * v1;
      w3 = t & 0xFFFFFFFF;
      k = t >> 32;
      t = u0 * v1 + k;
      w2 = t & 0xFFFFFFFF;
      w1 = t >> 32;
      ts = u1 * v0 + w2;
      ks = ts >> 32;
      w_0 = u0 * v0 + w1 + ks;
      w_1 = (ts << 32) + w3;
      sigProdLow = w_1 & ((1ULL << (64 - __low_high)) - 1);
      sigProdHigh = ((w_0 << __low_high) | ((w_1 >> (64 - __low_high)) & ((1ULL << __low_high) - 1))) &
                    ((1ULL << __frac_bits) - 1);
#endif

      norm = (sigProdHigh >> (__frac_bits - 1)) & 1;
      expPostNorm = expSum + norm;

      sigProdExtLow = ((sigProdLow << 1) << !norm) & ((1ULL << __frac_full) - 1);
      sigProdLow = ((sigProdLow >> __frac_bits) & 3) >> norm;
      sigProdHigh = sigProdHigh << !norm;
      sigProdExtHigh = ((sigProdHigh << 1) | sigProdLow) & ((1ULL << __frac_bits) - 1);
      expSig = (((__uint64_t)expPostNorm) << __frac_bits) | sigProdExtHigh;
      sigProdExt = 0;
   }
   else
   {
      sigProd = (__uint64_t)(__uint32_t)(aSig) * (__uint32_t)(bSig);
      norm = (sigProd >> __frac_mul) & 1;
      expPostNorm = expSum + norm;
      sigProdExt = sigProd << !norm;
      sigProdExt = (sigProdExt & ((1ULL << __frac_mul) - 1)) << 1;
      expSig = (((__uint64_t)expPostNorm) << __frac_bits) | ((sigProdExt >> __frac_full) & ((1ULL << __frac_bits) - 1));
      sigProdExtHigh = 0;
      sigProdExtLow = 0;
   }
   if((__exp_bits + __frac_bits) < 62)
   {
      expSig = expSig & ((1ULL << (__exp_bits + __frac_bits + 2)) - 1);
   }
   expSigOvf0 = (expPostNorm >> (__exp_bits + 1)) & 1;

   if(__rnd == FLOAT_RND_NEVN)
   {
      if(__frac_almost > 32)
      {
         sticky = (sigProdExtLow >> __frac_almost) & 1;
         guard = (sigProdExtLow & ((1ULL << __frac_almost) - 1)) != 0;
         round = sticky & (guard | (sigProdExtHigh & 1));
      }
      else
      {
         sticky = (sigProdExt >> __frac_almost) & 1;
         guard = (sigProdExt & ((1ULL << __frac_almost) - 1)) != 0;
         round = sticky & (guard | ((sigProdExt >> __frac_full) & 1));
      }

      expSigPostRound = expSig + round;
      if((__exp_bits + __frac_bits) == 63)
      {
         expSigOvf1 = round & (expSig == ((__uint64_t)-1));
      }
      else
      {
         expSigOvf1 =
             round & (expSigPostRound >> (__exp_bits + __frac_bits + 1)) & !(expSig >> (__exp_bits + __frac_bits + 1));
      }
      expSigOvf2 = expSigOvf0 ^ expSigOvf1;
   }
   else
   {
      expSigPostRound = expSig;
      expSigOvf2 = expSigOvf0;
   }

   excPostNorm = (expSigOvf2 << 1) | ((expSigPostRound >> (__exp_bits + __frac_bits)) & 1) |
                 ((expSigPostRound >> __frac_bits) & ((1ULL << __exp_bits) - 1)) == ((1ULL << __exp_bits) - 1);
   zSig = (((__uint64_t)zSign) << (__exp_bits + __frac_bits)) |
          (expSigPostRound & ((1ULL << (__exp_bits + __frac_bits)) - 1));
   if(z_c == FP_CLS_NORMAL)
   {
      z_c = ((excPostNorm == 1) << 1) | (excPostNorm == 0);
   }

   if(z_c == FP_CLS_NORMAL)
      return zSig;
   else if(z_c == FP_CLS_ZERO)
      return ((__uint64_t)zSign) << (__exp_bits + __frac_bits); // __packFloat(zSign, 0, 0, __exp_bits, __frac_bits);
   else if(z_c == FP_CLS_NAN && (__exc == FLOAT_EXC_STD))
      return __float_nan(__exp_bits, __frac_bits, __sign);
   else
      return (((__uint64_t)zSign) << (__exp_bits + __frac_bits)) | (((1ULL << __exp_bits) - 1) << __frac_bits) |
             ((__exc == FLOAT_EXC_STD) ? 0ULL : ((1ULL << __frac_bits) - 1));
}

/*----------------------------------------------------------------------------
| Returns the result of dividing the double-precision floating-point value `a'
| by the corresponding value `b'.  The operation is performed according to
| the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

#define SRT4_DIV_STEP(z, n, data)                                  \
   current_sel = (((current >> (__div_p3 - 4)) & 15) << 1) | MsbB; \
   q_i0 = (0xF1FFFF6C >> current_sel) & 1;                         \
   q_i1 = (0xFE00FFD0 >> current_sel) & 1;                         \
   q_i2 = SELECT_BIT(current_sel, 4);                              \
   nq_i2 = !q_i2;                                                  \
   /*q_i = tableR4[current_sel];*/                                 \
   q_i = (q_i2 << 2) | (q_i1 << 1) | q_i0;                         \
   positive |= (q_i1 << 1) | q_i0;                                 \
   positive <<= 2;                                                 \
   negative |= q_i2 << 1;                                          \
   negative <<= 2;                                                 \
   switch(q_i)                                                     \
   {                                                               \
      case 1:                                                      \
         w = nbSig;                                                \
         break;                                                    \
      case 7:                                                      \
         w = bSig;                                                 \
         break;                                                    \
      case 2:                                                      \
         w = nbSigx2;                                              \
         break;                                                    \
      case 6:                                                      \
         w = bSigx2;                                               \
         break;                                                    \
      case 3:                                                      \
         w = nbSigx3;                                              \
         break;                                                    \
      case 5:                                                      \
         w = bSigx3;                                               \
         break;                                                    \
      default: /*case 0: case 4:*/                                 \
         w = 0;                                                    \
         break;                                                    \
   }                                                               \
   current = (current << 1) + w;                                   \
   current = current & ((1ULL << (__div_p3 - 1)) - 1);             \
   current <<= 1;

#define SRT4_MAX_UNROLL 31
#define SRT4_UNROLL_FACTOR 1

static __tfloat_t __FORCE_INLINE __kernel_float_divSRT4(__tfloat_t a, __tfloat_t b, __uint8_t __exp_bits,
                                                        __uint8_t __frac_bits, __int32_t __exp_bias, __rnd_mode_t __rnd,
                                                        __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign,
                                                        bool __full_unroll)
{
   __uint8_t a_c, b_c, z_c;
   bool a_c_zero, b_c_zero, a_c_inf, b_c_inf, a_c_nan, b_c_nan, a_c_normal, b_c_normal;
   bool aSign, bSign, zSign, q_i2, q_i1, q_i0, nq_i2;
   bool MsbB = (b >> (__frac_bits - 1)) & 1, correction;
   __int32_t aExp, bExp, zExp;
   __uint64_t aSig, bSig, nbSig, bSigx2, nbSigx2, zSig1, zSig0;
   __uint64_t zExpSig;
   __uint64_t bSigx3, nbSigx3, current, w, positive = 0, negative = 0;
   __uint8_t current_sel, q_i, index;
   bool LSB_bit, Guard_bit, Round_bit, round;
   bool MSB1zExp, MSB0zExp, MSBzExp, ovfCond;
   __uint8_t __frac_p3, __div_p3, __div_it, __div_bits, __div_waste;
   bool __frac_odd;
   __frac_p3 = (__rnd == FLOAT_RND_NEVN) ? (__frac_bits + 3) : (__frac_bits + 1);
   __div_p3 = __frac_bits + 3;
   __frac_odd = __frac_p3 & 1;
   __div_it = __frac_p3 / (SRT4_UNROLL_FACTOR * 2);
   __div_it = (__frac_p3 % (SRT4_UNROLL_FACTOR * 2)) != 0 ? (__div_it + 1) : __div_it;
   __div_bits = __div_it * SRT4_UNROLL_FACTOR * 2;
   __div_waste = __div_bits - __frac_p3 - __frac_odd;

   if(__sign == 1)
   {
      // Negative numbers division result is always out of negative numbers domain
      return (__exc == FLOAT_EXC_STD) ? __float_nan(__exp_bits, __frac_bits, __sign) : 0ULL;
   }

   aSig = __extractFloatFrac(a, __frac_bits);
   aExp = __extractFloatExp(a, __exp_bits, __frac_bits);
   aSign = __extractFloatSign(a, __exp_bits, __frac_bits, __sign);
   bSig = __extractFloatFrac(b, __frac_bits);
   bExp = __extractFloatExp(b, __exp_bits, __frac_bits);
   bSign = __extractFloatSign(b, __exp_bits, __frac_bits, __sign);
   zSign = aSign ^ bSign;
   bool aExp_null = aExp == 0;
   bool bExp_null = bExp == 0;
   bool aExpMax = aExp == ((1ULL << __exp_bits) - 1);
   bool bExpMax = bExp == ((1ULL << __exp_bits) - 1);
   bool aSig_null = aSig == 0;
   bool bSig_null = bSig == 0;

   a_c_zero = aExp_null & (aSig_null | !__subnorm);
   a_c_inf = aExpMax && (aSig_null || __exc == FLOAT_EXC_NONAN);
   a_c_inf = (__exc != FLOAT_EXC_OVF) ? a_c_inf : 0;
   a_c_nan = aExpMax && !aSig_null;
   a_c_nan = (__exc == FLOAT_EXC_STD) ? a_c_nan : 0;
   a_c_normal = !aExpMax && !a_c_zero;
   a_c_normal = (__exc != FLOAT_EXC_OVF) ? a_c_normal : !a_c_zero;
   a_c = /*((a_c_zero << 1 | a_c_zero) & FP_CLS_ZERO) |*/ ((a_c_normal << 1 | a_c_normal) & FP_CLS_NORMAL) |
         ((a_c_inf << 1 | a_c_inf) & FP_CLS_INF) | ((a_c_nan << 1 | a_c_nan) & FP_CLS_NAN);

   b_c_zero = bExp_null & (bSig_null | !__subnorm);
   b_c_inf = bExpMax && (bSig_null | __exc == FLOAT_EXC_NONAN);
   b_c_inf = (__exc != FLOAT_EXC_OVF) ? b_c_inf : 0;
   b_c_nan = bExpMax && !bSig_null;
   b_c_nan = (__exc == FLOAT_EXC_STD) ? b_c_nan : 0;
   b_c_normal = !bExpMax && !b_c_zero;
   b_c_normal = (__exc != FLOAT_EXC_OVF) ? b_c_normal : !b_c_zero;
   b_c = /*((b_c_zero << 1 | b_c_zero) & FP_CLS_ZERO) |*/ ((b_c_normal << 1 | b_c_normal) & FP_CLS_NORMAL) |
         ((b_c_inf << 1 | b_c_inf) & FP_CLS_INF) | ((b_c_nan << 1 | b_c_nan) & FP_CLS_NAN);

   z_c = ((a_c >> 1 | (1 & (~(b_c >> 1)) & (~(b_c & 1))) | (1 & (b_c >> 1) & b_c)) << 1) |
         ((1 & (a_c >> 1) & a_c) | (1 & (b_c >> 1) & b_c) | (1 & (a_c >> 1) & (b_c >> 1)) | (1 & a_c & b_c) |
          (1 & (~(a_c >> 1)) & (~(a_c & 1)) & (~(b_c >> 1)) & (~(b_c & 1))));
   if(__exc == FLOAT_EXC_OVF)
   {
      z_c = z_c & 1;
   }

   if(__subnorm)
   {
      if(aExp_null && !aSig_null)
      {
         __uint64_t subnormal_lz, mshifted;
         count_leading_zero_lshift_runtime_macro(__frac_bits, aSig, subnormal_lz, mshifted);
         aExp = -subnormal_lz;
         aSig = (mshifted << 1) & ((1ULL << __frac_bits) - 1);
      }
      if(bExp_null && !bSig_null)
      {
         __uint64_t subnormal_lz, mshifted;
         count_leading_zero_lshift_runtime_macro(__frac_bits, bSig, subnormal_lz, mshifted);
         bExp = -subnormal_lz;
         bSig = (mshifted << 1) & ((1ULL << __frac_bits) - 1);
      }
   }

   aSig = aSig | (((__uint64_t)(__one & (a_c_normal | !__subnorm))) << __frac_bits);
   bSig = bSig | (((__uint64_t)(__one & (b_c_normal | !__subnorm))) << __frac_bits);
   nbSig = -bSig;
   bSigx2 = bSig << 1;
   nbSigx2 = -bSigx2;
   bSigx3 = bSigx2 + bSig;
   nbSigx3 = -bSigx3;
   current = aSig;
   if(__full_unroll)
   {
      switch(__div_it)
      {
#define STEP_CASE(z, n, data)                         \
   case(SRT4_MAX_UNROLL + 1 - n):                     \
      SRT4_DIV_STEP(z, SRT4_MAX_UNROLL + 1 - n, data) \
      __attribute__((fallthrough));
         BOOST_PP_REPEAT(SRT4_MAX_UNROLL, STEP_CASE, index);
#undef STEP_CASE
         case 1:
            SRT4_DIV_STEP(_, _, _)
            break;
         default:
            break;
      }
   }
   else
   {
      for(index = 0; index < __div_it; ++index)
      {
         BOOST_PP_REPEAT(SRT4_UNROLL_FACTOR, SRT4_DIV_STEP, index);
      }
   }
   if(__div_waste > 1)
   {
      positive >>= __div_waste;
      negative >>= __div_waste;
   }

   positive |= (current != 0) << 1;
   negative |= (current >> (__frac_p3 - 2)) & 2;

   negative <<= 1;
   negative = negative & ((1ULL << (__frac_p3 + 2)) - 1);
   zSig0 = positive - negative;
   zSig0 >>= 1;
   zSig0 = (__frac_odd) ? ((zSig0 >> 1) | (zSig0 & 1)) : zSig0;
   zSig0 = zSig0 & ((1ULL << (__frac_p3 + 1)) - 1);
   correction = (zSig0 >> __frac_p3) & 1;
   zSig1 = (zSig0 >> correction) | (zSig0 & 1);
   zSig1 = zSig1 & ((1ULL << (__frac_p3 - 1)) - 1);
   if(__rnd == FLOAT_RND_NEVN)
   {
      LSB_bit = SELECT_BIT(zSig1, 2);
      Guard_bit = SELECT_BIT(zSig1, 1);
      Round_bit = SELECT_BIT(zSig1, 0);
      round = Guard_bit & (LSB_bit | Round_bit);
   }
   else
   {
      round = 0;
   }

   if(__exp_bias & 1)
   {
      zExp = aExp - bExp + ((~__exp_bias) | correction); // ~__exp_bias = -1 - __exp_bias
   }
   else
   {
      zExp = aExp - bExp - __exp_bias - !correction;
   }
   MSB1zExp = zExp >> (__exp_bits + 1);
   MSB0zExp = zExp >> __exp_bits;
   zExp = zExp & ((1ULL << (__exp_bits + 1)) - 1);
   if(__rnd == FLOAT_RND_NEVN)
   {
      zExpSig = ((((__uint64_t)zExp) << __frac_bits) | (zSig1 >> 2)) + round;
   }
   else
   {
      zExpSig = (((__uint64_t)zExp) << __frac_bits) | zSig1;
   }
   MSBzExp = zExpSig >> (__exp_bits + __frac_bits);
   ovfCond = ((((MSB0zExp & ((~MSBzExp) & 1)) & 1) ^ MSB1zExp) & 1);
   if(z_c == FP_CLS_NORMAL)
   {
      if(ovfCond)
         return ((__uint64_t)zSign) << ((__exp_bits + __frac_bits));
      else if(MSBzExp || ((zExp == ((1ULL << __exp_bits) - 1)) && (__exc != FLOAT_EXC_OVF)))
         return (((__uint64_t)zSign) << (__exp_bits + __frac_bits)) | (((1ULL << __exp_bits) - 1) << __frac_bits) |
                ((__exc != FLOAT_EXC_OVF) ? 0ULL : ((1ULL << __frac_bits) - 1));
      else
         return (((__uint64_t)zSign) << (__exp_bits + __frac_bits)) |
                (zExpSig & ((1ULL << (__exp_bits + __frac_bits)) - 1));
   }
   else if(z_c == FP_CLS_ZERO)
      return ((__uint64_t)zSign) << ((__exp_bits + __frac_bits));
   else if(z_c == FP_CLS_NAN && __exc != FLOAT_EXC_NONAN)
      return (((__uint64_t)(a_c_nan ? aSign : bSign) | (a_c_inf & b_c_inf) | (a_c_zero & b_c_zero))
              << (__exp_bits + __frac_bits)) |
             ((__exc == FLOAT_EXC_STD) ? ((((1ULL << __exp_bits) - 1) << __frac_bits) | (1ULL << (__frac_bits - 1)) |
                                          (a_c_nan ? aSig : (b_c_nan ? bSig : 0))) :
                                         (0ULL));
   else
      return (((__uint64_t)(!(__exc == FLOAT_EXC_NONAN && z_c == FP_CLS_NAN) & zSign)) << (__exp_bits + __frac_bits)) |
             (((1ULL << __exp_bits) - 1) << __frac_bits) |
             ((__exc != FLOAT_EXC_OVF) ? 0ULL : ((1ULL << __frac_bits) - 1));
}

__tfloat_t __float_divSRT4(__tfloat_t a, __tfloat_t b, __uint8_t __exp_bits, __uint8_t __frac_bits,
                           __int32_t __exp_bias, __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm,
                           __int8_t __sign)
{
   return __kernel_float_divSRT4(a, b, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign, 0);
}

__tfloat_t __float_divSRT4U(__tfloat_t a, __tfloat_t b, __uint8_t __exp_bits, __uint8_t __frac_bits,
                            __int32_t __exp_bias, __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm,
                            __int8_t __sign)
{
   return __kernel_float_divSRT4(a, b, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign, 1);
}

__tfloat_t __float_divG(__tfloat_t a, __tfloat_t b, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                        __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   __uint8_t a_c, b_c, z_c;
   bool a_c_zero, b_c_zero, a_c_inf, b_c_inf, a_c_nan, b_c_nan, a_c_normal, b_c_normal;
   bool aSign, bSign, zSign;
   __int16_t aExp, bExp, zExp;
   __uint64_t aSig, bSig, aSigInitial, bSigInitial, zSig;
   __uint64_t term0, term1;
   __uint64_t c5, c4, c3, c2, c1, c0, shft, p1, bSigsqr1, bSigsqr2, p2, p3, p, ga0, gb0, ga1, gb1, rem0, rem, rnd;

   if(__sign == 1)
   {
      // Negative numbers division result is always out of negative numbers domain
      return (__exc == FLOAT_EXC_STD) ? __float_nan(__exp_bits, __frac_bits, __sign) : 0ULL;
   }

   aSig = __extractFloatFrac(a, __frac_bits);
   aExp = __extractFloatExp(a, __exp_bits, __frac_bits);
   aSign = __extractFloatSign(a, __exp_bits, __frac_bits, __sign);
   bSig = __extractFloatFrac(b, __frac_bits);
   bExp = __extractFloatExp(b, __exp_bits, __frac_bits);
   bSign = __extractFloatSign(b, __exp_bits, __frac_bits, __sign);
   zSign = aSign ^ bSign;
   bool aExp_null = aExp == 0;
   bool bExp_null = bExp == 0;
   bool aExpMax = aExp == ((1ULL << __exp_bits) - 1);
   bool bExpMax = bExp == ((1ULL << __exp_bits) - 1);
   bool aSig_null = aSig == 0;
   bool bSig_null = bSig == 0;

   a_c_zero = aExp_null & (aSig_null | !__subnorm);
   a_c_inf = aExpMax && aSig_null;
   a_c_inf = (__exc == FLOAT_EXC_STD) ? a_c_inf : 0;
   a_c_nan = aExpMax && !aSig_null;
   a_c_nan = (__exc == FLOAT_EXC_STD) ? a_c_nan : 0;
   a_c_normal = !aExpMax && !a_c_zero;
   a_c_normal = (__exc == FLOAT_EXC_STD) ? a_c_normal : !a_c_zero;
   a_c = /*((a_c_zero << 1 | a_c_zero) & FP_CLS_ZERO) |*/ ((a_c_normal << 1 | a_c_normal) & FP_CLS_NORMAL) |
         ((a_c_inf << 1 | a_c_inf) & FP_CLS_INF) | ((a_c_nan << 1 | a_c_nan) & FP_CLS_NAN);

   b_c_zero = bExp_null & (bSig_null | !__subnorm);
   b_c_inf = bExpMax && bSig_null;
   b_c_inf = (__exc == FLOAT_EXC_STD) ? b_c_inf : 0;
   b_c_nan = bExpMax && !bSig_null;
   b_c_nan = (__exc == FLOAT_EXC_STD) ? b_c_nan : 0;
   b_c_normal = !bExpMax && !b_c_zero;
   b_c_normal = (__exc == FLOAT_EXC_STD) ? b_c_normal : !b_c_zero;
   b_c = /*((b_c_zero << 1 | b_c_zero) & FP_CLS_ZERO) |*/ ((b_c_normal << 1 | b_c_normal) & FP_CLS_NORMAL) |
         ((b_c_inf << 1 | b_c_inf) & FP_CLS_INF) | ((b_c_nan << 1 | b_c_nan) & FP_CLS_NAN);

   z_c = ((a_c >> 1 | (1 & (~(b_c >> 1)) & (~(b_c & 1))) | (1 & (b_c >> 1) & b_c)) << 1) |
         ((1 & (a_c >> 1) & a_c) | (1 & (b_c >> 1) & b_c) | (1 & (a_c >> 1) & (b_c >> 1)) | (1 & a_c & b_c) |
          (1 & (~(a_c >> 1)) & (~(a_c & 1)) & (~(b_c >> 1)) & (~(b_c & 1))));
   if(__exc == FLOAT_EXC_OVF)
   {
      z_c = z_c & 1;
   }

   if(__subnorm)
   {
      if(aExp_null && !aSig_null)
      {
         __uint64_t subnormal_lz, mshifted;
         count_leading_zero_lshift_runtime_macro(__frac_bits, aSig, subnormal_lz, mshifted);
         aExp = -subnormal_lz;
         aSig = (mshifted << 1) & ((1ULL << __frac_bits) - 1);
      }
      if(bExp_null && !bSig_null)
      {
         __uint64_t subnormal_lz, mshifted;
         count_leading_zero_lshift_runtime_macro(__frac_bits, bSig, subnormal_lz, mshifted);
         bExp = -subnormal_lz;
         bSig = (mshifted << 1) & ((1ULL << __frac_bits) - 1);
      }
   }
   aSigInitial = aSig;
   bSigInitial = bSig;
   GOLDSCHMIDT_MANTISSA_DIVISION_64();
   if(z_c == FP_CLS_NORMAL)
      return __roundAndPackFloat64(zSign, zExp, zSig, __exp_bits, __frac_bits, __exc, 1);
   else if(z_c == FP_CLS_NAN)
      return (((__uint64_t)(a_c_nan ? aSign : bSign) | (a_c_inf & b_c_inf) | (a_c_zero & b_c_zero))
              << (__exp_bits + __frac_bits)) |
             ((__exc == FLOAT_EXC_STD) ? ((((1ULL << __exp_bits) - 1) << __frac_bits) | (1ULL << (__frac_bits - 1)) |
                                          (a_c_nan ? aSigInitial : (b_c_nan ? bSigInitial : 0))) :
                                         (0ULL));
   else if(z_c == FP_CLS_INF)
      return (((__uint64_t)zSign) << (__exp_bits + __frac_bits)) | (((1ULL << __exp_bits) - 1) << __frac_bits) |
             ((__exc == FLOAT_EXC_STD) ? 0ULL : ((1ULL << __frac_bits) - 1));
   return ((__uint64_t)zSign) << ((__exp_bits + __frac_bits));
}

/*----------------------------------------------------------------------------
| Returns 1 if the custom-precision floating-point value `a' is equal to the
| corresponding value `b', and 0 otherwise.  The comparison is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

#define BIT_MASK64(var, msbpp) (var & (((__uint64_t)0xFFFFFFFFFFFFFFFFULL) >> (64 - (msbpp))))

static __FORCE_INLINE bool __FloatEQ(__tfloat_t a, __tfloat_t b, __uint8_t __exp_bits, __uint8_t __frac_bits,
                                     __int32_t __exp_bias, __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one,
                                     bool __subnorm, __int8_t __sign)
{
   if(__exc == FLOAT_EXC_STD)
   {
      if(((__extractFloatExp(a, __exp_bits, __frac_bits) == ((1ULL << __exp_bits) - 1)) &&
          __extractFloatFrac(a, __frac_bits)) ||
         ((__extractFloatExp(b, __exp_bits, __frac_bits) == ((1ULL << __exp_bits) - 1)) &&
          __extractFloatFrac(b, __frac_bits)))
      {
         if(__float_is_signaling_nan(a, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign) ||
            __float_is_signaling_nan(b, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign))
         {
            __float_raise(float_flag_invalid);
         }
         return 0;
      }
   }
   if(__sign == -1)
   {
      return (BIT_MASK64(a, __exp_bits + __frac_bits + 1) == BIT_MASK64(b, __exp_bits + __frac_bits + 1)) ||
             (BIT_MASK64((a | b), __exp_bits + __frac_bits) == 0);
   }
   else
   {
      return BIT_MASK64(a, __exp_bits + __frac_bits + 1) == BIT_MASK64(b, __exp_bits + __frac_bits + 1);
   }
}

bool __float_eq(__tfloat_t a, __tfloat_t b, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   return __FloatEQ(a, b, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}

bool __float_ltgt_quiet(__tfloat_t a, __tfloat_t b, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                        __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   return !__FloatEQ(a, b, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}

/*----------------------------------------------------------------------------
| Returns 1 if the custom-precision floating-point value `a' is less than or
| equal to the corresponding value `b', and 0 otherwise.  The comparison is
| performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE bool __FloatLE(__float64_t a, __float64_t b, __uint8_t __exp_bits, __uint8_t __frac_bits,
                                     __exc_mode_t __exc, __int8_t __sign)
{
   bool aSign, bSign;

   if(__exc == FLOAT_EXC_STD)
   {
      if(((__extractFloatExp(a, __exp_bits, __frac_bits) == ((1ULL << __exp_bits) - 1)) &&
          __extractFloatFrac(a, __frac_bits)) ||
         ((__extractFloatExp(b, __exp_bits, __frac_bits) == ((1ULL << __exp_bits) - 1)) &&
          __extractFloatFrac(b, __frac_bits)))
      {
         __float_raise(float_flag_invalid);
         return 0;
      }
   }
   aSign = __extractFloatSign(a, __exp_bits, __frac_bits, __sign);
   bSign = __extractFloatSign(b, __exp_bits, __frac_bits, __sign);
   if(aSign != bSign)
      return aSign || (BIT_MASK64((a | b), __exp_bits + __frac_bits) == 0);
   return (BIT_MASK64(a, __exp_bits + __frac_bits + (__sign == -1)) ==
           BIT_MASK64(b, __exp_bits + __frac_bits + (__sign == -1))) ||
          (aSign ^ (BIT_MASK64(a, __exp_bits + __frac_bits + (__sign == -1)) <
                    BIT_MASK64(b, __exp_bits + __frac_bits + (__sign == -1))));
}

bool __float_le(__tfloat_t a, __tfloat_t b, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   return __FloatLE(a, b, __exp_bits, __frac_bits, __exc, __sign);
}

bool __float_ge(__tfloat_t a, __tfloat_t b, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   return __FloatLE(b, a, __exp_bits, __frac_bits, __exc, __sign);
}

/*----------------------------------------------------------------------------
| Returns 1 if the custom-precision floating-point value `a' is less than
| the corresponding value `b', and 0 otherwise.  The comparison is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE bool __FloatLT(__tfloat_t a, __tfloat_t b, __uint8_t __exp_bits, __uint8_t __frac_bits,
                                     __exc_mode_t __exc, __int8_t __sign)
{
   bool aSign, bSign;

   if(__exc == FLOAT_EXC_STD)
   {
      if(((__extractFloatExp(a, __exp_bits, __frac_bits) == ((1ULL << __exp_bits) - 1)) &&
          __extractFloatFrac(a, __frac_bits)) ||
         ((__extractFloatExp(b, __exp_bits, __frac_bits) == ((1ULL << __exp_bits) - 1)) &&
          __extractFloatFrac(b, __frac_bits)))
      {
         __float_raise(float_flag_invalid);
         return 0;
      }
   }
   aSign = __extractFloatSign(a, __exp_bits, __frac_bits, __sign);
   bSign = __extractFloatSign(b, __exp_bits, __frac_bits, __sign);
   if(aSign != bSign)
      return aSign && (BIT_MASK64((a | b), __exp_bits + __frac_bits) != 0);
   return (BIT_MASK64(a, __exp_bits + __frac_bits + (__sign == -1)) !=
           BIT_MASK64(b, __exp_bits + __frac_bits + (__sign == -1))) &&
          (aSign ^ (BIT_MASK64(a, __exp_bits + __frac_bits + (__sign == -1)) <
                    BIT_MASK64(b, __exp_bits + __frac_bits + (__sign == -1))));
}

bool __float_lt(__tfloat_t a, __tfloat_t b, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   return __FloatLT(a, b, __exp_bits, __frac_bits, __exc, __sign);
}

bool __float_gt(__tfloat_t a, __tfloat_t b, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   return __FloatLT(b, a, __exp_bits, __frac_bits, __exc, __sign);
}

__int32_t __isunordered(__tfloat_t a, __tfloat_t b, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                        __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   __uint64_t aExp, aSig, bExp, bSig;
   __uint64_t __exp_max = ((1ULL << __exp_bits) - 1ULL);
   aExp = (a >> __frac_bits) & ((1ULL << __exp_bits) - 1ULL);
   bExp = (b >> __frac_bits) & ((1ULL << __exp_bits) - 1ULL);
   aSig = a & ((1ULL << __frac_bits) - 1ULL);
   bSig = b & ((1ULL << __frac_bits) - 1ULL);

   return __exc == FLOAT_EXC_STD && ((aExp == __exp_max && aSig) || (bExp == __exp_max && bSig));
}

/*----------------------------------------------------------------------------
| Returns 1 if the custom precision floating-point value `a' is a signaling
| NaN; otherwise returns 0.
*----------------------------------------------------------------------------*/

bool __float_is_signaling_nan(__float64_t a, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                              __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   if(__exc == FLOAT_EXC_STD)
   {
      return (((a >> (__frac_bits - 1)) & ((1ULL << (__exp_bits + 1)) - 1)) == ((1ULL << (__exp_bits + 1)) - 2)) &&
             ((a & ((1ULL << (__frac_bits - 1)) - 1)) != 0);
   }
   else
   {
      return 0;
   }
}
