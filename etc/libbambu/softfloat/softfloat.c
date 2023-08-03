/**
 * SoftFloat package modified by:
 *    Fabrizio Ferrandi - Politecnico di Milano
 *    Michele Fiorito - Politecnico di Milano
 * Changes made are mainly oriented to improve the results of a generic high
 * level synthesis framework and to add support for custom floating-point
 * data types.
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

#include "milieu.h"

#include "softfloat.h"

#define COND_EXPR_MACRO32(cond, a, b)                         \
   ((((__bits32)((((__sbits32)(cond)) << 31) >> 31)) & (a)) | \
    ((~((__bits32)((((__sbits32)(cond)) << 31) >> 31))) & (b)))
#define COND_EXPR_MACRO64(cond, a, b)                         \
   ((((__bits64)((((__sbits64)(cond)) << 63) >> 63)) & (a)) | \
    ((~((__bits64)((((__sbits64)(cond)) << 63) >> 63))) & (b)))

#define FLOAT_EXC_OVF 0
#define FLOAT_EXC_STD 1
#define FLOAT_EXC_SAT 2

#define FLOAT_RND_NONE 0
#define FLOAT_RND_NEVN 1

#define IEEE16_FRAC_BITS 10
#define IEEE16_EXP_BITS 5
#define IEEE16_EXP_BIAS -15
#define IEEE32_FRAC_BITS 23
#define IEEE32_EXP_BITS 8
#define IEEE32_EXP_BIAS -127
#define IEEE64_FRAC_BITS 52
#define IEEE64_EXP_BITS 11
#define IEEE64_EXP_BIAS -1023
#define IEEE_SIGN -1
#define IEEE_RND FLOAT_RND_NEVN
#define IEEE_EXC FLOAT_EXC_STD
#define IEEE_ONE 1
#ifdef NO_SUBNORMALS
#define IEEE_SUBNORM 0
#else
#define IEEE_SUBNORM 1
#endif
#define IEEE_SIGN -1

#define IEEE16_EXTRACT_FRAC IEEE16_FRAC_BITS
#define IEEE16_EXTRACT_EXP IEEE16_EXP_BITS, IEEE16_FRAC_BITS
#define IEEE16_EXTRACT_SIGN IEEE16_EXP_BITS, IEEE16_FRAC_BITS, IEEE_SIGN
#define IEEE16_SPEC_ARGS \
   IEEE16_EXP_BITS, IEEE16_FRAC_BITS, IEEE16_EXP_BIAS, IEEE_RND, IEEE_EXC, IEEE_ONE, IEEE_SUBNORM, IEEE_SIGN
#define IEEE16_PACK IEEE16_EXP_BITS, IEEE16_FRAC_BITS

#define IEEE32_EXTRACT_FRAC IEEE32_FRAC_BITS
#define IEEE32_EXTRACT_EXP IEEE32_EXP_BITS, IEEE32_FRAC_BITS
#define IEEE32_EXTRACT_SIGN IEEE32_EXP_BITS, IEEE32_FRAC_BITS, IEEE_SIGN
#define IEEE32_SPEC_ARGS \
   IEEE32_EXP_BITS, IEEE32_FRAC_BITS, IEEE32_EXP_BIAS, IEEE_RND, IEEE_EXC, IEEE_ONE, IEEE_SUBNORM, IEEE_SIGN
#define IEEE32_PACK IEEE32_EXP_BITS, IEEE32_FRAC_BITS

#define IEEE64_EXTRACT_FRAC IEEE64_FRAC_BITS
#define IEEE64_EXTRACT_EXP IEEE64_EXP_BITS, IEEE64_FRAC_BITS
#define IEEE64_EXTRACT_SIGN IEEE64_EXP_BITS, IEEE64_FRAC_BITS, IEEE_SIGN
#define IEEE64_SPEC_ARGS \
   IEEE64_EXP_BITS, IEEE64_FRAC_BITS, IEEE64_EXP_BIAS, IEEE_RND, IEEE_EXC, IEEE_ONE, IEEE_SUBNORM, IEEE_SIGN
#define IEEE64_PACK IEEE64_EXP_BITS, IEEE64_FRAC_BITS

/// specific functions for the division
#include "div_utilities"
/// generic macros used to control bitsize of data
#include "bambu_macros.h"

/*----------------------------------------------------------------------------
| Floating-point rounding mode, extended double-precision rounding precision,
| and exception flags.
*----------------------------------------------------------------------------*/
#ifndef NO_PARAMETRIC
__int8 __float_rounding_mode = float_round_nearest_even;
__int8 __float_exception_flags = 0;
#endif
#ifdef FLOATX80
__int8 __floatx80_rounding_precision = 80;
#endif

/*----------------------------------------------------------------------------
| Primitive arithmetic functions, including multi-word arithmetic, and
| division and square root approximations.  (Can be specialized to target if
| desired.)
*----------------------------------------------------------------------------*/
#include "softfloat-macros"

/*----------------------------------------------------------------------------
| Functions and definitions to determine:  (1) whether tininess for underflow
| is detected before or after rounding by default, (2) what (if anything)
| happens when exceptions are raised, (3) how signaling NaNs are distinguished
| from quiet NaNs, (4) the default generated quiet NaNs, and (5) how NaNs
| are propagated from function inputs to output.  These details are target-
| specific.
*----------------------------------------------------------------------------*/
#include "softfloat-specialize"

/*----------------------------------------------------------------------------
| Takes a 64-bit fixed-point value `absZ' with binary point between bits 6
| and 7, and returns the properly rounded 32-bit integer corresponding to the
| input.  If `zSign' is 1, the input is negated before being converted to an
| integer.  Bit 63 of `absZ' must be zero.  Ordinarily, the fixed-point input
| is simply rounded to an integer, with the inexact exception raised if the
| input cannot be represented exactly as an integer.  However, if the fixed-
| point input is too large, the invalid exception is raised and the largest
| positive or negative integer is returned.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __int32 __roundAndPackInt32(__flag zSign, __bits64 absZ)
{
   __int8 roundingMode;
   __flag roundNearestEven;
   __int8 roundIncrement, roundBits;
   __int32 z;

   roundingMode = __float_rounding_mode;
   roundNearestEven = (roundingMode == float_round_nearest_even);
   roundIncrement = 0x40;
   if(!roundNearestEven)
   {
      if(roundingMode == float_round_to_zero)
      {
         roundIncrement = 0;
      }
      else
      {
         roundIncrement = 0x7F;
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
   roundBits = absZ & 0x7F;
   absZ = (absZ + roundIncrement) >> 7;
   absZ &= ~(((roundBits ^ 0x40) == 0) & roundNearestEven);
   z = absZ;
   if(zSign)
      z = -z;
   if((absZ >> 32) || (z && ((z < 0) ^ zSign)))
   {
      __float_raise(float_flag_invalid);
      return zSign ? (__sbits32)0x80000000 : 0x7FFFFFFF;
   }
#ifndef NO_PARAMETRIC
   if(roundBits)
      __float_exception_flags |= float_flag_inexact;
#endif
   return z;
}

/*----------------------------------------------------------------------------
| Takes the 128-bit fixed-point value formed by concatenating `absZ0' and
| `absZ1', with binary point between bits 63 and 64 (between the input words),
| and returns the properly rounded 64-bit integer corresponding to the input.
| If `zSign' is 1, the input is negated before being converted to an integer.
| Ordinarily, the fixed-point input is simply rounded to an integer, with
| the inexact exception raised if the input cannot be represented exactly as
| an integer.  However, if the fixed-point input is too large, the invalid
| exception is raised and the largest positive or negative integer is
| returned.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __int64 __roundAndPackInt64(__flag zSign, __bits64 absZ0, __bits64 absZ1)
{
   __int8 roundingMode;
   __flag roundNearestEven, increment;
   __int64 z;

   roundingMode = __float_rounding_mode;
   roundNearestEven = (roundingMode == float_round_nearest_even);
   increment = ((__sbits64)absZ1 < 0);
   if(!roundNearestEven)
   {
      if(roundingMode == float_round_to_zero)
      {
         increment = 0;
      }
      else
      {
         if(zSign)
         {
            increment = (roundingMode == float_round_down) && absZ1;
         }
         else
         {
            increment = (roundingMode == float_round_up) && absZ1;
         }
      }
   }
   if(increment)
   {
      ++absZ0;
      if(absZ0 == 0)
         goto overflow;
      absZ0 &= ~(((__bits64)(absZ1 << 1) == 0) & roundNearestEven);
   }
   z = absZ0;
   if(zSign)
      z = -z;
   if(z && ((z < 0) ^ zSign))
   {
   overflow:
      __float_raise(float_flag_invalid);
      return zSign ? (__sbits64)LIT64(0x8000000000000000) : LIT64(0x7FFFFFFFFFFFFFFF);
   }
#ifndef NO_PARAMETRIC
   if(absZ1)
      __float_exception_flags |= float_flag_inexact;
#endif
   return z;
}

/*----------------------------------------------------------------------------
| Returns the fraction bits of the single-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __bits32 __extractFloat32Frac(__float32 a, __bits8 __frac_bits)
{
   return a & ((1U << __frac_bits) - 1);
}

/*----------------------------------------------------------------------------
| Returns the exponent bits of the single-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __bits32 __extractFloat32Exp(__float32 a, __bits8 __exp_bits, __bits8 __frac_bits)
{
   return (a >> __frac_bits) & ((1U << __exp_bits) - 1);
}

/*----------------------------------------------------------------------------
| Returns the sign bit of the single-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag __extractFloat32Sign(__float32 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits8 __sign)
{
   return __sign == -1 ? (a >> (__exp_bits + __frac_bits)) : __sign;
}

/*----------------------------------------------------------------------------
| Normalizes the subnormal single-precision floating-point value represented
| by the denormalized significand `aSig'.  The normalized exponent and
| significand are stored at the locations pointed to by `zExpPtr' and
| `zSigPtr', respectively.
*----------------------------------------------------------------------------*/

#define __normalizeFloat32Subnormal(aSig, zExp, zSig, __frac_bits)     \
   {                                                                   \
      __int8 __shiftCount;                                             \
      __shiftCount = __countLeadingZeros32(aSig) - (31 - __frac_bits); \
      zSig = aSig << __shiftCount;                                     \
      zExp = 1 - __shiftCount;                                         \
   }

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

static __FORCE_INLINE __float32 __packFloat32(__flag zSign, __bits32 zExp, __bits32 zSig, __bits8 __exp_bits,
                                              __bits8 __frac_bits)
{
   return (((__bits32)zSign) << (__exp_bits + __frac_bits)) + (zExp << __frac_bits) + zSig;
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

static __FORCE_INLINE __float32 __roundAndPackFloat32(__flag zSign, __int32 zExp, __bits32 zSig, __bits8 __exp_bits,
                                                      __bits8 __frac_bits, FLOAT_EXC_TYPE __exc, __flag __subnorm)
{
   __int8 roundingMode;
   __flag roundNearestEven;
   __int32 roundIncrement, roundBits;
   __flag isTiny;
   __bits8 __ext_bits = (30 - __frac_bits);
   __int32 __exp_max = ((1 << __exp_bits) - 1);

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
   if((__exp_max - ((__exc != FLOAT_EXC_STD) ? 2 : 1)) <= (__bits32)zExp)
   {
      if(((__exp_max - ((__exc != FLOAT_EXC_STD) ? 2 : 1)) < zExp) ||
         ((zExp == (__exp_max - ((__exc != FLOAT_EXC_STD) ? 2 : 1))) && ((__sbits32)(zSig + roundIncrement) < 0)))
      {
         __float_raise(float_flag_overflow | float_flag_inexact);
         return ((((__bits32)zSign) << (__exp_bits + __frac_bits)) | (__exp_max << __frac_bits) |
                 (((__bits32) !(__exc != FLOAT_EXC_STD) << __frac_bits) - !(__exc != FLOAT_EXC_STD))) -
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
            return ((__bits32)zSign) << (__exp_bits + __frac_bits); // __packFloat32(zSign, 0, 0, IEEE32_PACK);
         }
      }
   }
#ifndef NO_PARAMETRIC
   if(roundBits)
      __float_exception_flags |= float_flag_inexact;
#endif
   zSig = (zSig + roundIncrement) >> __ext_bits;
   zSig &= ~(((roundBits ^ (1 << (__ext_bits - 1))) == 0) & roundNearestEven);
   if(zSig == 0)
      zExp = 0;
   return __packFloat32(zSign, zExp, zSig, __exp_bits, __frac_bits);
}

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent `zExp',
| and significand `zSig', and returns the proper single-precision floating-
| point value corresponding to the abstract input.  This routine is just like
| `__roundAndPackFloat32' except that `zSig' does not have to be normalized.
| Bit 31 of `zSig' must be zero, and `zExp' must be 1 less than the ``true''
| floating-point exponent.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float32 __normalizeRoundAndPackFloat32(__flag zSign, __int16 zExp, __bits32 zSig,
                                                               __bits8 __exp_bits, __bits8 __frac_bits,
                                                               FLOAT_EXC_TYPE __exc, __flag __subnorm)
{
   __int8 shiftCount;

   shiftCount = __countLeadingZeros32(zSig) - 1;
   return __roundAndPackFloat32(zSign, zExp - shiftCount, zSig << shiftCount, __exp_bits, __frac_bits, __exc,
                                __subnorm);
}

/*----------------------------------------------------------------------------
| Returns the fraction bits of the double-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __bits64 __extractFloat64Frac(__float64 a, __bits8 __frac_bits)
{
   return a & ((1ULL << __frac_bits) - 1ULL);
}

/*----------------------------------------------------------------------------
| Returns the exponent bits of the double-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __bits32 __extractFloat64Exp(__float64 a, __bits8 __exp_bits, __bits8 __frac_bits)
{
   return (a >> __frac_bits) & ((1ULL << __exp_bits) - 1ULL);
}

/*----------------------------------------------------------------------------
| Returns the sign bit of the double-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag __extractFloat64Sign(__float64 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits8 __sign)
{
   return __sign == -1 ? (a >> (__exp_bits + __frac_bits)) : __sign;
}

/*----------------------------------------------------------------------------
| Returns the fraction bits of the floating-point value `a'.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __bits64 __extractFloatFrac(__float a, __bits8 __frac_bits)
{
   return a & ((1ULL << __frac_bits) - 1ULL);
}

/*----------------------------------------------------------------------------
| Returns the exponent bits of the floating-point value `a'.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __bits64 __extractFloatExp(__float a, __bits8 __exp_bits, __bits8 __frac_bits)
{
   return (a >> __frac_bits) & ((1ULL << __exp_bits) - 1ULL);
}

/*----------------------------------------------------------------------------
| Returns the sign bit of the floating-point value `a'.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag __extractFloatSign(__float a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits8 __sign)
{
   return __sign == -1 ? (a >> (__exp_bits + __frac_bits)) : __sign;
}

/*----------------------------------------------------------------------------
| Normalizes the subnormal double-precision floating-point value represented
| by the denormalized significand `aSig'.  The normalized exponent and
| significand are stored at the locations pointed to by `zExpPtr' and
| `zSigPtr', respectively.
*----------------------------------------------------------------------------*/

