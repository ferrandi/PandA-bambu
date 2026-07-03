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

This C header file is part of the SoftFloat IEC/IEEE Floating-point Arithmetic
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
#ifndef _SOFTFLOAT_H
#define _SOFTFLOAT_H

#include <bambu_config.h>

#include "softfloat_features.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point underflow tininess-detection mode.
*----------------------------------------------------------------------------*/
#ifdef NO_PARAMETRIC
#define __float_detect_tininess 0
#else
extern __int8_t __float_detect_tininess;
#endif
   enum
   {
      float_tininess_after_rounding = 0,
      float_tininess_before_rounding = 1
   };

/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point rounding mode.
*----------------------------------------------------------------------------*/
#ifdef NO_PARAMETRIC
#define __float_rounding_mode 0
#else
extern __int8_t __float_rounding_mode;
#endif
   enum
   {
      float_round_nearest_even = 0,
      float_round_to_zero = 1,
      float_round_down = 2,
      float_round_up = 3
   };

/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point exception flags.
*----------------------------------------------------------------------------*/
#ifdef NO_PARAMETRIC
#define __float_exception_flags 0
#else
extern __int8_t __float_exception_flags;
#endif
   enum
   {
      float_flag_inexact = 1,
      float_flag_underflow = 2,
      float_flag_overflow = 4,
      float_flag_divbyzero = 8,
      float_flag_invalid = 16
   };

/*----------------------------------------------------------------------------
| Raises the exceptions specified by `flags'.  Floating-point traps can be
| defined here if desired.  It is currently not possible for such a trap
| to substitute a result value.  If traps are not implemented, this routine
| should be simply `__float_exception_flags |= flags;'.
*----------------------------------------------------------------------------*/
#ifdef NO_PARAMETRIC
#define __float_raise(x)
#else
#define __float_raise(x) __float_exception_flags |= x
#endif

#define TRUEFLOAT(...) (__VA_ARGS__, __uint8_t, __uint8_t, __int32_t, __rnd_mode_t, __exc_mode_t, bool, bool, __int8_t)

   /*----------------------------------------------------------------------------
   | Software IEC/IEEE integer-to-floating-point conversion routines.
   *----------------------------------------------------------------------------*/
   __tfloat_t __int_to_float TRUEFLOAT(__int64_t, __uint8_t);
   __tfloat_t __int64_to_float TRUEFLOAT(__int64_t);
   __tfloat_t __int32_to_float TRUEFLOAT(__int32_t);
   __tfloat_t __int16_to_float TRUEFLOAT(__int16_t);
   __tfloat_t __int8_to_float TRUEFLOAT(__int8_t);

   __tfloat_t __uint_to_float TRUEFLOAT(__uint64_t, __uint8_t);
   __tfloat_t __uint64_to_float TRUEFLOAT(__uint64_t);
   __tfloat_t __uint32_to_float TRUEFLOAT(__uint32_t);
   __tfloat_t __uint16_to_float TRUEFLOAT(__uint16_t);
   __tfloat_t __uint8_to_float TRUEFLOAT(__uint8_t);

   /*----------------------------------------------------------------------------
   | Software IEC/IEEE floating-point conversion routines.
   *----------------------------------------------------------------------------*/
   __int64_t __float_to_int TRUEFLOAT(__tfloat_t, __uint8_t);
   __int64_t __float_to_int64 TRUEFLOAT(__tfloat_t);
   __int32_t __float_to_int32 TRUEFLOAT(__tfloat_t);
   __uint64_t __float_to_uint TRUEFLOAT(__tfloat_t, __uint8_t);
   __uint64_t __float_to_uint64 TRUEFLOAT(__tfloat_t);
   __uint32_t __float_to_uint32 TRUEFLOAT(__tfloat_t);
   __float64_t __float32_to_float64_ieee(__float32_t, __exc_mode_t, bool);
   __float32_t __float64_to_float32_ieee(__float64_t, __exc_mode_t, bool);

   /*----------------------------------------------------------------------------
   | Software IEC/IEEE arbitrary precision conversion routines.
   *----------------------------------------------------------------------------*/
   __tfloat_t __float_cast(__tfloat_t, __uint8_t, __uint8_t, __int32_t, __rnd_mode_t, __exc_mode_t, bool, bool,
                           __int8_t, __uint8_t, __uint8_t, __int32_t, __rnd_mode_t, __exc_mode_t, bool, bool, __int8_t);

   /*----------------------------------------------------------------------------
   | Software IEC/IEEE arbitrary precision operations.
   *----------------------------------------------------------------------------*/
   __tfloat_t __float_add TRUEFLOAT(__tfloat_t, __tfloat_t);
   __tfloat_t __float_sub TRUEFLOAT(__tfloat_t, __tfloat_t);
   __tfloat_t __float_mul TRUEFLOAT(__tfloat_t, __tfloat_t);
   __tfloat_t __float_divSRT4 TRUEFLOAT(__tfloat_t, __tfloat_t);
   __tfloat_t __float_divG TRUEFLOAT(__tfloat_t, __tfloat_t);
   bool __float_eq TRUEFLOAT(__tfloat_t, __tfloat_t);
   bool __float_le TRUEFLOAT(__tfloat_t, __tfloat_t);
   bool __float_lt TRUEFLOAT(__tfloat_t, __tfloat_t);
   bool __float_ge TRUEFLOAT(__tfloat_t, __tfloat_t);
   bool __float_gt TRUEFLOAT(__tfloat_t, __tfloat_t);
   bool __float_is_signaling_nan TRUEFLOAT(__tfloat_t);
   bool __float_ltgt_quiet TRUEFLOAT(__tfloat_t, __tfloat_t);
   __int32_t __isunordered TRUEFLOAT(__tfloat_t, __tfloat_t);

   /*----------------------------------------------------------------------------
   | Returns the fraction bits of the floating-point value `a'.
   *----------------------------------------------------------------------------*/

   static __FORCE_INLINE __uint64_t __extractFloatFrac(__tfloat_t a, __uint8_t __frac_bits)
   {
      return a & ((1ULL << __frac_bits) - 1ULL);
   }

   /*----------------------------------------------------------------------------
   | Returns the exponent bits of the floating-point value `a'.
   *----------------------------------------------------------------------------*/

   static __FORCE_INLINE __uint64_t __extractFloatExp(__tfloat_t a, __uint8_t __exp_bits, __uint8_t __frac_bits)
   {
      return (a >> __frac_bits) & ((1ULL << __exp_bits) - 1ULL);
   }

   /*----------------------------------------------------------------------------
   | Returns the sign bit of the floating-point value `a'.
   *----------------------------------------------------------------------------*/

   static __FORCE_INLINE bool __extractFloatSign(__tfloat_t a, __uint8_t __exp_bits, __uint8_t __frac_bits,
                                                 __int8_t __sign)
   {
      return __sign == -1 ? (a >> (__exp_bits + __frac_bits)) : __sign;
   }