#define __normalizeFloat64Subnormal(aSig, zExp, zSig, __frac_bits)     \
   {                                                                   \
      __int8 __shiftCount;                                             \
      __shiftCount = __countLeadingZeros64(aSig) - (63 - __frac_bits); \
      zSig = aSig << __shiftCount;                                     \
      zExp = 1 - __shiftCount;                                         \
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

static __FORCE_INLINE __float64 __packFloat64(__flag zSign, __bits64 zExp, __bits64 zSig, __bits8 __exp_bits,
                                              __bits8 __frac_bits)
{
   return (((__bits64)zSign) << (__exp_bits + __frac_bits)) + (zExp << __frac_bits) + zSig;
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

static __FORCE_INLINE __float64 __roundAndPackFloat64(__flag zSign, __int32 zExp, __bits64 zSig, __bits8 __exp_bits,
                                                      __bits8 __frac_bits, FLOAT_EXC_TYPE __exc, __flag __subnorm)
{
   __int8 roundingMode;
   __flag roundNearestEven;
   __int64 roundIncrement, roundBits;
   __flag isTiny;
   __bits8 __ext_bits = (62 - __frac_bits);
   __int32 __exp_max = ((1LL << __exp_bits) - 1);

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
   if((__exp_max - ((__exc == FLOAT_EXC_STD) ? 2 : 1)) <= (__bits32)zExp)
   {
      if(((__exp_max - ((__exc == FLOAT_EXC_STD) ? 2 : 1)) < zExp) ||
         ((zExp == (__exp_max - ((__exc == FLOAT_EXC_STD) ? 2 : 1))) && ((__sbits64)(zSig + roundIncrement) < 0)))
      {
         __float_raise(float_flag_overflow | float_flag_inexact);
         return ((((__bits64)zSign) << (__exp_bits + __frac_bits)) | (((__bits64)__exp_max) << __frac_bits) |
                 (((__bits64)(__exc != FLOAT_EXC_STD) << __frac_bits) - (__exc != FLOAT_EXC_STD))) -
                (roundIncrement == 0);
      }
      if(zExp < 0)
      {
         if(__subnorm)
         {
            isTiny = (__float_detect_tininess == float_tininess_before_rounding) || (zExp < -1) ||
                     (zSig + roundIncrement < LIT64(0x8000000000000000));
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
            return ((__bits64)zSign) << (__exp_bits + __frac_bits); // __packFloat64(zSign, 0, 0, IEEE32_PACK);
         }
      }
   }
#ifndef NO_PARAMETRIC
   if(roundBits)
   {
      __float_exception_flags |= float_flag_inexact;
   }
#endif
   zSig = (zSig + roundIncrement) >> __ext_bits;
   zSig &= ~(((roundBits ^ (1ULL << (__ext_bits - 1))) == 0) & roundNearestEven);
   if(zSig == 0)
   {
      zExp = 0;
   }
   return __packFloat64(zSign, zExp, zSig, __exp_bits, __frac_bits);
}

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent `zExp',
| and significand `zSig', and returns the proper double-precision floating-
| point value corresponding to the abstract input.  This routine is just like
| `__roundAndPackFloat64' except that `zSig' does not have to be normalized.
| Bit 63 of `zSig' must be zero, and `zExp' must be 1 less than the ``true''
| floating-point exponent.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float64 __normalizeRoundAndPackFloat64(__flag zSign, __int16 zExp, __bits64 zSig,
                                                               __bits8 __exp_bits, __bits8 __frac_bits,
                                                               FLOAT_EXC_TYPE __exc, __flag __subnorm)
{
   __int8 shiftCount;

   shiftCount = __countLeadingZeros64(zSig) - 1;
   return __roundAndPackFloat64(zSign, zExp - shiftCount, zSig << shiftCount, __exp_bits, __frac_bits, __exc,
                                __subnorm);
}

#ifdef FLOATX80

/*----------------------------------------------------------------------------
| Returns the fraction bits of the extended double-precision floating-point
| value `a'.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __bits64 __extractFloatx80Frac(__floatx80 a)
{
   return a.low;
}

/*----------------------------------------------------------------------------
| Returns the exponent bits of the extended double-precision floating-point
| value `a'.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __int32 __extractFloatx80Exp(__floatx80 a)
{
   return a.high & 0x7FFF;
}

/*----------------------------------------------------------------------------
| Returns the sign bit of the extended double-precision floating-point value
| `a'.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag __extractFloatx80Sign(__floatx80 a)
{
   return a.high >> 15;
}

/*----------------------------------------------------------------------------
| Normalizes the subnormal extended double-precision floating-point value
| represented by the denormalized significand `aSig'.  The normalized exponent
| and significand are stored at the locations pointed to by `zExpPtr' and
| `zSigPtr', respectively.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE void __normalizeFloatx80Subnormal(__bits64 aSig, __int32* zExpPtr, __bits64* zSigPtr)
{
   __int8 shiftCount;

   shiftCount = __countLeadingZeros64(aSig);
   *zSigPtr = aSig << shiftCount;
   *zExpPtr = 1 - shiftCount;
}

/*----------------------------------------------------------------------------
| Packs the sign `zSign', exponent `zExp', and significand `zSig' into an
| extended double-precision floating-point value, returning the result.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __floatx80 __packFloatx80(__flag zSign, __int32 zExp, __bits64 zSig)
{
   __floatx80 z;

   z.low = zSig;
   z.high = (((__bits16)zSign) << 15) + zExp;
   return z;
}

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent `zExp',
| and extended significand formed by the concatenation of `zSig0' and `zSig1',
| and returns the proper extended double-precision floating-point value
| corresponding to the abstract input.  Ordinarily, the abstract value is
| rounded and packed into the extended double-precision format, with the
| inexact exception raised if the abstract input cannot be represented
| exactly.  However, if the abstract value is too large, the overflow and
| inexact exceptions are raised and an infinity or maximal finite value is
| returned.  If the abstract value is too small, the input value is rounded to
| a subnormal number, and the underflow and inexact exceptions are raised if
| the abstract input cannot be represented exactly as a subnormal extended
| double-precision floating-point number.
|     If `roundingPrecision' is 32 or 64, the result is rounded to the same
| number of bits as single or double precision, respectively.  Otherwise, the
| result is rounded to the full precision of the extended double-precision
| format.
|     The input significand must be normalized or smaller.  If the input
| significand is not normalized, `zExp' must be 0; in that case, the result
| returned is a subnormal number, and it must not require rounding.  The
| handling of underflow and overflow follows the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __floatx80 __roundAndPackFloatx80(__int8 roundingPrecision, __flag zSign, __int32 zExp,
                                                        __bits64 zSig0, __bits64 zSig1)
{
   __int8 roundingMode;
   __flag roundNearestEven, increment, isTiny;
   __int64 roundIncrement, roundMask, roundBits;

   roundingMode = __float_rounding_mode;
   roundNearestEven = (roundingMode == float_round_nearest_even);
   if(roundingPrecision == 80)
      goto precision80;
   if(roundingPrecision == 64)
   {
      roundIncrement = LIT64(0x0000000000000400);
      roundMask = LIT64(0x00000000000007FF);
   }
   else if(roundingPrecision == 32)
   {
      roundIncrement = LIT64(0x0000008000000000);
      roundMask = LIT64(0x000000FFFFFFFFFF);
   }
   else
   {
      goto precision80;
   }
   zSig0 |= (zSig1 != 0);
   if(!roundNearestEven)
   {
      if(roundingMode == float_round_to_zero)
      {
         roundIncrement = 0;
      }
      else
      {
         roundIncrement = roundMask;
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
   roundBits = zSig0 & roundMask;
   if(0x7FFD <= (__bits32)(zExp - 1))
   {
      if((0x7FFE < zExp) || ((zExp == 0x7FFE) && (zSig0 + roundIncrement < zSig0)))
      {
         goto overflow;
      }
      if(zExp <= 0)
      {
         isTiny = (__float_detect_tininess == float_tininess_before_rounding) || (zExp < 0) ||
                  (zSig0 <= zSig0 + roundIncrement);
         __shift64RightJamming(zSig0, 1 - zExp, &zSig0);
         zExp = 0;
         roundBits = zSig0 & roundMask;
         if(isTiny && roundBits)
            __float_raise(float_flag_underflow);
         if(roundBits)
            __float_exception_flags |= float_flag_inexact;
         zSig0 += roundIncrement;
         if((__sbits64)zSig0 < 0)
            zExp = 1;
         roundIncrement = roundMask + 1;
         if(roundNearestEven && (roundBits << 1 == roundIncrement))
         {
            roundMask |= roundIncrement;
         }
         zSig0 &= ~roundMask;
         return __packFloatx80(zSign, zExp, zSig0);
      }
   }
   if(roundBits)
      __float_exception_flags |= float_flag_inexact;
   zSig0 += roundIncrement;
   if(zSig0 < roundIncrement)
   {
      ++zExp;
      zSig0 = LIT64(0x8000000000000000);
   }
   roundIncrement = roundMask + 1;
   if(roundNearestEven && (roundBits << 1 == roundIncrement))
   {
      roundMask |= roundIncrement;
   }
   zSig0 &= ~roundMask;
   if(zSig0 == 0)
      zExp = 0;
   return __packFloatx80(zSign, zExp, zSig0);
precision80:
   increment = ((__sbits64)zSig1 < 0);
   if(!roundNearestEven)
   {
      if(roundingMode == float_round_to_zero)
      {
         increment = 0;
      }
      else
      {
         if(zSign)
         {
            increment = (roundingMode == float_round_down) && zSig1;
         }
         else
         {
            increment = (roundingMode == float_round_up) && zSig1;
         }
      }
   }
   if(0x7FFD <= (__bits32)(zExp - 1))
   {
      if((0x7FFE < zExp) || ((zExp == 0x7FFE) && (zSig0 == LIT64(0xFFFFFFFFFFFFFFFF)) && increment))
      {
         roundMask = 0;
      overflow:
         __float_raise(float_flag_overflow | float_flag_inexact);
         if((roundingMode == float_round_to_zero) || (zSign && (roundingMode == float_round_up)) ||
            (!zSign && (roundingMode == float_round_down)))
         {
            return __packFloatx80(zSign, 0x7FFE, ~roundMask);
         }
         return __packFloatx80(zSign, 0x7FFF, LIT64(0x8000000000000000));
      }
      if(zExp <= 0)
      {
         isTiny = (__float_detect_tininess == float_tininess_before_rounding) || (zExp < 0) || !increment ||
                  (zSig0 < LIT64(0xFFFFFFFFFFFFFFFF));
         __shift64ExtraRightJamming(zSig0, zSig1, 1 - zExp, &zSig0, &zSig1);
         zExp = 0;
         if(isTiny && zSig1)
            __float_raise(float_flag_underflow);
         if(zSig1)
            __float_exception_flags |= float_flag_inexact;
         if(roundNearestEven)
         {
            increment = ((__sbits64)zSig1 < 0);
         }
         else
         {
            if(zSign)
            {
               increment = (roundingMode == float_round_down) && zSig1;
            }
            else
            {
               increment = (roundingMode == float_round_up) && zSig1;
            }
         }
         if(increment)
         {
            ++zSig0;
            zSig0 &= ~(((__bits64)(zSig1 << 1) == 0) & roundNearestEven);
            if((__sbits64)zSig0 < 0)
               zExp = 1;
         }
         return __packFloatx80(zSign, zExp, zSig0);
      }
   }
   if(zSig1)
      __float_exception_flags |= float_flag_inexact;
   if(increment)
   {
      ++zSig0;
      if(zSig0 == 0)
      {
         ++zExp;
         zSig0 = LIT64(0x8000000000000000);
      }
      else
      {
         zSig0 &= ~(((__bits64)(zSig1 << 1) == 0) & roundNearestEven);
      }
   }
   else
   {
      if(zSig0 == 0)
         zExp = 0;
   }
   return __packFloatx80(zSign, zExp, zSig0);
}

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent
| `zExp', and significand formed by the concatenation of `zSig0' and `zSig1',
| and returns the proper extended double-precision floating-point value
| corresponding to the abstract input.  This routine is just like
| `__roundAndPackFloatx80' except that the input significand does not have to be
| normalized.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __floatx80 __normalizeRoundAndPackFloatx80(__int8 roundingPrecision, __flag zSign, __int32 zExp,
                                                                 __bits64 zSig0, __bits64 zSig1)
{
   __int8 shiftCount;

   if(zSig0 == 0)
   {
      zSig0 = zSig1;
      zSig1 = 0;
      zExp -= 64;
   }
   shiftCount = __countLeadingZeros64(zSig0);
   __shortShift128Left(zSig0, zSig1, shiftCount, &zSig0, &zSig1);
   zExp -= shiftCount;
   return __roundAndPackFloatx80(roundingPrecision, zSign, zExp, zSig0, zSig1);
}

#endif

#ifdef FLOAT128

/*----------------------------------------------------------------------------
| Returns the least-significant 64 fraction bits of the quadruple-precision
| floating-point value `a'.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __bits64 __extractFloat128Frac1(__float128 a)
{
   return a.low;
}

/*----------------------------------------------------------------------------
| Returns the most-significant 48 fraction bits of the quadruple-precision
| floating-point value `a'.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __bits64 __extractFloat128Frac0(__float128 a)
{
   return a.high & LIT64(0x0000FFFFFFFFFFFF);
}

/*----------------------------------------------------------------------------
| Returns the exponent bits of the quadruple-precision floating-point value
| `a'.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __int32 __extractFloat128Exp(__float128 a)
{
   return (a.high >> 48) & 0x7FFF;
}

/*----------------------------------------------------------------------------
| Returns the sign bit of the quadruple-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag __extractFloat128Sign(__float128 a)
{
   return a.high >> 63;
}

/*----------------------------------------------------------------------------
| Normalizes the subnormal quadruple-precision floating-point value
| represented by the denormalized significand formed by the concatenation of
| `aSig0' and `aSig1'.  The normalized exponent is stored at the location
| pointed to by `zExpPtr'.  The most significant 49 bits of the normalized
| significand are stored at the location pointed to by `zSig0Ptr', and the
| least significant 64 bits of the normalized significand are stored at the
| location pointed to by `zSig1Ptr'.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE void __normalizeFloat128Subnormal(__bits64 aSig0, __bits64 aSig1, __int32* zExpPtr,
                                                        __bits64* zSig0Ptr, __bits64* zSig1Ptr)
{
   __int8 shiftCount;

   if(aSig0 == 0)
   {
      shiftCount = __countLeadingZeros64(aSig1) - 15;
      if(shiftCount < 0)
      {
         *zSig0Ptr = aSig1 >> (-shiftCount);
         *zSig1Ptr = aSig1 << (shiftCount & 63);
      }
      else
      {
         *zSig0Ptr = aSig1 << shiftCount;
         *zSig1Ptr = 0;
      }
      *zExpPtr = -shiftCount - 63;
   }
   else
   {
      shiftCount = __countLeadingZeros64(aSig0) - 15;
      __shortShift128Left(aSig0, aSig1, shiftCount, zSig0Ptr, zSig1Ptr);
      *zExpPtr = 1 - shiftCount;
   }
}

/*----------------------------------------------------------------------------
| Packs the sign `zSign', the exponent `zExp', and the significand formed
| by the concatenation of `zSig0' and `zSig1' into a quadruple-precision
| floating-point value, returning the result.  After being shifted into the
| proper positions, the three fields `zSign', `zExp', and `zSig0' are simply
| added together to form the most significant 32 bits of the result.  This
| means that any integer portion of `zSig0' will be added into the exponent.
| Since a properly normalized significand will have an integer portion equal
| to 1, the `zExp' input should be 1 less than the desired result exponent
| whenever `zSig0' and `zSig1' concatenated form a complete, normalized
| significand.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float128 __packFloat128(__flag zSign, __int32 zExp, __bits64 zSig0, __bits64 zSig1)
{
   __float128 z;

   z.low = zSig1;
   z.high = (((__bits64)zSign) << 63) + (((__bits64)zExp) << 48) + zSig0;
   return z;
}

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent `zExp',
| and extended significand formed by the concatenation of `zSig0', `zSig1',
| and `zSig2', and returns the proper quadruple-precision floating-point value
| corresponding to the abstract input.  Ordinarily, the abstract value is
| simply rounded and packed into the quadruple-precision format, with the
| inexact exception raised if the abstract input cannot be represented
| exactly.  However, if the abstract value is too large, the overflow and
| inexact exceptions are raised and an infinity or maximal finite value is
| returned.  If the abstract value is too small, the input value is rounded to
| a subnormal number, and the underflow and inexact exceptions are raised if
| the abstract input cannot be represented exactly as a subnormal quadruple-
| precision floating-point number.
|     The input significand must be normalized or smaller.  If the input
| significand is not normalized, `zExp' must be 0; in that case, the result
| returned is a subnormal number, and it must not require rounding.  In the
| usual case that the input significand is normalized, `zExp' must be 1 less
| than the ``true'' floating-point exponent.  The handling of underflow and
| overflow follows the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float128 __roundAndPackFloat128(__flag zSign, __int32 zExp, __bits64 zSig0, __bits64 zSig1,
                                                        __bits64 zSig2)
{
   __int8 roundingMode;
   __flag roundNearestEven, increment, isTiny;

   roundingMode = __float_rounding_mode;
   roundNearestEven = (roundingMode == float_round_nearest_even);
   increment = ((__sbits64)zSig2 < 0);
   if(!roundNearestEven)
   {
      if(roundingMode == float_round_to_zero)
      {
         increment = 0;
      }
      else
      {
         if(zSign)
         {
            increment = (roundingMode == float_round_down) && zSig2;
         }
         else
         {
            increment = (roundingMode == float_round_up) && zSig2;
         }
      }
   }
   if(0x7FFD <= (__bits32)zExp)
   {
      if((0x7FFD < zExp) ||
         ((zExp == 0x7FFD) && __eq128(LIT64(0x0001FFFFFFFFFFFF), LIT64(0xFFFFFFFFFFFFFFFF), zSig0, zSig1) && increment))
      {
         __float_raise(float_flag_overflow | float_flag_inexact);
         if((roundingMode == float_round_to_zero) || (zSign && (roundingMode == float_round_up)) ||
            (!zSign && (roundingMode == float_round_down)))
         {
            return __packFloat128(zSign, 0x7FFE, LIT64(0x0000FFFFFFFFFFFF), LIT64(0xFFFFFFFFFFFFFFFF));
         }
         return __packFloat128(zSign, 0x7FFF, 0, 0);
      }
      if(zExp < 0)
      {
         isTiny = (__float_detect_tininess == float_tininess_before_rounding) || (zExp < -1) || !increment ||
                  __lt128(zSig0, zSig1, LIT64(0x0001FFFFFFFFFFFF), LIT64(0xFFFFFFFFFFFFFFFF));
         __shift128ExtraRightJamming(zSig0, zSig1, zSig2, -zExp, &zSig0, &zSig1, &zSig2);
         zExp = 0;
         if(isTiny && zSig2)
            __float_raise(float_flag_underflow);
         if(roundNearestEven)
         {
            increment = ((__sbits64)zSig2 < 0);
         }
         else
         {
            if(zSign)
            {
               increment = (roundingMode == float_round_down) && zSig2;
            }
            else
            {
               increment = (roundingMode == float_round_up) && zSig2;
            }
         }
      }
   }
   if(zSig2)
      __float_exception_flags |= float_flag_inexact;
   if(increment)
   {
      __add128(zSig0, zSig1, 0, 1, &zSig0, &zSig1);
      zSig1 &= ~((zSig2 + zSig2 == 0) & roundNearestEven);
   }
   else
   {
      if((zSig0 | zSig1) == 0)
         zExp = 0;
   }
   return __packFloat128(zSign, zExp, zSig0, zSig1);
}

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent `zExp',
| and significand formed by the concatenation of `zSig0' and `zSig1', and
| returns the proper quadruple-precision floating-point value corresponding
| to the abstract input.  This routine is just like `__roundAndPackFloat128'
| except that the input significand has fewer bits and does not have to be
| normalized.  In all cases, `zExp' must be 1 less than the ``true'' floating-
| point exponent.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float128 __normalizeRoundAndPackFloat128(__flag zSign, __int32 zExp, __bits64 zSig0,
                                                                 __bits64 zSig1)
{
   __int8 shiftCount;
   __bits64 zSig2;

   if(zSig0 == 0)
   {
      zSig0 = zSig1;
      zSig1 = 0;
      zExp -= 64;
   }
   shiftCount = __countLeadingZeros64(zSig0) - 15;
   if(0 <= shiftCount)
   {
      zSig2 = 0;
      __shortShift128Left(zSig0, zSig1, shiftCount, &zSig0, &zSig1);
   }
   else
   {
      __shift128ExtraRightJamming(zSig0, zSig1, 0, -shiftCount, &zSig0, &zSig1, &zSig2);
   }
   zExp -= shiftCount;
   return __roundAndPackFloat128(zSign, zExp, zSig0, zSig1, zSig2);
}

#endif

/*----------------------------------------------------------------------------
| Returns the result of converting the 32-bit two's complement integer `a'
| to the single-precision floating-point format.  The conversion is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float32 __Int32ToFloat32(__int32 a, __bits8 __exp_bits, __bits8 __frac_bits,
                                                 __sbits32 __exp_bias, FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc,
                                                 __flag __one, __flag __subnorm, __sbits8 __sign)
{
   __flag zSign;
   __int32 __exp_int = 29 - __exp_bias;

   if(a == 0)
      return 0;

   if(((1 << __exp_bits) - 1) < (31 - __exp_bias))
   {
      // TODO: fix __exp_int and a to comply with lower exponent
   }

   if(__sign == 1 && a > 0)
   {
      return 0;
   }
   if(__sign == 0 && a < 0)
   {
      return 0;
   }
   if(a == (__sbits32)0x80000000)
   {
      if(__exp_int + 2 >= __subnorm && ((((__exp_int + 2) >> __exp_bits) & ((1 << (32 - __exp_bits)) - 1)) == 0))
         return (1 << (__exp_bits + __frac_bits)) | (((__exp_int + 2) & ((1 << __exp_bits) - 1))
                                                     << __frac_bits); // __packFloat32(1, 0x9E, 0, IEEE32_PACK); -2^31
      else
         return (1 << (__exp_bits + __frac_bits)) | (((1 << __exp_bits) - 1) << __frac_bits) |
                (((__bits32) !(__exc != FLOAT_EXC_STD) << __frac_bits) - !(__exc != FLOAT_EXC_STD));
   }
   zSign = (a < 0);
   return __normalizeRoundAndPackFloat32(zSign, __exp_int, zSign ? -a : a, __exp_bits, __frac_bits, __exc,
                                         __subnorm); // 2^30 * a
}

__float32 __int32_to_float32(__int32 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                             FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm,
                             __sbits8 __sign)
{
   return __Int32ToFloat32(a, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}

__float32 __int16_to_float32(__int16 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                             FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm,
                             __sbits8 __sign)
{
   return __Int32ToFloat32(a, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}

__float32 __int8_to_float32(__int8 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                            FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   return __Int32ToFloat32(a, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}

/*----------------------------------------------------------------------------
| Returns the result of converting the 32-bit two's complement integer `a'
| to the single-precision floating-point format.  The conversion is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float32 __UInt32ToFloat32(__uint32 a, __bits8 __exp_bits, __bits8 __frac_bits,
                                                  __sbits32 __exp_bias, FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc,
                                                  __flag __one, __flag __subnorm, __sbits8 __sign)
{
   __flag zSign;
   __uint64 absA;
   __int8 shiftCount;
   __bits8 __shift_fix = 30 - __frac_bits;
   __int32 __exp_int = 29 - __exp_bias;

   if(__sign == 1)
   {
      return 0;
   }

   if(((1 << __exp_bits) - 1) < (31 - __exp_bias))
   {
      // TODO: fix __exp_int and a to comply with lower exponent
   }

   if(a == 0)
      return 0;
   zSign = 0;
   absA = a;
   shiftCount = __countLeadingZeros64(absA) - (63 - __frac_bits);
   if(shiftCount >= 0)
   {
      return __packFloat32(zSign, (__exp_int - __shift_fix) - shiftCount, absA << shiftCount, __exp_bits, __frac_bits);
   }
   else
   {
      shiftCount += __shift_fix;
      if(shiftCount < 0)
      {
         __shift64RightJamming(absA, -shiftCount, &absA);
      }
      else
      {
         absA <<= shiftCount;
      }
      return __roundAndPackFloat32(zSign, __exp_int - shiftCount, absA, __exp_bits, __frac_bits, __exc, __subnorm);
   }
}

__float32 __uint32_to_float32(__uint32 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                              FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm,
                              __sbits8 __sign)
{
   return __UInt32ToFloat32(a, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}

__float32 __uint16_to_float32(__uint16 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                              FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm,
                              __sbits8 __sign)
{
   return __UInt32ToFloat32(a, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}

__float32 __uint8_to_float32(__uint8 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                             FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm,
                             __sbits8 __sign)
{
   return __UInt32ToFloat32(a, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}

/*----------------------------------------------------------------------------
| Returns the result of converting the 32-bit two's complement integer `a'
| to the double-precision floating-point format.  The conversion is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__float64 __int32_to_float64(__int32 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                             FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm,
                             __sbits8 __sign)
{
   __flag zSign;
   __uint32 absA;
   __int8 shiftCount;
   __bits64 zSig;
   __int32 __exp_int = __frac_bits - 1 - __exp_bias;

   if(a == 0)
      return 0;
   zSign = (a < 0);
   absA = zSign ? -a : a;
   shiftCount = __countLeadingZeros32(absA) + (__frac_bits - 31);
   zSig = absA;
   if(shiftCount < 0 && __frac_bits < 31)
   {
      return __packFloat64(zSign, __exp_int - shiftCount, zSig >> -shiftCount, __exp_bits, __frac_bits);
   }
   return __packFloat64(zSign, __exp_int - shiftCount, zSig << shiftCount, __exp_bits, __frac_bits);
}

__float64 __uint32_to_float64(__uint32 absA, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                              FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm,
                              __sbits8 __sign)
{
   __int8 shiftCount;
   __bits64 zSig;
   __int32 __exp_int = __frac_bits - 1 - __exp_bias;

   if(absA == 0)
      return 0;
   shiftCount = __countLeadingZeros32(absA) + (__frac_bits - 31);
   zSig = absA;
   if(shiftCount < 0 && __frac_bits < 31)
   {
      return __packFloat64(0, __exp_int - shiftCount, zSig >> -shiftCount, __exp_bits, __frac_bits);
   }
   return __packFloat64(0, __exp_int - shiftCount, zSig << shiftCount, __exp_bits, __frac_bits);
}

#ifdef FLOATX80

/*----------------------------------------------------------------------------
| Returns the result of converting the 32-bit two's complement integer `a'
| to the extended double-precision floating-point format.  The conversion
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

__floatx80 __int32_to_floatx80_ieee(__int32 a)
{
   __flag zSign;
   __uint32 absA;
   __int8 shiftCount;
   __bits64 zSig;

   if(a == 0)
      return __packFloatx80(0, 0, 0);
   zSign = (a < 0);
   absA = zSign ? -a : a;
   shiftCount = __countLeadingZeros32(absA) + 32;
   zSig = absA;
   return __packFloatx80(zSign, 0x403E - shiftCount, zSig << shiftCount);
}

#endif

#ifdef FLOAT128

/*----------------------------------------------------------------------------
| Returns the result of converting the 32-bit two's complement integer `a' to
| the quadruple-precision floating-point format.  The conversion is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__float128 __int32_to_float128_ieee(__int32 a)
{
   __flag zSign;
   __uint32 absA;
   __int8 shiftCount;
   __bits64 zSig0;

   if(a == 0)
      return __packFloat128(0, 0, 0, 0);
   zSign = (a < 0);
   absA = zSign ? -a : a;
   shiftCount = __countLeadingZeros32(absA) + 17;
   zSig0 = absA;
   return __packFloat128(zSign, 0x402E - shiftCount, zSig0 << shiftCount, 0);
}

#endif

/*----------------------------------------------------------------------------
| Returns the result of converting the 64-bit two's complement integer `a'
| to the single-precision floating-point format.  The conversion is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__float32 __int64_to_float32(__int64 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                             FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm,
                             __sbits8 __sign)
{
   __flag zSign;
   __uint64 absA;
   __int8 shiftCount;
   __bits8 __shift_fix = 30 - __frac_bits;
   __int32 __exp_int = 29 - __exp_bias;

   if(a == 0)
      return 0;
   zSign = (a < 0);
   absA = zSign ? -a : a;
   shiftCount = __countLeadingZeros64(absA) - (63 - __frac_bits);
   if(shiftCount >= 0)
   {
      return __packFloat32(zSign, ((__exp_int - __shift_fix) - shiftCount) & ((1 << __exp_bits) - 1),
                           absA << shiftCount, __exp_bits, __frac_bits);
   }
   else
   {
      shiftCount += __shift_fix;
      if(shiftCount < 0)
      {
         __shift64RightJamming(absA, -shiftCount, &absA);
      }
      else
      {
         absA <<= shiftCount;
      }
      return __roundAndPackFloat32(zSign, (__exp_int - shiftCount) & ((1 << __exp_bits) - 1), absA, __exp_bits,
                                   __frac_bits, __exc, __subnorm);
   }
}

__float32 __uint64_to_float32(__uint64 absA, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                              FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm,
                              __sbits8 __sign)
{
   __int8 shiftCount;
   __bits8 __shift_fix = 30 - __frac_bits;
   __int32 __exp_int = 29 - __exp_bias;

   if(absA == 0)
      return 0;
   shiftCount = __countLeadingZeros64(absA) - (63 - __frac_bits);
   if(shiftCount >= 0)
   {
      return __packFloat32(0, ((__exp_int - __shift_fix) - shiftCount) & ((1 << __exp_bits) - 1), absA << shiftCount,
                           __exp_bits, __frac_bits);
   }
   else
   {
      shiftCount += __shift_fix;
      if(shiftCount < 0)
      {
         __shift64RightJamming(absA, -shiftCount, &absA);
      }
      else
      {
         absA <<= shiftCount;
      }
      return __roundAndPackFloat32(0, (__exp_int - shiftCount) & ((1 << __exp_bits) - 1), absA, __exp_bits, __frac_bits,
                                   __exc, __subnorm);
   }
}

/*----------------------------------------------------------------------------
| Returns the result of converting the 64-bit two's complement integer `a'
| to the double-precision floating-point format.  The conversion is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__float64 __int64_to_float64(__int64 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                             FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm,
                             __sbits8 __sign)
{
   __flag zSign;
   __int32 __exp_int = 61 - __exp_bias;

   if(a == 0)
      return 0;

   if((((1 << __exp_bits) - 1) + __exp_bias) < 63)
   {
      // TODO: fix __exp_int and a to comply with lower exponent
   }

   if(__sign == 1 && a > 0)
   {
      return 0;
   }
   if(__sign == 0 && a < 0)
   {
      return 0;
   }
   if(a == (__sbits64)LIT64(0x8000000000000000))
   {
      return (1ULL << (__exp_bits + __frac_bits)) | (((((__bits64)__exp_int) + 2) & ((1ULL << __exp_bits) - 1))
                                                     << __frac_bits); // __packFloat64(1, 0x43E, 0, IEEE64_PACK);
   }
   zSign = (a < 0);
   return __normalizeRoundAndPackFloat64(zSign, __exp_int & ((1 << __exp_bits) - 1), zSign ? -a : a, __exp_bits,
                                         __frac_bits, __exc, __subnorm);
}

__float64 __uint64_to_float64(__uint64 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                              FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm,
                              __sbits8 __sign)
{
   __int16 zExp;
   __bits64 zSig;
   __int32 __exp_int = 61 - __exp_bias;

   if(__sign == 1)
   {
      return 0;
   }

   if(a == 0)
      return 0;

   if((((1 << __exp_bits) - 1) + __exp_bias) < 63)
   {
      // TODO: fix __exp_int and a to comply with lower exponent
   }
   if(a & (__bits64)LIT64(0x8000000000000000))
   {
      zExp = __exp_int + 1;
      zSig = a >> 1;
   }
   else
   {
      zExp = __exp_int;
      zSig = a;
   }
   return __normalizeRoundAndPackFloat64(0, zExp & ((1 << __exp_bits) - 1), zSig, __exp_bits, __frac_bits, __exc,
                                         __subnorm);
}

#ifdef FLOATX80

/*----------------------------------------------------------------------------
| Returns the result of converting the 64-bit two's complement integer `a'
| to the extended double-precision floating-point format.  The conversion
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

__floatx80 __int64_to_floatx80_ieee(__int64 a)
{
   __flag zSign;
   __uint64 absA;
   __int8 shiftCount;

   if(a == 0)
      return __packFloatx80(0, 0, 0);
   zSign = (a < 0);
   absA = zSign ? -a : a;
   shiftCount = __countLeadingZeros64(absA);
   return __packFloatx80(zSign, 0x403E - shiftCount, absA << shiftCount);
}

#endif

#ifdef FLOAT128

/*----------------------------------------------------------------------------
| Returns the result of converting the 64-bit two's complement integer `a' to
| the quadruple-precision floating-point format.  The conversion is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__float128 __int64_to_float128_ieee(__int64 a)
{
   __flag zSign;
   __uint64 absA;
   __int8 shiftCount;
   __int32 zExp;
   __bits64 zSig0, zSig1;

   if(a == 0)
      return __packFloat128(0, 0, 0, 0);
   zSign = (a < 0);
   absA = zSign ? -a : a;
   shiftCount = __countLeadingZeros64(absA) + 49;
   zExp = 0x406E - shiftCount;
   if(64 <= shiftCount)
   {
      zSig1 = 0;
      zSig0 = absA;
      shiftCount -= 64;
   }
   else
   {
      zSig1 = absA;
      zSig0 = 0;
   }
   __shortShift128Left(zSig0, zSig1, shiftCount, &zSig0, &zSig1);
   return __packFloat128(zSign, zExp, zSig0, zSig1);
}

#endif

/*----------------------------------------------------------------------------
| Returns the result of converting the single-precision floating-point value
| `a' to the 32-bit two's complement integer format.  The conversion is
| performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic---which means in particular that the conversion is rounded
| according to the current rounding mode.  If `a' is a NaN, the largest
| positive integer is returned.  Otherwise, if the conversion overflows, the
| largest integer with the same sign as `a' is returned.
*----------------------------------------------------------------------------*/

__int32 __float32_to_int32(__float32 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                           FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   __flag aSign;
   __int32 aExp, shiftCount;
   __bits32 aSig;
   __bits64 aSig64;

   aSig = __extractFloat32Frac(a, __frac_bits);
   aExp = __extractFloat32Exp(a, __exp_bits, __frac_bits);
   aSign = __extractFloat32Sign(a, __exp_bits, __frac_bits, __sign);
   if(__exc == FLOAT_EXC_STD)
   {
      if((aExp == ((1 << __exp_bits) - 1)) && aSig)
      {
         aSign = 0;
      }
   }
   if(aExp && __one)
   {
      aSig |= (1 << __frac_bits);
   }
   shiftCount = (48 - __exp_bias) - aExp;
   aSig64 = aSig;
   aSig64 <<= 32;
   if(shiftCount > 0)
      __shift64RightJamming(aSig64, shiftCount, &aSig64);
   return __roundAndPackInt32(aSign, aSig64);
}

/*----------------------------------------------------------------------------
| Returns the result of converting the single-precision floating-point value
| `a' to the 32-bit two's complement integer format.  The conversion is
| performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic, except that the conversion is always rounded toward zero.
| If `a' is a NaN, the largest positive integer is returned.  Otherwise, if
| the conversion overflows, the largest integer with the same sign as `a' is
| returned.
*----------------------------------------------------------------------------*/

__int32 __float32_to_int32_round_to_zero(__float32 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                                         FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm,
                                         __sbits8 __sign)
{
   __flag aSign;
   __int32 aExp, shiftCount;
   __bits32 aSig;
   __int32 z;

   aSig = __extractFloat32Frac(a, __frac_bits);
   aExp = __extractFloat32Exp(a, __exp_bits, __frac_bits);
   aSign = __extractFloat32Sign(a, __exp_bits, __frac_bits, __sign);
   shiftCount = aExp + (__exp_bias - 31);
   if(shiftCount >= 0)
   {
      if(a != ((1 << (__exp_bits + __frac_bits)) | (((31 - __exp_bias) & ((1 << __exp_bits) - 1)) << __frac_bits)))
      {
         __float_raise(float_flag_invalid);
         if(!aSign || ((aExp == ((1 << __exp_bits) - 1)) && aSig && (__exc == FLOAT_EXC_STD)))
         {
            return 0x7FFFFFFF;
         }
      }
      return (__sbits32)0x80000000;
   }
   else if(aExp <= (-1 - __exp_bias))
   {
#ifndef NO_PARAMETRIC
      if(aExp | aSig)
         __float_exception_flags |= float_flag_inexact;
#endif
      return 0;
   }
   aSig = (aSig | (1 << __frac_bits)) << (31 - __frac_bits);
   z = aSig >> (-shiftCount);
   if((__bits32)(aSig << (shiftCount & 31)))
   {
#ifndef NO_PARAMETRIC
      __float_exception_flags |= float_flag_inexact;
#endif
   }
   if(aSign)
      z = -z;
   return z;
}

__uint32 __float32_to_uint32_round_to_zero(__float32 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                                           FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm,
                                           __sbits8 __sign)
{
   __flag aSign;
   __int32 aExp, shiftCount;
   __bits32 aSig;
   __uint32 z;

   aSig = __extractFloat32Frac(a, __frac_bits);
   aExp = __extractFloat32Exp(a, __exp_bits, __frac_bits);
   aSign = __extractFloat32Sign(a, __exp_bits, __frac_bits, __sign);
   shiftCount = aExp + (__exp_bias - 31);
   if(0 <= shiftCount)
   {
      return (__bits32)0x80000000;
   }
   else if(aExp <= (-1 - __exp_bias))
   {
#ifndef NO_PARAMETRIC
      if(aExp | aSig)
         __float_exception_flags |= float_flag_inexact;
#endif
      return 0;
   }
   aSig = (aSig | (1 << __frac_bits)) << (31 - __frac_bits);
   z = aSig >> (-shiftCount);
   if((__bits32)(aSig << (shiftCount & 31)))
   {
#ifndef NO_PARAMETRIC
      __float_exception_flags |= float_flag_inexact;
#endif
   }
   if(aSign)
      z = -z;
   return z;
}

/*----------------------------------------------------------------------------
| Returns the result of converting the single-precision floating-point value
| `a' to the 64-bit two's complement integer format.  The conversion is
| performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic---which means in particular that the conversion is rounded
| according to the current rounding mode.  If `a' is a NaN, the largest
| positive integer is returned.  Otherwise, if the conversion overflows, the
| largest integer with the same sign as `a' is returned.
*----------------------------------------------------------------------------*/

__int64 __float32_to_int64(__float32 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                           FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   __flag aSign;
   __int16 aExp, shiftCount;
   __bits32 aSig;
   __bits64 aSig64, aSigExtra;

   aSig = __extractFloat32Frac(a, __frac_bits);
   aExp = __extractFloat32Exp(a, __exp_bits, __frac_bits);
   aSign = __extractFloat32Sign(a, __exp_bits, __frac_bits, __sign);
   shiftCount = (63 - __exp_bias) - aExp;
   if(shiftCount < 0)
   {
      __float_raise(float_flag_invalid);
      if(!aSign || ((aExp == ((1 << __exp_bits) - 1)) && aSig && (__exc == FLOAT_EXC_STD)))
      {
         return LIT64(0x7FFFFFFFFFFFFFFF);
      }
      return (__sbits64)LIT64(0x8000000000000000);
   }
   if(aExp)
      aSig |= (1 << __frac_bits);
   aSig64 = aSig;
   aSig64 <<= (63 - __frac_bits);
   __shift64ExtraRightJamming(aSig64, 0, shiftCount, &aSig64, &aSigExtra);
   return __roundAndPackInt64(aSign, aSig64, aSigExtra);
}

/*----------------------------------------------------------------------------
| Returns the result of converting the single-precision floating-point value
| `a' to the 64-bit two's complement integer format.  The conversion is
| performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic, except that the conversion is always rounded toward zero.  If
| `a' is a NaN, the largest positive integer is returned.  Otherwise, if the
| conversion overflows, the largest integer with the same sign as `a' is
| returned.
*----------------------------------------------------------------------------*/

__int64 __float32_to_int64_round_to_zero(__float32 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                                         FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm,
                                         __sbits8 __sign)
{
   __flag aSign;
   __int16 aExp, shiftCount;
   __bits32 aSig;
   __bits64 aSig64;
   __int64 z;

   aSig = __extractFloat32Frac(a, __frac_bits);
   aExp = __extractFloat32Exp(a, __exp_bits, __frac_bits);
   aSign = __extractFloat32Sign(a, __exp_bits, __frac_bits, __sign);
   shiftCount = aExp + (__exp_bias - 63);
   if(0 <= shiftCount)
   {
      if(a != ((1 << (__exp_bits + __frac_bits)) | (((63 - __exp_bias) & ((1 << __exp_bits) - 1)) << __frac_bits)))
      {
         __float_raise(float_flag_invalid);
         if(!aSign || ((aExp == ((1 << __exp_bits) - 1)) && aSig && (__exc == FLOAT_EXC_STD)))
         {
            return LIT64(0x7FFFFFFFFFFFFFFF);
         }
      }
      return (__sbits64)LIT64(0x8000000000000000);
   }
   else if(aExp <= (-1 - __exp_bias))
   {
#ifndef NO_PARAMETRIC
      if(aExp | aSig)
         __float_exception_flags |= float_flag_inexact;
#endif
      return 0;
   }
   aSig64 = aSig | (1 << __frac_bits);
   aSig64 <<= (63 - __frac_bits);
   z = aSig64 >> (-shiftCount);
   if((__bits64)(aSig64 << (shiftCount & 63)))
   {
#ifndef NO_PARAMETRIC
      __float_exception_flags |= float_flag_inexact;
#endif
   }
   if(aSign)
      z = -z;
   return z;
}

__uint64 __float32_to_uint64_round_to_zero(__float32 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                                           FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm,
                                           __sbits8 __sign)
{
   __flag aSign;
   __int16 aExp, shiftCount;
   __bits32 aSig;
   __bits64 aSig64;
   __uint64 z;

   aSig = __extractFloat32Frac(a, __frac_bits);
   aExp = __extractFloat32Exp(a, __exp_bits, __frac_bits);
   aSign = __extractFloat32Sign(a, __exp_bits, __frac_bits, __sign);
   shiftCount = aExp + (__exp_bias - 63);
   if(0 <= shiftCount)
   {
      return (__bits64)LIT64(0x8000000000000000);
   }
   else if(aExp <= (-1 - __exp_bias))
   {
#ifndef NO_PARAMETRIC
      if(aExp | aSig)
         __float_exception_flags |= float_flag_inexact;
#endif
      return 0;
   }
   aSig64 = aSig | (1 << __frac_bits);
   aSig64 <<= (63 - __frac_bits);
   z = aSig64 >> (-shiftCount);
   if((__bits64)(aSig64 << (shiftCount & 63)))
   {
#ifndef NO_PARAMETRIC
      __float_exception_flags |= float_flag_inexact;
#endif
   }
   if(aSign)
      z = -z;
   return z;
}

/*----------------------------------------------------------------------------
| Returns the result of converting the single-precision floating-point value
| `a' to the double-precision floating-point format.  The conversion is
| performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

__float64 __float32_to_float64_ieee(__float32 a, FLOAT_EXC_TYPE __exc, __flag __subnorm)
{
   __flag aSign;
   __int16 aExp;
   __bits32 aSig;

   aSig = __extractFloat32Frac(a, IEEE32_EXTRACT_FRAC);
   aExp = __extractFloat32Exp(a, IEEE32_EXTRACT_EXP);
   aSign = __extractFloat32Sign(a, IEEE32_EXTRACT_SIGN);
   if(aExp == 0xFF && (__exc == FLOAT_EXC_STD))
   {
      if(aSig)
         return __commonNaNToFloat64_ieee(__float32ToCommonNaN_ieee(a));
      return (((__bits64)aSign) << 63) | 0x7FF0000000000000; // __packFloat64(aSign, 0x7FF, 0, IEEE64_PACK);
   }
   if(aExp == 0)
   {
      if(__subnorm)
      {
         if(aSig == 0)
            return ((__bits64)aSign) << 63; // __packFloat64(aSign, 0, 0, IEEE64_PACK);
         __normalizeFloat32Subnormal(aSig, aExp, aSig, IEEE32_FRAC_BITS);
         --aExp;
      }
      else
      {
         return ((__bits64)aSign) << 63; // __packFloat64(aSign, 0, 0, IEEE64_PACK);
      }
   }
   return __packFloat64(aSign, aExp + ((__int16)0x380), ((__bits64)aSig) << 29, IEEE64_PACK);
}

#define needed_bits(in, count)                        \
   {                                                  \
      __uint32 i;                                     \
      __uint8 lz;                                     \
      i = in > 0 ? in : -in;                          \
      count_leading_zero_runtime_macro(32, in, lz);   \
      lz = in > 0 ? lz : (lz + ((i & (i - 1)) != 0)); \
      count = 32 - lz;                                \
   }

#define MAX(a, b) (a > b ? a : b)

__float __float_cast(__float bits, __bits8 __in_exp_bits, __bits8 __in_frac_bits, __int32 __in_exp_bias,
                     FLOAT_RND_TYPE __in_rnd, FLOAT_EXC_TYPE __in_exc, __flag __in_has_one, __flag __in_has_subnorm,
                     __sbits8 __in_sign, __bits8 __out_exp_bits, __bits8 __out_frac_bits, __int32 __out_exp_bias,
                     FLOAT_RND_TYPE __out_rnd, FLOAT_EXC_TYPE __out_exc, __flag __out_has_one, __flag __out_has_subnorm,
                     __sbits8 __out_sign)
{
   __bits64 Sign, Exp, Frac, FExp, SFrac, RExp, NFrac, RFrac, expOverflow, ExExp, FSign, out_val;
   __int32 __biasDiff, __rangeDiff;
   __bits8 __exp_bits_diff, __bits_diff;
   __flag ExpOverflow, ExpUnderflow, ExpNull, FracNull, inputZero;
   __flag GuardBit, LSB, RoundBit, Sticky, Round;
   __flag in_nan, out_nan;

   Sign = bits >> (__in_exp_bits + __in_frac_bits);
   Exp = (bits >> (__in_frac_bits)) & ((1ULL << __in_exp_bits) - 1);
   Frac = bits & ((1ULL << __in_frac_bits) - 1);

   __exp_bits_diff =
       __in_exp_bits > __out_exp_bits ? (__in_exp_bits - __out_exp_bits) : (__out_exp_bits - __in_exp_bits);
   __uint8 __nb_in_exp_bias, __nb_out_exp_bias, __exp_type_size;
   needed_bits(__in_exp_bias, __nb_in_exp_bias);
   needed_bits(__out_exp_bias, __nb_out_exp_bias);
   __exp_type_size =
       MAX((MAX(__in_exp_bits, __out_exp_bits) + (__exp_bits_diff == 1)), MAX(__nb_in_exp_bias, __nb_out_exp_bias));

   __biasDiff = __in_exp_bias - __out_exp_bias;
   __rangeDiff = ((1 << __out_exp_bits) - !__out_has_subnorm) - ((1 << __in_exp_bits) - !__in_has_subnorm);
   if((__in_exp_bits != __out_exp_bits) || (__in_exp_bias != __out_exp_bias))
   {
      FExp = Exp + ((__bits64)__biasDiff);
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
         SFrac = SFrac | ((__bits64)Round);
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

#ifdef FLOATX80

/*----------------------------------------------------------------------------
| Returns the result of converting the single-precision floating-point value
| `a' to the extended double-precision floating-point format.  The conversion
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

__floatx80 __float32_to_floatx80_ieee(__float32 a)
{
   __flag aSign;
   __int16 aExp;
   __bits32 aSig;

   aSig = __extractFloat32Frac(a, IEEE32_EXTRACT_FRAC);
   aExp = __extractFloat32Exp(a, IEEE32_EXTRACT_EXP);
   aSign = __extractFloat32Sign(a, IEEE32_EXTRACT_SIGN);
   if(aExp == 0xFF)
   {
      if(aSig)
         return __commonNaNToFloatx80(__float32ToCommonNaN(a));
      return __packFloatx80(aSign, 0x7FFF, LIT64(0x8000000000000000));
   }
   if(aExp == 0)
   {
#ifdef NO_SUBNORMALS
      return __packFloatx80(aSign, 0, 0);
#else
      if(aSig == 0)
         return __packFloatx80(aSign, 0, 0);
      __normalizeFloat32Subnormal(aSig, &aExp, &aSig);
#endif
   }
   aSig |= 0x00800000;
   return __packFloatx80(aSign, aExp + 0x3F80, ((__bits64)aSig) << 40);
}

#endif

#ifdef FLOAT128

/*----------------------------------------------------------------------------
| Returns the result of converting the single-precision floating-point value
| `a' to the double-precision floating-point format.  The conversion is
| performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

__float128 __float32_to_float128_ieee(__float32 a)
{
   __flag aSign;
   __int16 aExp;
   __bits32 aSig;

   aSig = __extractFloat32Frac(a, IEEE32_EXTRACT_FRAC);
   aExp = __extractFloat32Exp(a, IEEE32_EXTRACT_EXP);
   aSign = __extractFloat32Sign(a, IEEE32_EXTRACT_SIGN);
   if(aExp == 0xFF)
   {
      if(aSig)
         return __commonNaNToFloat128(__float32ToCommonNaN(a));
      return __packFloat128(aSign, 0x7FFF, 0, 0);
   }
   if(aExp == 0)
   {
#ifdef NO_SUBNORMALS
      return __packFloat128(aSign, 0, 0, 0);
#else
      if(aSig == 0)
         return __packFloat128(aSign, 0, 0, 0);
      __normalizeFloat32Subnormal(aSig, &aExp, &aSig);
#endif
      --aExp;
   }
   return __packFloat128(aSign, aExp + 0x3F80, ((__bits64)aSig) << 25, 0);
}

#endif

/*----------------------------------------------------------------------------
| Rounds the single-precision floating-point value `a' to an integer, and
| returns the result as a single-precision floating-point value.  The
| operation is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__float32 __float32_round_to_int_ieee(__float32 a)
{
   __flag aSign;
   __int16 aExp;
   __bits32 lastBitMask, roundBitsMask;
   __int8 roundingMode;
   __float32 z;

   aExp = __extractFloat32Exp(a, IEEE32_EXTRACT_EXP);
   if(0x96 <= aExp)
   {
      if((aExp == 0xFF) && __extractFloat32Frac(a, IEEE32_EXTRACT_FRAC))
      {
         return __propagateFloat32NaN_ieee(a, a);
      }
      return a;
   }
   if(aExp <= 0x7E)
   {
      if((__bits32)(a << 1) == 0)
         return a;
#ifndef NO_PARAMETRIC
      __float_exception_flags |= float_flag_inexact;
#endif
      aSign = __extractFloat32Sign(a, IEEE32_EXTRACT_SIGN);
      switch(__float_rounding_mode)
      {
         case float_round_nearest_even:
            if((aExp == 0x7E) && __extractFloat32Frac(a, IEEE32_EXTRACT_FRAC))
            {
               return (((__bits32)aSign) << 31) | 0x3F800000; // __packFloat32(aSign, 0x7F, 0, IEEE32_PACK);
            }
            break;
         case float_round_down:
            return aSign ? 0xBF800000 : 0;
         case float_round_up:
            return aSign ? 0x80000000 : 0x3F800000;
      }
      return ((__bits32)aSign) << 31; // __packFloat32(aSign, 0, 0, IEEE32_PACK);
   }
   lastBitMask = 1;
   lastBitMask <<= 0x96 - aExp;
   roundBitsMask = lastBitMask - 1;
   z = a;
   roundingMode = __float_rounding_mode;
   if(roundingMode == float_round_nearest_even)
   {
      z += lastBitMask >> 1;
      if((z & roundBitsMask) == 0)
         z &= ~lastBitMask;
   }
   else if(roundingMode != float_round_to_zero)
   {
      if(__extractFloat32Sign(z, IEEE32_EXTRACT_SIGN) ^ (roundingMode == float_round_up))
      {
         z += roundBitsMask;
      }
   }
   z &= ~roundBitsMask;
#ifndef NO_PARAMETRIC
   if(z != a)
      __float_exception_flags |= float_flag_inexact;
#endif
   return z;
}

#define FP_CLS_ZERO 0U
#define FP_CLS_NORMAL 1U
#define FP_CLS_INF 2U
#define FP_CLS_NAN 3U

/*----------------------------------------------------------------------------
| Returns the result of multiplying the single-precision floating-point values
| `a' and `b'.  The operation is performed according to the IEC/IEEE Standard
| for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/
typedef unsigned int SF_USItype __attribute__((mode(SI)));
typedef unsigned int SF_UDItype __attribute__((mode(DI)));

__float32 __float32_muladd_ieee(__float32 uiA, __float32 uiB, __float32 uiC)
{
   _Bool signA;
   __int16 expA;
   __bits32 sigA;
   _Bool signB;
   __int16 expB;
   __bits32 sigB;
   _Bool signC;
   __int16 expC;
   __bits32 sigC;
   _Bool signProd;
   __bits32 magBits, uiZ;
   __int16 expProd;
   __bits64 sigProd;
   _Bool signZ;
   __int16 expZ;
   __bits32 sigZ;
   __int16 expDiff;
   __bits64 sig64Z, sig64C;
   __int8 shiftDist;

   /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
   signA = __extractFloat32Sign(uiA, IEEE32_EXTRACT_SIGN);
   expA = __extractFloat32Exp(uiA, IEEE32_EXTRACT_EXP);
   sigA = __extractFloat32Frac(uiA, IEEE32_EXTRACT_FRAC);
   signB = __extractFloat32Sign(uiB, IEEE32_EXTRACT_SIGN);
   expB = __extractFloat32Exp(uiB, IEEE32_EXTRACT_EXP);
   sigB = __extractFloat32Frac(uiB, IEEE32_EXTRACT_FRAC);
   signC = __extractFloat32Sign(uiC, IEEE32_EXTRACT_SIGN);
   expC = __extractFloat32Exp(uiC, IEEE32_EXTRACT_EXP);
   sigC = __extractFloat32Frac(uiC, IEEE32_EXTRACT_FRAC);
   signProd = signA ^ signB;
   /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
   if(expA == 0xFF)
   {
      if(sigA || ((expB == 0xFF) && sigB))
         goto propagateNaN_ABC;
      magBits = expB | sigB;
      goto infProdArg;
   }
   if(expB == 0xFF)
   {
      if(sigB)
         goto propagateNaN_ABC;
      magBits = expA | sigA;
      goto infProdArg;
   }
   if(expC == 0xFF)
   {
      if(sigC)
      {
         uiZ = 0;
         goto propagateNaN_ZC;
      }
      uiZ = uiC;
      goto uiZ;
   }
   /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
   if(!expA)
   {
      if(!sigA)
         goto zeroProd;
      __int8 nZeros;
      __bits32 shift_0;
      count_leading_zero_lshift_macro(24, sigA, nZeros, shift_0);
      expA = 1 - nZeros;
      sigA = shift_0;
   }
   if(!expB)
   {
      if(!sigB)
         goto zeroProd;
      __int8 nZeros;
      __bits32 shift_0;
      count_leading_zero_lshift_macro(24, sigB, nZeros, shift_0);
      expB = 1 - nZeros;
      sigB = shift_0;
   }
   /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
   expProd = expA + expB - 0x7E;
   sigA = (sigA | 0x00800000) << 7;
   sigB = (sigB | 0x00800000) << 7;
   sigProd = (__bits64)sigA * sigB;
   if(sigProd < 0x2000000000000000ULL)
   {
      --expProd;
      sigProd <<= 1;
   }
   signZ = signProd;
   if(!expC)
   {
      if(!sigC)
      {
         expZ = expProd - 1;
         sigZ = sigProd >> 31 | ((sigProd & (((__bits64)1 << 31) - 1)) != 0);
         goto roundPack;
      }
      __int8 nZeros;
      __bits32 shift_0;
      count_leading_zero_lshift_macro(24, sigC, nZeros, shift_0);
      expC = 1 - nZeros;
      sigC = shift_0;
   }
   sigC = (sigC | 0x00800000) << 6;
   /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
   expDiff = expProd - expC;
   if(signProd == signC)
   {
      /*--------------------------------------------------------------------
       *--------------------------------------------------------------------*/
      if(expDiff <= 0)
      {
         __bits32 dist = 32 - expDiff;
         __bits64 softfloat_shiftRightJam64 =
             (dist < 63) ? sigProd >> dist | ((__bits64)(sigProd << (-dist & 63)) != 0) : (sigProd != 0);
         expZ = expC;
         sigZ = sigC + softfloat_shiftRightJam64;
      }
      else
      {
         __bits64 a = (__bits64)sigC << 32;
         __bits64 softfloat_shiftRightJam64 =
             (expDiff < 63) ? a >> expDiff | ((__bits64)(a << (-expDiff & 63)) != 0) : (a != 0);
         expZ = expProd;
         sig64Z = sigProd + softfloat_shiftRightJam64;
         sigZ = sig64Z >> 32 | ((sig64Z & (((__bits64)1 << 32) - 1)) != 0);
      }
      if(sigZ < 0x40000000)
      {
         --expZ;
         sigZ <<= 1;
      }
   }
   else
   {
      /*--------------------------------------------------------------------
       *--------------------------------------------------------------------*/
      sig64C = (__bits64)sigC << 32;
      if(expDiff < 0)
      {
         __bits32 dist = -expDiff;
         __bits64 softfloat_shiftRightJam64 =
             (dist < 63) ? sigProd >> dist | ((__bits64)(sigProd << (-dist & 63)) != 0) : (sigProd != 0);
         signZ = signC;
         expZ = expC;
         sig64Z = sig64C - softfloat_shiftRightJam64;
      }
      else if(!expDiff)
      {
         expZ = expProd;
         sig64Z = sigProd - sig64C;
         if(!sig64Z)
            goto completeCancellation;
         if(sig64Z & 0x8000000000000000ULL)
         {
            signZ = !signZ;
            sig64Z = -sig64Z;
         }
      }
      else
      {
         __bits64 softfloat_shiftRightJam64 =
             (expDiff < 63) ? sig64C >> expDiff | ((__bits64)(sig64C << (-expDiff & 63)) != 0) : (sig64C != 0);
         expZ = expProd;
         sig64Z = 0;
      }
      shiftDist = __countLeadingZeros64(sig64Z) - 1;
      expZ -= shiftDist;
      shiftDist -= 32;
      if(shiftDist < 0)
      {
         sigZ = sig64Z >> (-shiftDist) | ((sig64Z & (((__bits64)1 << (-shiftDist)) - 1)) != 0);
      }
      else
      {
         sigZ = (__bits64)sig64Z << shiftDist;
      }
   }
roundPack:
   return __roundAndPackFloat32(signZ, expZ, sigZ, IEEE32_PACK, IEEE_EXC, IEEE_SUBNORM);
   /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
propagateNaN_ABC:
   uiZ = __propagateFloat32NaN_ieee(uiA, uiB);
   goto propagateNaN_ZC;
   /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
infProdArg:
   if(magBits)
   {
      uiZ = (((__bits32)signProd) << 31) | 0x7F800000; // __packFloat32(signProd, 0xFF, 0, IEEE32_PACK);
      if(expC != 0xFF)
         goto uiZ;
      if(sigC)
         goto propagateNaN_ZC;
      if(signProd == signC)
         goto uiZ;
   }
   __float_raise(float_flag_invalid);
   uiZ = 0xFFC00000;
propagateNaN_ZC:
   uiZ = __propagateFloat32NaN_ieee(uiZ, uiC);
   goto uiZ;
   /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
zeroProd:
   uiZ = uiC;
   if(!(expC | sigC) && (signProd != signC))
   {
   completeCancellation:
      uiZ = ((__bits32)(__float_rounding_mode == float_round_down))
            << 31; // __packFloat32((__float_rounding_mode == float_round_down), 0, 0, IEEE32_PACK);
   }
uiZ:
   return uiZ;
}

/*----------------------------------------------------------------------------
| Returns the remainder of the single-precision floating-point value `a'
| with respect to the corresponding value `b'.  The operation is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/
///   UNUSED
//    __float32 __float32_rem_ieee(__float32 a, __float32 b)
//    {
//       __flag aSign, bSign, zSign;
//       __int16 aExp, bExp, expDiff;
//       __bits32 aSig, bSig;
//       __bits32 q;
//       __bits64 aSig64, bSig64, q64;
//       __bits32 alternateASig;
//       __sbits32 sigMean;
//
//       aSig = __extractFloat32Frac(a, IEEE32_EXTRACT_FRAC);
//       aExp = __extractFloat32Exp(a, IEEE32_EXTRACT_EXP);
//       aSign = __extractFloat32Sign(a, IEEE32_EXTRACT_SIGN);
//       bSig = __extractFloat32Frac(b, IEEE32_EXTRACT_FRAC);
//       bExp = __extractFloat32Exp(b, IEEE32_EXTRACT_EXP);
//       bSign = __extractFloat32Sign(b, IEEE32_EXTRACT_SIGN);
//       if(aExp == 0xFF)
//       {
//          if(aSig || ((bExp == 0xFF) && bSig))
//          {
//             return __propagateFloat32NaN_ieee(a, b);
//          }
//          __float_raise(float_flag_invalid);
//          return __float32_default_nan;
//       }
//       if(bExp == 0xFF)
//       {
//          if(bSig)
//             return __propagateFloat32NaN_ieee(a, b);
//          return a;
//       }
//       if(bExp == 0)
//       {
//          if(bSig == 0)
//          {
//             __float_raise(float_flag_invalid);
//             return __float32_default_nan;
//          }
//    #ifdef NO_SUBNORMALS
//          return __float32_default_nan;
//    #else
//          __normalizeFloat32Subnormal(bSig, bExp, bSig, IEEE32_FRAC_BITS);
//    #endif
//       }
//       if(aExp == 0)
//       {
//    #ifdef NO_SUBNORMALS
//          return a;
//    #else
//          if(aSig == 0)
//             return a;
//          __normalizeFloat32Subnormal(aSig, aExp, aSig, IEEE32_FRAC_BITS);
//    #endif
//       }
//       expDiff = aExp - bExp;
//       aSig |= 0x00800000;
//       bSig |= 0x00800000;
//       if(expDiff < 32)
//       {
//          aSig <<= 8;
//          bSig <<= 8;
//          if(expDiff < 0)
//          {
//             if(expDiff < -1)
//                return a;
//             aSig >>= 1;
//          }
//          q = (bSig <= aSig);
//          if(q)
//             aSig -= bSig;
//          if(0 < expDiff)
//          {
//             q = (((__bits64)aSig) << 32) / bSig;
//             q >>= 32 - expDiff;
//             bSig >>= 2;
//             aSig = ((aSig >> 1) << (expDiff - 1)) - bSig * q;
//          }
//          else
//          {
//             aSig >>= 2;
//             bSig >>= 2;
//          }
//       }
//       else
//       {
//          if(bSig <= aSig)
//             aSig -= bSig;
//          aSig64 = ((__bits64)aSig) << 40;
//          bSig64 = ((__bits64)bSig) << 40;
//          expDiff -= 64;
//          while(0 < expDiff)
//          {
//             q64 = __estimateDiv128To64(aSig64, 0, bSig64);
//             q64 = (2 < q64) ? q64 - 2 : 0;
//             aSig64 = -((bSig * q64) << 38);
//             expDiff -= 62;
//          }
//          expDiff += 64;
//          q64 = __estimateDiv128To64(aSig64, 0, bSig64);
//          q64 = (2 < q64) ? q64 - 2 : 0;
//          q = q64 >> (64 - expDiff);
//          bSig <<= 6;
//          aSig = ((aSig64 >> 33) << (expDiff - 1)) - bSig * q;
//       }
//       do
//       {
//          alternateASig = aSig;
//          ++q;
//          aSig -= bSig;
//       } while(0 <= (__sbits32)aSig);
//       sigMean = aSig + alternateASig;
//       if((sigMean < 0) || ((sigMean == 0) && (q & 1)))
//       {
//          aSig = alternateASig;
//       }
//       zSign = ((__sbits32)aSig < 0);
//       if(zSign)
//          aSig = -aSig;
//       return __normalizeRoundAndPackFloat32_ieee(aSign ^ zSign, bExp, aSig);
//    }

/*----------------------------------------------------------------------------
| Returns the square root of the single-precision floating-point value `a'.
| The operation is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/
///   UNUSED
//    __float32 __float32_sqrt(__float32 a)
//    {
//       __flag aSign;
//       __int16 aExp, zExp;
//       __bits32 aSig, zSig;
//       __bits64 rem, term;
//
//       aSig = __extractFloat32Frac(a, IEEE32_EXTRACT_FRAC);
//       aExp = __extractFloat32Exp(a, IEEE32_EXTRACT_EXP);
//       aSign = __extractFloat32Sign(a, IEEE32_EXTRACT_SIGN);
//       if(aExp == 0xFF)
//       {
//          if(aSig)
//             return __propagateFloat32NaN_ieee(a, 0);
//          if(!aSign)
//             return a;
//          __float_raise(float_flag_invalid);
//          return __float32_default_nan;
//       }
//       if(aSign)
//       {
//          if((aExp | aSig) == 0)
//             return a;
//          __float_raise(float_flag_invalid);
//          return __float32_default_nan;
//       }
//       if(aExp == 0)
//       {
//    #ifdef NO_SUBNORMALS
//          return 0;
//    #else
//          if(aSig == 0)
//             return 0;
//          __normalizeFloat32Subnormal(aSig, aExp, aSig, IEEE32_FRAC_BITS);
//    #endif
//       }
//       zExp = ((aExp - 0x7F) >> 1) + 0x7E;
//       aSig = (aSig | 0x00800000) << 8;
//       zSig = __estimateSqrt32(aExp, aSig) + 2;
//       if((zSig & 0x7F) <= 5)
//       {
//          if(zSig < 2)
//          {
//             zSig = 0x7FFFFFFF;
//             goto roundAndPack;
//          }
//          aSig >>= aExp & 1;
//          term = ((__bits64)zSig) * zSig;
//          rem = (((__bits64)aSig) << 32) - term;
//          while((__sbits64)rem < 0)
//          {
//             --zSig;
//             rem += (((__bits64)zSig) << 1) | 1;
//          }
//          zSig |= (rem != 0);
//       }
//       __shift32RightJamming(zSig, 1, &zSig);
//    roundAndPack:
//       return __roundAndPackFloat32(0, zExp, zSig, IEEE32_PACK, IEEE_EXC);
//    }

/*----------------------------------------------------------------------------
| Returns 1 if the single-precision floating-point value `a' is equal to
| the corresponding value `b', and 0 otherwise.  The invalid exception is
| raised if either operand is a NaN.  Otherwise, the comparison is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__flag __float32_eq_signaling_ieee(__float32 a, __float32 b)
{
   if(((__extractFloat32Exp(a, IEEE32_EXTRACT_EXP) == 0xFF) && __extractFloat32Frac(a, IEEE32_EXTRACT_FRAC)) ||
      ((__extractFloat32Exp(b, IEEE32_EXTRACT_EXP) == 0xFF) && __extractFloat32Frac(b, IEEE32_EXTRACT_FRAC)))
   {
      __float_raise(float_flag_invalid);
      return 0;
   }
   return (a == b) || ((__bits32)((a | b) << 1) == 0);
}

/*----------------------------------------------------------------------------
| Returns 1 if the single-precision floating-point value `a' is less than or
| equal to the corresponding value `b', and 0 otherwise.  Quiet NaNs do not
| cause an exception.  Otherwise, the comparison is performed according to the
| IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__flag __float32_le_quiet_ieee(__float32 a, __float32 b)
{
   __flag aSign, bSign;
   __int16 aExp, bExp;

   if(((__extractFloat32Exp(a, IEEE32_EXTRACT_EXP) == 0xFF) && __extractFloat32Frac(a, IEEE32_EXTRACT_FRAC)) ||
      ((__extractFloat32Exp(b, IEEE32_EXTRACT_EXP) == 0xFF) && __extractFloat32Frac(b, IEEE32_EXTRACT_FRAC)))
   {
      if(__float_is_signaling_nan(a, IEEE32_SPEC_ARGS) || __float_is_signaling_nan(b, IEEE32_SPEC_ARGS))
      {
         __float_raise(float_flag_invalid);
      }
      return 0;
   }
   aSign = __extractFloat32Sign(a, IEEE32_EXTRACT_SIGN);
   bSign = __extractFloat32Sign(b, IEEE32_EXTRACT_SIGN);
   if(aSign != bSign)
      return aSign || ((__bits32)((a | b) << 1) == 0);
   return (a == b) || (aSign ^ (a < b));
}

/*----------------------------------------------------------------------------
| Returns 1 if the single-precision floating-point value `a' is less than
| the corresponding value `b', and 0 otherwise.  Quiet NaNs do not cause an
| exception.  Otherwise, the comparison is performed according to the IEC/IEEE
| Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__flag __float32_lt_quiet_ieee(__float32 a, __float32 b)
{
   __flag aSign, bSign;

   if(((__extractFloat32Exp(a, IEEE32_EXTRACT_EXP) == 0xFF) && __extractFloat32Frac(a, IEEE32_EXTRACT_FRAC)) ||
      ((__extractFloat32Exp(b, IEEE32_EXTRACT_EXP) == 0xFF) && __extractFloat32Frac(b, IEEE32_EXTRACT_FRAC)))
   {
      if(__float_is_signaling_nan(a, IEEE32_SPEC_ARGS) || __float_is_signaling_nan(b, IEEE32_SPEC_ARGS))
      {
         __float_raise(float_flag_invalid);
      }
      return 0;
   }
   aSign = __extractFloat32Sign(a, IEEE32_EXTRACT_SIGN);
   bSign = __extractFloat32Sign(b, IEEE32_EXTRACT_SIGN);
   if(aSign != bSign)
      return aSign && ((__bits32)((a | b) << 1) != 0);
   return (a != b) && (aSign ^ (a < b));
}

/*----------------------------------------------------------------------------
| Returns the result of converting the double-precision floating-point value
| `a' to the 32-bit two's complement integer format.  The conversion is
| performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic---which means in particular that the conversion is rounded
| according to the current rounding mode.  If `a' is a NaN, the largest
| positive integer is returned.  Otherwise, if the conversion overflows, the
| largest integer with the same sign as `a' is returned.
*----------------------------------------------------------------------------*/