/*----------------------------------------------------------------------------
| The pattern for a default generated arbitrary-precision quiet NaN.
*----------------------------------------------------------------------------*/
#define __float_nan(__exp_bits, __frac_bits, __sign)                                                            \
   ((((__uint64_t)(__sign == -1)) << (__exp_bits + __frac_bits)) | (((1LL << __exp_bits) - 1) << __frac_bits) | \
    (1LL << (__frac_bits - 1)))

   /*----------------------------------------------------------------------------
   | Shifts `a' right by the number of bits given in `count'.  If any nonzero
   | bits are shifted off, they are ``jammed'' into the least significant bit of
   | the result by setting the least significant bit to 1.  The value of `count'
   | can be arbitrarily large; in particular, if `count' is greater than 32, the
   | result will be either 0 or 1, depending on whether `a' is zero or nonzero.
   | The result is stored in the location pointed to by `zPtr'.
   *----------------------------------------------------------------------------*/

   static __FORCE_INLINE void __shift32RightJamming(__uint32_t a, __int16_t count, __uint32_t* zPtr)
   {
      __uint32_t z;

      if(count == 0)
      {
         z = a;
      }
      else if(count < 32)
      {
         z = (a >> count) | ((a << ((-count) & 31)) != 0);
      }
      else
      {
         z = (a != 0);
      }
      *zPtr = z;
   }

   /*----------------------------------------------------------------------------
   | Shifts `a' right by the number of bits given in `count'.  If any nonzero
   | bits are shifted off, they are ``jammed'' into the least significant bit of
   | the result by setting the least significant bit to 1.  The value of `count'
   | can be arbitrarily large; in particular, if `count' is greater than 64, the
   | result will be either 0 or 1, depending on whether `a' is zero or nonzero.
   | The result is stored in the location pointed to by `zPtr'.
   *----------------------------------------------------------------------------*/

   static __FORCE_INLINE void __shift64RightJamming(__uint64_t a, __int16_t count, __uint64_t* zPtr)
   {
      __uint64_t z;

      if(count == 0)
      {
         z = a;
      }
      else if(count < 64)
      {
         z = (a >> count) | ((a << ((-count) & 63)) != 0);
      }
      else
      {
         z = (a != 0);
      }
      *zPtr = z;
   }

#ifdef __cplusplus
}
#endif

#endif // _SOFTFLOAT_H