__int32 __float64_to_int32(__float64 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                           FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   __flag aSign;
   __int32 aExp, shiftCount;
   __bits64 aSig;

   aSig = __extractFloat64Frac(a, __frac_bits);
   aExp = __extractFloat64Exp(a, __exp_bits, __frac_bits);
   aSign = __extractFloat64Sign(a, __exp_bits, __frac_bits, __sign);
   if(__exc == FLOAT_EXC_STD)
   {
      if((aExp == ((1ULL << __exp_bits) - 1)) && aSig)
      {
         aSign = 0;
      }
   }
   if(aExp)
   {
      aSig |= (1ULL << __frac_bits);
   }
   shiftCount = (45 - __exp_bias) - aExp;
   if(0 < shiftCount)
      __shift64RightJamming(aSig, shiftCount, &aSig);
   return __roundAndPackInt32(aSign, aSig);
}

/*----------------------------------------------------------------------------
| Returns the result of converting the double-precision floating-point value
| `a' to the 32-bit two's complement integer format.  The conversion is
| performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic, except that the conversion is always rounded toward zero.
| If `a' is a NaN, the largest positive integer is returned.  Otherwise, if
| the conversion overflows, the largest integer with the same sign as `a' is
| returned.
*----------------------------------------------------------------------------*/

__int32 __float64_to_int32_round_to_zero(__float64 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                                         FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm,
                                         __sbits8 __sign)
{
   __flag aSign;
   __int32 aExp, shiftCount;
   __bits64 aSig, savedASig;
   __int32 z;

   aSig = __extractFloat64Frac(a, __frac_bits);
   aExp = __extractFloat64Exp(a, __exp_bits, __frac_bits);
   aSign = __extractFloat64Sign(a, __exp_bits, __frac_bits, __sign);
   if(aExp > (31 - __exp_bias))
   {
      if((aExp == ((1ULL << __exp_bits) - 1)) && aSig && (__exc == FLOAT_EXC_STD))
         aSign = 0;
      goto invalid;
   }
   else if(aExp < -__exp_bias)
   {
#ifndef NO_PARAMETRIC
      if(aExp || aSig)
         __float_exception_flags |= float_flag_inexact;
#endif
      return 0;
   }
   aSig |= (1ULL << __frac_bits);
   shiftCount = (__frac_bits - __exp_bias) - aExp;
   savedASig = aSig;
   if(shiftCount < 0 && __frac_bits < 30)
   {
      aSig <<= -shiftCount;
   }
   else
   {
      aSig >>= shiftCount;
   }
   z = aSig;
   if(aSign)
      z = -z;
   if((z < 0) ^ aSign)
   {
   invalid:
      __float_raise(float_flag_invalid);
      return aSign ? (__sbits32)0x80000000 : 0x7FFFFFFF;
   }
   if(shiftCount < 0 && __frac_bits < 30)
   {
      if((aSig >> -shiftCount) != savedASig)
      {
#ifndef NO_PARAMETRIC
         __float_exception_flags |= float_flag_inexact;
#endif
      }
   }
   else
   {
      if((aSig << shiftCount) != savedASig)
      {
#ifndef NO_PARAMETRIC
         __float_exception_flags |= float_flag_inexact;
#endif
      }
   }
   return z;
}

__uint32 __float64_to_uint32_round_to_zero(__float64 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                                           FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm,
                                           __sbits8 __sign)
{
   __flag aSign;
   __int32 aExp, shiftCount;
   __bits64 aSig, savedASig;
   __uint32 z;

   aSig = __extractFloat64Frac(a, __frac_bits);
   aExp = __extractFloat64Exp(a, __exp_bits, __frac_bits);
   aSign = __extractFloat64Sign(a, __exp_bits, __frac_bits, __sign);
   if(aExp > (31 - __exp_bias))
   {
      if((aExp == ((1ULL << __exp_bits) - 1)) && aSig)
         aSign = 0;
      __float_raise(float_flag_invalid);
      return 0x80000000;
   }
   else if(aExp < -__exp_bias)
   {
#ifndef NO_PARAMETRIC
      if(aExp || aSig)
         __float_exception_flags |= float_flag_inexact;
#endif
      return 0;
   }
   aSig |= (1ULL << __frac_bits);
   shiftCount = (__frac_bits - __exp_bias) - aExp;
   savedASig = aSig;
   if(shiftCount < 0 && __frac_bits < 30)
   {
      aSig <<= -shiftCount;
   }
   else
   {
      aSig >>= shiftCount;
   }
   z = aSig;
   if(aSign)
      z = -z;
   if(shiftCount < 0 && __frac_bits < 30)
   {
      if((aSig >> -shiftCount) != savedASig)
      {
#ifndef NO_PARAMETRIC
         __float_exception_flags |= float_flag_inexact;
#endif
      }
   }
   else
   {
      if((aSig << shiftCount) != savedASig)
      {
#ifndef NO_PARAMETRIC
         __float_exception_flags |= float_flag_inexact;
#endif
      }
   }
   return z;
}

/*----------------------------------------------------------------------------
| Returns the result of converting the double-precision floating-point value
| `a' to the 64-bit two's complement integer format.  The conversion is
| performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic---which means in particular that the conversion is rounded
| according to the current rounding mode.  If `a' is a NaN, the largest
| positive integer is returned.  Otherwise, if the conversion overflows, the
| largest integer with the same sign as `a' is returned.
*----------------------------------------------------------------------------*/

__int64 __float64_to_int64(__float64 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                           FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   __flag aSign;
   __int16 aExp, shiftCount;
   __bits64 aSig, aSigExtra;

   aSig = __extractFloat64Frac(a, __frac_bits);
   aExp = __extractFloat64Exp(a, __exp_bits, __frac_bits);
   aSign = __extractFloat64Sign(a, __exp_bits, __frac_bits, __sign);
   if(aExp)
      aSig |= (1ULL << __frac_bits);
   shiftCount = (__frac_bits - __exp_bias) - aExp;
   if(shiftCount <= 0)
   {
      if(aExp > (63 - __exp_bias))
      {
         __float_raise(float_flag_invalid);
         if(!aSign || ((aExp == ((1ULL << __exp_bits) - 1)) && (aSig != (1ULL << __frac_bits))))
         {
            return LIT64(0x7FFFFFFFFFFFFFFF);
         }
         return (__sbits64)LIT64(0x8000000000000000);
      }
      aSigExtra = 0;
      aSig <<= -shiftCount;
   }
   else
   {
      __shift64ExtraRightJamming(aSig, 0, shiftCount, &aSig, &aSigExtra);
   }
   return __roundAndPackInt64(aSign, aSig, aSigExtra);
}

/*----------------------------------------------------------------------------
| Returns the result of converting the double-precision floating-point value
| `a' to the 64-bit two's complement integer format.  The conversion is
| performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic, except that the conversion is always rounded toward zero.
| If `a' is a NaN, the largest positive integer is returned.  Otherwise, if
| the conversion overflows, the largest integer with the same sign as `a' is
| returned.
*----------------------------------------------------------------------------*/

__int64 __float64_to_int64_round_to_zero(__float64 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                                         FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm,
                                         __sbits8 __sign)
{
   __flag aSign;
   __int32 aExp, shiftCount;
   __bits64 aSig;
   __int64 z;

   aSig = __extractFloat64Frac(a, __frac_bits);
   aExp = __extractFloat64Exp(a, __exp_bits, __frac_bits);
   aSign = __extractFloat64Sign(a, __exp_bits, __frac_bits, __sign);
   if(aExp)
      aSig |= (1ULL << __frac_bits);
   shiftCount = aExp + (__exp_bias - __frac_bits);
   if(0 <= shiftCount)
   {
      if(aExp >= (63 - __exp_bias))
      {
         if(a !=
            ((1ULL << (__exp_bits + __frac_bits)) | (((63 + __exp_bias) & ((1ULL << __exp_bits) - 1)) << __frac_bits)))
         {
            __float_raise(float_flag_invalid);
            if(!aSign || ((aExp == ((1ULL << __exp_bits) - 1)) && (aSig != (1ULL << __frac_bits))))
            {
               return LIT64(0x7FFFFFFFFFFFFFFF);
            }
         }
         return (__sbits64)LIT64(0x8000000000000000);
      }
      z = aSig << shiftCount;
   }
   else
   {
      if(aExp < (-1 - __exp_bias))
      {
#ifndef NO_PARAMETRIC
         if(aExp | aSig)
            __float_exception_flags |= float_flag_inexact;
#endif
         return 0;
      }
      z = aSig >> (-shiftCount);
      if((__bits64)(aSig << (shiftCount & 63)))
      {
#ifndef NO_PARAMETRIC
         __float_exception_flags |= float_flag_inexact;
#endif
      }
   }
   if(aSign)
      z = -z;
   return z;
}

__uint64 __float64_to_uint64_round_to_zero(__float64 a, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                                           FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm,
                                           __sbits8 __sign)
{
   __flag aSign;
   __int16 aExp, shiftCount;
   __bits64 aSig;
   __uint64 z;

   aSig = __extractFloat64Frac(a, __frac_bits);
   aExp = __extractFloat64Exp(a, __exp_bits, __frac_bits);
   aSign = __extractFloat64Sign(a, __exp_bits, __frac_bits, __sign);
   if(aExp)
      aSig |= (1ULL << __frac_bits);
   shiftCount = aExp + (__exp_bias - __frac_bits);
   if(0 <= shiftCount)
   {
      if(aExp >= (63 - __exp_bias))
      {
         return (__bits64)LIT64(0x8000000000000000);
      }
      z = aSig << shiftCount;
   }
   else
   {
      if(aExp < (-1 - __exp_bias))
      {
#ifndef NO_PARAMETRIC
         if(aExp | aSig)
            __float_exception_flags |= float_flag_inexact;
#endif
         return 0;
      }
      z = aSig >> (-shiftCount);
      if((__bits64)(aSig << (shiftCount & 63)))
      {
#ifndef NO_PARAMETRIC
         __float_exception_flags |= float_flag_inexact;
#endif
      }
   }
   if(aSign)
      z = -z;
   return z;
}

/*----------------------------------------------------------------------------
| Returns the result of converting the double-precision floating-point value
| `a' to the single-precision floating-point format.  The conversion is
| performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

__float32 __float64_to_float32_ieee(__float64 a, FLOAT_EXC_TYPE __exc, __flag __subnorm)
{
   __flag aSign;
   __int16 aExp;
   __bits64 aSig;
   __bits32 zSig;

   aSig = __extractFloat64Frac(a, IEEE64_EXTRACT_FRAC);
   aExp = __extractFloat64Exp(a, IEEE64_EXTRACT_EXP);
   aSign = __extractFloat64Sign(a, IEEE64_EXTRACT_SIGN);
   if(aExp == 0x7FF && (__exc == FLOAT_EXC_STD))
   {
      if(aSig)
         return __commonNaNToFloat32_ieee(__float64ToCommonNaN_ieee(a));
      return (((__bits32)aSign) << 31) | 0x7F800000; // __packFloat32(aSign, 0xFF, 0, IEEE32_PACK);
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

#ifdef FLOATX80

/*----------------------------------------------------------------------------
| Returns the result of converting the double-precision floating-point value
| `a' to the extended double-precision floating-point format.  The conversion
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

__floatx80 __float64_to_floatx80_ieee(__float64 a)
{
   __flag aSign;
   __int16 aExp;
   __bits64 aSig;

   aSig = __extractFloat64Frac(a, IEEE64_EXTRACT_FRAC);
   aExp = __extractFloat64Exp(a, IEEE64_EXTRACT_EXP);
   aSign = __extractFloat64Sign(a, IEEE64_EXTRACT_SIGN);
   if(aExp == 0x7FF)
   {
      if(aSig)
         return __commonNaNToFloatx80(__float64ToCommonNaN(a));
      return __packFloatx80(aSign, 0x7FFF, LIT64(0x8000000000000000));
   }
   if(aExp == 0)
   {
      if(aSig == 0)
         return __packFloatx80(aSign, 0, 0);
      __normalizeFloat64Subnormal(aSig, &aExp, &aSig);
   }
   return __packFloatx80(aSign, aExp + 0x3C00, (aSig | LIT64(0x0010000000000000)) << 11);
}

#endif

#ifdef FLOAT128

/*----------------------------------------------------------------------------
| Returns the result of converting the double-precision floating-point value
| `a' to the quadruple-precision floating-point format.  The conversion is
| performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

__float128 __float64_to_float128_ieee(__float64 a)
{
   __flag aSign;
   __int16 aExp;
   __bits64 aSig, zSig0, zSig1;

   aSig = __extractFloat64Frac(a, IEEE64_EXTRACT_FRAC);
   aExp = __extractFloat64Exp(a, IEEE64_EXTRACT_EXP);
   aSign = __extractFloat64Sign(a, IEEE64_EXTRACT_SIGN);
   if(aExp == 0x7FF)
   {
      if(aSig)
         return __commonNaNToFloat128(__float64ToCommonNaN(a));
      return __packFloat128(aSign, 0x7FFF, 0, 0);
   }
   if(aExp == 0)
   {
      if(aSig == 0)
         return __packFloat128(aSign, 0, 0, 0);
      __normalizeFloat64Subnormal(aSig, &aExp, &aSig);
      --aExp;
   }
   __shift128Right(aSig, 0, 4, &zSig0, &zSig1);
   return __packFloat128(aSign, aExp + 0x3C00, zSig0, zSig1);
}

#endif

/*----------------------------------------------------------------------------
| Rounds the double-precision floating-point value `a' to an integer, and
| returns the result as a double-precision floating-point value.  The
| operation is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__float64 __float64_round_to_int_ieee(__float64 a)
{
   __flag aSign;
   __int16 aExp;
   __bits64 lastBitMask, roundBitsMask;
   __int8 roundingMode;
   __float64 z;

   aExp = __extractFloat64Exp(a, IEEE64_EXTRACT_EXP);
   if(0x433 <= aExp)
   {
      if((aExp == 0x7FF) && __extractFloat64Frac(a, IEEE64_EXTRACT_FRAC))
      {
         return __propagateFloat64NaN_ieee(a, a);
      }
      return a;
   }
   if(aExp < 0x3FF)
   {
      if((__bits64)(a << 1) == 0)
         return a;
#ifndef NO_PARAMETRIC
      __float_exception_flags |= float_flag_inexact;
#endif
      aSign = __extractFloat64Sign(a, IEEE64_EXTRACT_SIGN);
      switch(__float_rounding_mode)
      {
         case float_round_nearest_even:
            if((aExp == 0x3FE) && __extractFloat64Frac(a, IEEE64_EXTRACT_FRAC))
            {
               return (((__bits64)aSign) << 63) | 0x3FF0000000000000; // __packFloat64(aSign, 0x3FF, 0, IEEE64_PACK);
            }
            break;
         case float_round_down:
            return aSign ? LIT64(0xBFF0000000000000) : 0;
         case float_round_up:
            return aSign ? LIT64(0x8000000000000000) : LIT64(0x3FF0000000000000);
      }
      return ((__bits64)aSign) << 63; // __packFloat64(aSign, 0, 0, IEEE64_PACK);
   }
   lastBitMask = 1;
   lastBitMask <<= 0x433 - aExp;
   roundBitsMask = lastBitMask - 1;
   z = a;
   roundingMode = __float_rounding_mode;
   if(roundingMode == float_round_nearest_even)
   {
      z += lastBitMask >> 1;
      if((z & roundBitsMask) == 0)
         z &= ~lastBitMask;
   }
   else if(roundingMode != float_round_to_zero)
   {
      if(__extractFloat64Sign(z, IEEE64_EXTRACT_SIGN) ^ (roundingMode == float_round_up))
      {
         z += roundBitsMask;
      }
   }
   z &= ~roundBitsMask;
#ifndef NO_PARAMETRIC
   if(z != a)
      __float_exception_flags |= float_flag_inexact;
#endif
   return z;
}

/*----------------------------------------------------------------------------
| Returns the result of adding the absolute values of the double-precision
| floating-point values `a' and `b'.  If `zSign' is 1, the sum is negated
| before being returned.  `zSign' is ignored if the result is a NaN.
| The addition is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float __float_addsub(__float _a, __float _b, __flag sub, __bits8 __exp_bits,
                                             __bits8 __frac_bits, __sbits32 __exp_bias, FLOAT_RND_TYPE __rnd,
                                             FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   __bits64 a, b;
   __bits64 aSig, bSig, shift_0;
   __bits64 aExp, bExp, expDiff;
   __bits16 nZeros;
   _Bool a_c_nan, b_c_nan, a_c_normal, b_c_normal, tmp_c_normal, swap, sAB, aSign, bSign;
   __bits64 abs_a, abs_b;
   __bits64 fA, fB, fB_shifted;
   __bits64 fB_shifted_low, fBleft_shifted;
   _Bool LSB_bit, Guard_bit, Round_bit, Sticky_bit, round, sb;
   _Bool ge_frac_bits;
   // __bits64 tmp_x;
   _Bool subnormal_exp_correction;
   __bits64 fB_shifted1;
   __bits64 fR0;
   _Bool R_c_zero;
   __bits64 RExp0, RExp1;
   __bits64 RSig0, RSig1, RSig2, RSig3;
   __bits64 RExp0RSig1, Rrounded;
   _Bool overflow_to_infinite, saturation;
   _Bool aExpMax, bExpMax, aExp_null, bExp_null, aSig_not, bSig_not;
   __bits8 __frac_shift, __frac_full, __frac_almost, __exp_shift, __nzeros_bits;

   __frac_shift = (__rnd == FLOAT_RND_NEVN) ? 2 : 0;
   __frac_almost = __frac_bits + __frac_shift + 1;
   __frac_full = __frac_almost + 1;

   abs_a = _a & ((1ULL << (__exp_bits + __frac_bits)) - 1);
   abs_b = _b & ((1ULL << (__exp_bits + __frac_bits)) - 1);
   swap = abs_a < abs_b;

   // Faster, but more area (~0.35 slice/bit more than COND_EXPR_MACRO64)
   // tmp_x = (_a ^ _b) & ((__bits64)((((__sbits64)(swap)) << 63) >> 63));
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

   fA = (aSig | (((__bits64)a_c_normal) << __frac_bits)) << __frac_shift;
   fB = (bSig | (((__bits64)b_c_normal) << __frac_bits)) << __frac_shift;

   __exp_shift =
       (__frac_bits > 1 ?
            (__frac_bits > 2 ?
                 (__frac_bits > 4 ? (__frac_bits > 8 ? (__frac_bits > 16 ? (__frac_bits > 32 ? 6 : 5) : 4) : 3) : 2) :
                 1) :
            0);
   ge_frac_bits = (expDiff >> __exp_shift) != 0;
   expDiff = (expDiff | (__bits32)(((((__sbits32)ge_frac_bits) << 31) >> 31))) & ((1ULL << __exp_shift) - 1);

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

   fB_shifted1 = ((__bits64)((((__sbits64)sAB) << 63) >> 63)) ^ fB_shifted;
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
      RSig2 = (overflow_to_infinite ? (__bits64)((((__sbits64)!R_c_zero) << 63) >> 63) : Rrounded) &
              ((1ULL << __frac_bits) - 1);
   }

   aSign = aSign && (!R_c_zero || !sAB);

   if(__exc == FLOAT_EXC_STD)
   {
#ifdef NO_SIGNALLING
      RSig3 = (((__bits64)(a_c_nan || b_c_nan || (sAB && aExpMax && bExpMax))) << (__frac_bits - 1)) | RSig2;
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
   return (((__bits64)aSign) << (__exp_bits + __frac_bits)) | (RExp1 << __frac_bits) | RSig3;
}

/*----------------------------------------------------------------------------
| Returns the result of adding the double-precision floating-point values `a'
| and `b'.  The operation is performed according to the IEC/IEEE Standard for
| Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__float __float_add(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                    FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   return __float_addsub(a, b, 0, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}

/*----------------------------------------------------------------------------
| Returns the result of subtracting the double-precision floating-point values
| `a' and `b'.  The operation is performed according to the IEC/IEEE Standard
| for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__float __float_sub(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                    FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   return __float_addsub(a, b, 1, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}

/*----------------------------------------------------------------------------
| Returns the result of multiplying the double-precision floating-point values
| `a' and `b'.  The operation is performed according to the IEC/IEEE Standard
| for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__float __float_mul(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                    FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   __flag aSign, bSign, zSign;
   __flag aExpMax, bExpMax;
   __bits32 aExp, bExp;
   __bits64 aSig, bSig;
   __bits8 a_c, b_c, z_c;
   _Bool a_c_zero, b_c_zero, a_c_inf, b_c_inf, a_c_nan, b_c_nan, a_c_normal, b_c_normal;
   __bits32 expSum, expPostNorm;
   _Bool norm, expSigOvf0, expSigOvf2;
   _Bool sticky, guard, round, expSigOvf1;
   __bits64 expSig, expSigPostRound, zSig;
   __bits64 excPostNorm;
   __bits64 u1, u0, v1, v0, k0, k1, k2, k3, k4, k5, k6, k7, res_2K_0, res_full_2K;
   __bits64 sigProdHigh, sigProdLow, sigProdExtHigh, sigProdExtLow;
   SF_UDItype sigProd, sigProdExt;
   __bits8 __frac_mul, __frac_full, __frac_almost, __k_bits;
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
   expSum = aExp + bExp + ((__bits32)__exp_bias);

   aSig = (aSig | (((__bits64)(__one & (a_c_normal | !__subnorm))) << __frac_bits));
   bSig = (bSig | (((__bits64)(__one & (b_c_normal | !__subnorm))) << __frac_bits));

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
      __bits64 u0, u1, v1, v0, ts, ks, k, t, w1, w2, w3, w_0, w_1;
      __bits8 __low_high = __frac_bits - (__frac_mul - 63);
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
      expSig = (((__bits64)expPostNorm) << __frac_bits) | sigProdExtHigh;
      sigProdExt = 0;
   }
   else
   {
      sigProd = (SF_UDItype)(SF_USItype)(aSig) * (SF_USItype)(bSig);
      norm = (sigProd >> __frac_mul) & 1;
      expPostNorm = expSum + norm;
      sigProdExt = sigProd << !norm;
      sigProdExt = (sigProdExt & ((1ULL << __frac_mul) - 1)) << 1;
      expSig = (((__bits64)expPostNorm) << __frac_bits) | ((sigProdExt >> __frac_full) & ((1ULL << __frac_bits) - 1));
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
         expSigOvf1 = round & (expSig == ((__bits64)-1));
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
   zSig = (((__bits64)zSign) << (__exp_bits + __frac_bits)) |
          (expSigPostRound & ((1ULL << (__exp_bits + __frac_bits)) - 1));
   if(z_c == FP_CLS_NORMAL)
   {
      z_c = ((excPostNorm == 1) << 1) | (excPostNorm == 0);
   }

   if(z_c == FP_CLS_NORMAL)
      return zSig;
   else if(z_c == FP_CLS_ZERO)
      return ((__bits64)zSign) << (__exp_bits + __frac_bits); // __packFloat(zSign, 0, 0, __exp_bits, __frac_bits);
   else if(z_c == FP_CLS_NAN && (__exc == FLOAT_EXC_STD))
      return __float_nan(__exp_bits, __frac_bits, __sign);
   else
      return (((__bits64)zSign) << (__exp_bits + __frac_bits)) | (((1ULL << __exp_bits) - 1) << __frac_bits) |
             ((__exc == FLOAT_EXC_STD) ? 0ULL : ((1ULL << __frac_bits) - 1));
}

/*----------------------------------------------------------------------------
| Returns the result of dividing the double-precision floating-point value `a'
| by the corresponding value `b'.  The operation is performed according to
| the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

#define LOOP_BODY_F_DIV(z, n, data)                                \
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

#define FLOAT_SRT4_UNROLL 1

__float __float_divSRT4(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                        FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   __bits8 a_c, b_c, z_c;
   _Bool a_c_zero, b_c_zero, a_c_inf, b_c_inf, a_c_nan, b_c_nan, a_c_normal, b_c_normal;
   _Bool aSign, bSign, zSign, q_i2, q_i1, q_i0, nq_i2;
   _Bool MsbB = (b >> (__frac_bits - 1)) & 1, correction;
   __int32 aExp, bExp, zExp;
   __bits64 aSig, bSig, nbSig, bSigx2, nbSigx2, zSig1, zSig0;
   __bits64 zExpSig;
   __bits64 bSigx3, nbSigx3, current, w, positive = 0, negative = 0;
   __bits8 current_sel, q_i, index;
   _Bool LSB_bit, Guard_bit, Round_bit, round;
   _Bool MSB1zExp, MSB0zExp, MSBzExp, ovfCond;
   __bits8 __frac_p3, __div_p3, __div_it, __div_bits, __div_waste;
   _Bool __frac_odd;
   __frac_p3 = (__rnd == FLOAT_RND_NEVN) ? (__frac_bits + 3) : (__frac_bits + 1);
   __div_p3 = __frac_bits + 3;
   __frac_odd = __frac_p3 & 1;
   __div_it = __frac_p3 / (FLOAT_SRT4_UNROLL * 2);
   __div_it = (__frac_p3 % (FLOAT_SRT4_UNROLL * 2)) != 0 ? (__div_it + 1) : __div_it;
   __div_bits = __div_it * FLOAT_SRT4_UNROLL * 2;
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
   _Bool aExp_null = aExp == 0;
   _Bool bExp_null = bExp == 0;
   _Bool aExpMax = aExp == ((1ULL << __exp_bits) - 1);
   _Bool bExpMax = bExp == ((1ULL << __exp_bits) - 1);
   _Bool aSig_null = aSig == 0;
   _Bool bSig_null = bSig == 0;

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
         unsigned long long int subnormal_lz, mshifted;
         count_leading_zero_lshift_runtime_macro(__frac_bits, aSig, subnormal_lz, mshifted);
         aExp = -subnormal_lz;
         aSig = (mshifted << 1) & ((1ULL << __frac_bits) - 1);
      }
      if(bExp_null && !bSig_null)
      {
         unsigned long long int subnormal_lz, mshifted;
         count_leading_zero_lshift_runtime_macro(__frac_bits, bSig, subnormal_lz, mshifted);
         bExp = -subnormal_lz;
         bSig = (mshifted << 1) & ((1ULL << __frac_bits) - 1);
      }
   }

   aSig = aSig | (((__bits64)(__one & (a_c_normal | !__subnorm))) << __frac_bits);
   bSig = bSig | (((__bits64)(__one & (b_c_normal | !__subnorm))) << __frac_bits);
   nbSig = -bSig;
   bSigx2 = bSig << 1;
   nbSigx2 = -bSigx2;
   bSigx3 = bSigx2 + bSig;
   nbSigx3 = -bSigx3;
   current = aSig;
   for(index = 0; index < __div_it; ++index)
   {
      BOOST_PP_REPEAT(FLOAT_SRT4_UNROLL, LOOP_BODY_F_DIV, index);
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
      zExpSig = ((((__bits64)zExp) << __frac_bits) | (zSig1 >> 2)) + round;
   }
   else
   {
      zExpSig = (((__bits64)zExp) << __frac_bits) | zSig1;
   }
   MSBzExp = zExpSig >> (__exp_bits + __frac_bits);
   ovfCond = ((((MSB0zExp & ((~MSBzExp) & 1)) & 1) ^ MSB1zExp) & 1);
   if(z_c == FP_CLS_NORMAL)
   {
      if(ovfCond)
         return ((__bits64)zSign) << ((__exp_bits + __frac_bits));
      else if(MSBzExp || ((zExp == ((1ULL << __exp_bits) - 1)) && (__exc != FLOAT_EXC_OVF)))
         return (((__bits64)zSign) << (__exp_bits + __frac_bits)) | (((1ULL << __exp_bits) - 1) << __frac_bits) |
                ((__exc == FLOAT_EXC_STD) ? 0ULL : ((1ULL << __frac_bits) - 1));
      else
         return (((__bits64)zSign) << (__exp_bits + __frac_bits)) |
                (zExpSig & ((1ULL << (__exp_bits + __frac_bits)) - 1));
   }
   else if(z_c == FP_CLS_ZERO)
      return ((__bits64)zSign) << ((__exp_bits + __frac_bits));
   else if(z_c == FP_CLS_NAN)
      return (((__bits64)(a_c_nan ? aSign : bSign) | (a_c_inf & b_c_inf) | (a_c_zero & b_c_zero))
              << (__exp_bits + __frac_bits)) |
             ((__exc == FLOAT_EXC_STD) ? ((((1ULL << __exp_bits) - 1) << __frac_bits) | (1ULL << (__frac_bits - 1)) |
                                          (a_c_nan ? aSig : (b_c_nan ? bSig : 0))) :
                                         (0ULL));
   else
      return (((__bits64)zSign) << (__exp_bits + __frac_bits)) | (((1ULL << __exp_bits) - 1) << __frac_bits) |
             ((__exc == FLOAT_EXC_STD) ? 0ULL : ((1ULL << __frac_bits) - 1));
}

__float __float_divG(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                     FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   __bits8 a_c, b_c, z_c;
   _Bool a_c_zero, b_c_zero, a_c_inf, b_c_inf, a_c_nan, b_c_nan, a_c_normal, b_c_normal;
   __flag aSign, bSign, zSign;
   __int16 aExp, bExp, zExp;
   __bits64 aSig, bSig, aSigInitial, bSigInitial, zSig;
   __bits64 term0, term1;
   __bits64 c5, c4, c3, c2, c1, c0, shft, p1, bSigsqr1, bSigsqr2, p2, p3, p, ga0, gb0, ga1, gb1, rem0, rem, rnd;

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
   _Bool aExp_null = aExp == 0;
   _Bool bExp_null = bExp == 0;
   _Bool aExpMax = aExp == ((1ULL << __exp_bits) - 1);
   _Bool bExpMax = bExp == ((1ULL << __exp_bits) - 1);
   _Bool aSig_null = aSig == 0;
   _Bool bSig_null = bSig == 0;

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
         unsigned long long int subnormal_lz, mshifted;
         count_leading_zero_lshift_runtime_macro(__frac_bits, aSig, subnormal_lz, mshifted);
         aExp = -subnormal_lz;
         aSig = (mshifted << 1) & ((1ULL << __frac_bits) - 1);
      }
      if(bExp_null && !bSig_null)
      {
         unsigned long long int subnormal_lz, mshifted;
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
   else if(z_c == FP_CLS_ZERO)
      return ((__bits64)zSign) << ((__exp_bits + __frac_bits));
   else if(z_c == FP_CLS_NAN)
      return (((__bits64)(a_c_nan ? aSign : bSign) | (a_c_inf & b_c_inf) | (a_c_zero & b_c_zero))
              << (__exp_bits + __frac_bits)) |
             ((__exc == FLOAT_EXC_STD) ? ((((1ULL << __exp_bits) - 1) << __frac_bits) | (1ULL << (__frac_bits - 1)) |
                                          (a_c_nan ? aSigInitial : (b_c_nan ? bSigInitial : 0))) :
                                         (0ULL));
   else if(z_c == FP_CLS_INF)
      return (((__bits64)zSign) << (__exp_bits + __frac_bits)) | (((1ULL << __exp_bits) - 1) << __frac_bits) |
             ((__exc == FLOAT_EXC_STD) ? 0ULL : ((1ULL << __frac_bits) - 1));
}

/*----------------------------------------------------------------------------
| Returns the remainder of the double-precision floating-point value `a'
| with respect to the corresponding value `b'.  The operation is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/
///   UNUSED
//    __float64 __float64_rem_ieee(__float64 a, __float64 b)
//    {
//       __flag aSign, bSign, zSign;
//       __int16 aExp, bExp, expDiff;
//       __bits64 aSig, bSig;
//       __bits64 q, alternateASig;
//       __sbits64 sigMean;
//
//       aSig = __extractFloat64Frac(a, IEEE64_EXTRACT_FRAC);
//       aExp = __extractFloat64Exp(a, IEEE64_EXTRACT_EXP);
//       aSign = __extractFloat64Sign(a, IEEE64_EXTRACT_SIGN);
//       bSig = __extractFloat64Frac(b, IEEE64_EXTRACT_FRAC);
//       bExp = __extractFloat64Exp(b, IEEE64_EXTRACT_EXP);
//       bSign = __extractFloat64Sign(b, IEEE64_EXTRACT_SIGN);
//       if(aExp == 0x7FF)
//       {
//          if(aSig || ((bExp == 0x7FF) && bSig))
//          {
//             return __propagateFloat64NaN_ieee(a, b);
//          }
//          __float_raise(float_flag_invalid);
//          return __float64_default_nan;
//       }
//       if(bExp == 0x7FF)
//       {
//          if(bSig)
//             return __propagateFloat64NaN_ieee(a, b);
//          return a;
//       }
//       if(bExp == 0)
//       {
//          if(bSig == 0)
//          {
//             __float_raise(float_flag_invalid);
//             return __float64_default_nan;
//          }
//          __normalizeFloat64Subnormal(bSig, bExp, bSig, IEEE64_FRAC_BITS);
//       }
//       if(aExp == 0)
//       {
//          if(aSig == 0)
//             return a;
//          __normalizeFloat64Subnormal(aSig, aExp, aSig, IEEE64_FRAC_BITS);
//       }
//       expDiff = aExp - bExp;
//       aSig = (aSig | LIT64(0x0010000000000000)) << 11;
//       bSig = (bSig | LIT64(0x0010000000000000)) << 11;
//       if(expDiff < 0)
//       {
//          if(expDiff < -1)
//             return a;
//          aSig >>= 1;
//       }
//       q = (bSig <= aSig);
//       if(q)
//          aSig -= bSig;
//       expDiff -= 64;
//       while(0 < expDiff)
//       {
//          q = __estimateDiv128To64(aSig, 0, bSig);
//          q = (2 < q) ? q - 2 : 0;
//          aSig = -((bSig >> 2) * q);
//          expDiff -= 62;
//       }
//       expDiff += 64;
//       if(0 < expDiff)
//       {
//          q = __estimateDiv128To64(aSig, 0, bSig);
//          q = (2 < q) ? q - 2 : 0;
//          q >>= 64 - expDiff;
//          bSig >>= 2;
//          aSig = ((aSig >> 1) << (expDiff - 1)) - bSig * q;
//       }
//       else
//       {
//          aSig >>= 2;
//          bSig >>= 2;
//       }
//       do
//       {
//          alternateASig = aSig;
//          ++q;
//          aSig -= bSig;
//       } while(0 <= (__sbits64)aSig);
//       sigMean = aSig + alternateASig;
//       if((sigMean < 0) || ((sigMean == 0) && (q & 1)))
//       {
//          aSig = alternateASig;
//       }
//       zSign = ((__sbits64)aSig < 0);
//       if(zSign)
//          aSig = -aSig;
//       return __normalizeRoundAndPackFloat64_ieee(aSign ^ zSign, bExp, aSig);
//    }

/*----------------------------------------------------------------------------
| Returns the square root of the double-precision floating-point value `a'.
| The operation is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/
///   UNUSED
//    __float64 __float64_sqrt(__float64 a)
//    {
//       __flag aSign;
//       __int16 aExp, zExp;
//       __bits64 aSig, zSig, doubleZSig;
//       __bits64 rem0, rem1, term0, term1;
//       __float64 z;
//
//       aSig = __extractFloat64Frac(a, IEEE64_EXTRACT_FRAC);
//       aExp = __extractFloat64Exp(a, IEEE64_EXTRACT_EXP);
//       aSign = __extractFloat64Sign(a, IEEE64_EXTRACT_SIGN);
//       if(aExp == 0x7FF)
//       {
//          if(aSig)
//             return __propagateFloat64NaN_ieee(a, a);
//          if(!aSign)
//             return a;
//          __float_raise(float_flag_invalid);
//          return __float64_default_nan;
//       }
//       if(aSign)
//       {
//          if((aExp | aSig) == 0)
//             return a;
//          __float_raise(float_flag_invalid);
//          return __float64_default_nan;
//       }
//       if(aExp == 0)
//       {
//          if(aSig == 0)
//             return 0;
//          __normalizeFloat64Subnormal(aSig, aExp, aSig, IEEE64_FRAC_BITS);
//       }
//       zExp = ((aExp - 0x3FF) >> 1) + 0x3FE;
//       aSig |= LIT64(0x0010000000000000);
//       zSig = __estimateSqrt32(aExp, aSig >> 21);
//       aSig <<= 9 - (aExp & 1);
//       zSig = __estimateDiv128To64(aSig, 0, zSig << 32) + (zSig << 30);
//       if((zSig & 0x1FF) <= 5)
//       {
//          doubleZSig = zSig << 1;
//          __mul64To128(zSig, zSig, &term0, &term1);
//          __sub128(aSig, 0, term0, term1, &rem0, &rem1);
//          while((__sbits64)rem0 < 0)
//          {
//             --zSig;
//             doubleZSig -= 2;
//             __add128(rem0, rem1, zSig >> 63, doubleZSig | 1, &rem0, &rem1);
//          }
//          zSig |= ((rem0 | rem1) != 0);
//       }
//       return __roundAndPackFloat64(0, zExp, zSig, IEEE64_PACK, IEEE_EXC);
//    }

/*----------------------------------------------------------------------------
| Returns 1 if the double-precision floating-point value `a' is equal to the
| corresponding value `b', and 0 otherwise.  The comparison is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

#define BIT_MASK64(var, msbpp) (var & (((__bits64)0xFFFFFFFFFFFFFFFFULL) >> (64 - (msbpp))))

static __FORCE_INLINE __flag __FloatEQ(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits,
                                       __sbits32 __exp_bias, FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one,
                                       __flag __subnorm, __sbits8 __sign)
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

__flag __float_eq(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                  FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   return __FloatEQ(a, b, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}

__flag __float_ltgt_quiet(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                          FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   return !__FloatEQ(a, b, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}

/*----------------------------------------------------------------------------
| Returns 1 if the double-precision floating-point value `a' is less than or
| equal to the corresponding value `b', and 0 otherwise.  The comparison is
| performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag __FloatLE(__float64 a, __float64 b, __bits8 __exp_bits, __bits8 __frac_bits,
                                       FLOAT_EXC_TYPE __exc, __sbits8 __sign)
{
   __flag aSign, bSign;

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

__flag __float_le(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                  FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   return __FloatLE(a, b, __exp_bits, __frac_bits, __exc, __sign);
}

__flag __float_ge(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                  FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   return __FloatLE(b, a, __exp_bits, __frac_bits, __exc, __sign);
}

/*----------------------------------------------------------------------------
| Returns 1 if the double-precision floating-point value `a' is less than
| the corresponding value `b', and 0 otherwise.  The comparison is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag __FloatLT(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits,
                                       FLOAT_EXC_TYPE __exc, __sbits8 __sign)
{
   __flag aSign, bSign;

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

__flag __float_lt(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                  FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   return __FloatLT(a, b, __exp_bits, __frac_bits, __exc, __sign);
}

__flag __float_gt(__float a, __float b, __bits8 __exp_bits, __bits8 __frac_bits, __sbits32 __exp_bias,
                  FLOAT_RND_TYPE __rnd, FLOAT_EXC_TYPE __exc, __flag __one, __flag __subnorm, __sbits8 __sign)
{
   return __FloatLT(b, a, __exp_bits, __frac_bits, __exc, __sign);
}

/*----------------------------------------------------------------------------
| Returns 1 if the double-precision floating-point value `a' is equal to the
| corresponding value `b', and 0 otherwise.  The invalid exception is raised
| if either operand is a NaN.  Otherwise, the comparison is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__flag __float64_eq_signaling_ieee(__float64 a, __float64 b)
{
   if(((__extractFloat64Exp(a, IEEE64_EXTRACT_EXP) == 0x7FF) && __extractFloat64Frac(a, IEEE64_EXTRACT_FRAC)) ||
      ((__extractFloat64Exp(b, IEEE64_EXTRACT_EXP) == 0x7FF) && __extractFloat64Frac(b, IEEE64_EXTRACT_FRAC)))
   {
      __float_raise(float_flag_invalid);
      return 0;
   }
   return (a == b) || ((__bits64)((a | b) << 1) == 0);
}

/*----------------------------------------------------------------------------
| Returns 1 if the double-precision floating-point value `a' is less than or
| equal to the corresponding value `b', and 0 otherwise.  Quiet NaNs do not
| cause an exception.  Otherwise, the comparison is performed according to the
| IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__flag __float64_le_quiet_ieee(__float64 a, __float64 b)
{
   __flag aSign, bSign;
   __int16 aExp, bExp;

   if(((__extractFloat64Exp(a, IEEE64_EXTRACT_EXP) == 0x7FF) && __extractFloat64Frac(a, IEEE64_EXTRACT_FRAC)) ||
      ((__extractFloat64Exp(b, IEEE64_EXTRACT_EXP) == 0x7FF) && __extractFloat64Frac(b, IEEE64_EXTRACT_FRAC)))
   {
      if(__float_is_signaling_nan(a, IEEE64_SPEC_ARGS) || __float_is_signaling_nan(b, IEEE64_SPEC_ARGS))
      {
         __float_raise(float_flag_invalid);
      }
      return 0;
   }
   aSign = __extractFloat64Sign(a, IEEE64_EXTRACT_SIGN);
   bSign = __extractFloat64Sign(b, IEEE64_EXTRACT_SIGN);
   if(aSign != bSign)
      return aSign || ((__bits64)((a | b) << 1) == 0);
   return (a == b) || (aSign ^ (a < b));
}

/*----------------------------------------------------------------------------
| Returns 1 if the double-precision floating-point value `a' is less than
| the corresponding value `b', and 0 otherwise.  Quiet NaNs do not cause an
| exception.  Otherwise, the comparison is performed according to the IEC/IEEE
| Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__flag __float64_lt_quiet_ieee(__float64 a, __float64 b)
{
   __flag aSign, bSign;

   if(((__extractFloat64Exp(a, IEEE64_EXTRACT_EXP) == 0x7FF) && __extractFloat64Frac(a, IEEE64_EXTRACT_FRAC)) ||
      ((__extractFloat64Exp(b, IEEE64_EXTRACT_EXP) == 0x7FF) && __extractFloat64Frac(b, IEEE64_EXTRACT_FRAC)))
   {
      if(__float_is_signaling_nan(a, IEEE64_SPEC_ARGS) || __float_is_signaling_nan(b, IEEE64_SPEC_ARGS))
      {
         __float_raise(float_flag_invalid);
      }
      return 0;
   }
   aSign = __extractFloat64Sign(a, IEEE64_EXTRACT_SIGN);
   bSign = __extractFloat64Sign(b, IEEE64_EXTRACT_SIGN);
   if(aSign != bSign)
      return aSign && ((__bits64)((a | b) << 1) != 0);
   return (a != b) && (aSign ^ (a < b));
}

#ifdef FLOATX80

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point value `a' to the 32-bit two's complement integer format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic---which means in particular that the conversion
| is rounded according to the current rounding mode.  If `a' is a NaN, the
| largest positive integer is returned.  Otherwise, if the conversion
| overflows, the largest integer with the same sign as `a' is returned.
*----------------------------------------------------------------------------*/

__int32 __floatx80_to_int32_ieee(__floatx80 a)
{
   __flag aSign;
   __int32 aExp, shiftCount;
   __bits64 aSig;

   aSig = __extractFloatx80Frac(a);
   aExp = __extractFloatx80Exp(a);
   aSign = __extractFloatx80Sign(a);
   if((aExp == 0x7FFF) && (__bits64)(aSig << 1))
      aSign = 0;
   shiftCount = 0x4037 - aExp;
   if(shiftCount <= 0)
      shiftCount = 1;
   __shift64RightJamming(aSig, shiftCount, &aSig);
   return __roundAndPackInt32(aSign, aSig);
}

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point value `a' to the 32-bit two's complement integer format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic, except that the conversion is always rounded
| toward zero.  If `a' is a NaN, the largest positive integer is returned.
| Otherwise, if the conversion overflows, the largest integer with the same
| sign as `a' is returned.
*----------------------------------------------------------------------------*/

__int32 __floatx80_to_int32_round_to_zero_ieee(__floatx80 a)
{
   __flag aSign;
   __int32 aExp, shiftCount;
   __bits64 aSig, savedASig;
   __int32 z;

   aSig = __extractFloatx80Frac(a);
   aExp = __extractFloatx80Exp(a);
   aSign = __extractFloatx80Sign(a);
   if(0x401E < aExp)
   {
      if((aExp == 0x7FFF) && (__bits64)(aSig << 1))
         aSign = 0;
      goto invalid;
   }
   else if(aExp < 0x3FFF)
   {
      if(aExp || aSig)
         __float_exception_flags |= float_flag_inexact;
      return 0;
   }
   shiftCount = 0x403E - aExp;
   savedASig = aSig;
   aSig >>= shiftCount;
   z = aSig;
   if(aSign)
      z = -z;
   if((z < 0) ^ aSign)
   {
   invalid:
      __float_raise(float_flag_invalid);
      return aSign ? (__sbits32)0x80000000 : 0x7FFFFFFF;
   }
   if((aSig << shiftCount) != savedASig)
   {
      __float_exception_flags |= float_flag_inexact;
   }
   return z;
}

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point value `a' to the 64-bit two's complement integer format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic---which means in particular that the conversion
| is rounded according to the current rounding mode.  If `a' is a NaN,
| the largest positive integer is returned.  Otherwise, if the conversion
| overflows, the largest integer with the same sign as `a' is returned.
*----------------------------------------------------------------------------*/

__int64 __floatx80_to_int64_ieee(__floatx80 a)
{
   __flag aSign;
   __int32 aExp, shiftCount;
   __bits64 aSig, aSigExtra;

   aSig = __extractFloatx80Frac(a);
   aExp = __extractFloatx80Exp(a);
   aSign = __extractFloatx80Sign(a);
   shiftCount = 0x403E - aExp;
   if(shiftCount <= 0)
   {
      if(shiftCount)
      {
         __float_raise(float_flag_invalid);
         if(!aSign || ((aExp == 0x7FFF) && (aSig != LIT64(0x8000000000000000))))
         {
            return LIT64(0x7FFFFFFFFFFFFFFF);
         }
         return (__sbits64)LIT64(0x8000000000000000);
      }
      aSigExtra = 0;
   }
   else
   {
      __shift64ExtraRightJamming(aSig, 0, shiftCount, &aSig, &aSigExtra);
   }
   return __roundAndPackInt64(aSign, aSig, aSigExtra);
}

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point value `a' to the 64-bit two's complement integer format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic, except that the conversion is always rounded
| toward zero.  If `a' is a NaN, the largest positive integer is returned.
| Otherwise, if the conversion overflows, the largest integer with the same
| sign as `a' is returned.
*----------------------------------------------------------------------------*/

__int64 __floatx80_to_int64_round_to_zero_ieee(__floatx80 a)
{
   __flag aSign;
   __int32 aExp, shiftCount;
   __bits64 aSig;
   __int64 z;

   aSig = __extractFloatx80Frac(a);
   aExp = __extractFloatx80Exp(a);
   aSign = __extractFloatx80Sign(a);
   shiftCount = aExp - 0x403E;
   if(0 <= shiftCount)
   {
      aSig &= LIT64(0x7FFFFFFFFFFFFFFF);
      if((a.high != 0xC03E) || aSig)
      {
         __float_raise(float_flag_invalid);
         if(!aSign || ((aExp == 0x7FFF) && aSig))
         {
            return LIT64(0x7FFFFFFFFFFFFFFF);
         }
      }
      return (__sbits64)LIT64(0x8000000000000000);
   }
   else if(aExp < 0x3FFF)
   {
      if(aExp | aSig)
         __float_exception_flags |= float_flag_inexact;
      return 0;
   }
   z = aSig >> (-shiftCount);
   if((__bits64)(aSig << (shiftCount & 63)))
   {
      __float_exception_flags |= float_flag_inexact;
   }
   if(aSign)
      z = -z;
   return z;
}

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point value `a' to the single-precision floating-point format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__float32 __floatx80_to_float32_ieee(__floatx80 a)
{
   __flag aSign;
   __int32 aExp;
   __bits64 aSig;

   aSig = __extractFloatx80Frac(a);
   aExp = __extractFloatx80Exp(a);
   aSign = __extractFloatx80Sign(a);
   if(aExp == 0x7FFF)
   {
      if((__bits64)(aSig << 1))
      {
         return __commonNaNToFloat32(__floatx80ToCommonNaN(a));
      }
      return (((__bits32)aSign) << 31) | 0x7F800000; // __packFloat32(aSign, 0xFF, 0, IEEE32_PACK);
   }
   __shift64RightJamming(aSig, 33, &aSig);
   if(aExp || aSig)
      aExp -= 0x3F81;
   return __roundAndPackFloat32(aSign, aExp, aSig);
}

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point value `a' to the double-precision floating-point format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__float64 __floatx80_to_float64_ieee(__floatx80 a)
{
   __flag aSign;
   __int32 aExp;
   __bits64 aSig, zSig;

   aSig = __extractFloatx80Frac(a);
   aExp = __extractFloatx80Exp(a);
   aSign = __extractFloatx80Sign(a);
   if(aExp == 0x7FFF)
   {
      if((__bits64)(aSig << 1))
      {
         return __commonNaNToFloat64(__floatx80ToCommonNaN(a));
      }
      return (((__bits64)aSign) << 63) | LIT64(0x7FF0000000000000); // __packFloat64(aSign, 0x7FF, 0, IEEE64_PACK);
   }
   __shift64RightJamming(aSig, 1, &zSig);
   if(aExp || aSig)
      aExp -= 0x3C01;
   return __roundAndPackFloat64(aSign, aExp, zSig);
}

#ifdef FLOAT128

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point value `a' to the quadruple-precision floating-point format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__float128 __floatx80_to_float128_ieee(__floatx80 a)
{
   __flag aSign;
   __int16 aExp;
   __bits64 aSig, zSig0, zSig1;

   aSig = __extractFloatx80Frac(a);
   aExp = __extractFloatx80Exp(a);
   aSign = __extractFloatx80Sign(a);
   if((aExp == 0x7FFF) && (__bits64)(aSig << 1))
   {
      return __commonNaNToFloat128(__floatx80ToCommonNaN(a));
   }
   __shift128Right(aSig << 1, 0, 16, &zSig0, &zSig1);
   return __packFloat128(aSign, aExp, zSig0, zSig1);
}

#endif

/*----------------------------------------------------------------------------
| Rounds the extended double-precision floating-point value `a' to an integer,
| and returns the result as an extended quadruple-precision floating-point
| value.  The operation is performed according to the IEC/IEEE Standard for
| Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__floatx80 __floatx80_round_to_int_ieee(__floatx80 a)
{
   __flag aSign;
   __int32 aExp;
   __bits64 lastBitMask, roundBitsMask;
   __int8 roundingMode;
   __floatx80 z;

   aExp = __extractFloatx80Exp(a);
   if(0x403E <= aExp)
   {
      if((aExp == 0x7FFF) && (__bits64)(__extractFloatx80Frac(a) << 1))
      {
         return __propagateFloatx80NaN(a, a);
      }
      return a;
   }
   if(aExp < 0x3FFF)
   {
      if((aExp == 0) && ((__bits64)(__extractFloatx80Frac(a) << 1) == 0))
      {
         return a;
      }
      __float_exception_flags |= float_flag_inexact;
      aSign = __extractFloatx80Sign(a);
      switch(__float_rounding_mode)
      {
         case float_round_nearest_even:
            if((aExp == 0x3FFE) && (__bits64)(__extractFloatx80Frac(a) << 1))
            {
               return __packFloatx80(aSign, 0x3FFF, LIT64(0x8000000000000000));
            }
            break;
         case float_round_down:
            return aSign ? __packFloatx80(1, 0x3FFF, LIT64(0x8000000000000000)) : __packFloatx80(0, 0, 0);
         case float_round_up:
            return aSign ? __packFloatx80(1, 0, 0) : __packFloatx80(0, 0x3FFF, LIT64(0x8000000000000000));
      }
      return __packFloatx80(aSign, 0, 0);
   }
   lastBitMask = 1;
   lastBitMask <<= 0x403E - aExp;
   roundBitsMask = lastBitMask - 1;
   z = a;
   roundingMode = __float_rounding_mode;
   if(roundingMode == float_round_nearest_even)
   {
      z.low += lastBitMask >> 1;
      if((z.low & roundBitsMask) == 0)
         z.low &= ~lastBitMask;
   }
   else if(roundingMode != float_round_to_zero)
   {
      if(__extractFloatx80Sign(z) ^ (roundingMode == float_round_up))
      {
         z.low += roundBitsMask;
      }
   }
   z.low &= ~roundBitsMask;
   if(z.low == 0)
   {
      ++z.high;
      z.low = LIT64(0x8000000000000000);
   }
   if(z.low != a.low)
      __float_exception_flags |= float_flag_inexact;
   return z;
}

/*----------------------------------------------------------------------------
| Returns the result of adding the absolute values of the extended double-
| precision floating-point values `a' and `b'.  If `zSign' is 1, the sum is
| negated before being returned.  `zSign' is ignored if the result is a NaN.
| The addition is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __floatx80 __addFloatx80Sigs(__floatx80 a, __floatx80 b, __flag zSign)
{
   __int32 aExp, bExp, zExp;
   __bits64 aSig, bSig, zSig0, zSig1;
   __int32 expDiff;

   aSig = __extractFloatx80Frac(a);
   aExp = __extractFloatx80Exp(a);
   bSig = __extractFloatx80Frac(b);
   bExp = __extractFloatx80Exp(b);
   expDiff = aExp - bExp;
   if(0 < expDiff)
   {
      if(aExp == 0x7FFF)
      {
         if((__bits64)(aSig << 1))
            return __propagateFloatx80NaN(a, b);
         return a;
      }
      if(bExp == 0)
         --expDiff;
      __shift64ExtraRightJamming(bSig, 0, expDiff, &bSig, &zSig1);
      zExp = aExp;
   }
   else if(expDiff < 0)
   {
      if(bExp == 0x7FFF)
      {
         if((__bits64)(bSig << 1))
            return __propagateFloatx80NaN(a, b);
         return __packFloatx80(zSign, 0x7FFF, LIT64(0x8000000000000000));
      }
      if(aExp == 0)
         ++expDiff;
      __shift64ExtraRightJamming(aSig, 0, -expDiff, &aSig, &zSig1);
      zExp = bExp;
   }
   else
   {
      if(aExp == 0x7FFF)
      {
         if((__bits64)((aSig | bSig) << 1))
         {
            return __propagateFloatx80NaN(a, b);
         }
         return a;
      }
      zSig1 = 0;
      zSig0 = aSig + bSig;
      if(aExp == 0)
      {
         __normalizeFloatx80Subnormal(zSig0, &zExp, &zSig0);
         goto roundAndPack;
      }
      zExp = aExp;
      goto shiftRight1;
   }
   zSig0 = aSig + bSig;
   if((__sbits64)zSig0 < 0)
      goto roundAndPack;
shiftRight1:
   __shift64ExtraRightJamming(zSig0, zSig1, 1, &zSig0, &zSig1);
   zSig0 |= LIT64(0x8000000000000000);
   ++zExp;
roundAndPack:
   return __roundAndPackFloatx80(__floatx80_rounding_precision, zSign, zExp, zSig0, zSig1);
}

/*----------------------------------------------------------------------------
| Returns the result of subtracting the absolute values of the extended
| double-precision floating-point values `a' and `b'.  If `zSign' is 1, the
| difference is negated before being returned.  `zSign' is ignored if the
| result is a NaN.  The subtraction is performed according to the IEC/IEEE
| Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __floatx80 __subFloatx80Sigs(__floatx80 a, __floatx80 b, __flag zSign)
{
   __int32 aExp, bExp, zExp;
   __bits64 aSig, bSig, zSig0, zSig1;
   __int32 expDiff;
   __floatx80 z;

   aSig = __extractFloatx80Frac(a);
   aExp = __extractFloatx80Exp(a);
   bSig = __extractFloatx80Frac(b);
   bExp = __extractFloatx80Exp(b);
   expDiff = aExp - bExp;
   if(0 < expDiff)
      goto aExpBigger;
   if(expDiff < 0)
      goto bExpBigger;
   if(aExp == 0x7FFF)
   {
      if((__bits64)((aSig | bSig) << 1))
      {
         return __propagateFloatx80NaN(a, b);
      }
      __float_raise(float_flag_invalid);
      z.low = __floatx80_default_nan_low;
      z.high = __floatx80_default_nan_high;
      return z;
   }
   if(aExp == 0)
   {
      aExp = 1;
      bExp = 1;
   }
   zSig1 = 0;
   if(bSig < aSig)
      goto aBigger;
   if(aSig < bSig)
      goto bBigger;
   return __packFloatx80(__float_rounding_mode == float_round_down, 0, 0);
bExpBigger:
   if(bExp == 0x7FFF)
   {
      if((__bits64)(bSig << 1))
         return __propagateFloatx80NaN(a, b);
      return __packFloatx80(zSign ^ 1, 0x7FFF, LIT64(0x8000000000000000));
   }
   if(aExp == 0)
      ++expDiff;
   __shift128RightJamming(aSig, 0, -expDiff, &aSig, &zSig1);
bBigger:
   __sub128(bSig, 0, aSig, zSig1, &zSig0, &zSig1);
   zExp = bExp;
   zSign ^= 1;
   goto normalizeRoundAndPack;
aExpBigger:
   if(aExp == 0x7FFF)
   {
      if((__bits64)(aSig << 1))
         return __propagateFloatx80NaN(a, b);
      return a;
   }
   if(bExp == 0)
      --expDiff;
   __shift128RightJamming(bSig, 0, expDiff, &bSig, &zSig1);
aBigger:
   __sub128(aSig, 0, bSig, zSig1, &zSig0, &zSig1);
   zExp = aExp;
normalizeRoundAndPack:
   return __normalizeRoundAndPackFloatx80(__floatx80_rounding_precision, zSign, zExp, zSig0, zSig1);
}

/*----------------------------------------------------------------------------
| Returns the result of adding the extended double-precision floating-point
| values `a' and `b'.  The operation is performed according to the IEC/IEEE
| Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__floatx80 floatx80_add_ieee(__floatx80 a, __floatx80 b)
{
   __flag aSign, bSign;

   aSign = __extractFloatx80Sign(a);
   bSign = __extractFloatx80Sign(b);
   if(aSign == bSign)
   {
      return __addFloatx80Sigs(a, b, aSign);
   }
   else
   {
      return __subFloatx80Sigs(a, b, aSign);
   }
}

/*----------------------------------------------------------------------------
| Returns the result of subtracting the extended double-precision floating-
| point values `a' and `b'.  The operation is performed according to the
| IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__floatx80 __floatx80_sub_ieee(__floatx80 a, __floatx80 b)
{
   __flag aSign, bSign;

   aSign = __extractFloatx80Sign(a);
   bSign = __extractFloatx80Sign(b);
   if(aSign == bSign)
   {
      return __subFloatx80Sigs(a, b, aSign);
   }
   else
   {
      return __addFloatx80Sigs(a, b, aSign);
   }
}

/*----------------------------------------------------------------------------
| Returns the result of multiplying the extended double-precision floating-
| point values `a' and `b'.  The operation is performed according to the
| IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__floatx80 __floatx80_mul_ieee(__floatx80 a, __floatx80 b)
{
   __flag aSign, bSign, zSign;
   __int32 aExp, bExp, zExp;
   __bits64 aSig, bSig, zSig0, zSig1;
   __floatx80 z;

   aSig = __extractFloatx80Frac(a);
   aExp = __extractFloatx80Exp(a);
   aSign = __extractFloatx80Sign(a);
   bSig = __extractFloatx80Frac(b);
   bExp = __extractFloatx80Exp(b);
   bSign = __extractFloatx80Sign(b);
   zSign = aSign ^ bSign;
   if(aExp == 0x7FFF)
   {
      if((__bits64)(aSig << 1) || ((bExp == 0x7FFF) && (__bits64)(bSig << 1)))
      {
         return __propagateFloatx80NaN(a, b);
      }
      if((bExp | bSig) == 0)
         goto invalid;
      return __packFloatx80(zSign, 0x7FFF, LIT64(0x8000000000000000));
   }
   if(bExp == 0x7FFF)
   {
      if((__bits64)(bSig << 1))
         return __propagateFloatx80NaN(a, b);
      if((aExp | aSig) == 0)
      {
      invalid:
         __float_raise(float_flag_invalid);
         z.low = __floatx80_default_nan_low;
         z.high = __floatx80_default_nan_high;
         return z;
      }
      return __packFloatx80(zSign, 0x7FFF, LIT64(0x8000000000000000));
   }
   if(aExp == 0)
   {
      if(aSig == 0)
         return __packFloatx80(zSign, 0, 0);
      __normalizeFloatx80Subnormal(aSig, &aExp, &aSig);
   }
   if(bExp == 0)
   {
      if(bSig == 0)
         return __packFloatx80(zSign, 0, 0);
      __normalizeFloatx80Subnormal(bSig, &bExp, &bSig);
   }
   zExp = aExp + bExp - 0x3FFE;
   __mul64To128(aSig, bSig, &zSig0, &zSig1);
   if(0 < (__sbits64)zSig0)
   {
      __shortShift128Left(zSig0, zSig1, 1, &zSig0, &zSig1);
      --zExp;
   }
   return __roundAndPackFloatx80(__floatx80_rounding_precision, zSign, zExp, zSig0, zSig1);
}

/*----------------------------------------------------------------------------
| Returns the result of dividing the extended double-precision floating-point
| value `a' by the corresponding value `b'.  The operation is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__floatx80 __floatx80_div_ieee(__floatx80 a, __floatx80 b)
{
   __flag aSign, bSign, zSign;
   __int32 aExp, bExp, zExp;
   __bits64 aSig, bSig, zSig0, zSig1;
   __bits64 rem0, rem1, rem2, term0, term1, term2;
   __floatx80 z;

   aSig = __extractFloatx80Frac(a);
   aExp = __extractFloatx80Exp(a);
   aSign = __extractFloatx80Sign(a);
   bSig = __extractFloatx80Frac(b);
   bExp = __extractFloatx80Exp(b);
   bSign = __extractFloatx80Sign(b);
   zSign = aSign ^ bSign;
   if(aExp == 0x7FFF)
   {
      if((__bits64)(aSig << 1))
         return __propagateFloatx80NaN(a, b);
      if(bExp == 0x7FFF)
      {
         if((__bits64)(bSig << 1))
            return __propagateFloatx80NaN(a, b);
         goto invalid;
      }
      return __packFloatx80(zSign, 0x7FFF, LIT64(0x8000000000000000));
   }
   if(bExp == 0x7FFF)
   {
      if((__bits64)(bSig << 1))
         return __propagateFloatx80NaN(a, b);
      return __packFloatx80(zSign, 0, 0);
   }
   if(bExp == 0)
   {
      if(bSig == 0)
      {
         if((aExp | aSig) == 0)
         {
         invalid:
            __float_raise(float_flag_invalid);
            z.low = __floatx80_default_nan_low;
            z.high = __floatx80_default_nan_high;
            return z;
         }
         __float_raise(float_flag_divbyzero);
         return __packFloatx80(zSign, 0x7FFF, LIT64(0x8000000000000000));
      }
      __normalizeFloatx80Subnormal(bSig, &bExp, &bSig);
   }
   if(aExp == 0)
   {
      if(aSig == 0)
         return __packFloatx80(zSign, 0, 0);
      __normalizeFloatx80Subnormal(aSig, &aExp, &aSig);
   }
   zExp = aExp - bExp + 0x3FFE;
   rem1 = 0;
   if(bSig <= aSig)
   {
      __shift128Right(aSig, 0, 1, &aSig, &rem1);
      ++zExp;
   }
   zSig0 = __estimateDiv128To64(aSig, rem1, bSig);
   __mul64To128(bSig, zSig0, &term0, &term1);
   __sub128(aSig, rem1, term0, term1, &rem0, &rem1);
   while((__sbits64)rem0 < 0)
   {
      --zSig0;
      __add128(rem0, rem1, 0, bSig, &rem0, &rem1);
   }
   zSig1 = __estimateDiv128To64(rem1, 0, bSig);
   if((__bits64)(zSig1 << 1) <= 8)
   {
      __mul64To128(bSig, zSig1, &term1, &term2);
      __sub128(rem1, 0, term1, term2, &rem1, &rem2);
      while((__sbits64)rem1 < 0)
      {
         --zSig1;
         __add128(rem1, rem2, 0, bSig, &rem1, &rem2);
      }
      zSig1 |= ((rem1 | rem2) != 0);
   }
   return __roundAndPackFloatx80(__floatx80_rounding_precision, zSign, zExp, zSig0, zSig1);
}

/*----------------------------------------------------------------------------
| Returns the remainder of the extended double-precision floating-point value
| `a' with respect to the corresponding value `b'.  The operation is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__floatx80 __floatx80_rem_ieee(__floatx80 a, __floatx80 b)
{
   __flag aSign, bSign, zSign;
   __int32 aExp, bExp, expDiff;
   __bits64 aSig0, aSig1, bSig;
   __bits64 q, term0, term1, alternateASig0, alternateASig1;
   __floatx80 z;

   aSig0 = __extractFloatx80Frac(a);
   aExp = __extractFloatx80Exp(a);
   aSign = __extractFloatx80Sign(a);
   bSig = __extractFloatx80Frac(b);
   bExp = __extractFloatx80Exp(b);
   bSign = __extractFloatx80Sign(b);
   if(aExp == 0x7FFF)
   {
      if((__bits64)(aSig0 << 1) || ((bExp == 0x7FFF) && (__bits64)(bSig << 1)))
      {
         return __propagateFloatx80NaN(a, b);
      }
      goto invalid;
   }
   if(bExp == 0x7FFF)
   {
      if((__bits64)(bSig << 1))
         return __propagateFloatx80NaN(a, b);
      return a;
   }
   if(bExp == 0)
   {
      if(bSig == 0)
      {
      invalid:
         __float_raise(float_flag_invalid);
         z.low = __floatx80_default_nan_low;
         z.high = __floatx80_default_nan_high;
         return z;
      }
      __normalizeFloatx80Subnormal(bSig, &bExp, &bSig);
   }
   if(aExp == 0)
   {
      if((__bits64)(aSig0 << 1) == 0)
         return a;
      __normalizeFloatx80Subnormal(aSig0, &aExp, &aSig0);
   }
   bSig |= LIT64(0x8000000000000000);
   zSign = aSign;
   expDiff = aExp - bExp;
   aSig1 = 0;
   if(expDiff < 0)
   {
      if(expDiff < -1)
         return a;
      __shift128Right(aSig0, 0, 1, &aSig0, &aSig1);
      expDiff = 0;
   }
   q = (bSig <= aSig0);
   if(q)
      aSig0 -= bSig;
   expDiff -= 64;
   while(0 < expDiff)
   {
      q = __estimateDiv128To64(aSig0, aSig1, bSig);
      q = (2 < q) ? q - 2 : 0;
      __mul64To128(bSig, q, &term0, &term1);
      __sub128(aSig0, aSig1, term0, term1, &aSig0, &aSig1);
      __shortShift128Left(aSig0, aSig1, 62, &aSig0, &aSig1);
      expDiff -= 62;
   }
   expDiff += 64;
   if(0 < expDiff)
   {
      q = __estimateDiv128To64(aSig0, aSig1, bSig);
      q = (2 < q) ? q - 2 : 0;
      q >>= 64 - expDiff;
      __mul64To128(bSig, q << (64 - expDiff), &term0, &term1);
      __sub128(aSig0, aSig1, term0, term1, &aSig0, &aSig1);
      __shortShift128Left(0, bSig, 64 - expDiff, &term0, &term1);
      while(__le128(term0, term1, aSig0, aSig1))
      {
         ++q;
         __sub128(aSig0, aSig1, term0, term1, &aSig0, &aSig1);
      }
   }
   else
   {
      term1 = 0;
      term0 = bSig;
   }
   __sub128(term0, term1, aSig0, aSig1, &alternateASig0, &alternateASig1);
   if(__lt128(alternateASig0, alternateASig1, aSig0, aSig1) ||
      (__eq128(alternateASig0, alternateASig1, aSig0, aSig1) && (q & 1)))
   {
      aSig0 = alternateASig0;
      aSig1 = alternateASig1;
      zSign = !zSign;
   }
   return __normalizeRoundAndPackFloatx80(80, zSign, bExp + expDiff, aSig0, aSig1);
}

/*----------------------------------------------------------------------------
| Returns the square root of the extended double-precision floating-point
| value `a'.  The operation is performed according to the IEC/IEEE Standard
| for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__floatx80 __floatx80_sqrt(__floatx80 a)
{
   __flag aSign;
   __int32 aExp, zExp;
   __bits64 aSig0, aSig1, zSig0, zSig1, doubleZSig0;
   __bits64 rem0, rem1, rem2, rem3, term0, term1, term2, term3;
   __floatx80 z;

   aSig0 = __extractFloatx80Frac(a);
   aExp = __extractFloatx80Exp(a);
   aSign = __extractFloatx80Sign(a);
   if(aExp == 0x7FFF)
   {
      if((__bits64)(aSig0 << 1))
         return __propagateFloatx80NaN(a, a);
      if(!aSign)
         return a;
      goto invalid;
   }
   if(aSign)
   {
      if((aExp | aSig0) == 0)
         return a;
   invalid:
      __float_raise(float_flag_invalid);
      z.low = __floatx80_default_nan_low;
      z.high = __floatx80_default_nan_high;
      return z;
   }
   if(aExp == 0)
   {
      if(aSig0 == 0)
         return __packFloatx80(0, 0, 0);
      __normalizeFloatx80Subnormal(aSig0, &aExp, &aSig0);
   }
   zExp = ((aExp - 0x3FFF) >> 1) + 0x3FFF;
   zSig0 = __estimateSqrt32(aExp, aSig0 >> 32);
   __shift128Right(aSig0, 0, 2 + (aExp & 1), &aSig0, &aSig1);
   zSig0 = __estimateDiv128To64(aSig0, aSig1, zSig0 << 32) + (zSig0 << 30);
   doubleZSig0 = zSig0 << 1;
   __mul64To128(zSig0, zSig0, &term0, &term1);
   __sub128(aSig0, aSig1, term0, term1, &rem0, &rem1);
   while((__sbits64)rem0 < 0)
   {
      --zSig0;
      doubleZSig0 -= 2;
      __add128(rem0, rem1, zSig0 >> 63, doubleZSig0 | 1, &rem0, &rem1);
   }
   zSig1 = __estimateDiv128To64(rem1, 0, doubleZSig0);
   if((zSig1 & LIT64(0x3FFFFFFFFFFFFFFF)) <= 5)
   {
      if(zSig1 == 0)
         zSig1 = 1;
      __mul64To128(doubleZSig0, zSig1, &term1, &term2);
      __sub128(rem1, 0, term1, term2, &rem1, &rem2);
      __mul64To128(zSig1, zSig1, &term2, &term3);
      __sub192(rem1, rem2, 0, 0, term2, term3, &rem1, &rem2, &rem3);
      while((__sbits64)rem1 < 0)
      {
         --zSig1;
         __shortShift128Left(0, zSig1, 1, &term2, &term3);
         term3 |= 1;
         term2 |= doubleZSig0;
         __add192(rem1, rem2, rem3, 0, term2, term3, &rem1, &rem2, &rem3);
      }
      zSig1 |= ((rem1 | rem2 | rem3) != 0);
   }
   __shortShift128Left(0, zSig1, 1, &zSig0, &zSig1);
   zSig0 |= doubleZSig0;
   return __roundAndPackFloatx80(__floatx80_rounding_precision, 0, zExp, zSig0, zSig1);
}

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is
| equal to the corresponding value `b', and 0 otherwise.  The comparison is
| performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

__flag __floatx80_eq_ieee(__floatx80 a, __floatx80 b)
{
   if(((__extractFloatx80Exp(a) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(a) << 1)) ||
      ((__extractFloatx80Exp(b) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(b) << 1)))
   {
      if(__floatx80_is_signaling_nan(a) || __floatx80_is_signaling_nan(b))
      {
         __float_raise(float_flag_invalid);
      }
      return 0;
   }
   return (a.low == b.low) && ((a.high == b.high) || ((a.low == 0) && ((__bits16)((a.high | b.high) << 1) == 0)));
}

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is
| less than or equal to the corresponding value `b', and 0 otherwise.  The
| comparison is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag _Floatx80LE(__floatx80 a, __floatx80 b)
{
   __flag aSign, bSign;

   if(((__extractFloatx80Exp(a) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(a) << 1)) ||
      ((__extractFloatx80Exp(b) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(b) << 1)))
   {
      __float_raise(float_flag_invalid);
      return 0;
   }
   aSign = __extractFloatx80Sign(a);
   bSign = __extractFloatx80Sign(b);
   if(aSign != bSign)
   {
      return aSign || ((((__bits16)((a.high | b.high) << 1)) | a.low | b.low) == 0);
   }
   return aSign ? __le128(b.high, b.low, a.high, a.low) : __le128(a.high, a.low, b.high, b.low);
}

__flag __floatx80_le_ieee(__floatx80 a, __floatx80 b)
{
   return _Floatx80LE(a, b);
}

__flag __floatx80_ge_ieee(__floatx80 a, __floatx80 b)
{
   return _Floatx80LE(b, a);
}

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is
| less than the corresponding value `b', and 0 otherwise.  The comparison
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag __Floatx80LT(__floatx80 a, __floatx80 b)
{
   __flag aSign, bSign;

   if(((__extractFloatx80Exp(a) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(a) << 1)) ||
      ((__extractFloatx80Exp(b) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(b) << 1)))
   {
      __float_raise(float_flag_invalid);
      return 0;
   }
   aSign = __extractFloatx80Sign(a);
   bSign = __extractFloatx80Sign(b);
   if(aSign != bSign)
   {
      return aSign && ((((__bits16)((a.high | b.high) << 1)) | a.low | b.low) != 0);
   }
   return aSign ? __lt128(b.high, b.low, a.high, a.low) : __lt128(a.high, a.low, b.high, b.low);
}

__flag __floatx80_lt_ieee(__floatx80, __floatx80)
{
   return __Floatx80LT(a, b);
}

__flag __floatx80_gt_ieee(__floatx80 a, __floatx80 b)
{
   return __Floatx80LT(b, a);
}

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is equal
| to the corresponding value `b', and 0 otherwise.  The invalid exception is
| raised if either operand is a NaN.  Otherwise, the comparison is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__flag __floatx80_eq_signaling_ieee(__floatx80 a, __floatx80 b)
{
   if(((__extractFloatx80Exp(a) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(a) << 1)) ||
      ((__extractFloatx80Exp(b) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(b) << 1)))
   {
      __float_raise(float_flag_invalid);
      return 0;
   }
   return (a.low == b.low) && ((a.high == b.high) || ((a.low == 0) && ((__bits16)((a.high | b.high) << 1) == 0)));
}

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is less
| than or equal to the corresponding value `b', and 0 otherwise.  Quiet NaNs
| do not cause an exception.  Otherwise, the comparison is performed according
| to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__flag __floatx80_le_quiet_ieee(__floatx80 a, __floatx80 b)
{
   __flag aSign, bSign;

   if(((__extractFloatx80Exp(a) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(a) << 1)) ||
      ((__extractFloatx80Exp(b) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(b) << 1)))
   {
      if(__floatx80_is_signaling_nan(a) || __floatx80_is_signaling_nan(b))
      {
         __float_raise(float_flag_invalid);
      }
      return 0;
   }
   aSign = __extractFloatx80Sign(a);
   bSign = __extractFloatx80Sign(b);
   if(aSign != bSign)
   {
      return aSign || ((((__bits16)((a.high | b.high) << 1)) | a.low | b.low) == 0);
   }
   return aSign ? __le128(b.high, b.low, a.high, a.low) : __le128(a.high, a.low, b.high, b.low);
}

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is less
| than the corresponding value `b', and 0 otherwise.  Quiet NaNs do not cause
| an exception.  Otherwise, the comparison is performed according to the
| IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__flag __floatx80_lt_quiet_ieee(__floatx80 a, __floatx80 b)
{
   __flag aSign, bSign;

   if(((__extractFloatx80Exp(a) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(a) << 1)) ||
      ((__extractFloatx80Exp(b) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(b) << 1)))
   {
      if(__floatx80_is_signaling_nan(a) || __floatx80_is_signaling_nan(b))
      {
         __float_raise(float_flag_invalid);
      }
      return 0;
   }
   aSign = __extractFloatx80Sign(a);
   bSign = __extractFloatx80Sign(b);
   if(aSign != bSign)
   {
      return aSign && ((((__bits16)((a.high | b.high) << 1)) | a.low | b.low) != 0);
   }
   return aSign ? __lt128(b.high, b.low, a.high, a.low) : __lt128(a.high, a.low, b.high, b.low);
}

#endif

#ifdef FLOAT128

/*----------------------------------------------------------------------------
| Returns the result of converting the quadruple-precision floating-point
| value `a' to the 32-bit two's complement integer format.  The conversion
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic---which means in particular that the conversion is rounded
| according to the current rounding mode.  If `a' is a NaN, the largest
| positive integer is returned.  Otherwise, if the conversion overflows, the
| largest integer with the same sign as `a' is returned.
*----------------------------------------------------------------------------*/

__int32 __float128_to_int32_ieee(__float128 a)
{
   __flag aSign;
   __int32 aExp, shiftCount;
   __bits64 aSig0, aSig1;

   aSig1 = __extractFloat128Frac1(a);
   aSig0 = __extractFloat128Frac0(a);
   aExp = __extractFloat128Exp(a);
   aSign = __extractFloat128Sign(a);
   if((aExp == 0x7FFF) && (aSig0 | aSig1))
      aSign = 0;
   if(aExp)
      aSig0 |= LIT64(0x0001000000000000);
   aSig0 |= (aSig1 != 0);
   shiftCount = 0x4028 - aExp;
   if(0 < shiftCount)
      __shift64RightJamming(aSig0, shiftCount, &aSig0);
   return __roundAndPackInt32(aSign, aSig0);
}

/*----------------------------------------------------------------------------
| Returns the result of converting the quadruple-precision floating-point
| value `a' to the 32-bit two's complement integer format.  The conversion
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic, except that the conversion is always rounded toward zero.  If
| `a' is a NaN, the largest positive integer is returned.  Otherwise, if the
| conversion overflows, the largest integer with the same sign as `a' is
| returned.
*----------------------------------------------------------------------------*/

__int32 __float128_to_int32_round_to_zero_ieee(__float128 a)
{
   __flag aSign;
   __int32 aExp, shiftCount;
   __bits64 aSig0, aSig1, savedASig;
   __int32 z;

   aSig1 = __extractFloat128Frac1(a);
   aSig0 = __extractFloat128Frac0(a);
   aExp = __extractFloat128Exp(a);
   aSign = __extractFloat128Sign(a);
   aSig0 |= (aSig1 != 0);
   if(0x401E < aExp)
   {
      if((aExp == 0x7FFF) && aSig0)
         aSign = 0;
      goto invalid;
   }
   else if(aExp < 0x3FFF)
   {
      if(aExp || aSig0)
         __float_exception_flags |= float_flag_inexact;
      return 0;
   }
   aSig0 |= LIT64(0x0001000000000000);
   shiftCount = 0x402F - aExp;
   savedASig = aSig0;
   aSig0 >>= shiftCount;
   z = aSig0;
   if(aSign)
      z = -z;
   if((z < 0) ^ aSign)
   {
   invalid:
      __float_raise(float_flag_invalid);
      return aSign ? (__sbits32)0x80000000 : 0x7FFFFFFF;
   }
   if((aSig0 << shiftCount) != savedASig)
   {
      __float_exception_flags |= float_flag_inexact;
   }
   return z;
}

/*----------------------------------------------------------------------------
| Returns the result of converting the quadruple-precision floating-point
| value `a' to the 64-bit two's complement integer format.  The conversion
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic---which means in particular that the conversion is rounded
| according to the current rounding mode.  If `a' is a NaN, the largest
| positive integer is returned.  Otherwise, if the conversion overflows, the
| largest integer with the same sign as `a' is returned.
*----------------------------------------------------------------------------*/

__int64 __float128_to_int64_ieee(__float128 a)
{
   __flag aSign;
   __int32 aExp, shiftCount;
   __bits64 aSig0, aSig1;

   aSig1 = __extractFloat128Frac1(a);
   aSig0 = __extractFloat128Frac0(a);
   aExp = __extractFloat128Exp(a);
   aSign = __extractFloat128Sign(a);
   if(aExp)
      aSig0 |= LIT64(0x0001000000000000);
   shiftCount = 0x402F - aExp;
   if(shiftCount <= 0)
   {
      if(0x403E < aExp)
      {
         __float_raise(float_flag_invalid);
         if(!aSign || ((aExp == 0x7FFF) && (aSig1 || (aSig0 != LIT64(0x0001000000000000)))))
         {
            return LIT64(0x7FFFFFFFFFFFFFFF);
         }
         return (__sbits64)LIT64(0x8000000000000000);
      }
      __shortShift128Left(aSig0, aSig1, -shiftCount, &aSig0, &aSig1);
   }
   else
   {
      __shift64ExtraRightJamming(aSig0, aSig1, shiftCount, &aSig0, &aSig1);
   }
   return __roundAndPackInt64(aSign, aSig0, aSig1);
}

/*----------------------------------------------------------------------------
| Returns the result of converting the quadruple-precision floating-point
| value `a' to the 64-bit two's complement integer format.  The conversion
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic, except that the conversion is always rounded toward zero.
| If `a' is a NaN, the largest positive integer is returned.  Otherwise, if
| the conversion overflows, the largest integer with the same sign as `a' is
| returned.
*----------------------------------------------------------------------------*/

__int64 __float128_to_int64_round_to_zero_ieee(__float128 a)
{
   __flag aSign;
   __int32 aExp, shiftCount;
   __bits64 aSig0, aSig1;
   __int64 z;

   aSig1 = __extractFloat128Frac1(a);
   aSig0 = __extractFloat128Frac0(a);
   aExp = __extractFloat128Exp(a);
   aSign = __extractFloat128Sign(a);
   if(aExp)
      aSig0 |= LIT64(0x0001000000000000);
   shiftCount = aExp - 0x402F;
   if(0 < shiftCount)
   {
      if(0x403E <= aExp)
      {
         aSig0 &= LIT64(0x0000FFFFFFFFFFFF);
         if((a.high == LIT64(0xC03E000000000000)) && (aSig1 < LIT64(0x0002000000000000)))
         {
            if(aSig1)
               __float_exception_flags |= float_flag_inexact;
         }
         else
         {
            __float_raise(float_flag_invalid);
            if(!aSign || ((aExp == 0x7FFF) && (aSig0 | aSig1)))
            {
               return LIT64(0x7FFFFFFFFFFFFFFF);
            }
         }
         return (__sbits64)LIT64(0x8000000000000000);
      }
      z = (aSig0 << shiftCount) | (aSig1 >> ((-shiftCount) & 63));
      if((__bits64)(aSig1 << shiftCount))
      {
         __float_exception_flags |= float_flag_inexact;
      }
   }
   else
   {
      if(aExp < 0x3FFF)
      {
         if(aExp | aSig0 | aSig1)
         {
            __float_exception_flags |= float_flag_inexact;
         }
         return 0;
      }
      z = aSig0 >> (-shiftCount);
      if(aSig1 || (shiftCount && (__bits64)(aSig0 << (shiftCount & 63))))
      {
         __float_exception_flags |= float_flag_inexact;
      }
   }
   if(aSign)
      z = -z;
   return z;
}

/*----------------------------------------------------------------------------
| Returns the result of converting the quadruple-precision floating-point
| value `a' to the single-precision floating-point format.  The conversion
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

__float32 __float128_to_float32_ieee(__float128 a)
{
   __flag aSign;
   __int32 aExp;
   __bits64 aSig0, aSig1;
   __bits32 zSig;

   aSig1 = __extractFloat128Frac1(a);
   aSig0 = __extractFloat128Frac0(a);
   aExp = __extractFloat128Exp(a);
   aSign = __extractFloat128Sign(a);
   if(aExp == 0x7FFF)
   {
      if(aSig0 | aSig1)
      {
         return __commonNaNToFloat32(__float128ToCommonNaN(a));
      }
      return (((__bits32)aSign) << 31) | 0x7F800000; // __packFloat32(aSign, 0xFF, 0, IEEE32_PACK);
   }
   aSig0 |= (aSig1 != 0);
   __shift64RightJamming(aSig0, 18, &aSig0);
   zSig = aSig0;
   if(aExp || zSig)
   {
      zSig |= 0x40000000;
      aExp -= 0x3F81;
   }
   return __roundAndPackFloat32(aSign, aExp, zSig);
}

/*----------------------------------------------------------------------------
| Returns the result of converting the quadruple-precision floating-point
| value `a' to the double-precision floating-point format.  The conversion
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

__float64 __float128_to_float64_ieee(__float128 a)
{
   __flag aSign;
   __int32 aExp;
   __bits64 aSig0, aSig1;

   aSig1 = __extractFloat128Frac1(a);
   aSig0 = __extractFloat128Frac0(a);
   aExp = __extractFloat128Exp(a);
   aSign = __extractFloat128Sign(a);
   if(aExp == 0x7FFF)
   {
      if(aSig0 | aSig1)
      {
         return __commonNaNToFloat64(__float128ToCommonNaN(a));
      }
      return (((__bits64)aSign) << 63) | LIT64(0x7FF0000000000000); //  __packFloat64(aSign, 0x7FF, 0, IEEE64_PACK);
   }
   __shortShift128Left(aSig0, aSig1, 14, &aSig0, &aSig1);
   aSig0 |= (aSig1 != 0);
   if(aExp || aSig0)
   {
      aSig0 |= LIT64(0x4000000000000000);
      aExp -= 0x3C01;
   }
   return __roundAndPackFloat64(aSign, aExp, aSig0);
}

#ifdef FLOATX80

/*----------------------------------------------------------------------------
| Returns the result of converting the quadruple-precision floating-point
| value `a' to the extended double-precision floating-point format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__floatx80 __float128_to_floatx80_ieee(__float128 a)
{
   __flag aSign;
   __int32 aExp;
   __bits64 aSig0, aSig1;

   aSig1 = __extractFloat128Frac1(a);
   aSig0 = __extractFloat128Frac0(a);
   aExp = __extractFloat128Exp(a);
   aSign = __extractFloat128Sign(a);
   if(aExp == 0x7FFF)
   {
      if(aSig0 | aSig1)
      {
         return __commonNaNToFloatx80(__float128ToCommonNaN(a));
      }
      return __packFloatx80(aSign, 0x7FFF, LIT64(0x8000000000000000));
   }
   if(aExp == 0)
   {
      if((aSig0 | aSig1) == 0)
         return __packFloatx80(aSign, 0, 0);
      __normalizeFloat128Subnormal(aSig0, aSig1, &aExp, &aSig0, &aSig1);
   }
   else
   {
      aSig0 |= LIT64(0x0001000000000000);
   }
   __shortShift128Left(aSig0, aSig1, 15, &aSig0, &aSig1);
   return __roundAndPackFloatx80(80, aSign, aExp, aSig0, aSig1);
}

#endif

/*----------------------------------------------------------------------------
| Rounds the quadruple-precision floating-point value `a' to an integer, and
| returns the result as a quadruple-precision floating-point value.  The
| operation is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__float128 __float128_round_to_int_ieee(__float128 a)
{
   __flag aSign;
   __int32 aExp;
   __bits64 lastBitMask, roundBitsMask;
   __int8 roundingMode;
   __float128 z;

   aExp = __extractFloat128Exp(a);
   if(0x402F <= aExp)
   {
      if(0x406F <= aExp)
      {
         if((aExp == 0x7FFF) && (__extractFloat128Frac0(a) | __extractFloat128Frac1(a)))
         {
            return __propagateFloat128NaN(a, a);
         }
         return a;
      }
      lastBitMask = 1;
      lastBitMask = (lastBitMask << (0x406E - aExp)) << 1;
      roundBitsMask = lastBitMask - 1;
      z = a;
      roundingMode = __float_rounding_mode;
      if(roundingMode == float_round_nearest_even)
      {
         if(lastBitMask)
         {
            __add128(z.high, z.low, 0, lastBitMask >> 1, &z.high, &z.low);
            if((z.low & roundBitsMask) == 0)
               z.low &= ~lastBitMask;
         }
         else
         {
            if((__sbits64)z.low < 0)
            {
               ++z.high;
               if((__bits64)(z.low << 1) == 0)
                  z.high &= ~1;
            }
         }
      }
      else if(roundingMode != float_round_to_zero)
      {
         if(__extractFloat128Sign(z) ^ (roundingMode == float_round_up))
         {
            __add128(z.high, z.low, 0, roundBitsMask, &z.high, &z.low);
         }
      }
      z.low &= ~roundBitsMask;
   }
   else
   {
      if(aExp < 0x3FFF)
      {
         if((((__bits64)(a.high << 1)) | a.low) == 0)
            return a;
         __float_exception_flags |= float_flag_inexact;
         aSign = __extractFloat128Sign(a);
         switch(__float_rounding_mode)
         {
            case float_round_nearest_even:
               if((aExp == 0x3FFE) && (__extractFloat128Frac0(a) | __extractFloat128Frac1(a)))
               {
                  return __packFloat128(aSign, 0x3FFF, 0, 0);
               }
               break;
            case float_round_down:
               return aSign ? __packFloat128(1, 0x3FFF, 0, 0) : __packFloat128(0, 0, 0, 0);
            case float_round_up:
               return aSign ? __packFloat128(1, 0, 0, 0) : __packFloat128(0, 0x3FFF, 0, 0);
         }
         return __packFloat128(aSign, 0, 0, 0);
      }
      lastBitMask = 1;
      lastBitMask <<= 0x402F - aExp;
      roundBitsMask = lastBitMask - 1;
      z.low = 0;
      z.high = a.high;
      roundingMode = __float_rounding_mode;
      if(roundingMode == float_round_nearest_even)
      {
         z.high += lastBitMask >> 1;
         if(((z.high & roundBitsMask) | a.low) == 0)
         {
            z.high &= ~lastBitMask;
         }
      }
      else if(roundingMode != float_round_to_zero)
      {
         if(__extractFloat128Sign(z) ^ (roundingMode == float_round_up))
         {
            z.high |= (a.low != 0);
            z.high += roundBitsMask;
         }
      }
      z.high &= ~roundBitsMask;
   }
   if((z.low != a.low) || (z.high != a.high))
   {
      __float_exception_flags |= float_flag_inexact;
   }
   return z;
}

/*----------------------------------------------------------------------------
| Returns the result of adding the absolute values of the quadruple-precision
| floating-point values `a' and `b'.  If `zSign' is 1, the sum is negated
| before being returned.  `zSign' is ignored if the result is a NaN.
| The addition is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float128 __addFloat128Sigs(__float128 a, __float128 b, __flag zSign)
{
   __int32 aExp, bExp, zExp;
   __bits64 aSig0, aSig1, bSig0, bSig1, zSig0, zSig1, zSig2;
   __int32 expDiff;

   aSig1 = __extractFloat128Frac1(a);
   aSig0 = __extractFloat128Frac0(a);
   aExp = __extractFloat128Exp(a);
   bSig1 = __extractFloat128Frac1(b);
   bSig0 = __extractFloat128Frac0(b);
   bExp = __extractFloat128Exp(b);
   expDiff = aExp - bExp;
   if(0 < expDiff)
   {
      if(aExp == 0x7FFF)
      {
         if(aSig0 | aSig1)
            return __propagateFloat128NaN(a, b);
         return a;
      }
      if(bExp == 0)
      {
         --expDiff;
      }
      else
      {
         bSig0 |= LIT64(0x0001000000000000);
      }
      __shift128ExtraRightJamming(bSig0, bSig1, 0, expDiff, &bSig0, &bSig1, &zSig2);
      zExp = aExp;
   }
   else if(expDiff < 0)
   {
      if(bExp == 0x7FFF)
      {
         if(bSig0 | bSig1)
            return __propagateFloat128NaN(a, b);
         return __packFloat128(zSign, 0x7FFF, 0, 0);
      }
      if(aExp == 0)
      {
         ++expDiff;
      }
      else
      {
         aSig0 |= LIT64(0x0001000000000000);
      }
      __shift128ExtraRightJamming(aSig0, aSig1, 0, -expDiff, &aSig0, &aSig1, &zSig2);
      zExp = bExp;
   }
   else
   {
      if(aExp == 0x7FFF)
      {
         if(aSig0 | aSig1 | bSig0 | bSig1)
         {
            return __propagateFloat128NaN(a, b);
         }
         return a;
      }
      __add128(aSig0, aSig1, bSig0, bSig1, &zSig0, &zSig1);
      if(aExp == 0)
         return __packFloat128(zSign, 0, zSig0, zSig1);
      zSig2 = 0;
      zSig0 |= LIT64(0x0002000000000000);
      zExp = aExp;
      goto shiftRight1;
   }
   aSig0 |= LIT64(0x0001000000000000);
   __add128(aSig0, aSig1, bSig0, bSig1, &zSig0, &zSig1);
   --zExp;
   if(zSig0 < LIT64(0x0002000000000000))
      goto roundAndPack;
   ++zExp;
shiftRight1:
   __shift128ExtraRightJamming(zSig0, zSig1, zSig2, 1, &zSig0, &zSig1, &zSig2);
roundAndPack:
   return __roundAndPackFloat128(zSign, zExp, zSig0, zSig1, zSig2);
}

/*----------------------------------------------------------------------------
| Returns the result of subtracting the absolute values of the quadruple-
| precision floating-point values `a' and `b'.  If `zSign' is 1, the
| difference is negated before being returned.  `zSign' is ignored if the
| result is a NaN.  The subtraction is performed according to the IEC/IEEE
| Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float128 __subFloat128Sigs(__float128 a, __float128 b, __flag zSign)
{
   __int32 aExp, bExp, zExp;
   __bits64 aSig0, aSig1, bSig0, bSig1, zSig0, zSig1;
   __int32 expDiff;
   __float128 z;

   aSig1 = __extractFloat128Frac1(a);
   aSig0 = __extractFloat128Frac0(a);
   aExp = __extractFloat128Exp(a);
   bSig1 = __extractFloat128Frac1(b);
   bSig0 = __extractFloat128Frac0(b);
   bExp = __extractFloat128Exp(b);
   expDiff = aExp - bExp;
   __shortShift128Left(aSig0, aSig1, 14, &aSig0, &aSig1);
   __shortShift128Left(bSig0, bSig1, 14, &bSig0, &bSig1);
   if(0 < expDiff)
      goto aExpBigger;
   if(expDiff < 0)
      goto bExpBigger;
   if(aExp == 0x7FFF)
   {
      if(aSig0 | aSig1 | bSig0 | bSig1)
      {
         return __propagateFloat128NaN(a, b);
      }
      __float_raise(float_flag_invalid);
      z.low = __float128_default_nan_low;
      z.high = __float128_default_nan_high;
      return z;
   }
   if(aExp == 0)
   {
      aExp = 1;
      bExp = 1;
   }
   if(bSig0 < aSig0)
      goto aBigger;
   if(aSig0 < bSig0)
      goto bBigger;
   if(bSig1 < aSig1)
      goto aBigger;
   if(aSig1 < bSig1)
      goto bBigger;
   return __packFloat128(__float_rounding_mode == float_round_down, 0, 0, 0);
bExpBigger:
   if(bExp == 0x7FFF)
   {
      if(bSig0 | bSig1)
         return __propagateFloat128NaN(a, b);
      return __packFloat128(zSign ^ 1, 0x7FFF, 0, 0);
   }
   if(aExp == 0)
   {
      ++expDiff;
   }
   else
   {
      aSig0 |= LIT64(0x4000000000000000);
   }
   __shift128RightJamming(aSig0, aSig1, -expDiff, &aSig0, &aSig1);
   bSig0 |= LIT64(0x4000000000000000);
bBigger:
   __sub128(bSig0, bSig1, aSig0, aSig1, &zSig0, &zSig1);
   zExp = bExp;
   zSign ^= 1;
   goto normalizeRoundAndPack;
aExpBigger:
   if(aExp == 0x7FFF)
   {
      if(aSig0 | aSig1)
         return __propagateFloat128NaN(a, b);
      return a;
   }
   if(bExp == 0)
   {
      --expDiff;
   }
   else
   {
      bSig0 |= LIT64(0x4000000000000000);
   }
   __shift128RightJamming(bSig0, bSig1, expDiff, &bSig0, &bSig1);
   aSig0 |= LIT64(0x4000000000000000);
aBigger:
   __sub128(aSig0, aSig1, bSig0, bSig1, &zSig0, &zSig1);
   zExp = aExp;
normalizeRoundAndPack:
   --zExp;
   return __normalizeRoundAndPackFloat128(zSign, zExp - 14, zSig0, zSig1);
}

/*----------------------------------------------------------------------------
| Returns the result of adding the quadruple-precision floating-point values
| `a' and `b'.  The operation is performed according to the IEC/IEEE Standard
| for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__float128 __float128_add_ieee(__float128 a, __float128 b)
{
   __flag aSign, bSign;

   aSign = __extractFloat128Sign(a);
   bSign = __extractFloat128Sign(b);
   if(aSign == bSign)
   {
      return __addFloat128Sigs(a, b, aSign);
   }
   else
   {
      return __subFloat128Sigs(a, b, aSign);
   }
}

/*----------------------------------------------------------------------------
| Returns the result of subtracting the quadruple-precision floating-point
| values `a' and `b'.  The operation is performed according to the IEC/IEEE
| Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__float128 __float128_sub_ieee(__float128 a, __float128 b)
{
   __flag aSign, bSign;

   aSign = __extractFloat128Sign(a);
   bSign = __extractFloat128Sign(b);
   if(aSign == bSign)
   {
      return __subFloat128Sigs(a, b, aSign);
   }
   else
   {
      return __addFloat128Sigs(a, b, aSign);
   }
}

/*----------------------------------------------------------------------------
| Returns the result of multiplying the quadruple-precision floating-point
| values `a' and `b'.  The operation is performed according to the IEC/IEEE
| Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__float128 __float128_mul_ieee(__float128 a, __float128 b)
{
   __flag aSign, bSign, zSign;
   __int32 aExp, bExp, zExp;
   __bits64 aSig0, aSig1, bSig0, bSig1, zSig0, zSig1, zSig2, zSig3;
   __float128 z;

   aSig1 = __extractFloat128Frac1(a);
   aSig0 = __extractFloat128Frac0(a);
   aExp = __extractFloat128Exp(a);
   aSign = __extractFloat128Sign(a);
   bSig1 = __extractFloat128Frac1(b);
   bSig0 = __extractFloat128Frac0(b);
   bExp = __extractFloat128Exp(b);
   bSign = __extractFloat128Sign(b);
   zSign = aSign ^ bSign;
   if(aExp == 0x7FFF)
   {
      if((aSig0 | aSig1) || ((bExp == 0x7FFF) && (bSig0 | bSig1)))
      {
         return __propagateFloat128NaN(a, b);
      }
      if((bExp | bSig0 | bSig1) == 0)
         goto invalid;
      return __packFloat128(zSign, 0x7FFF, 0, 0);
   }
   if(bExp == 0x7FFF)
   {
      if(bSig0 | bSig1)
         return __propagateFloat128NaN(a, b);
      if((aExp | aSig0 | aSig1) == 0)
      {
      invalid:
         __float_raise(float_flag_invalid);
         z.low = __float128_default_nan_low;
         z.high = __float128_default_nan_high;
         return z;
      }
      return __packFloat128(zSign, 0x7FFF, 0, 0);
   }
   if(aExp == 0)
   {
      if((aSig0 | aSig1) == 0)
         return __packFloat128(zSign, 0, 0, 0);
      __normalizeFloat128Subnormal(aSig0, aSig1, &aExp, &aSig0, &aSig1);
   }
   if(bExp == 0)
   {
      if((bSig0 | bSig1) == 0)
         return __packFloat128(zSign, 0, 0, 0);
      __normalizeFloat128Subnormal(bSig0, bSig1, &bExp, &bSig0, &bSig1);
   }
   zExp = aExp + bExp - 0x4000;
   aSig0 |= LIT64(0x0001000000000000);
   __shortShift128Left(bSig0, bSig1, 16, &bSig0, &bSig1);
   __mul128To256(aSig0, aSig1, bSig0, bSig1, &zSig0, &zSig1, &zSig2, &zSig3);
   __add128(zSig0, zSig1, aSig0, aSig1, &zSig0, &zSig1);
   zSig2 |= (zSig3 != 0);
   if(LIT64(0x0002000000000000) <= zSig0)
   {
      __shift128ExtraRightJamming(zSig0, zSig1, zSig2, 1, &zSig0, &zSig1, &zSig2);
      ++zExp;
   }
   return __roundAndPackFloat128(zSign, zExp, zSig0, zSig1, zSig2);
}

/*----------------------------------------------------------------------------
| Returns the result of dividing the quadruple-precision floating-point value
| `a' by the corresponding value `b'.  The operation is performed according to
| the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__float128 __float128_div_ieee(__float128 a, __float128 b)
{
   __flag aSign, bSign, zSign;
   __int32 aExp, bExp, zExp;
   __bits64 aSig0, aSig1, bSig0, bSig1, zSig0, zSig1, zSig2;
   __bits64 rem0, rem1, rem2, rem3, term0, term1, term2, term3;
   __float128 z;

   aSig1 = __extractFloat128Frac1(a);
   aSig0 = __extractFloat128Frac0(a);
   aExp = __extractFloat128Exp(a);
   aSign = __extractFloat128Sign(a);
   bSig1 = __extractFloat128Frac1(b);
   bSig0 = __extractFloat128Frac0(b);
   bExp = __extractFloat128Exp(b);
   bSign = __extractFloat128Sign(b);
   zSign = aSign ^ bSign;
   if(aExp == 0x7FFF)
   {
      if(aSig0 | aSig1)
         return __propagateFloat128NaN(a, b);
      if(bExp == 0x7FFF)
      {
         if(bSig0 | bSig1)
            return __propagateFloat128NaN(a, b);
         goto invalid;
      }
      return __packFloat128(zSign, 0x7FFF, 0, 0);
   }
   if(bExp == 0x7FFF)
   {
      if(bSig0 | bSig1)
         return __propagateFloat128NaN(a, b);
      return __packFloat128(zSign, 0, 0, 0);
   }
   if(bExp == 0)
   {
      if((bSig0 | bSig1) == 0)
      {
         if((aExp | aSig0 | aSig1) == 0)
         {
         invalid:
            __float_raise(float_flag_invalid);
            z.low = __float128_default_nan_low;
            z.high = __float128_default_nan_high;
            return z;
         }
         __float_raise(float_flag_divbyzero);
         return __packFloat128(zSign, 0x7FFF, 0, 0);
      }
      __normalizeFloat128Subnormal(bSig0, bSig1, &bExp, &bSig0, &bSig1);
   }
   if(aExp == 0)
   {
      if((aSig0 | aSig1) == 0)
         return __packFloat128(zSign, 0, 0, 0);
      __normalizeFloat128Subnormal(aSig0, aSig1, &aExp, &aSig0, &aSig1);
   }
   zExp = aExp - bExp + 0x3FFD;
   __shortShift128Left(aSig0 | LIT64(0x0001000000000000), aSig1, 15, &aSig0, &aSig1);
   __shortShift128Left(bSig0 | LIT64(0x0001000000000000), bSig1, 15, &bSig0, &bSig1);
   if(__le128(bSig0, bSig1, aSig0, aSig1))
   {
      __shift128Right(aSig0, aSig1, 1, &aSig0, &aSig1);
      ++zExp;
   }
   zSig0 = __estimateDiv128To64(aSig0, aSig1, bSig0);
   __mul128By64To192(bSig0, bSig1, zSig0, &term0, &term1, &term2);
   __sub192(aSig0, aSig1, 0, term0, term1, term2, &rem0, &rem1, &rem2);
   while((__sbits64)rem0 < 0)
   {
      --zSig0;
      __add192(rem0, rem1, rem2, 0, bSig0, bSig1, &rem0, &rem1, &rem2);
   }
   zSig1 = __estimateDiv128To64(rem1, rem2, bSig0);
   if((zSig1 & 0x3FFF) <= 4)
   {
      __mul128By64To192(bSig0, bSig1, zSig1, &term1, &term2, &term3);
      __sub192(rem1, rem2, 0, term1, term2, term3, &rem1, &rem2, &rem3);
      while((__sbits64)rem1 < 0)
      {
         --zSig1;
         __add192(rem1, rem2, rem3, 0, bSig0, bSig1, &rem1, &rem2, &rem3);
      }
      zSig1 |= ((rem1 | rem2 | rem3) != 0);
   }
   __shift128ExtraRightJamming(zSig0, zSig1, 0, 15, &zSig0, &zSig1, &zSig2);
   return __roundAndPackFloat128(zSign, zExp, zSig0, zSig1, zSig2);
}

/*----------------------------------------------------------------------------
| Returns the remainder of the quadruple-precision floating-point value `a'
| with respect to the corresponding value `b'.  The operation is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__float128 __float128_rem_ieee(__float128 a, __float128 b)
{
   __flag aSign, bSign, zSign;
   __int32 aExp, bExp, expDiff;
   __bits64 aSig0, aSig1, bSig0, bSig1, q, term0, term1, term2;
   __bits64 allZero, alternateASig0, alternateASig1, sigMean1;
   __sbits64 sigMean0;
   __float128 z;

   aSig1 = __extractFloat128Frac1(a);
   aSig0 = __extractFloat128Frac0(a);
   aExp = __extractFloat128Exp(a);
   aSign = __extractFloat128Sign(a);
   bSig1 = __extractFloat128Frac1(b);
   bSig0 = __extractFloat128Frac0(b);
   bExp = __extractFloat128Exp(b);
   bSign = __extractFloat128Sign(b);
   if(aExp == 0x7FFF)
   {
      if((aSig0 | aSig1) || ((bExp == 0x7FFF) && (bSig0 | bSig1)))
      {
         return __propagateFloat128NaN(a, b);
      }
      goto invalid;
   }
   if(bExp == 0x7FFF)
   {
      if(bSig0 | bSig1)
         return __propagateFloat128NaN(a, b);
      return a;
   }
   if(bExp == 0)
   {
      if((bSig0 | bSig1) == 0)
      {
      invalid:
         __float_raise(float_flag_invalid);
         z.low = __float128_default_nan_low;
         z.high = __float128_default_nan_high;
         return z;
      }
      __normalizeFloat128Subnormal(bSig0, bSig1, &bExp, &bSig0, &bSig1);
   }
   if(aExp == 0)
   {
      if((aSig0 | aSig1) == 0)
         return a;
      __normalizeFloat128Subnormal(aSig0, aSig1, &aExp, &aSig0, &aSig1);
   }
   expDiff = aExp - bExp;
   if(expDiff < -1)
      return a;
   __shortShift128Left(aSig0 | LIT64(0x0001000000000000), aSig1, 15 - (expDiff < 0), &aSig0, &aSig1);
   __shortShift128Left(bSig0 | LIT64(0x0001000000000000), bSig1, 15, &bSig0, &bSig1);
   q = __le128(bSig0, bSig1, aSig0, aSig1);
   if(q)
      __sub128(aSig0, aSig1, bSig0, bSig1, &aSig0, &aSig1);
   expDiff -= 64;
   while(0 < expDiff)
   {
      q = __estimateDiv128To64(aSig0, aSig1, bSig0);
      q = (4 < q) ? q - 4 : 0;
      __mul128By64To192(bSig0, bSig1, q, &term0, &term1, &term2);
      __shortShift192Left(term0, term1, term2, 61, &term1, &term2, &allZero);
      __shortShift128Left(aSig0, aSig1, 61, &aSig0, &allZero);
      __sub128(aSig0, 0, term1, term2, &aSig0, &aSig1);
      expDiff -= 61;
   }
   if(-64 < expDiff)
   {
      q = __estimateDiv128To64(aSig0, aSig1, bSig0);
      q = (4 < q) ? q - 4 : 0;
      q >>= -expDiff;
      __shift128Right(bSig0, bSig1, 12, &bSig0, &bSig1);
      expDiff += 52;
      if(expDiff < 0)
      {
         __shift128Right(aSig0, aSig1, -expDiff, &aSig0, &aSig1);
      }
      else
      {
         __shortShift128Left(aSig0, aSig1, expDiff, &aSig0, &aSig1);
      }
      __mul128By64To192(bSig0, bSig1, q, &term0, &term1, &term2);
      __sub128(aSig0, aSig1, term1, term2, &aSig0, &aSig1);
   }
   else
   {
      __shift128Right(aSig0, aSig1, 12, &aSig0, &aSig1);
      __shift128Right(bSig0, bSig1, 12, &bSig0, &bSig1);
   }
   do
   {
      alternateASig0 = aSig0;
      alternateASig1 = aSig1;
      ++q;
      __sub128(aSig0, aSig1, bSig0, bSig1, &aSig0, &aSig1);
   } while(0 <= (__sbits64)aSig0);
   __add128(aSig0, aSig1, alternateASig0, alternateASig1, &sigMean0, &sigMean1);
   if((sigMean0 < 0) || (((sigMean0 | sigMean1) == 0) && (q & 1)))
   {
      aSig0 = alternateASig0;
      aSig1 = alternateASig1;
   }
   zSign = ((__sbits64)aSig0 < 0);
   if(zSign)
      __sub128(0, 0, aSig0, aSig1, &aSig0, &aSig1);
   return __normalizeRoundAndPackFloat128(aSign ^ zSign, bExp - 4, aSig0, aSig1);
}

/*----------------------------------------------------------------------------
| Returns the square root of the quadruple-precision floating-point value `a'.
| The operation is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__float128 __float128_sqrt(__float128 a)
{
   __flag aSign;
   __int32 aExp, zExp;
   __bits64 aSig0, aSig1, zSig0, zSig1, zSig2, doubleZSig0;
   __bits64 rem0, rem1, rem2, rem3, term0, term1, term2, term3;
   __float128 z;

   aSig1 = __extractFloat128Frac1(a);
   aSig0 = __extractFloat128Frac0(a);
   aExp = __extractFloat128Exp(a);
   aSign = __extractFloat128Sign(a);
   if(aExp == 0x7FFF)
   {
      if(aSig0 | aSig1)
         return __propagateFloat128NaN(a, a);
      if(!aSign)
         return a;
      goto invalid;
   }
   if(aSign)
   {
      if((aExp | aSig0 | aSig1) == 0)
         return a;
   invalid:
      __float_raise(float_flag_invalid);
      z.low = __float128_default_nan_low;
      z.high = __float128_default_nan_high;
      return z;
   }
   if(aExp == 0)
   {
      if((aSig0 | aSig1) == 0)
         return __packFloat128(0, 0, 0, 0);
      __normalizeFloat128Subnormal(aSig0, aSig1, &aExp, &aSig0, &aSig1);
   }
   zExp = ((aExp - 0x3FFF) >> 1) + 0x3FFE;
   aSig0 |= LIT64(0x0001000000000000);
   zSig0 = __estimateSqrt32(aExp, aSig0 >> 17);
   __shortShift128Left(aSig0, aSig1, 13 - (aExp & 1), &aSig0, &aSig1);
   zSig0 = __estimateDiv128To64(aSig0, aSig1, zSig0 << 32) + (zSig0 << 30);
   doubleZSig0 = zSig0 << 1;
   __mul64To128(zSig0, zSig0, &term0, &term1);
   __sub128(aSig0, aSig1, term0, term1, &rem0, &rem1);
   while((__sbits64)rem0 < 0)
   {
      --zSig0;
      doubleZSig0 -= 2;
      __add128(rem0, rem1, zSig0 >> 63, doubleZSig0 | 1, &rem0, &rem1);
   }
   zSig1 = __estimateDiv128To64(rem1, 0, doubleZSig0);
   if((zSig1 & 0x1FFF) <= 5)
   {
      if(zSig1 == 0)
         zSig1 = 1;
      __mul64To128(doubleZSig0, zSig1, &term1, &term2);
      __sub128(rem1, 0, term1, term2, &rem1, &rem2);
      __mul64To128(zSig1, zSig1, &term2, &term3);
      __sub192(rem1, rem2, 0, 0, term2, term3, &rem1, &rem2, &rem3);
      while((__sbits64)rem1 < 0)
      {
         --zSig1;
         __shortShift128Left(0, zSig1, 1, &term2, &term3);
         term3 |= 1;
         term2 |= doubleZSig0;
         __add192(rem1, rem2, rem3, 0, term2, term3, &rem1, &rem2, &rem3);
      }
      zSig1 |= ((rem1 | rem2 | rem3) != 0);
   }
   __shift128ExtraRightJamming(zSig0, zSig1, 0, 14, &zSig0, &zSig1, &zSig2);
   return __roundAndPackFloat128(0, zExp, zSig0, zSig1, zSig2);
}

/*----------------------------------------------------------------------------
| Returns 1 if the quadruple-precision floating-point value `a' is equal to
| the corresponding value `b', and 0 otherwise.  The comparison is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__flag __float128_eq_ieee(__float128 a, __float128 b)
{
   if(((__extractFloat128Exp(a) == 0x7FFF) && (__extractFloat128Frac0(a) | __extractFloat128Frac1(a))) ||
      ((__extractFloat128Exp(b) == 0x7FFF) && (__extractFloat128Frac0(b) | __extractFloat128Frac1(b))))
   {
      if(__float128_is_signaling_nan(a) || __float128_is_signaling_nan(b))
      {
         __float_raise(float_flag_invalid);
      }
      return 0;
   }
   return (a.low == b.low) && ((a.high == b.high) || ((a.low == 0) && ((__bits64)((a.high | b.high) << 1) == 0)));
}

/*----------------------------------------------------------------------------
| Returns 1 if the quadruple-precision floating-point value `a' is less than
| or equal to the corresponding value `b', and 0 otherwise.  The comparison
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag _Float128LE(__float128 a, __float128 b)
{
   __flag aSign, bSign;

   if(((__extractFloat128Exp(a) == 0x7FFF) && (__extractFloat128Frac0(a) | __extractFloat128Frac1(a))) ||
      ((__extractFloat128Exp(b) == 0x7FFF) && (__extractFloat128Frac0(b) | __extractFloat128Frac1(b))))
   {
      __float_raise(float_flag_invalid);
      return 0;
   }
   aSign = __extractFloat128Sign(a);
   bSign = __extractFloat128Sign(b);
   if(aSign != bSign)
   {
      return aSign || ((((__bits64)((a.high | b.high) << 1)) | a.low | b.low) == 0);
   }
   return aSign ? __le128(b.high, b.low, a.high, a.low) : __le128(a.high, a.low, b.high, b.low);
}

__flag __float128_le_ieee(__float128, __float128)
{
   return _Float128LE(a, b);
}

__flag __float128_ge_ieee(__float128 a, __float128 b)
{
   return _Float128LE(b, a);
}

/*----------------------------------------------------------------------------
| Returns 1 if the quadruple-precision floating-point value `a' is less than
| the corresponding value `b', and 0 otherwise.  The comparison is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag _Float128LT(__float128 a, __float128 b)
{
   __flag aSign, bSign;

   if(((__extractFloat128Exp(a) == 0x7FFF) && (__extractFloat128Frac0(a) | __extractFloat128Frac1(a))) ||
      ((__extractFloat128Exp(b) == 0x7FFF) && (__extractFloat128Frac0(b) | __extractFloat128Frac1(b))))
   {
      __float_raise(float_flag_invalid);
      return 0;
   }
   aSign = __extractFloat128Sign(a);
   bSign = __extractFloat128Sign(b);
   if(aSign != bSign)
   {
      return aSign && ((((__bits64)((a.high | b.high) << 1)) | a.low | b.low) != 0);
   }
   return aSign ? __lt128(b.high, b.low, a.high, a.low) : __lt128(a.high, a.low, b.high, b.low);
}

__flag __float128_lt_ieee(__float128 a, __float128 b)
{
   return _Float128LT(a, b);
}

__flag __float128_gt_ieee(__float128 a, __float128 b)
{
   return _Float128LT(b, a);
}

/*----------------------------------------------------------------------------
| Returns 1 if the quadruple-precision floating-point value `a' is equal to
| the corresponding value `b', and 0 otherwise.  The invalid exception is
| raised if either operand is a NaN.  Otherwise, the comparison is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__flag __float128_eq_signaling_ieee(__float128 a, __float128 b)
{
   if(((__extractFloat128Exp(a) == 0x7FFF) && (__extractFloat128Frac0(a) | __extractFloat128Frac1(a))) ||
      ((__extractFloat128Exp(b) == 0x7FFF) && (__extractFloat128Frac0(b) | __extractFloat128Frac1(b))))
   {
      __float_raise(float_flag_invalid);
      return 0;
   }
   return (a.low == b.low) && ((a.high == b.high) || ((a.low == 0) && ((__bits64)((a.high | b.high) << 1) == 0)));
}

/*----------------------------------------------------------------------------
| Returns 1 if the quadruple-precision floating-point value `a' is less than
| or equal to the corresponding value `b', and 0 otherwise.  Quiet NaNs do not
| cause an exception.  Otherwise, the comparison is performed according to the
| IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__flag __float128_le_quiet_ieee(__float128 a, __float128 b)
{
   __flag aSign, bSign;

   if(((__extractFloat128Exp(a) == 0x7FFF) && (__extractFloat128Frac0(a) | __extractFloat128Frac1(a))) ||
      ((__extractFloat128Exp(b) == 0x7FFF) && (__extractFloat128Frac0(b) | __extractFloat128Frac1(b))))
   {
      if(__float128_is_signaling_nan(a) || __float128_is_signaling_nan(b))
      {
         __float_raise(float_flag_invalid);
      }
      return 0;
   }
   aSign = __extractFloat128Sign(a);
   bSign = __extractFloat128Sign(b);
   if(aSign != bSign)
   {
      return aSign || ((((__bits64)((a.high | b.high) << 1)) | a.low | b.low) == 0);
   }
   return aSign ? __le128(b.high, b.low, a.high, a.low) : __le128(a.high, a.low, b.high, b.low);
}

/*----------------------------------------------------------------------------
| Returns 1 if the quadruple-precision floating-point value `a' is less than
| the corresponding value `b', and 0 otherwise.  Quiet NaNs do not cause an
| exception.  Otherwise, the comparison is performed according to the IEC/IEEE
| Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

__flag __float128_lt_quiet_ieee(__float128 a, __float128 b)
{
   __flag aSign, bSign;

   if(((__extractFloat128Exp(a) == 0x7FFF) && (__extractFloat128Frac0(a) | __extractFloat128Frac1(a))) ||
      ((__extractFloat128Exp(b) == 0x7FFF) && (__extractFloat128Frac0(b) | __extractFloat128Frac1(b))))
   {
      if(__float128_is_signaling_nan(a) || __float128_is_signaling_nan(b))
      {
         __float_raise(float_flag_invalid);
      }
      return 0;
   }
   aSign = __extractFloat128Sign(a);
   bSign = __extractFloat128Sign(b);
   if(aSign != bSign)
   {
      return aSign && ((((__bits64)((a.high | b.high) << 1)) | a.low | b.low) != 0);
   }
   return aSign ? __lt128(b.high, b.low, a.high, a.low) : __lt128(a.high, a.low, b.high, b.low);
}

#endif
