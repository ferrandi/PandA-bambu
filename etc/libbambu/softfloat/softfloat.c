/**
 * SoftFloat package modified by Fabrizio Ferrandi - Politecnico di Milano
 * Changes made are mainly oriented to improve the results of a generic high
 * level synthesis framework.
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

#define COND_EXPR_MACRO32(cond, a, b) ((((__bits32)((((__sbits32)(cond)) << 31) >> 31)) & (a)) | ((~((__bits32)((((__sbits32)(cond)) << 31) >> 31))) & (b)))
#define COND_EXPR_MACRO64(cond, a, b) ((((__bits64)((((__sbits64)(cond)) << 63) >> 63)) & (a)) | ((~((__bits64)((((__sbits64)(cond)) << 63) >> 63))) & (b)))

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

static __FORCE_INLINE __bits32 __extractFloat32Frac(__float32 a)
{
   return a & 0x007FFFFF;
}

/*----------------------------------------------------------------------------
| Returns the exponent bits of the single-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __int16 __extractFloat32Exp(__float32 a)
{
   return (a >> 23) & 0xFF;
}

/*----------------------------------------------------------------------------
| Returns the sign bit of the single-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag __extractFloat32Sign(__float32 a)
{
   return a >> 31;
}

/*----------------------------------------------------------------------------
| Normalizes the subnormal single-precision floating-point value represented
| by the denormalized significand `aSig'.  The normalized exponent and
| significand are stored at the locations pointed to by `zExpPtr' and
| `zSigPtr', respectively.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE void __normalizeFloat32Subnormal(__bits32 aSig, __int16* zExpPtr, __bits32* zSigPtr)
{
   __int8 shiftCount;

   shiftCount = __countLeadingZeros32(aSig) - 8;
   *zSigPtr = aSig << shiftCount;
   *zExpPtr = 1 - shiftCount;
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

static __FORCE_INLINE __float32 __packFloat32(__flag zSign, __int16 zExp, __bits32 zSig)
{
   return (((__bits32)zSign) << 31) + (((__bits32)zExp) << 23) + zSig;
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

static __FORCE_INLINE __float32 __roundAndPackFloat32(__flag zSign, __int16 zExp, __bits32 zSig)
{
   __int8 roundingMode;
   __flag roundNearestEven;
   __int8 roundIncrement, roundBits;
   __flag isTiny;

   roundingMode = __float_rounding_mode;
   roundNearestEven = (roundingMode == float_round_nearest_even);
#ifdef NO_PARAMETRIC
   roundIncrement = 0x40;
#else
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
#endif
   roundBits = zSig & 0x7F;
   if(0xFD <= (__bits16)zExp)
   {
      if((0xFD < zExp) || ((zExp == 0xFD) && ((__sbits32)(zSig + roundIncrement) < 0)))
      {
         __float_raise(float_flag_overflow | float_flag_inexact);
         return __packFloat32(zSign, 0xFF, 0) - (roundIncrement == 0);
      }
      if(zExp < 0)
      {
#ifdef NO_SUBNORMALS
         return __packFloat32(zSign, 0, 0);
#else
         isTiny = (__float_detect_tininess == float_tininess_before_rounding) || (zExp < -1) || (zSig + roundIncrement < 0x80000000);
         __shift32RightJamming(zSig, -zExp, &zSig);
         zExp = 0;
         roundBits = zSig & 0x7F;
         if(isTiny && roundBits)
            __float_raise(float_flag_underflow);
#endif
      }
   }
#ifndef NO_PARAMETRIC
   if(roundBits)
      __float_exception_flags |= float_flag_inexact;
#endif
   zSig = (zSig + roundIncrement) >> 7;
   zSig &= ~(((roundBits ^ 0x40) == 0) & roundNearestEven);
   if(zSig == 0)
      zExp = 0;
   return __packFloat32(zSign, zExp, zSig);
}

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent `zExp',
| and significand `zSig', and returns the proper single-precision floating-
| point value corresponding to the abstract input.  This routine is just like
| `__roundAndPackFloat32' except that `zSig' does not have to be normalized.
| Bit 31 of `zSig' must be zero, and `zExp' must be 1 less than the ``true''
| floating-point exponent.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float32 __normalizeRoundAndPackFloat32(__flag zSign, __int16 zExp, __bits32 zSig)
{
   __int8 shiftCount;

   shiftCount = __countLeadingZeros32(zSig) - 1;
   return __roundAndPackFloat32(zSign, zExp - shiftCount, zSig << shiftCount);
}

/*----------------------------------------------------------------------------
| Returns the fraction bits of the double-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __bits64 __extractFloat64Frac(__float64 a)
{
   return a & LIT64(0x000FFFFFFFFFFFFF);
}

/*----------------------------------------------------------------------------
| Returns the exponent bits of the double-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __int16 __extractFloat64Exp(__float64 a)
{
   return (a >> 52) & 0x7FF;
}

/*----------------------------------------------------------------------------
| Returns the sign bit of the double-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag __extractFloat64Sign(__float64 a)
{
   return a >> 63;
}

/*----------------------------------------------------------------------------
| Normalizes the subnormal double-precision floating-point value represented
| by the denormalized significand `aSig'.  The normalized exponent and
| significand are stored at the locations pointed to by `zExpPtr' and
| `zSigPtr', respectively.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE void __normalizeFloat64Subnormal(__bits64 aSig, __int16* zExpPtr, __bits64* zSigPtr)
{
   __int8 shiftCount;

   shiftCount = __countLeadingZeros64(aSig) - 11;
   *zSigPtr = aSig << shiftCount;
   *zExpPtr = 1 - shiftCount;
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

static __FORCE_INLINE __float64 __packFloat64(__flag zSign, __int16 zExp, __bits64 zSig)
{
   return (((__bits64)zSign) << 63) + (((__bits64)zExp) << 52) + zSig;
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

static __FORCE_INLINE __float64 __roundAndPackFloat64(__flag zSign, __int16 zExp, __bits64 zSig)
{
   __int8 roundingMode;
   __flag roundNearestEven;
   __int16 roundIncrement, roundBits;
   __flag isTiny;

   roundingMode = __float_rounding_mode;
   roundNearestEven = (roundingMode == float_round_nearest_even);
   roundIncrement = 0x200;
   if(!roundNearestEven)
   {
      if(roundingMode == float_round_to_zero)
      {
         roundIncrement = 0;
      }
      else
      {
         roundIncrement = 0x3FF;
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
   roundBits = zSig & 0x3FF;
   if(0x7FD <= (__bits16)zExp)
   {
      if((0x7FD < zExp) || ((zExp == 0x7FD) && ((__sbits64)(zSig + roundIncrement) < 0)))
      {
         __float_raise(float_flag_overflow | float_flag_inexact);
         return __packFloat64(zSign, 0x7FF, 0) - (roundIncrement == 0);
      }
      if(zExp < 0)
      {
#ifdef NO_SUBNORMALS
         return __packFloat32(zSign, 0, 0);
#else
         isTiny = (__float_detect_tininess == float_tininess_before_rounding) || (zExp < -1) || (zSig + roundIncrement < LIT64(0x8000000000000000));
         __shift64RightJamming(zSig, -zExp, &zSig);
         zExp = 0;
         roundBits = zSig & 0x3FF;
         if(isTiny && roundBits)
            __float_raise(float_flag_underflow);
#endif
      }
   }
#ifndef NO_PARAMETRIC
   if(roundBits)
      __float_exception_flags |= float_flag_inexact;
#endif
   zSig = (zSig + roundIncrement) >> 10;
   zSig &= ~(((roundBits ^ 0x200) == 0) & roundNearestEven);
   if(zSig == 0)
      zExp = 0;
   return __packFloat64(zSign, zExp, zSig);
}

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent `zExp',
| and significand `zSig', and returns the proper double-precision floating-
| point value corresponding to the abstract input.  This routine is just like
| `__roundAndPackFloat64' except that `zSig' does not have to be normalized.
| Bit 63 of `zSig' must be zero, and `zExp' must be 1 less than the ``true''
| floating-point exponent.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float64 __normalizeRoundAndPackFloat64(__flag zSign, __int16 zExp, __bits64 zSig)
{
   __int8 shiftCount;

   shiftCount = __countLeadingZeros64(zSig) - 1;
   return __roundAndPackFloat64(zSign, zExp - shiftCount, zSig << shiftCount);
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

static __FORCE_INLINE __floatx80 __roundAndPackFloatx80(__int8 roundingPrecision, __flag zSign, __int32 zExp, __bits64 zSig0, __bits64 zSig1)
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
         isTiny = (__float_detect_tininess == float_tininess_before_rounding) || (zExp < 0) || (zSig0 <= zSig0 + roundIncrement);
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
         if((roundingMode == float_round_to_zero) || (zSign && (roundingMode == float_round_up)) || (!zSign && (roundingMode == float_round_down)))
         {
            return __packFloatx80(zSign, 0x7FFE, ~roundMask);
         }
         return __packFloatx80(zSign, 0x7FFF, LIT64(0x8000000000000000));
      }
      if(zExp <= 0)
      {
         isTiny = (__float_detect_tininess == float_tininess_before_rounding) || (zExp < 0) || !increment || (zSig0 < LIT64(0xFFFFFFFFFFFFFFFF));
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

static __FORCE_INLINE __floatx80 __normalizeRoundAndPackFloatx80(__int8 roundingPrecision, __flag zSign, __int32 zExp, __bits64 zSig0, __bits64 zSig1)
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

static __FORCE_INLINE void __normalizeFloat128Subnormal(__bits64 aSig0, __bits64 aSig1, __int32* zExpPtr, __bits64* zSig0Ptr, __bits64* zSig1Ptr)
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

static __FORCE_INLINE __float128 __roundAndPackFloat128(__flag zSign, __int32 zExp, __bits64 zSig0, __bits64 zSig1, __bits64 zSig2)
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
      if((0x7FFD < zExp) || ((zExp == 0x7FFD) && __eq128(LIT64(0x0001FFFFFFFFFFFF), LIT64(0xFFFFFFFFFFFFFFFF), zSig0, zSig1) && increment))
      {
         __float_raise(float_flag_overflow | float_flag_inexact);
         if((roundingMode == float_round_to_zero) || (zSign && (roundingMode == float_round_up)) || (!zSign && (roundingMode == float_round_down)))
         {
            return __packFloat128(zSign, 0x7FFE, LIT64(0x0000FFFFFFFFFFFF), LIT64(0xFFFFFFFFFFFFFFFF));
         }
         return __packFloat128(zSign, 0x7FFF, 0, 0);
      }
      if(zExp < 0)
      {
         isTiny = (__float_detect_tininess == float_tininess_before_rounding) || (zExp < -1) || !increment || __lt128(zSig0, zSig1, LIT64(0x0001FFFFFFFFFFFF), LIT64(0xFFFFFFFFFFFFFFFF));
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

static __FORCE_INLINE __float128 __normalizeRoundAndPackFloat128(__flag zSign, __int32 zExp, __bits64 zSig0, __bits64 zSig1)
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

static __FORCE_INLINE __float32 __int32_to_float32(__int32 a)
{
   __flag zSign;

   if(a == 0)
      return 0;
   if(a == (__sbits32)0x80000000)
      return __packFloat32(1, 0x9E, 0);
   zSign = (a < 0);
   return __normalizeRoundAndPackFloat32(zSign, 0x9C, zSign ? -a : a);
}

/*----------------------------------------------------------------------------
| Returns the result of converting the 32-bit two's complement integer `a'
| to the single-precision floating-point format.  The conversion is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float32 __uint32_to_float32(__uint32 a_orig)
{
   __flag zSign;
   __uint64 absA;
   __int8 shiftCount;

   if(a_orig == 0)
      return 0;
   zSign = 0;
   absA = a_orig;
   shiftCount = __countLeadingZeros64(absA) - 40;
   if(0 <= shiftCount)
   {
      return __packFloat32(zSign, 0x95 - shiftCount, absA << shiftCount);
   }
   else
   {
      shiftCount += 7;
      if(shiftCount < 0)
      {
         __shift64RightJamming(absA, -shiftCount, &absA);
      }
      else
      {
         absA <<= shiftCount;
      }
      return __roundAndPackFloat32(zSign, 0x9C - shiftCount, absA);
   }
}

/*----------------------------------------------------------------------------
| Returns the result of converting the 32-bit two's complement integer `a'
| to the double-precision floating-point format.  The conversion is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float64 __int32_to_float64(__int32 a)
{
   __flag zSign;
   __uint32 absA;
   __int8 shiftCount;
   __bits64 zSig;

   if(a == 0)
      return 0;
   zSign = (a < 0);
   absA = zSign ? -a : a;
   shiftCount = __countLeadingZeros32(absA) + 21;
   zSig = absA;
   return __packFloat64(zSign, 0x432 - shiftCount, zSig << shiftCount);
}

static __FORCE_INLINE __float64 __uint32_to_float64(__uint32 absA)
{
   __int8 shiftCount;
   __bits64 zSig;

   if(absA == 0)
      return 0;
   shiftCount = __countLeadingZeros32(absA) + 21;
   zSig = absA;
   return __packFloat64(0, 0x432 - shiftCount, zSig << shiftCount);
}

#ifdef FLOATX80

/*----------------------------------------------------------------------------
| Returns the result of converting the 32-bit two's complement integer `a'
| to the extended double-precision floating-point format.  The conversion
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __floatx80 __int32_to_floatx80(__int32 a)
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

static __FORCE_INLINE __float128 __int32_to_float128(__int32 a)
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

static __FORCE_INLINE __float32 __int64_to_float32(__int64 a)
{
   __flag zSign;
   __uint64 absA;
   __int8 shiftCount;

   if(a == 0)
      return 0;
   zSign = (a < 0);
   absA = zSign ? -a : a;
   shiftCount = __countLeadingZeros64(absA) - 40;
   if(0 <= shiftCount)
   {
      return __packFloat32(zSign, 0x95 - shiftCount, absA << shiftCount);
   }
   else
   {
      shiftCount += 7;
      if(shiftCount < 0)
      {
         __shift64RightJamming(absA, -shiftCount, &absA);
      }
      else
      {
         absA <<= shiftCount;
      }
      return __roundAndPackFloat32(zSign, 0x9C - shiftCount, absA);
   }
}

static __FORCE_INLINE __float32 __uint64_to_float32(__uint64 absA)
{
   __int8 shiftCount;

   if(absA == 0)
      return 0;
   shiftCount = __countLeadingZeros64(absA) - 40;
   if(0 <= shiftCount)
   {
      return __packFloat32(0, 0x95 - shiftCount, absA << shiftCount);
   }
   else
   {
      shiftCount += 7;
      if(shiftCount < 0)
      {
         __shift64RightJamming(absA, -shiftCount, &absA);
      }
      else
      {
         absA <<= shiftCount;
      }
      return __roundAndPackFloat32(0, 0x9C - shiftCount, absA);
   }
}

/*----------------------------------------------------------------------------
| Returns the result of converting the 64-bit two's complement integer `a'
| to the double-precision floating-point format.  The conversion is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float64 __int64_to_float64(__int64 a)
{
   __flag zSign;

   if(a == 0)
      return 0;
   if(a == (__sbits64)LIT64(0x8000000000000000))
   {
      return __packFloat64(1, 0x43E, 0);
   }
   zSign = (a < 0);
   return __normalizeRoundAndPackFloat64(zSign, 0x43C, zSign ? -a : a);
}

static __FORCE_INLINE __float64 __uint64_to_float64(__uint64 a)
{
   __int16 zExp;
   __bits64 zSig;

   if(a == 0)
      return 0;
   if(a & (__bits64)LIT64(0x8000000000000000))
   {
      zExp = 0x43D;
      zSig = a >> 1;
   }
   else
   {
      zExp = 0x43C;
      zSig = a;
   }
   return __normalizeRoundAndPackFloat64(0, zExp, zSig);
}

#ifdef FLOATX80

/*----------------------------------------------------------------------------
| Returns the result of converting the 64-bit two's complement integer `a'
| to the extended double-precision floating-point format.  The conversion
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __floatx80 __int64_to_floatx80(__int64 a)
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

static __FORCE_INLINE __float128 __int64_to_float128(__int64 a)
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

static __FORCE_INLINE __int32 __float32_to_int32(__float32 a)
{
   __flag aSign;
   __int16 aExp, shiftCount;
   __bits32 aSig;
   __bits64 aSig64;

   aSig = __extractFloat32Frac(a);
   aExp = __extractFloat32Exp(a);
   aSign = __extractFloat32Sign(a);
   if((aExp == 0xFF) && aSig)
      aSign = 0;
   if(aExp)
      aSig |= 0x00800000;
   shiftCount = 0xAF - aExp;
   aSig64 = aSig;
   aSig64 <<= 32;
   if(0 < shiftCount)
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

static __FORCE_INLINE __int32 __float32_to_int32_round_to_zero(__float32 a)
{
   __flag aSign;
   __int16 aExp, shiftCount;
   __bits32 aSig;
   __int32 z;

   aSig = __extractFloat32Frac(a);
   aExp = __extractFloat32Exp(a);
   aSign = __extractFloat32Sign(a);
   shiftCount = aExp - 0x9E;
   if(0 <= shiftCount)
   {
      if(a != 0xCF000000)
      {
         __float_raise(float_flag_invalid);
         if(!aSign || ((aExp == 0xFF) && aSig))
            return 0x7FFFFFFF;
      }
      return (__sbits32)0x80000000;
   }
   else if(aExp <= 0x7E)
   {
#ifndef NO_PARAMETRIC
      if(aExp | aSig)
         __float_exception_flags |= float_flag_inexact;
#endif
      return 0;
   }
   aSig = (aSig | 0x00800000) << 8;
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

static __FORCE_INLINE __uint32 __float32_to_uint32_round_to_zero(__float32 a)
{
   __flag aSign;
   __int16 aExp, shiftCount;
   __bits32 aSig;
   __uint32 z;

   aSig = __extractFloat32Frac(a);
   aExp = __extractFloat32Exp(a);
   aSign = __extractFloat32Sign(a);
   shiftCount = aExp - 0x9E;
   if(0 <= shiftCount)
   {
      return (__bits32)0x80000000;
   }
   else if(aExp <= 0x7E)
   {
#ifndef NO_PARAMETRIC
      if(aExp | aSig)
         __float_exception_flags |= float_flag_inexact;
#endif
      return 0;
   }
   aSig = (aSig | 0x00800000) << 8;
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

static __FORCE_INLINE __int64 __float32_to_int64(__float32 a)
{
   __flag aSign;
   __int16 aExp, shiftCount;
   __bits32 aSig;
   __bits64 aSig64, aSigExtra;

   aSig = __extractFloat32Frac(a);
   aExp = __extractFloat32Exp(a);
   aSign = __extractFloat32Sign(a);
   shiftCount = 0xBE - aExp;
   if(shiftCount < 0)
   {
      __float_raise(float_flag_invalid);
      if(!aSign || ((aExp == 0xFF) && aSig))
      {
         return LIT64(0x7FFFFFFFFFFFFFFF);
      }
      return (__sbits64)LIT64(0x8000000000000000);
   }
   if(aExp)
      aSig |= 0x00800000;
   aSig64 = aSig;
   aSig64 <<= 40;
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

static __FORCE_INLINE __int64 __float32_to_int64_round_to_zero(__float32 a)
{
   __flag aSign;
   __int16 aExp, shiftCount;
   __bits32 aSig;
   __bits64 aSig64;
   __int64 z;

   aSig = __extractFloat32Frac(a);
   aExp = __extractFloat32Exp(a);
   aSign = __extractFloat32Sign(a);
   shiftCount = aExp - 0xBE;
   if(0 <= shiftCount)
   {
      if(a != 0xDF000000)
      {
         __float_raise(float_flag_invalid);
         if(!aSign || ((aExp == 0xFF) && aSig))
         {
            return LIT64(0x7FFFFFFFFFFFFFFF);
         }
      }
      return (__sbits64)LIT64(0x8000000000000000);
   }
   else if(aExp <= 0x7E)
   {
#ifndef NO_PARAMETRIC
      if(aExp | aSig)
         __float_exception_flags |= float_flag_inexact;
#endif
      return 0;
   }
   aSig64 = aSig | 0x00800000;
   aSig64 <<= 40;
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

static __FORCE_INLINE __uint64 __float32_to_uint64_round_to_zero(__float32 a)
{
   __flag aSign;
   __int16 aExp, shiftCount;
   __bits32 aSig;
   __bits64 aSig64;
   __uint64 z;

   aSig = __extractFloat32Frac(a);
   aExp = __extractFloat32Exp(a);
   aSign = __extractFloat32Sign(a);
   shiftCount = aExp - 0xBE;
   if(0 <= shiftCount)
   {
      return (__bits64)LIT64(0x8000000000000000);
   }
   else if(aExp <= 0x7E)
   {
#ifndef NO_PARAMETRIC
      if(aExp | aSig)
         __float_exception_flags |= float_flag_inexact;
#endif
      return 0;
   }
   aSig64 = aSig | 0x00800000;
   aSig64 <<= 40;
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

static __FORCE_INLINE __float64 __float32_to_float64(__float32 a)
{
   __flag aSign;
   __int16 aExp;
   __bits32 aSig;

   aSig = __extractFloat32Frac(a);
   aExp = __extractFloat32Exp(a);
   aSign = __extractFloat32Sign(a);
   if(aExp == 0xFF)
   {
      if(aSig)
         return __commonNaNToFloat64(__float32ToCommonNaN(a));
      return __packFloat64(aSign, 0x7FF, 0);
   }
   if(aExp == 0)
   {
#ifdef NO_SUBNORMALS
      return __packFloat64(aSign, 0, 0);
#else
      if(aSig == 0)
         return __packFloat64(aSign, 0, 0);
      __normalizeFloat32Subnormal(aSig, &aExp, &aSig);
#endif
      --aExp;
   }
   return __packFloat64(aSign, aExp + 0x380, ((__bits64)aSig) << 29);
}

#ifdef FLOATX80

/*----------------------------------------------------------------------------
| Returns the result of converting the single-precision floating-point value
| `a' to the extended double-precision floating-point format.  The conversion
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __floatx80 __float32_to_floatx80(__float32 a)
{
   __flag aSign;
   __int16 aExp;
   __bits32 aSig;

   aSig = __extractFloat32Frac(a);
   aExp = __extractFloat32Exp(a);
   aSign = __extractFloat32Sign(a);
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

static __FORCE_INLINE __float128 __float32_to_float128(__float32 a)
{
   __flag aSign;
   __int16 aExp;
   __bits32 aSig;

   aSig = __extractFloat32Frac(a);
   aExp = __extractFloat32Exp(a);
   aSign = __extractFloat32Sign(a);
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

static __FORCE_INLINE __float32 __float32_round_to_int(__float32 a)
{
   __flag aSign;
   __int16 aExp;
   __bits32 lastBitMask, roundBitsMask;
   __int8 roundingMode;
   __float32 z;

   aExp = __extractFloat32Exp(a);
   if(0x96 <= aExp)
   {
      if((aExp == 0xFF) && __extractFloat32Frac(a))
      {
         return __propagateFloat32NaN(a, a);
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
      aSign = __extractFloat32Sign(a);
      switch(__float_rounding_mode)
      {
         case float_round_nearest_even:
            if((aExp == 0x7E) && __extractFloat32Frac(a))
            {
               return __packFloat32(aSign, 0x7F, 0);
            }
            break;
         case float_round_down:
            return aSign ? 0xBF800000 : 0;
         case float_round_up:
            return aSign ? 0x80000000 : 0x3F800000;
      }
      return __packFloat32(aSign, 0, 0);
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
      if(__extractFloat32Sign(z) ^ (roundingMode == float_round_up))
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
| Returns the result of adding the absolute values of the single-precision
| floating-point values `a' and `b'.  If `zSign' is 1, the sum is negated
| before being returned.  `zSign' is ignored if the result is a NaN.
| The addition is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float32 __addFloat32Sigs(__float32 a, __float32 b, __flag zSign)
{
   __int16 aExp, bExp, zExp;
   __bits32 aSig, bSig, zSig;
   __int16 expDiff;

   aSig = __extractFloat32Frac(a);
   aExp = __extractFloat32Exp(a);
   bSig = __extractFloat32Frac(b);
   bExp = __extractFloat32Exp(b);
   expDiff = aExp - bExp;
   aSig <<= 6;
   bSig <<= 6;
   if(0 < expDiff)
   {
      if(aExp == 0xFF)
      {
         if(aSig)
            return __propagateFloat32NaN(a, b);
         return a;
      }
      if(bExp == 0)
      {
         --expDiff;
      }
      else
      {
         bSig |= 0x20000000;
      }
      __shift32RightJamming(bSig, expDiff, &bSig);
      zExp = aExp;
   }
   else if(expDiff < 0)
   {
      if(bExp == 0xFF)
      {
         if(bSig)
            return __propagateFloat32NaN(a, b);
         return __packFloat32(zSign, 0xFF, 0);
      }
      if(aExp == 0)
      {
         ++expDiff;
      }
      else
      {
         aSig |= 0x20000000;
      }
      __shift32RightJamming(aSig, -expDiff, &aSig);
      zExp = bExp;
   }
   else
   {
      if(aExp == 0xFF)
      {
         if(aSig | bSig)
            return __propagateFloat32NaN(a, b);
         return a;
      }
      if(aExp == 0)
         return __packFloat32(zSign, 0, (aSig + bSig) >> 6);
      zSig = 0x40000000 + aSig + bSig;
      zExp = aExp;
      goto roundAndPack;
   }
   aSig |= 0x20000000;
   zSig = (aSig + bSig) << 1;
   --zExp;
   if((__sbits32)zSig < 0)
   {
      zSig = aSig + bSig;
      ++zExp;
   }
roundAndPack:
   return __roundAndPackFloat32(zSign, zExp, zSig);
}

/*----------------------------------------------------------------------------
| Returns the result of subtracting the absolute values of the single-
| precision floating-point values `a' and `b'.  If `zSign' is 1, the
| difference is negated before being returned.  `zSign' is ignored if the
| result is a NaN.  The subtraction is performed according to the IEC/IEEE
| Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float32 __subFloat32Sigs(__float32 a, __float32 b, __flag zSign)
{
   __int16 aExp, bExp, zExp;
   __bits32 aSig, bSig, zSig;
   __int16 expDiff;

   aSig = __extractFloat32Frac(a);
   aExp = __extractFloat32Exp(a);
   bSig = __extractFloat32Frac(b);
   bExp = __extractFloat32Exp(b);
   expDiff = aExp - bExp;
   aSig <<= 7;
   bSig <<= 7;
   if(0 < expDiff)
      goto aExpBigger;
   if(expDiff < 0)
      goto bExpBigger;
   if(aExp == 0xFF)
   {
      if(aSig | bSig)
         return __propagateFloat32NaN(a, b);
      __float_raise(float_flag_invalid);
      return __float32_default_nan;
   }
   if(aExp == 0)
   {
      aExp = 1;
      bExp = 1;
   }
   if(bSig < aSig)
      goto aBigger;
   if(aSig < bSig)
      goto bBigger;
   return __packFloat32(__float_rounding_mode == float_round_down, 0, 0);
bExpBigger:
   if(bExp == 0xFF)
   {
      if(bSig)
         return __propagateFloat32NaN(a, b);
      return __packFloat32(zSign ^ 1, 0xFF, 0);
   }
   if(aExp == 0)
   {
      ++expDiff;
   }
   else
   {
      aSig |= 0x40000000;
   }
   __shift32RightJamming(aSig, -expDiff, &aSig);
   bSig |= 0x40000000;
bBigger:
   zSig = bSig - aSig;
   zExp = bExp;
   zSign ^= 1;
   goto normalizeRoundAndPack;
aExpBigger:
   if(aExp == 0xFF)
   {
      if(aSig)
         return __propagateFloat32NaN(a, b);
      return a;
   }
   if(bExp == 0)
   {
      --expDiff;
   }
   else
   {
      bSig |= 0x40000000;
   }
   __shift32RightJamming(bSig, expDiff, &bSig);
   aSig |= 0x40000000;
aBigger:
   zSig = aSig - bSig;
   zExp = aExp;
normalizeRoundAndPack:
   --zExp;
   return __normalizeRoundAndPackFloat32(zSign, zExp, zSig);
}

#define FP_CLS_ZERO 0U
#define FP_CLS_NORMAL 1U
#define FP_CLS_INF 2U
#define FP_CLS_NAN 3U

#ifdef SIMPLE_CLZ25
static __FORCE_INLINE __bits8 __CLZ25(__bits32 v)
{
   __bits32 r; // result of log2(v) will go here
   __bits32 shift;
   r = (v > 0xFFFF) << 4;
   v >>= r;
   shift = (v > 0xFF) << 3;
   v >>= shift;
   r |= shift;
   shift = (v > 0xF) << 2;
   v >>= shift;
   r |= shift;
   shift = (v > 0x3) << 1;
   v >>= shift;
   r |= shift;
   r |= (v >> 1);

   return 24 - r;
}
#else
static __FORCE_INLINE __bits8 __CLZ25(__bits32 v)
{
   __bits8 res;
   count_leading_zero_macro(25, v, res);
   return res;
}
#endif

static __FORCE_INLINE __float32 __addsubFloat32_old(__float32 a, __float32 b, __flag bSign)
{
   __bits16 eRc1, eRn0, eRn_0, eRc, eRf0, eRf1, eRf, eRn;
   __bits32 aSig, bSig, bSigOrig, tmp_swap, fAc1, fBc1, fRc0, fRc1, fRc, fBf1, fBf2_low, fBf2, fAf3, fRf0, fRf0_right, fRf0_t_right, fRf1, fRf_0, fRf, shift_0, fRn_0, fRn, fRr;
   __int16 expDiff;
   _Bool expDiffSign, eRnSign;
   __bits8 aExp, bExp, expDiff8, eRn8;
   __bits8 a_c, b_c, nZeros, lb;
   _Bool a_c_zero, b_c_zero, a_c_inf, b_c_inf, a_c_nan, b_c_nan, a_c_normal, b_c_normal, swap, sAB, aSign, roundc, roundf, eZero, close, sRn, eMax, eMin, eTest0, eTest1, SigDiffSign, sb;
   __int8 eRf1_correction;
   aSign = __extractFloat32Sign(a);
   aSig = __extractFloat32Frac(a);
   aExp = __extractFloat32Exp(a);
   bSig = __extractFloat32Frac(b);
   bExp = __extractFloat32Exp(b);
   a_c_zero = aExp == 0 && aSig == 0;
   a_c_inf = aExp == 0xFF && aSig == 0;
   a_c_nan = aExp == 0xFF && aSig != 0;
   a_c_normal = aExp != 0 && aExp != 0xFF;
   a_c = ((a_c_zero << 1 | a_c_zero) & FP_CLS_ZERO) | ((a_c_normal << 1 | a_c_normal) & FP_CLS_NORMAL) | ((a_c_inf << 1 | a_c_inf) & FP_CLS_INF) | ((a_c_nan << 1 | a_c_nan) & FP_CLS_NAN);

   b_c_zero = bExp == 0 && bSig == 0;
   b_c_inf = bExp == 0xFF && bSig == 0;
   b_c_nan = bExp == 0xFF && bSig != 0;
   b_c_normal = bExp != 0 && bExp != 0xFF;
   b_c = ((b_c_zero << 1 | b_c_zero) & FP_CLS_ZERO) | ((b_c_normal << 1 | b_c_normal) & FP_CLS_NORMAL) | ((b_c_inf << 1 | b_c_inf) & FP_CLS_INF) | ((b_c_nan << 1 | b_c_nan) & FP_CLS_NAN);
   expDiff = aExp - bExp;
   expDiffSign = (expDiff >> 8) & 1;
   expDiff8 = expDiff;
   sAB = aSign ^ bSign;
   swap = (sAB && aSig < bSig && expDiff8 == 0 && expDiffSign == 0 && a_c == b_c) || ((expDiffSign) && a_c == b_c) || a_c < b_c;
   expDiff8 = (swap ? -expDiff8 : expDiff8);
   tmp_swap = b;
   b = swap ? a : b;
   a = swap ? tmp_swap : a;
   aSign = swap ? bSign : aSign;
   close = ((expDiff8 >> 1) == 0) && sAB;
   aSig = __extractFloat32Frac(a);
   bSigOrig = __extractFloat32Frac(b);
   fAc1 = (aSig | 0x00800000) << 1;
   fAf3 = fAc1 << 2;
   BIT_RESIZE(fAf3, 27);
   fBf1 = bSigOrig | 0x00800000;
   fBc1 = (expDiff8 & 1) ? fBf1 : (fBf1 << 1);
   fRc0 = fAc1 - fBc1;
   BIT_RESIZE(fRc0, 26);
   fRc1 = fRc0;
   BIT_RESIZE(fRc1, 25);
   eRc1 = __extractFloat32Exp(a);
   BIT_RESIZE(eRc1, 9);
   // nZeros = __CLZ25(fRc1);
   // shift_0 = fRc1 << nZeros;
   count_leading_zero_macro_lshift(25, fRc1, nZeros, shift_0);
   roundc = (fRc1 & 1) & ((fRc1 >> 1) & 1) & ((fRc1 >> 24) & 1);
   fRn_0 = shift_0 >> 1;
   eRn0 = eRc1 - nZeros;
   eZero = (eRn0 >> 8) & 1;
   eRn_0 = eZero ? 0 : eRn0;
   fRr = ((fRc1 & ((1 << 24) - 1)) >> 2) + 1;
   eRc = roundc ? eRc1 + ((fRr >> 22) & 1) : eRn_0;
   BIT_RESIZE(eRc, 9);
   fRc = roundc ? ((fRr & ((1 << 22) - 1)) | (1 << 22)) << 1 : fRn_0;
   BIT_RESIZE(fRc, 24);
   fBf2 = expDiff8 >= 27 ? 0 : (fBf1 << 3) >> expDiff8;
   fBf2_low = expDiff8 >= 27 ? fBf1 : (fBf1 << 3) << (27 - expDiff8);
   BIT_RESIZE(fBf2, 27);
   BIT_RESIZE(fBf2_low, 27);
   sb = fBf2_low != 0;
   fRf0_t_right = (~fBf2) + !sb;
   fRf0_right = sAB ? fRf0_t_right : fBf2;
   fRf0 = fAf3 + fRf0_right;
   BIT_RESIZE(fRf0, 28);
   eRf0 = eRc1;
   BIT_RESIZE(eRf0, 9);
   lb = (fRf0 >> 26) & 3;
   fRf1 = lb == 1 ? ((fRf0 >> 1) | (fRf0 & 1)) : (lb == 0 ? fRf0 : ((fRf0 >> 2) | ((fRf0 & 2) >> 1) | (fRf0 & 1)));
   BIT_RESIZE(fRf1, 26);
   eRf1_correction = lb == 1 ? 0 : (lb == 0 ? -1 : +1);
   roundf = ((fRf1 & 2) >> 1) & (((fRf1 & 4) >> 2) | (fRf1 & 1) | sb);
   fRf_0 = (fRf1 >> 2) + roundf;
   BIT_RESIZE(fRf_0, 25);
   eRf1 = eRf1_correction + ((fRf_0 >> 24) & 1);
   BIT_RESIZE(eRf1, 9);
   eRf = eRf0 + eRf1;
   BIT_RESIZE(eRf, 9);
   fRf = ((fRf_0 & (1 << 24)) >> 1) | (fRf_0 & ((1 << 24) - 1));
   BIT_RESIZE(fRf, 24);
   eRn = close ? eRc : eRf;
   eRn8 = eRn & 0xFF;
   eRnSign = (eRn >> 8) & 1;
   fRn = close ? fRc : fRf;
   BIT_RESIZE(fRn, 24);
   sRn = aSign;
   eMax = eRn8 == 0xFF;
   eMin = eRn8 == 0;
   eTest1 = (eMax & (0 == (eRnSign))) | ((eRnSign) & (((eRn8 >> 7) & 1) == 0));
   eTest0 = (eMin & (0 == (eRnSign))) | ((eRnSign) & ((eRn8 >> 7) & 1)) | (0 == ((fRn >> 23) & 1));

   if(a_c_normal && b_c_normal)
   {
      if(!eTest1 && eTest0)
         return (sRn & 1) << 31;
      else if(eTest1 && !eTest0)
         return __float32_default_nan;
      else
         return (sRn << 31) | (eRn8 << 23) | (fRn & ((1 << 23) - 1));
   }
   else
   {
      if(a_c_inf && b_c_inf)
      {
         if(sAB)
            return __float32_default_nan;
         else
            return (sRn & 1) << 31 | 0xFF << 23;
      }
      else
      {
         if(eRc1 == 0xFF && aSig == 0)
            return (sRn & 1) << 31 | 0xFF << 23;
         else if(eRc1 != 0xFF)
            return (sRn << 31) | ((eRc1 & 255) << 23) | (aSig & ((1 << 23) - 1));
         else
            return __float32_default_nan;
      }
   }
}

//#define VOLATILE_DEF volatile
#define VOLATILE_DEF

static __FORCE_INLINE __float32 __addsubFloat32(__float32 a, __float32 b, __flag bSign)
{
   VOLATILE_DEF __bits32 aSig, bSig, shift_0;
   VOLATILE_DEF __bits8 aExp, bExp, expDiff8;
   VOLATILE_DEF __bits8 nZeros;
   VOLATILE_DEF _Bool a_c_nan, b_c_nan, a_c_normal, b_c_normal, tmp_c_normal, swap, sAB, aSign;
   VOLATILE_DEF __bits32 abs_a, abs_b;
   VOLATILE_DEF __bits32 fA, fB, fB_shifted;
#ifndef NO_ROUNDING
   VOLATILE_DEF _Bool sb, LSB_bit, Guard_bit, Round_bit, Sticky_bit, round;
   VOLATILE_DEF __bits32 fB_shifted_low, fBleft_shifted;
#endif
   VOLATILE_DEF _Bool ge_32;
   VOLATILE_DEF _Bool tmp_sign;
   VOLATILE_DEF __bits8 tmp_exp;
   VOLATILE_DEF __bits32 tmp_sig;
   VOLATILE_DEF _Bool subnormal_exp_correction;
   VOLATILE_DEF __bits32 fB_shifted1;
   VOLATILE_DEF __bits32 fR0;
   VOLATILE_DEF _Bool R_c_zero;
   VOLATILE_DEF __bits8 RExp0, RExp1;
   VOLATILE_DEF __bits32 RSig0, RSig1, RSig2, RSig3;
   VOLATILE_DEF __bits32 RExp0RSig1, Rrounded;
   VOLATILE_DEF _Bool overflow_to_infinite;
   VOLATILE_DEF _Bool aExp255, bExp255;

#ifdef NO_ROUNDING
#define FRAC_SHIFT 0
#define FRAC_FULL_BW 25
#define FRAC_ALMOST_BW 24
#else
#define FRAC_SHIFT 2
#define FRAC_FULL_BW 27
#define FRAC_ALMOST_BW 26
#endif

   aSign = __extractFloat32Sign(a);
   aSig = __extractFloat32Frac(a);
   aExp = __extractFloat32Exp(a);
   bSig = __extractFloat32Frac(b);
   bExp = __extractFloat32Exp(b);
   _Bool aSig_null = aSig == 0;
   _Bool bSig_null = bSig == 0;
   _Bool aExp_null = aExp == 0;
   _Bool bExp_null = bExp == 0;
   aExp255 = aExp == 0xFF;
   a_c_nan = aExp255 && !aSig_null;
   a_c_normal = !aExp_null /*&& !aExp255*/; /// not really needed the second condition
   bExp255 = bExp == 0xFF;
   b_c_nan = bExp255 && !bSig_null;
   b_c_normal = !bExp_null /*&& !bExp255*/; /// not really needed the second condition
   sAB = aSign ^ bSign;

   abs_a = a;
   BIT_RESIZE(abs_a, 31);
   abs_b = b;
   BIT_RESIZE(abs_b, 31);

   swap = (a_c_nan == b_c_nan && abs_a < abs_b) || a_c_nan < b_c_nan;

#ifdef NO_SUBNORMALS
   subnormal_exp_correction = 0;
#else
   subnormal_exp_correction = (aExp_null && !aSig_null) ^ (bExp_null && !bSig_null);
#endif

   tmp_exp = bExp;
   bExp = swap ? aExp : bExp;
   aExp = swap ? tmp_exp : aExp;

   expDiff8 = aExp - bExp - subnormal_exp_correction;

   tmp_sig = bSig;
   bSig = swap ? aSig : bSig;
   aSig = swap ? tmp_sig : aSig;

   tmp_sign = bSign;
   // bSign = swap ? aSign : bSign;
   aSign = swap ? tmp_sign : aSign;

   tmp_c_normal = b_c_normal;
   b_c_normal = swap ? a_c_normal : b_c_normal;
   a_c_normal = swap ? tmp_c_normal : a_c_normal;

   fA = (aSig | (((__bits32)a_c_normal) << 23)) << FRAC_SHIFT;
   fB = (bSig | (((__bits32)b_c_normal) << 23)) << FRAC_SHIFT;

   ge_32 = SELECT_BIT(expDiff8, 5) | SELECT_BIT(expDiff8, 6) | SELECT_BIT(expDiff8, 7);
#ifdef NO_ROUNDING
   fB_shifted = fB >> VAL_RESIZE((expDiff8 | (__bits32)(((((__sbits32)ge_32) << 31) >> 31))), 5);
#else
   __bits64 fB_shift = ((__bits64)fB) << 32;
   fB_shift = fB_shift >> VAL_RESIZE((expDiff8 | (__bits32)(((((__sbits32)ge_32) << 31) >> 31))), 5);
   fB_shifted = fB_shift >> 32;
   fBleft_shifted = fB_shift;

   fB_shifted_low = fBleft_shifted;
#endif
   BIT_RESIZE(fB_shifted, FRAC_ALMOST_BW);

#ifndef NO_ROUNDING
   sb = fB_shifted_low != 0;
#endif
   fB_shifted1 = ((__bits32)((((__sbits32)sAB) << 31) >> 31)) ^ fB_shifted;
   BIT_RESIZE(fB_shifted1, FRAC_FULL_BW);

#ifdef NO_ROUNDING
   fR0 = fA + fB_shifted1 + sAB;
#else
   fR0 = fA + fB_shifted1 + (sAB && (!sb));
#endif
   BIT_RESIZE(fR0, FRAC_FULL_BW);
   count_leading_zero_macro_lshift(FRAC_FULL_BW, fR0, nZeros, shift_0);

   R_c_zero = nZeros == VAL_RESIZE((~0ULL), 5);
   overflow_to_infinite = aExp == 254 && (fR0 >> FRAC_ALMOST_BW) & 1;

#ifdef NO_SUBNORMALS
   R_c_zero = R_c_zero || aExp < nZeros;
   RExp0 = R_c_zero ? 0 : aExp - nZeros + 1;
   RSig0 = shift_0;
#else
   RExp0 = R_c_zero || aExp < nZeros ? ((aExp_null) && (bExp_null) && nZeros == 1) : aExp - nZeros + 1;
   RSig0 = aExp < nZeros ? ((aExp_null) && (bExp_null) ? (fR0 << 1) : (fR0 << aExp)) : shift_0;
#endif
#ifndef NO_ROUNDING
   LSB_bit = SELECT_BIT(RSig0, 3);
   Guard_bit = SELECT_BIT(RSig0, 2);
   Round_bit = SELECT_BIT(RSig0, 1);
   Sticky_bit = SELECT_BIT(RSig0, 0) | sb;
   round = Guard_bit & (LSB_bit | Round_bit | Sticky_bit);
#endif

   RSig1 = VAL_RESIZE(RSig0 >> (FRAC_SHIFT + 1), 23);

   RExp0RSig1 = (((__bits32)RExp0) << 23) | RSig1;

#ifdef NO_ROUNDING
   Rrounded = RExp0RSig1;
#else
   Rrounded = RExp0RSig1 + round;
#endif

   RExp1 = aExp255 || bExp255 ? 0xFF : VAL_RESIZE(Rrounded >> 23, 8);

   RSig2 = R_c_zero || aExp255 || bExp255 || overflow_to_infinite ? 0 : VAL_RESIZE(Rrounded, 23);

   aSign = aSign && (!R_c_zero || !sAB);

#ifdef NO_SIGNALLING
   RSig3 = (((__bits32)(a_c_nan || b_c_nan || (sAB && aExp255 && bExp255))) << 22) | RSig2;
#else
   RSig3 = a_c_nan || b_c_nan || (sAB && aExp255 && bExp255) ? 1 << 22 | (aSig & ((1 << 22) - 1)) : RSig2;
#endif
   return (((__bits32)aSign) << 31) | (((__bits32)RExp1) << 23) | RSig3;
}

/*----------------------------------------------------------------------------
| Returns the result of adding the single-precision floating-point values `a'
| and `b'.  The operation is performed according to the IEC/IEEE Standard for
| Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float32 __float32_add(__float32 a, __float32 b)
{
#ifndef DEFAULT
   __flag bSign = __extractFloat32Sign(b);
   return __addsubFloat32(a, b, bSign);
#else
   __flag aSign, bSign;
   aSign = __extractFloat32Sign(a);
   bSign = __extractFloat32Sign(b);
   if(aSign == bSign)
   {
      return __addFloat32Sigs(a, b, aSign);
   }
   else
   {
      return __subFloat32Sigs(a, b, aSign);
   }
#endif
}

/*----------------------------------------------------------------------------
| Returns the result of subtracting the single-precision floating-point values
| `a' and `b'.  The operation is performed according to the IEC/IEEE Standard
| for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float32 __float32_sub(__float32 a, __float32 b)
{
#ifndef DEFAULT
   __flag bSign = __extractFloat32Sign(b) ^ 1;
   return __addsubFloat32(a, b, bSign);
#else
   __flag aSign, bSign;
   aSign = __extractFloat32Sign(a);
   bSign = __extractFloat32Sign(b);
   if(aSign == bSign)
   {
      return __subFloat32Sigs(a, b, aSign);
   }
   else
   {
      return __addFloat32Sigs(a, b, aSign);
   }
#endif
}

/*----------------------------------------------------------------------------
| Returns the result of multiplying the single-precision floating-point values
| `a' and `b'.  The operation is performed according to the IEC/IEEE Standard
| for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/
typedef unsigned int SF_USItype __attribute__((mode(SI)));
typedef unsigned int SF_UDItype __attribute__((mode(DI)));

static __FORCE_INLINE __float32 __float32_mul(__float32 a, __float32 b)
{
   VOLATILE_DEF __flag aSign, bSign, zSign;
   VOLATILE_DEF __int16 aExp, bExp, zExp;
   VOLATILE_DEF __bits32 aSig, bSig;
   VOLATILE_DEF __bits64 zSig64;
   VOLATILE_DEF __bits32 zSig;
   VOLATILE_DEF __bits32 zSigminus1;

   aSig = __extractFloat32Frac(a);
   aExp = __extractFloat32Exp(a);
   aSign = __extractFloat32Sign(a);
   bSig = __extractFloat32Frac(b);
   bExp = __extractFloat32Exp(b);
   bSign = __extractFloat32Sign(b);
   zSign = aSign ^ bSign;

#ifndef DEFAULT
   VOLATILE_DEF __bits8 a_c, b_c, z_c;
   VOLATILE_DEF _Bool a_c_zero, b_c_zero, a_c_inf, b_c_inf, a_c_nan, b_c_nan, a_c_normal, b_c_normal;
   a_c_zero = aExp == 0 && aSig == 0;
   a_c_inf = aExp == 0xFF && aSig == 0;
   a_c_nan = aExp == 0xFF && aSig != 0;
   a_c_normal = aExp != 0 && aExp != 0xFF;
   a_c = ((a_c_zero << 1 | a_c_zero) & FP_CLS_ZERO) | ((a_c_normal << 1 | a_c_normal) & FP_CLS_NORMAL) | ((a_c_inf << 1 | a_c_inf) & FP_CLS_INF) | ((a_c_nan << 1 | a_c_nan) & FP_CLS_NAN);

   b_c_zero = bExp == 0 && bSig == 0;
   b_c_inf = bExp == 0xFF && bSig == 0;
   b_c_nan = bExp == 0xFF && bSig != 0;
   b_c_normal = bExp != 0 && bExp != 0xFF;
   b_c = ((b_c_zero << 1 | b_c_zero) & FP_CLS_ZERO) | ((b_c_normal << 1 | b_c_normal) & FP_CLS_NORMAL) | ((b_c_inf << 1 | b_c_inf) & FP_CLS_INF) | ((b_c_nan << 1 | b_c_nan) & FP_CLS_NAN);

   z_c = ((a_c >> 1 | b_c >> 1) << 1) | (((a_c >> 1) & (a_c & 1)) | ((b_c >> 1) & (b_c & 1)) | ((a_c & 1) & (b_c & 1)) | (1 & (~(a_c >> 1)) & ((~a_c) & 1) & (b_c >> 1)) | (1 & (~(b_c >> 1)) & ((~b_c) & 1) & (a_c >> 1)));
#else

   if(aExp == 0xFF || bExp == 0xFF || bExp == 0 || aExp == 0)
   {
      if(aExp == 0xFF)
      {
         if(aSig || ((bExp == 0xFF) && bSig))
         {
            return __propagateFloat32NaN(a, b);
         }
         if((bExp | bSig) == 0)
         {
            __float_raise(float_flag_invalid);
            return __float32_default_nan;
         }
         return __packFloat32(zSign, 0xFF, 0);
      }
      if(bExp == 0xFF)
      {
         if(bSig)
            return __propagateFloat32NaN(a, b);
         if((aExp | aSig) == 0)
         {
            __float_raise(float_flag_invalid);
            return __float32_default_nan;
         }
         return __packFloat32(zSign, 0xFF, 0);
      }
      if(aExp == 0)
      {
#ifdef NO_SUBNORMALS
         return __packFloat32(zSign, 0, 0);
#else
         if(aSig == 0)
            return __packFloat32(zSign, 0, 0);
         __normalizeFloat32Subnormal(aSig, &aExp, &aSig);
#endif
      }
      if(bExp == 0)
      {
#ifdef NO_SUBNORMALS
         return __packFloat32(zSign, 0, 0);
#else
         if(bSig == 0)
            return __packFloat32(zSign, 0, 0);
         __normalizeFloat32Subnormal(bSig, &bExp, &bSig);
#endif
      }
   }
#endif

#ifndef DEFAULT
   {
      VOLATILE_DEF __bits16 expSumPreSub = aExp + bExp;
      VOLATILE_DEF __bits16 bias = 127;
      VOLATILE_DEF __bits16 expSum = expSumPreSub - bias;
      VOLATILE_DEF __bits16 expPostNorm;
      VOLATILE_DEF SF_UDItype sigProd, sigProdExt;
      VOLATILE_DEF _Bool norm, expSigOvf0, expSigOvf2;
#ifndef NO_ROUNDING
      VOLATILE_DEF _Bool sticky, guard, round, expSigOvf1;
#endif
      VOLATILE_DEF __bits32 expSig, expSigPostRound;
      VOLATILE_DEF __bits8 excPostNorm;
      aSig = (aSig | 0x00800000);
      bSig = (bSig | 0x00800000);
      sigProd = (SF_UDItype)(SF_USItype)(aSig) * (SF_USItype)(bSig);
      norm = (sigProd >> 47) & 1;
      expPostNorm = expSum + norm;
      sigProdExt = norm ? (sigProd & ((1ULL << 47) - 1)) << 1 : (sigProd & ((1ULL << 46) - 1)) << 2;
      expSig = (expPostNorm << 23) | ((sigProdExt >> 25) & ((1 << 23) - 1));
      expSigOvf0 = (expPostNorm >> 9) & 1;
#ifndef NO_ROUNDING
      sticky = (sigProdExt >> 24) & 1;
      guard = (sigProdExt & ((1 << 24) - 1)) != 0;
      round = sticky & ((guard & !((sigProdExt >> 25) & 1)) | ((sigProdExt >> 25) & 1));
      expSigPostRound = expSig + round;
      expSigOvf1 = round & (expSig == ((__bits32)-1));
      expSigOvf2 = expSigOvf0 ^ expSigOvf1;
#else
      expSigPostRound = expSig;
      expSigOvf2 = expSigOvf0;
#endif
      excPostNorm = (expSigOvf2 << 1) | ((expSigPostRound >> 31) & 1);
      zSig = (((__bits32)zSign) << 31) | (expSigPostRound & ((1U << 31) - 1));
      if(z_c == FP_CLS_NORMAL)
         z_c = excPostNorm == 0 ? FP_CLS_NORMAL : (excPostNorm == 1 ? FP_CLS_INF : FP_CLS_ZERO);

      if(z_c == FP_CLS_NORMAL)
         return zSig;
      else if(z_c == FP_CLS_ZERO)
         return __packFloat32(zSign, 0, 0);
      else if(z_c == FP_CLS_NAN)
         return __float32_default_nan;
      else
         return __packFloat32(zSign, 0xFF, 0);
   }
#else
   zExp = aExp + bExp - 0x7F;
   aSig = (aSig | 0x00800000) << 7;
   bSig = (bSig | 0x00800000) << 8;
   zSig = _umulh32(aSig, bSig);
   //__shift64RightJamming( ( (__bits64) aSig ) * bSig, 32, &zSig64 );
   // zSig = zSig64;
   if(0 <= (__sbits32)(zSig << 1))
   {
      zSig <<= 1;
      --zExp;
   }
   return __roundAndPackFloat32(zSign, zExp, zSig);
#endif
}

__float32 __float32_muladd(__float32 uiA, __float32 uiB, __float32 uiC)
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
   signA = __extractFloat32Sign(uiA);
   expA = __extractFloat32Exp(uiA);
   sigA = __extractFloat32Frac(uiA);
   signB = __extractFloat32Sign(uiB);
   expB = __extractFloat32Exp(uiB);
   sigB = __extractFloat32Frac(uiB);
   signC = __extractFloat32Sign(uiC);
   expC = __extractFloat32Exp(uiC);
   sigC = __extractFloat32Frac(uiC);
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
      count_leading_zero_macro_lshift(24, sigA, nZeros, shift_0);
      expA = 1 - nZeros;
      sigA = shift_0;
   }
   if(!expB)
   {
      if(!sigB)
         goto zeroProd;
      __int8 nZeros;
      __bits32 shift_0;
      count_leading_zero_macro_lshift(24, sigB, nZeros, shift_0);
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
      count_leading_zero_macro_lshift(24, sigC, nZeros, shift_0);
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
         __bits64 softfloat_shiftRightJam64 = (dist < 63) ? sigProd >> dist | ((__bits64)(sigProd << (-dist & 63)) != 0) : (sigProd != 0);
         expZ = expC;
         sigZ = sigC + softfloat_shiftRightJam64;
      }
      else
      {
         __bits64 a = (__bits64)sigC << 32;
         __bits64 softfloat_shiftRightJam64 = (expDiff < 63) ? a >> expDiff | ((__bits64)(a << (-expDiff & 63)) != 0) : (a != 0);
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
         __bits64 softfloat_shiftRightJam64 = (dist < 63) ? sigProd >> dist | ((__bits64)(sigProd << (-dist & 63)) != 0) : (sigProd != 0);
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
         __bits64 softfloat_shiftRightJam64 = (expDiff < 63) ? sig64C >> expDiff | ((__bits64)(sig64C << (-expDiff & 63)) != 0) : (sig64C != 0);
         expZ = expProd;
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
   return __roundAndPackFloat32(signZ, expZ, sigZ);
   /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
propagateNaN_ABC:
   uiZ = __propagateFloat32NaN(uiA, uiB);
   goto propagateNaN_ZC;
   /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
infProdArg:
   if(magBits)
   {
      uiZ = __packFloat32(signProd, 0xFF, 0);
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
   uiZ = __propagateFloat32NaN(uiZ, uiC);
   goto uiZ;
   /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
zeroProd:
   uiZ = uiC;
   if(!(expC | sigC) && (signProd != signC))
   {
   completeCancellation:
      uiZ = __packFloat32((__float_rounding_mode == float_round_down), 0, 0);
   }
uiZ:
   return uiZ;
}
/*----------------------------------------------------------------------------
| Returns the result of dividing the single-precision floating-point value `a'
| by the corresponding value `b'.  The operation is performed according to the
| IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

#ifndef UNROLL_FACTOR_F32_DIV
#define UNROLL_FACTOR_F32_DIV 1
#endif

#define LOOP_BODY_F32_DIV(z, n, data)                  \
   current_sel = (((current >> 22) & 15) << 1) | MsbB; \
   q_i0 = (0xF1FFFF6C >> current_sel) & 1;             \
   q_i1 = (0xFE00FFD0 >> current_sel) & 1;             \
   q_i2 = SELECT_BIT(current_sel, 4);                  \
   nq_i2 = !q_i2;                                      \
   /*q_i = tableR4[current_sel];*/                     \
   q_i = (q_i2 << 2) | (q_i1 << 1) | q_i0;             \
   positive |= (q_i1 << 1) | q_i0;                     \
   positive <<= 2;                                     \
   negative |= q_i2 << 1;                              \
   negative <<= 2;                                     \
   switch(q_i)                                         \
   {                                                   \
      case 1:                                          \
         w = nbSig;                                    \
         break;                                        \
      case 7:                                          \
         w = bSig;                                     \
         break;                                        \
      case 2:                                          \
         w = nbSigx2;                                  \
         break;                                        \
      case 6:                                          \
         w = bSigx2;                                   \
         break;                                        \
      case 3:                                          \
         w = nbSigx3;                                  \
         break;                                        \
      case 5:                                          \
         w = bSigx3;                                   \
         break;                                        \
      default: /*case 0: case 4:*/                     \
         w = 0;                                        \
         break;                                        \
   }                                                   \
   current = (current << 1) + w;                       \
   BIT_RESIZE(current, 25);                            \
   current <<= 1;

static __FORCE_INLINE __float32 __float32_divSRT4(__float32 a, __float32 b)
{
   // static const __bits8 tableR4[]= {0, 0, 1, 1, 2, 1, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 7, 7, 7, 7};
   VOLATILE_DEF _Bool aSign, bSign, zSign, q_i2, q_i1, q_i0, nq_i2;
   VOLATILE_DEF _Bool MsbB = SELECT_BIT(b, 22), correction;
   VOLATILE_DEF __int16 aExp, bExp, zExp;
   VOLATILE_DEF __bits32 aSig, bSig, nbSig, bSigx2, nbSigx2, zSig1, zSig0;
   VOLATILE_DEF __bits32 zExpSig;
   VOLATILE_DEF __bits32 bSigx3, nbSigx3, current, w, positive = 0, negative = 0;
   VOLATILE_DEF __bits8 current_sel, q_i, index;
#ifndef NO_ROUNDING
   VOLATILE_DEF _Bool LSB_bit, Guard_bit, Round_bit, round;
#endif

   aSig = __extractFloat32Frac(a);
   aExp = __extractFloat32Exp(a);
   aSign = __extractFloat32Sign(a);
   bSig = __extractFloat32Frac(b);
   bExp = __extractFloat32Exp(b);
   bSign = __extractFloat32Sign(b);
   zSign = aSign ^ bSign;
   _Bool aExp_null = aExp == 0;
   _Bool bExp_null = bExp == 0;
   __bits8 a_c, b_c, z_c;
   _Bool a_c_zero, b_c_zero, a_c_inf, b_c_inf, a_c_nan, b_c_nan, a_c_normal, b_c_normal;
#ifndef NO_SUBNORMALS
   a_c_zero = aExp_null && aSig == 0;
#else
   a_c_zero = aExp_null;
#endif
   a_c_inf = aExp == 0xFF && aSig == 0;
   a_c_nan = aExp == 0xFF && aSig != 0;
   a_c_normal = aExp != 0xFF && !a_c_zero;
   a_c = ((a_c_zero << 1 | a_c_zero) & FP_CLS_ZERO) | ((a_c_normal << 1 | a_c_normal) & FP_CLS_NORMAL) | ((a_c_inf << 1 | a_c_inf) & FP_CLS_INF) | ((a_c_nan << 1 | a_c_nan) & FP_CLS_NAN);

#ifndef NO_SUBNORMALS
   b_c_zero = bExp_null && bSig == 0;
#else
   b_c_zero = bExp_null;
#endif
   b_c_inf = bExp == 0xFF && bSig == 0;
   b_c_nan = bExp == 0xFF && bSig != 0;
   b_c_normal = bExp != 0xFF && !b_c_zero;
   b_c = ((b_c_zero << 1 | b_c_zero) & FP_CLS_ZERO) | ((b_c_normal << 1 | b_c_normal) & FP_CLS_NORMAL) | ((b_c_inf << 1 | b_c_inf) & FP_CLS_INF) | ((b_c_nan << 1 | b_c_nan) & FP_CLS_NAN);
   /*
   00 00 11
   00 01 00
   00 10 00
   00 11 11
   01 00 10
   01 01 01
   01 10 00
   01 11 11
   10 00 10
   10 01 10
   10 10 11
   10 11 11
   11 00 11
   11 01 11
   11 10 11
   11 11 11

    0  1  3  2
    4  5  7  6
   12 13 15 14
    8  9 11 10

      00 01 11 10
   00 1  0  1  0
   01 1  0  1  0
   11 1  1  1  1
   10 1  1  1  1
   I1 = a + c'd' + cd

      00 01 11 10
   00 1  0  1  0
   01 0  1  1  0
   11 1  1  1  1
   10 0  0  1  1
   I0 = ab + cd + ac + bd + a'b'c'd'
   */
   z_c = ((a_c >> 1 | (1 & (~(b_c >> 1)) & (~(b_c & 1))) | (1 & (b_c >> 1) & b_c)) << 1) |
         ((1 & (a_c >> 1) & a_c) | (1 & (b_c >> 1) & b_c) | (1 & (a_c >> 1) & (b_c >> 1)) | (1 & a_c & b_c) | (1 & (~(a_c >> 1)) & (~(a_c & 1)) & (~(b_c >> 1)) & (~(b_c & 1))));

#ifndef NO_SUBNORMALS
   if(aExp_null && !a_c_zero)
   {
      unsigned int subnormal_lz, mshifted;
      count_leading_zero_macro_lshift(23, aSig, subnormal_lz, mshifted);
      aExp = -subnormal_lz;
      aSig = SELECT_RANGE(mshifted, 21, 0) << 1;
   }
   if(bExp_null && !b_c_zero)
   {
      unsigned int subnormal_lz, mshifted;
      count_leading_zero_macro_lshift(23, bSig, subnormal_lz, mshifted);
      bExp = -subnormal_lz;
      bSig = SELECT_RANGE(mshifted, 21, 0) << 1;
   }
#endif

   aSig = aSig | 0x00800000U;
   bSig = bSig | 0x00800000U;
   nbSig = -bSig;
   bSigx2 = bSig * 2;
   nbSigx2 = -bSigx2;
   bSigx3 = bSigx2 + bSig;
   nbSigx3 = -bSigx3;
   current = aSig;
   for(index = 0; index < (13 / UNROLL_FACTOR_F32_DIV); ++index)
   {
      BOOST_PP_REPEAT(UNROLL_FACTOR_F32_DIV, LOOP_BODY_F32_DIV, index);
   }
   BOOST_PP_REPEAT(BOOST_PP_MOD(13, UNROLL_FACTOR_F32_DIV), LOOP_BODY_F32_DIV, index);
   if(current != 0)
   {
      positive |= 2;
      negative |= SELECT_BIT(current, 25) << 1;
   }
   negative <<= 1;
   BIT_RESIZE(negative, 28);
   zSig0 = positive - negative;
   zSig0 >>= 1;
   BIT_RESIZE(zSig0, 27);
   correction = SELECT_BIT(zSig0, 26);
   if(correction)
      zSig1 = SELECT_RANGE(zSig0, 25, 2) << 1 | (SELECT_BIT(zSig0, 1)) | (SELECT_BIT(zSig0, 0));
   else
      zSig1 = SELECT_RANGE(zSig0, 24, 0);
#ifndef NO_ROUNDING
   LSB_bit = SELECT_BIT(zSig1, 2);
   Guard_bit = SELECT_BIT(zSig1, 1);
   Round_bit = SELECT_BIT(zSig1, 0);
   round = Guard_bit & (LSB_bit | Round_bit);
#endif
   zExp = aExp - bExp + (0x7E | correction);
   _Bool MSB1zExp = SELECT_BIT(zExp, 9);
   _Bool MSB0zExp = SELECT_BIT(zExp, 8);
   BIT_RESIZE(zExp, 9);
#ifndef NO_ROUNDING
   zExpSig = ((((__bits32)zExp) << 23) | (zSig1 >> 2)) + round;
#else
   zExpSig = ((((__bits32)zExp) << 23) | (zSig1 >> 2));
#endif
   _Bool MSBzExp = SELECT_BIT(zExpSig, 31);
   _Bool ovfCond = ((((MSB0zExp & ((~MSBzExp) & 1)) & 1) ^ MSB1zExp) & 1);
   if(z_c == FP_CLS_NORMAL)
   {
      if(ovfCond)
         return __packFloat32(zSign, 0, 0);
      else if(SELECT_BIT(zExpSig, 31) || zExp == 255)
         return __packFloat32(zSign, 0xFF, 0);
      else
         return (zSign << 31) | SELECT_RANGE(zExpSig, 30, 0);
   }
   else if(z_c == FP_CLS_ZERO)
      return __packFloat32(zSign, 0, 0);
   else if(z_c == FP_CLS_NAN)
      return ((a_c_nan ? aSign : bSign) | (a_c_inf & b_c_inf) | (a_c_zero & b_c_zero)) << 31 | 0x7FC00000 | (a_c_nan ? aSig : (b_c_nan ? bSig : 0));
   else
      return __packFloat32(zSign, 0xFF, 0);
}
static __FORCE_INLINE __float32 __float32_divG(__float32 a, __float32 b)
{
   __flag aSign, bSign, zSign;
   __int16 aExp, bExp, zExp;
   __bits32 aSig, bSig, aSigInitial, bSigInitial, zSig;
   __bits32 c5, c4, c3, c2, c1, c0, shft, p1, bSigsqr1, bSigsqr2, p2, p3, p, ga0, gb0, ga1, gb1, rem0, rem, rnd;

   aSig = __extractFloat32Frac(a);
   aExp = __extractFloat32Exp(a);
   aSign = __extractFloat32Sign(a);
   bSig = __extractFloat32Frac(b);
   bExp = __extractFloat32Exp(b);
   bSign = __extractFloat32Sign(b);
   zSign = aSign ^ bSign;
#ifndef DEFAULT
   __bits8 a_c, b_c, z_c;
   _Bool aExp_null = aExp == 0;
   _Bool bExp_null = bExp == 0;
   _Bool a_c_zero, b_c_zero, a_c_inf, b_c_inf, a_c_nan, b_c_nan, a_c_normal, b_c_normal;
#ifndef NO_SUBNORMALS
   a_c_zero = aExp_null && aSig == 0;
#else
   a_c_zero = aExp_null;
#endif
   a_c_inf = aExp == 0xFF && aSig == 0;
   a_c_nan = aExp == 0xFF && aSig != 0;
   a_c_normal = aExp != 0xFF && !a_c_zero;
   a_c = ((a_c_zero << 1 | a_c_zero) & FP_CLS_ZERO) | ((a_c_normal << 1 | a_c_normal) & FP_CLS_NORMAL) | ((a_c_inf << 1 | a_c_inf) & FP_CLS_INF) | ((a_c_nan << 1 | a_c_nan) & FP_CLS_NAN);

#ifndef NO_SUBNORMALS
   b_c_zero = bExp_null && bSig == 0;
#else
   b_c_zero = bExp_null;
#endif
   b_c_inf = bExp == 0xFF && bSig == 0;
   b_c_nan = bExp == 0xFF && bSig != 0;
   b_c_normal = bExp != 0xFF && !b_c_zero;
   b_c = ((b_c_zero << 1 | b_c_zero) & FP_CLS_ZERO) | ((b_c_normal << 1 | b_c_normal) & FP_CLS_NORMAL) | ((b_c_inf << 1 | b_c_inf) & FP_CLS_INF) | ((b_c_nan << 1 | b_c_nan) & FP_CLS_NAN);
   /*
   00 00 11
   00 01 00
   00 10 00
   00 11 11
   01 00 10
   01 01 01
   01 10 00
   01 11 11
   10 00 10
   10 01 10
   10 10 11
   10 11 11
   11 00 11
   11 01 11
   11 10 11
   11 11 11

    0  1  3  2
    4  5  7  6
   12 13 15 14
    8  9 11 10

      00 01 11 10
   00 1  0  1  0
   01 1  0  1  0
   11 1  1  1  1
   10 1  1  1  1
   I1 = a + c'd' + cd

      00 01 11 10
   00 1  0  1  0
   01 0  1  1  0
   11 1  1  1  1
   10 0  0  1  1
   I0 = ab + cd + ac + bd + a'b'c'd'
   */
   z_c = ((a_c >> 1 | (1 & (~(b_c >> 1)) & (~(b_c & 1))) | (1 & (b_c >> 1) & b_c)) << 1) |
         ((1 & (a_c >> 1) & a_c) | (1 & (b_c >> 1) & b_c) | (1 & (a_c >> 1) & (b_c >> 1)) | (1 & a_c & b_c) | (1 & (~(a_c >> 1)) & (~(a_c & 1)) & (~(b_c >> 1)) & (~(b_c & 1))));

#ifndef NO_SUBNORMALS
   if(aExp_null && !a_c_zero)
   {
      unsigned int subnormal_lz, mshifted;
      count_leading_zero_macro_lshift(23, aSig, subnormal_lz, mshifted);
      aExp = -subnormal_lz;
      aSig = SELECT_RANGE(mshifted, 21, 0) << 1;
   }
   aSigInitial = aSig;
   if(bExp_null && !b_c_zero)
   {
      unsigned int subnormal_lz, mshifted;
      count_leading_zero_macro_lshift(23, bSig, subnormal_lz, mshifted);
      bExp = -subnormal_lz;
      bSig = SELECT_RANGE(mshifted, 21, 0) << 1;
   }
   bSigInitial = bSig;
#endif
#else

   if(aExp == 0xFF || bExp == 0xFF || bExp == 0 || aExp == 0)
   {
      if(aExp == 0xFF)
      {
         if(aSig)
            return __propagateFloat32NaN(a, b);
         if(bExp == 0xFF)
         {
            if(bSig)
               return __propagateFloat32NaN(a, b);
            __float_raise(float_flag_invalid);
            return __float32_default_nan;
         }
         return __packFloat32(zSign, 0xFF, 0);
      }
      if(bExp == 0xFF)
      {
         if(bSig)
            return __propagateFloat32NaN(a, b);
         return __packFloat32(zSign, 0, 0);
      }
      if(bExp == 0)
      {
         if(bSig == 0)
         {
            if((aExp | aSig) == 0)
            {
               __float_raise(float_flag_invalid);
               return __float32_default_nan;
            }
            __float_raise(float_flag_divbyzero);
            return __packFloat32(zSign, 0xFF, 0);
         }
#ifdef NO_SUBNORMALS
         return __packFloat32(zSign, 0xFF, 0);
#else
         __normalizeFloat32Subnormal(bSig, &bExp, &bSig);
#endif
      }
      if(aExp == 0)
      {
#ifdef NO_SUBNORMALS
         return __packFloat32(zSign, 0, 0);
#else
         if(aSig == 0)
            return __packFloat32(zSign, 0, 0);
         __normalizeFloat32Subnormal(aSig, &aExp, &aSig);
#endif
      }
   }
#endif
#ifndef DEFAULT
#if 1
   GOLDSCHMIDT_MANTISSA_DIVISION();
#else
   FLIP_MANTISSA_DIVISION();
#endif
   if(z_c == FP_CLS_NORMAL)
      return __roundAndPackFloat32(zSign, zExp, zSig);
   else if(z_c == FP_CLS_ZERO)
      return __packFloat32(zSign, 0, 0);
   else if(z_c == FP_CLS_NAN)
      return ((a_c_nan ? aSign : bSign) | (a_c_inf & b_c_inf) | (a_c_zero & b_c_zero)) << 31 | 0x7FC00000 | (a_c_nan ? aSigInitial : (b_c_nan ? bSigInitial : 0));
   else
      return __packFloat32(zSign, 0xFF, 0);
#else
   /* Original SoftFloat mantissa division*/
   zExp = aExp - bExp + 0x7D;
   aSig = (aSig | 0x00800000) << 7;
   bSig = (bSig | 0x00800000) << 8;
   if(bSig <= (aSig + aSig))
   {
      aSig >>= 1;
      ++zExp;
   }
   zSig = (((__bits64)aSig) << 32) / bSig;
   if((zSig & 0x3F) == 0)
   {
      zSig |= ((__bits64)bSig * zSig != ((__bits64)aSig) << 32);
   }
   return __roundAndPackFloat32(zSign, zExp, zSig);
#endif
}

/*----------------------------------------------------------------------------
| Returns the remainder of the single-precision floating-point value `a'
| with respect to the corresponding value `b'.  The operation is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float32 __float32_rem(__float32 a, __float32 b)
{
   __flag aSign, bSign, zSign;
   __int16 aExp, bExp, expDiff;
   __bits32 aSig, bSig;
   __bits32 q;
   __bits64 aSig64, bSig64, q64;
   __bits32 alternateASig;
   __sbits32 sigMean;

   aSig = __extractFloat32Frac(a);
   aExp = __extractFloat32Exp(a);
   aSign = __extractFloat32Sign(a);
   bSig = __extractFloat32Frac(b);
   bExp = __extractFloat32Exp(b);
   bSign = __extractFloat32Sign(b);
   if(aExp == 0xFF)
   {
      if(aSig || ((bExp == 0xFF) && bSig))
      {
         return __propagateFloat32NaN(a, b);
      }
      __float_raise(float_flag_invalid);
      return __float32_default_nan;
   }
   if(bExp == 0xFF)
   {
      if(bSig)
         return __propagateFloat32NaN(a, b);
      return a;
   }
   if(bExp == 0)
   {
      if(bSig == 0)
      {
         __float_raise(float_flag_invalid);
         return __float32_default_nan;
      }
#ifdef NO_SUBNORMALS
      return __float32_default_nan;
#else
      __normalizeFloat32Subnormal(bSig, &bExp, &bSig);
#endif
   }
   if(aExp == 0)
   {
#ifdef NO_SUBNORMALS
      return a;
#else
      if(aSig == 0)
         return a;
      __normalizeFloat32Subnormal(aSig, &aExp, &aSig);
#endif
   }
   expDiff = aExp - bExp;
   aSig |= 0x00800000;
   bSig |= 0x00800000;
   if(expDiff < 32)
   {
      aSig <<= 8;
      bSig <<= 8;
      if(expDiff < 0)
      {
         if(expDiff < -1)
            return a;
         aSig >>= 1;
      }
      q = (bSig <= aSig);
      if(q)
         aSig -= bSig;
      if(0 < expDiff)
      {
         q = (((__bits64)aSig) << 32) / bSig;
         q >>= 32 - expDiff;
         bSig >>= 2;
         aSig = ((aSig >> 1) << (expDiff - 1)) - bSig * q;
      }
      else
      {
         aSig >>= 2;
         bSig >>= 2;
      }
   }
   else
   {
      if(bSig <= aSig)
         aSig -= bSig;
      aSig64 = ((__bits64)aSig) << 40;
      bSig64 = ((__bits64)bSig) << 40;
      expDiff -= 64;
      while(0 < expDiff)
      {
         q64 = __estimateDiv128To64(aSig64, 0, bSig64);
         q64 = (2 < q64) ? q64 - 2 : 0;
         aSig64 = -((bSig * q64) << 38);
         expDiff -= 62;
      }
      expDiff += 64;
      q64 = __estimateDiv128To64(aSig64, 0, bSig64);
      q64 = (2 < q64) ? q64 - 2 : 0;
      q = q64 >> (64 - expDiff);
      bSig <<= 6;
      aSig = ((aSig64 >> 33) << (expDiff - 1)) - bSig * q;
   }
   do
   {
      alternateASig = aSig;
      ++q;
      aSig -= bSig;
   } while(0 <= (__sbits32)aSig);
   sigMean = aSig + alternateASig;
   if((sigMean < 0) || ((sigMean == 0) && (q & 1)))
   {
      aSig = alternateASig;
   }
   zSign = ((__sbits32)aSig < 0);
   if(zSign)
      aSig = -aSig;
   return __normalizeRoundAndPackFloat32(aSign ^ zSign, bExp, aSig);
}

/*----------------------------------------------------------------------------
| Returns the square root of the single-precision floating-point value `a'.
| The operation is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float32 __float32_sqrt(__float32 a)
{
   __flag aSign;
   __int16 aExp, zExp;
   __bits32 aSig, zSig;
   __bits64 rem, term;

   aSig = __extractFloat32Frac(a);
   aExp = __extractFloat32Exp(a);
   aSign = __extractFloat32Sign(a);
   if(aExp == 0xFF)
   {
      if(aSig)
         return __propagateFloat32NaN(a, 0);
      if(!aSign)
         return a;
      __float_raise(float_flag_invalid);
      return __float32_default_nan;
   }
   if(aSign)
   {
      if((aExp | aSig) == 0)
         return a;
      __float_raise(float_flag_invalid);
      return __float32_default_nan;
   }
   if(aExp == 0)
   {
#ifdef NO_SUBNORMALS
      return 0;
#else
      if(aSig == 0)
         return 0;
      __normalizeFloat32Subnormal(aSig, &aExp, &aSig);
#endif
   }
   zExp = ((aExp - 0x7F) >> 1) + 0x7E;
   aSig = (aSig | 0x00800000) << 8;
   zSig = __estimateSqrt32(aExp, aSig) + 2;
   if((zSig & 0x7F) <= 5)
   {
      if(zSig < 2)
      {
         zSig = 0x7FFFFFFF;
         goto roundAndPack;
      }
      aSig >>= aExp & 1;
      term = ((__bits64)zSig) * zSig;
      rem = (((__bits64)aSig) << 32) - term;
      while((__sbits64)rem < 0)
      {
         --zSig;
         rem += (((__bits64)zSig) << 1) | 1;
      }
      zSig |= (rem != 0);
   }
   __shift32RightJamming(zSig, 1, &zSig);
roundAndPack:
   return __roundAndPackFloat32(0, zExp, zSig);
}

/*----------------------------------------------------------------------------
| Returns 1 if the single-precision floating-point value `a' is equal to
| the corresponding value `b', and 0 otherwise.  The comparison is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag __float32_eq(__float32 a, __float32 b)
{
   if(((__extractFloat32Exp(a) == 0xFF) && __extractFloat32Frac(a)) || ((__extractFloat32Exp(b) == 0xFF) && __extractFloat32Frac(b)))
   {
      if(__float32_is_signaling_nan(a) || __float32_is_signaling_nan(b))
      {
         __float_raise(float_flag_invalid);
      }
      return 0;
   }
   return (a == b) || ((__bits32)((a | b) << 1) == 0);
}

/*----------------------------------------------------------------------------
| Returns 1 if the single-precision floating-point value `a' is less than
| or equal to the corresponding value `b', and 0 otherwise.  The comparison
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag __float32_le(__float32 a, __float32 b)
{
   __flag aSign, bSign;

   if(((__extractFloat32Exp(a) == 0xFF) && __extractFloat32Frac(a)) || ((__extractFloat32Exp(b) == 0xFF) && __extractFloat32Frac(b)))
   {
      __float_raise(float_flag_invalid);
      return 0;
   }
   aSign = __extractFloat32Sign(a);
   bSign = __extractFloat32Sign(b);
   if(aSign != bSign)
      return aSign || ((__bits32)((a | b) << 1) == 0);
   return (a == b) || (aSign ^ (a < b));
}

/*----------------------------------------------------------------------------
| Returns 1 if the single-precision floating-point value `a' is less than
| the corresponding value `b', and 0 otherwise.  The comparison is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag __float32_lt(__float32 a, __float32 b)
{
   __flag aSign, bSign;

   if(((__extractFloat32Exp(a) == 0xFF) && __extractFloat32Frac(a)) || ((__extractFloat32Exp(b) == 0xFF) && __extractFloat32Frac(b)))
   {
      __float_raise(float_flag_invalid);
      return 0;
   }
   aSign = __extractFloat32Sign(a);
   bSign = __extractFloat32Sign(b);
   if(aSign != bSign)
      return aSign && ((__bits32)((a | b) << 1) != 0);
   return (a != b) && (aSign ^ (a < b));
}

/*----------------------------------------------------------------------------
| Returns 1 if the single-precision floating-point value `a' is equal to
| the corresponding value `b', and 0 otherwise.  The invalid exception is
| raised if either operand is a NaN.  Otherwise, the comparison is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag __float32_eq_signaling(__float32 a, __float32 b)
{
   if(((__extractFloat32Exp(a) == 0xFF) && __extractFloat32Frac(a)) || ((__extractFloat32Exp(b) == 0xFF) && __extractFloat32Frac(b)))
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

static __FORCE_INLINE __flag __float32_le_quiet(__float32 a, __float32 b)
{
   __flag aSign, bSign;
   __int16 aExp, bExp;

   if(((__extractFloat32Exp(a) == 0xFF) && __extractFloat32Frac(a)) || ((__extractFloat32Exp(b) == 0xFF) && __extractFloat32Frac(b)))
   {
      if(__float32_is_signaling_nan(a) || __float32_is_signaling_nan(b))
      {
         __float_raise(float_flag_invalid);
      }
      return 0;
   }
   aSign = __extractFloat32Sign(a);
   bSign = __extractFloat32Sign(b);
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

static __FORCE_INLINE __flag __float32_lt_quiet(__float32 a, __float32 b)
{
   __flag aSign, bSign;

   if(((__extractFloat32Exp(a) == 0xFF) && __extractFloat32Frac(a)) || ((__extractFloat32Exp(b) == 0xFF) && __extractFloat32Frac(b)))
   {
      if(__float32_is_signaling_nan(a) || __float32_is_signaling_nan(b))
      {
         __float_raise(float_flag_invalid);
      }
      return 0;
   }
   aSign = __extractFloat32Sign(a);
   bSign = __extractFloat32Sign(b);
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

static __FORCE_INLINE __int32 __float64_to_int32(__float64 a)
{
   __flag aSign;
   __int16 aExp, shiftCount;
   __bits64 aSig;

   aSig = __extractFloat64Frac(a);
   aExp = __extractFloat64Exp(a);
   aSign = __extractFloat64Sign(a);
   if((aExp == 0x7FF) && aSig)
      aSign = 0;
   if(aExp)
      aSig |= LIT64(0x0010000000000000);
   shiftCount = 0x42C - aExp;
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

static __FORCE_INLINE __int32 __float64_to_int32_round_to_zero(__float64 a)
{
   __flag aSign;
   __int16 aExp, shiftCount;
   __bits64 aSig, savedASig;
   __int32 z;

   aSig = __extractFloat64Frac(a);
   aExp = __extractFloat64Exp(a);
   aSign = __extractFloat64Sign(a);
   if(0x41E < aExp)
   {
      if((aExp == 0x7FF) && aSig)
         aSign = 0;
      goto invalid;
   }
   else if(aExp < 0x3FF)
   {
#ifndef NO_PARAMETRIC
      if(aExp || aSig)
         __float_exception_flags |= float_flag_inexact;
#endif
      return 0;
   }
   aSig |= LIT64(0x0010000000000000);
   shiftCount = 0x433 - aExp;
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
#ifndef NO_PARAMETRIC
      __float_exception_flags |= float_flag_inexact;
#endif
   }
   return z;
}

static __FORCE_INLINE __uint32 __float64_to_uint32_round_to_zero(__float64 a)
{
   __flag aSign;
   __int16 aExp, shiftCount;
   __bits64 aSig, savedASig;
   __uint32 z;

   aSig = __extractFloat64Frac(a);
   aExp = __extractFloat64Exp(a);
   aSign = __extractFloat64Sign(a);
   if(0x41E < aExp)
   {
      if((aExp == 0x7FF) && aSig)
         aSign = 0;
      __float_raise(float_flag_invalid);
      return 0x80000000;
   }
   else if(aExp < 0x3FF)
   {
#ifndef NO_PARAMETRIC
      if(aExp || aSig)
         __float_exception_flags |= float_flag_inexact;
#endif
      return 0;
   }
   aSig |= LIT64(0x0010000000000000);
   shiftCount = 0x433 - aExp;
   savedASig = aSig;
   aSig >>= shiftCount;
   z = aSig;
   if(aSign)
      z = -z;
   if((aSig << shiftCount) != savedASig)
   {
#ifndef NO_PARAMETRIC
      __float_exception_flags |= float_flag_inexact;
#endif
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

static __FORCE_INLINE __int64 __float64_to_int64(__float64 a)
{
   __flag aSign;
   __int16 aExp, shiftCount;
   __bits64 aSig, aSigExtra;

   aSig = __extractFloat64Frac(a);
   aExp = __extractFloat64Exp(a);
   aSign = __extractFloat64Sign(a);
   if(aExp)
      aSig |= LIT64(0x0010000000000000);
   shiftCount = 0x433 - aExp;
   if(shiftCount <= 0)
   {
      if(0x43E < aExp)
      {
         __float_raise(float_flag_invalid);
         if(!aSign || ((aExp == 0x7FF) && (aSig != LIT64(0x0010000000000000))))
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

static __FORCE_INLINE __int64 __float64_to_int64_round_to_zero(__float64 a)
{
   __flag aSign;
   __int16 aExp, shiftCount;
   __bits64 aSig;
   __int64 z;

   aSig = __extractFloat64Frac(a);
   aExp = __extractFloat64Exp(a);
   aSign = __extractFloat64Sign(a);
   if(aExp)
      aSig |= LIT64(0x0010000000000000);
   shiftCount = aExp - 0x433;
   if(0 <= shiftCount)
   {
      if(0x43E <= aExp)
      {
         if(a != LIT64(0xC3E0000000000000))
         {
            __float_raise(float_flag_invalid);
            if(!aSign || ((aExp == 0x7FF) && (aSig != LIT64(0x0010000000000000))))
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
      if(aExp < 0x3FE)
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

static __FORCE_INLINE __uint64 __float64_to_uint64_round_to_zero(__float64 a)
{
   __flag aSign;
   __int16 aExp, shiftCount;
   __bits64 aSig;
   __uint64 z;

   aSig = __extractFloat64Frac(a);
   aExp = __extractFloat64Exp(a);
   aSign = __extractFloat64Sign(a);
   if(aExp)
      aSig |= LIT64(0x0010000000000000);
   shiftCount = aExp - 0x433;
   if(0 <= shiftCount)
   {
      if(0x43E <= aExp)
      {
         return (__bits64)LIT64(0x8000000000000000);
      }
      z = aSig << shiftCount;
   }
   else
   {
      if(aExp < 0x3FE)
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

static __FORCE_INLINE __float32 __float64_to_float32(__float64 a)
{
   __flag aSign;
   __int16 aExp;
   __bits64 aSig;
   __bits32 zSig;

   aSig = __extractFloat64Frac(a);
   aExp = __extractFloat64Exp(a);
   aSign = __extractFloat64Sign(a);
   if(aExp == 0x7FF)
   {
      if(aSig)
         return __commonNaNToFloat32(__float64ToCommonNaN(a));
      return __packFloat32(aSign, 0xFF, 0);
   }
   __shift64RightJamming(aSig, 22, &aSig);
   zSig = aSig;
   if(aExp || zSig)
   {
      zSig |= 0x40000000;
      aExp -= 0x381;
   }
   return __roundAndPackFloat32(aSign, aExp, zSig);
}

#ifdef FLOATX80

/*----------------------------------------------------------------------------
| Returns the result of converting the double-precision floating-point value
| `a' to the extended double-precision floating-point format.  The conversion
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __floatx80 __float64_to_floatx80(__float64 a)
{
   __flag aSign;
   __int16 aExp;
   __bits64 aSig;

   aSig = __extractFloat64Frac(a);
   aExp = __extractFloat64Exp(a);
   aSign = __extractFloat64Sign(a);
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

static __FORCE_INLINE __float128 __float64_to_float128(__float64 a)
{
   __flag aSign;
   __int16 aExp;
   __bits64 aSig, zSig0, zSig1;

   aSig = __extractFloat64Frac(a);
   aExp = __extractFloat64Exp(a);
   aSign = __extractFloat64Sign(a);
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

static __FORCE_INLINE __float64 __float64_round_to_int(__float64 a)
{
   __flag aSign;
   __int16 aExp;
   __bits64 lastBitMask, roundBitsMask;
   __int8 roundingMode;
   __float64 z;

   aExp = __extractFloat64Exp(a);
   if(0x433 <= aExp)
   {
      if((aExp == 0x7FF) && __extractFloat64Frac(a))
      {
         return __propagateFloat64NaN(a, a);
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
      aSign = __extractFloat64Sign(a);
      switch(__float_rounding_mode)
      {
         case float_round_nearest_even:
            if((aExp == 0x3FE) && __extractFloat64Frac(a))
            {
               return __packFloat64(aSign, 0x3FF, 0);
            }
            break;
         case float_round_down:
            return aSign ? LIT64(0xBFF0000000000000) : 0;
         case float_round_up:
            return aSign ? LIT64(0x8000000000000000) : LIT64(0x3FF0000000000000);
      }
      return __packFloat64(aSign, 0, 0);
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
      if(__extractFloat64Sign(z) ^ (roundingMode == float_round_up))
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

static __FORCE_INLINE __float64 __addFloat64Sigs(__float64 a, __float64 b, __flag zSign)
{
   __int16 aExp, bExp, zExp;
   __bits64 aSig, bSig, zSig;
   __int16 expDiff;

   aSig = __extractFloat64Frac(a);
   aExp = __extractFloat64Exp(a);
   bSig = __extractFloat64Frac(b);
   bExp = __extractFloat64Exp(b);
   expDiff = aExp - bExp;
   aSig <<= 9;
   bSig <<= 9;
   if(0 < expDiff)
   {
      if(aExp == 0x7FF)
      {
         if(aSig)
            return __propagateFloat64NaN(a, b);
         return a;
      }
      if(bExp == 0)
      {
         --expDiff;
      }
      else
      {
         bSig |= LIT64(0x2000000000000000);
      }
      __shift64RightJamming(bSig, expDiff, &bSig);
      zExp = aExp;
   }
   else if(expDiff < 0)
   {
      if(bExp == 0x7FF)
      {
         if(bSig)
            return __propagateFloat64NaN(a, b);
         return __packFloat64(zSign, 0x7FF, 0);
      }
      if(aExp == 0)
      {
         ++expDiff;
      }
      else
      {
         aSig |= LIT64(0x2000000000000000);
      }
      __shift64RightJamming(aSig, -expDiff, &aSig);
      zExp = bExp;
   }
   else
   {
      if(aExp == 0x7FF)
      {
         if(aSig | bSig)
            return __propagateFloat64NaN(a, b);
         return a;
      }
      if(aExp == 0)
         return __packFloat64(zSign, 0, (aSig + bSig) >> 9);
      zSig = LIT64(0x4000000000000000) + aSig + bSig;
      zExp = aExp;
      goto roundAndPack;
   }
   aSig |= LIT64(0x2000000000000000);
   zSig = (aSig + bSig) << 1;
   --zExp;
   if((__sbits64)zSig < 0)
   {
      zSig = aSig + bSig;
      ++zExp;
   }
roundAndPack:
   return __roundAndPackFloat64(zSign, zExp, zSig);
}

/*----------------------------------------------------------------------------
| Returns the result of subtracting the absolute values of the double-
| precision floating-point values `a' and `b'.  If `zSign' is 1, the
| difference is negated before being returned.  `zSign' is ignored if the
| result is a NaN.  The subtraction is performed according to the IEC/IEEE
| Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float64 __subFloat64Sigs(__float64 a, __float64 b, __flag zSign)
{
   __int16 aExp, bExp, zExp;
   __bits64 aSig, bSig, zSig;
   __int16 expDiff;

   aSig = __extractFloat64Frac(a);
   aExp = __extractFloat64Exp(a);
   bSig = __extractFloat64Frac(b);
   bExp = __extractFloat64Exp(b);
   expDiff = aExp - bExp;
   aSig <<= 10;
   bSig <<= 10;
   if(0 < expDiff)
      goto aExpBigger;
   if(expDiff < 0)
      goto bExpBigger;
   if(aExp == 0x7FF)
   {
      if(aSig | bSig)
         return __propagateFloat64NaN(a, b);
      __float_raise(float_flag_invalid);
      return __float64_default_nan;
   }
   if(aExp == 0)
   {
      aExp = 1;
      bExp = 1;
   }
   if(bSig < aSig)
      goto aBigger;
   if(aSig < bSig)
      goto bBigger;
   return __packFloat64(__float_rounding_mode == float_round_down, 0, 0);
bExpBigger:
   if(bExp == 0x7FF)
   {
      if(bSig)
         return __propagateFloat64NaN(a, b);
      return __packFloat64(zSign ^ 1, 0x7FF, 0);
   }
   if(aExp == 0)
   {
      ++expDiff;
   }
   else
   {
      aSig |= LIT64(0x4000000000000000);
   }
   __shift64RightJamming(aSig, -expDiff, &aSig);
   bSig |= LIT64(0x4000000000000000);
bBigger:
   zSig = bSig - aSig;
   zExp = bExp;
   zSign ^= 1;
   goto normalizeRoundAndPack;
aExpBigger:
   if(aExp == 0x7FF)
   {
      if(aSig)
         return __propagateFloat64NaN(a, b);
      return a;
   }
   if(bExp == 0)
   {
      --expDiff;
   }
   else
   {
      bSig |= LIT64(0x4000000000000000);
   }
   __shift64RightJamming(bSig, expDiff, &bSig);
   aSig |= LIT64(0x4000000000000000);
aBigger:
   zSig = aSig - bSig;
   zExp = aExp;
normalizeRoundAndPack:
   --zExp;
   return __normalizeRoundAndPackFloat64(zSign, zExp, zSig);
}

static __FORCE_INLINE __bits8 __CLZ54(__bits64 v)
{
   __bits8 res;
   count_leading_zero_macro(54, v, res);
   return res;
}

static __FORCE_INLINE __float64 __addsubFloat64_old(__float64 a, __float64 b, __flag bSign)
{
   __bits16 eRc1, eRn0, eRn_0, eRc, eRf0, eRf1, eRf, eRn;
   __bits64 aSig, bSig, bSigOrig, tmp_swap, fAc1, fBc1, fRc0, fRc1, fRc, fBf1, fBf2_low, fBf2, fAf3, fRf0, fRf0_right, fRf0_t_right, fRf1, fRf_0, fRf, shift_0, fRn_0, fRn, fRr;
   __int16 expDiff;
   _Bool expDiffSign, eRnSign;
   __bits16 aExp, bExp, expDiff8, eRn8;
   __bits8 a_c, b_c, nZeros, lb;
   _Bool a_c_zero, b_c_zero, a_c_inf, b_c_inf, a_c_nan, b_c_nan, a_c_normal, b_c_normal, swap, sAB, aSign, roundc, roundf, eZero, close, sRn, eMax, eMin, eTest0, eTest1, SigDiffSign, sb;
   __int16 eRf1_correction;
   aSign = __extractFloat64Sign(a);
   aSig = __extractFloat64Frac(a);
   aExp = __extractFloat64Exp(a);
   bSig = __extractFloat64Frac(b);
   bExp = __extractFloat64Exp(b);
   a_c_zero = aExp == 0 && aSig == 0;
   a_c_inf = aExp == 0x7FF && aSig == 0;
   a_c_nan = aExp == 0x7FF && aSig != 0;
   a_c_normal = aExp != 0 && aExp != 0x7FF;
   a_c = ((a_c_zero << 1 | a_c_zero) & FP_CLS_ZERO) | ((a_c_normal << 1 | a_c_normal) & FP_CLS_NORMAL) | ((a_c_inf << 1 | a_c_inf) & FP_CLS_INF) | ((a_c_nan << 1 | a_c_nan) & FP_CLS_NAN);

   b_c_zero = bExp == 0 && bSig == 0;
   b_c_inf = bExp == 0x7FF && bSig == 0;
   b_c_nan = bExp == 0x7FF && bSig != 0;
   b_c_normal = bExp != 0 && bExp != 0x7FF;
   b_c = ((b_c_zero << 1 | b_c_zero) & FP_CLS_ZERO) | ((b_c_normal << 1 | b_c_normal) & FP_CLS_NORMAL) | ((b_c_inf << 1 | b_c_inf) & FP_CLS_INF) | ((b_c_nan << 1 | b_c_nan) & FP_CLS_NAN);

   expDiff = aExp - bExp;
   expDiffSign = (expDiff >> 12) & 1;
   expDiff8 = expDiff;
   swap = ((expDiffSign) && a_c == b_c) || a_c < b_c;
   expDiff8 = (swap ? -expDiff8 : expDiff8);
   tmp_swap = b;
   b = swap ? a : b;
   a = swap ? tmp_swap : a;
   sAB = aSign ^ bSign;
   aSign = swap ? bSign : aSign;
   close = ((expDiff8 >> 1) == 0) && sAB;
   aSig = __extractFloat64Frac(a);
   bSigOrig = __extractFloat64Frac(b);
   fAc1 = (aSig | 0x0010000000000000) << 1;
   fAf3 = fAc1 << 2;
   BIT_RESIZE(fAf3, 56);
   fBf1 = bSigOrig | 0x0010000000000000;
   fBc1 = (expDiff8 & 1) ? fBf1 : (fBf1 << 1);
   fRc0 = fAc1 - fBc1;
   BIT_RESIZE(fRc0, 55);
   SigDiffSign = (fRc0 >> 54) & 1;
   fRc1 = SigDiffSign ? -fRc0 : fRc0;
   BIT_RESIZE(fRc1, 54);
   eRc1 = __extractFloat64Exp(a);
   BIT_RESIZE(eRc1, 12);
   // nZeros = __CLZ54(fRc1);
   // shift_0 = fRc1 << nZeros;
   count_leading_zero_macro_lshift(54, fRc1, nZeros, shift_0);
   roundc = (fRc1 & 1) & ((fRc1 >> 1) & 1) & ((fRc1 >> 53) & 1);
   fRn_0 = shift_0 >> 1;
   eRn0 = eRc1 - nZeros;
   eZero = (eRn0 >> 11) & 1;
   eRn_0 = eZero ? 0 : eRn0;
   fRr = ((fRc1 & (1ULL << 53) - 1) >> 2) + 1; // (1<<53) = 0x0020000000000000
   eRc = roundc ? eRc1 + ((fRr >> 51) & 1) : eRn_0;
   BIT_RESIZE(eRc, 12);
   fRc = roundc ? ((fRr & ((1ULL << 51) - 1)) | (1ULL << 51)) << 1 : fRn_0; // (1<<51) = 0x0008000000000000
   BIT_RESIZE(fRc, 53);
   fBf2 = expDiff8 >= 56 ? 0 : (fBf1 << 3) >> expDiff8;
   fBf2_low = expDiff8 >= 56 ? fBf1 : (fBf1 << 3) << (56 - expDiff8);
   BIT_RESIZE(fBf2, 56);
   BIT_RESIZE(fBf2_low, 56);
   sb = fBf2_low != 0;
   fRf0_t_right = (~fBf2) + !sb;
   fRf0_right = sAB ? fRf0_t_right : fBf2;
   fRf0 = fAf3 + fRf0_right;
   BIT_RESIZE(fRf0, 57);
   eRf0 = eRc1;
   BIT_RESIZE(eRf0, 12);
   lb = (fRf0 >> 55) & 3;
   fRf1 = lb == 1 ? ((fRf0 >> 1) | (fRf0 & 1)) : (lb == 0 ? fRf0 : ((fRf0 >> 2) | ((fRf0 & 2) >> 1) | (fRf0 & 1)));
   BIT_RESIZE(fRf1, 55);
   eRf1_correction = lb == 1 ? 0 : (lb == 0 ? -1 : +1);
   roundf = ((fRf1 & 2) >> 1) & (((fRf1 & 4) >> 2) | (fRf1 & 1) | sb);
   fRf_0 = (fRf1 >> 2) + roundf;
   BIT_RESIZE(fRf_0, 54);
   eRf1 = eRf1_correction + ((fRf_0 >> 53) & 1);
   BIT_RESIZE(eRf1, 12);
   eRf = eRf0 + eRf1;
   BIT_RESIZE(eRf, 12);
   fRf = ((fRf_0 & (1ULL << 53)) >> 1) | (fRf_0 & ((1ULL << 53) - 1)); // (1<<53) = 0x0020000000000000
   BIT_RESIZE(fRf, 53);
   eRn = close ? eRc : eRf;
   BIT_RESIZE(eRn, 12);
   eRn8 = eRn & 0x7FF;
   BIT_RESIZE(eRn8, 12);
   eRnSign = (eRn >> 11) & 1;
   fRn = close ? fRc : fRf;
   BIT_RESIZE(fRn, 53);
   sRn = (((~close) & 1) | (fRc1 != 0)) & (aSign ^ (close & SigDiffSign));
   eMax = eRn8 == 0x7FF;
   eMin = eRn8 == 0;
   eTest1 = (eMax & (0 == (eRnSign))) | ((eRnSign) & (((eRn8 >> 11) & 1) == 0));
   eTest0 = (eMin & (0 == (eRnSign))) | ((eRnSign) & ((eRn8 >> 11) & 1)) | (0 == ((fRn >> 52) & 1));

   if(a_c_normal && b_c_normal)
   {
      if(!eTest1 && eTest0)
         return ((__bits64)(sRn & 1)) << 63;
      else if(eTest1 && !eTest0)
         return __float64_default_nan;
      else
         return (((__bits64)sRn) << 63) | (((__bits64)eRn8) << 52) | (fRn & ((1ULL << 52) - 1)); // 1<<52 = 0x0010000000000000
   }
   else
   {
      if(a_c_inf && b_c_inf)
      {
         if(sAB)
            return __float64_default_nan;
         else
            return ((__bits64)(sRn & 1)) << 63 | ((__bits64)0x7FF) << 52;
      }
      else
      {
         if(eRc1 == 0x7FF && aSig == 0)
            return ((__bits64)(sRn & 1)) << 63 | ((__bits64)0x7FF) << 52;
         else if(eRc1 != 0x7FF)
            return (((__bits64)sRn) << 63) | (((__bits64)(eRc1 & 0x7ff)) << 52) | (aSig & ((1ULL << 52) - 1)); // 1<<52 = 0x0010000000000000
         else
            return __float64_default_nan;
      }
   }
}

static __FORCE_INLINE __float64 __addsubFloat64(__float64 a, __float64 b, __flag bSign)
{
   VOLATILE_DEF __bits64 aSig, bSig, shift_0;
   VOLATILE_DEF __bits16 aExp, bExp, expDiff11;
   VOLATILE_DEF __bits16 nZeros;
   VOLATILE_DEF _Bool a_c_nan, b_c_nan, a_c_normal, b_c_normal, tmp_c_normal, swap, sAB, aSign;
   VOLATILE_DEF __bits64 abs_a, abs_b;
   VOLATILE_DEF __bits64 fA, fB, fB_shifted;
#ifndef NO_ROUNDING
   VOLATILE_DEF __bits64 fB_shifted_low, fBleft_shifted;
   VOLATILE_DEF _Bool LSB_bit, Guard_bit, Round_bit, Sticky_bit, round, sb;
#endif
   VOLATILE_DEF _Bool ge_64;
   VOLATILE_DEF _Bool tmp_sign;
   VOLATILE_DEF __bits16 tmp_exp;
   VOLATILE_DEF __bits64 tmp_sig;
   VOLATILE_DEF _Bool subnormal_exp_correction;
   VOLATILE_DEF __bits64 fB_shifted1;
   VOLATILE_DEF __bits64 fR0;
   VOLATILE_DEF _Bool R_c_zero;
   VOLATILE_DEF __bits16 RExp0, RExp1;
   VOLATILE_DEF __bits64 RSig0, RSig1, RSig2, RSig3;
   VOLATILE_DEF __bits64 RExp0RSig1, Rrounded;
   VOLATILE_DEF _Bool overflow_to_infinite;
   VOLATILE_DEF _Bool aExp2047, bExp2047;

#ifdef NO_ROUNDING
#define FRAC_SHIFT 0
#define FRAC_FULL_BW 54
#define FRAC_ALMOST_BW 53
#else
#define FRAC_SHIFT 2
#define FRAC_FULL_BW 56
#define FRAC_ALMOST_BW 55
#endif

   aSign = __extractFloat64Sign(a);
   aSig = __extractFloat64Frac(a);
   aExp = __extractFloat64Exp(a);
   bSig = __extractFloat64Frac(b);
   bExp = __extractFloat64Exp(b);
   _Bool aSig_null = aSig == 0;
   _Bool bSig_null = bSig == 0;
   _Bool aExp_null = aExp == 0;
   _Bool bExp_null = bExp == 0;
   aExp2047 = aExp == 0x7FF;
   a_c_nan = aExp2047 && !aSig_null;
   a_c_normal = !aExp_null /*&& !aExp2047*/; /// not really needed the second condition
   bExp2047 = bExp == 0x7FF;
   b_c_nan = bExp2047 && !bSig_null;
   b_c_normal = !bExp_null /*&& !bExp2047*/; /// not really needed the second condition

   sAB = aSign ^ bSign;

   abs_a = a;
   BIT_RESIZE(abs_a, 63);
   abs_b = b;
   BIT_RESIZE(abs_b, 63);

   swap = (a_c_nan == b_c_nan && abs_a < abs_b) || a_c_nan < b_c_nan;

#ifdef NO_SUBNORMALS
   subnormal_exp_correction = 0;
#else
   subnormal_exp_correction = (aExp_null && !aSig_null) ^ (bExp_null && !bSig_null);
#endif

   tmp_exp = bExp;
   bExp = COND_EXPR_MACRO32(swap, aExp, bExp);
   aExp = COND_EXPR_MACRO32(swap, tmp_exp, aExp);

   expDiff11 = aExp - bExp - subnormal_exp_correction;

   tmp_sig = bSig;
   bSig = COND_EXPR_MACRO64(swap, aSig, bSig);
   aSig = COND_EXPR_MACRO64(swap, tmp_sig, aSig);

   tmp_sign = bSign;
   // bSign = swap ? aSign : bSign;
   aSign = swap ? tmp_sign : aSign;

   tmp_c_normal = b_c_normal;
   b_c_normal = swap ? a_c_normal : b_c_normal;
   a_c_normal = swap ? tmp_c_normal : a_c_normal;

   fA = (aSig | (((__bits64)a_c_normal) << 52)) << FRAC_SHIFT;
   fB = (bSig | (((__bits64)b_c_normal) << 52)) << FRAC_SHIFT;

   ge_64 = SELECT_BIT(expDiff11, 6) | SELECT_BIT(expDiff11, 7) | SELECT_BIT(expDiff11, 8) | SELECT_BIT(expDiff11, 9) | SELECT_BIT(expDiff11, 10);

#ifndef NO_ROUNDING
   fBleft_shifted = COND_EXPR_MACRO64(ge_64 | SELECT_BIT(expDiff11, 5), fB << 32, 0);
   fB_shifted = COND_EXPR_MACRO64(ge_64 | SELECT_BIT(expDiff11, 5), fB >> 32, fB);
   fBleft_shifted = COND_EXPR_MACRO64(ge_64 | SELECT_BIT(expDiff11, 4), (fB_shifted << 48) | (fBleft_shifted >> 16), fBleft_shifted);
   fB_shifted = COND_EXPR_MACRO64(ge_64 | SELECT_BIT(expDiff11, 4), fB_shifted >> 16, fB_shifted);
   fBleft_shifted = COND_EXPR_MACRO64(ge_64 | SELECT_BIT(expDiff11, 3), (fB_shifted << 56) | (fBleft_shifted >> 8), fBleft_shifted);
   fB_shifted = COND_EXPR_MACRO64(ge_64 | SELECT_BIT(expDiff11, 3), fB_shifted >> 8, fB_shifted);
   fBleft_shifted = COND_EXPR_MACRO64(ge_64 | SELECT_BIT(expDiff11, 2), (fB_shifted << 60) | (fBleft_shifted >> 4), fBleft_shifted);
   fB_shifted = COND_EXPR_MACRO64(ge_64 | SELECT_BIT(expDiff11, 2), fB_shifted >> 4, fB_shifted);
   fBleft_shifted = COND_EXPR_MACRO64(ge_64 | SELECT_BIT(expDiff11, 1), (fB_shifted << 62) | (fBleft_shifted >> 2), fBleft_shifted);
   fB_shifted = COND_EXPR_MACRO64(ge_64 | SELECT_BIT(expDiff11, 1), fB_shifted >> 2, fB_shifted);
   fBleft_shifted = COND_EXPR_MACRO64(ge_64 | SELECT_BIT(expDiff11, 0), (fB_shifted << 63) | (fBleft_shifted >> 1), fBleft_shifted);
   fB_shifted = COND_EXPR_MACRO64(ge_64 | SELECT_BIT(expDiff11, 0), fB_shifted >> 1, fB_shifted);
#else
   fB_shifted = fB >> VAL_RESIZE((expDiff11 | (__bits32)(((((__sbits32)ge_64) << 31) >> 31))), 6);
#endif

   // fB_shifted = COND_EXPR_MACRO64(ge_64, 0, fB_shifted);
   BIT_RESIZE(fB_shifted, FRAC_ALMOST_BW);

#ifndef NO_ROUNDING
   fB_shifted_low = fBleft_shifted;
   sb = fB_shifted_low != 0;
#endif
   fB_shifted1 = ((__bits64)((((__sbits64)sAB) << 63) >> 63)) ^ fB_shifted;
   BIT_RESIZE(fB_shifted1, FRAC_FULL_BW);

#ifdef NO_ROUNDING
   fR0 = fA + fB_shifted1 + sAB;
#else
   fR0 = fA + fB_shifted1 + (sAB && (!sb));
#endif
   BIT_RESIZE(fR0, FRAC_FULL_BW);
   count_leading_zero_macro_lshift64(FRAC_FULL_BW, fR0, nZeros, shift_0);

   R_c_zero = nZeros == VAL_RESIZE((~0ULL), 6);
   overflow_to_infinite = aExp == 2046 && (fR0 >> FRAC_ALMOST_BW) & 1;

#ifdef NO_SUBNORMALS
   R_c_zero = R_c_zero || aExp < nZeros;
   RExp0 = R_c_zero ? 0 : aExp - nZeros + 1;
   RSig0 = shift_0;
#else
   RExp0 = R_c_zero || aExp < nZeros ? ((aExp_null) && (bExp_null) && nZeros == 1) : aExp - nZeros + 1;
   RSig0 = aExp < nZeros ? ((aExp_null) && (bExp_null) ? (fR0 << 1) : (fR0 << aExp)) : shift_0;
#endif
#ifndef NO_ROUNDING
   LSB_bit = SELECT_BIT(RSig0, 3);
   Guard_bit = SELECT_BIT(RSig0, 2);
   Round_bit = SELECT_BIT(RSig0, 1);
   Sticky_bit = SELECT_BIT(RSig0, 0) | sb;
   round = Guard_bit & (LSB_bit | Round_bit | Sticky_bit);
#endif

   RSig1 = VAL_RESIZE(RSig0 >> (FRAC_SHIFT + 1), 52);

   RExp0RSig1 = (((__bits64)RExp0) << 52) | RSig1;

#ifdef NO_ROUNDING
   Rrounded = RExp0RSig1;
#else
   Rrounded = RExp0RSig1 + round;
#endif

   RExp1 = aExp2047 || bExp2047 ? 0x7FF : VAL_RESIZE(Rrounded >> 52, 11);

   RSig2 = R_c_zero || aExp2047 || bExp2047 || overflow_to_infinite ? 0 : VAL_RESIZE(Rrounded, 52);

   aSign = aSign && (!R_c_zero || !sAB);

#ifdef NO_SIGNALLING
   RSig3 = (((__bits64)(a_c_nan || b_c_nan || (sAB && aExp2047 && bExp2047))) << 51) | RSig2;
#else
   RSig3 = a_c_nan || b_c_nan || (sAB && aExp2047 && bExp2047) ? 1ULL << 51 | (aSig & ((1ULL << 51) - 1)) : RSig2;
#endif
   return (((__bits64)aSign) << 63) | (((__bits64)RExp1) << 52) | RSig3;
}

/*----------------------------------------------------------------------------
| Returns the result of adding the double-precision floating-point values `a'
| and `b'.  The operation is performed according to the IEC/IEEE Standard for
| Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float64 __float64_add(__float64 a, __float64 b)
{
#ifndef DEFAULT
   __flag bSign = __extractFloat64Sign(b);
   return __addsubFloat64(a, b, bSign);
#else
   __flag aSign, bSign;

   aSign = __extractFloat64Sign(a);
   bSign = __extractFloat64Sign(b);
   if(aSign == bSign)
   {
      return __addFloat64Sigs(a, b, aSign);
   }
   else
   {
      return __subFloat64Sigs(a, b, aSign);
   }
#endif
}

/*----------------------------------------------------------------------------
| Returns the result of subtracting the double-precision floating-point values
| `a' and `b'.  The operation is performed according to the IEC/IEEE Standard
| for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float64 __float64_sub(__float64 a, __float64 b)
{
#ifndef DEFAULT
   __flag bSign = __extractFloat64Sign(b) ^ 1;
   return __addsubFloat64(a, b, bSign);
#else
   __flag aSign, bSign;

   aSign = __extractFloat64Sign(a);
   bSign = __extractFloat64Sign(b);
   if(aSign == bSign)
   {
      return __subFloat64Sigs(a, b, aSign);
   }
   else
   {
      return __addFloat64Sigs(a, b, aSign);
   }
#endif
}

/*----------------------------------------------------------------------------
| Returns the result of multiplying the double-precision floating-point values
| `a' and `b'.  The operation is performed according to the IEC/IEEE Standard
| for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE SF_UDItype _umul64(SF_UDItype u, SF_UDItype v)
{
   SF_UDItype t;
   SF_USItype u0, u1, v0, v1, k;
   SF_USItype w0, w1;
   SF_USItype tlast;
   u1 = u >> 32;
   u0 = u;
   v1 = v >> 32;
   v0 = v;
   t = (SF_UDItype)u0 * v0;
   w0 = t;
   k = t >> 32;
   tlast = u1 * v0 + k;
   w1 = tlast;
   tlast = u0 * v1 + w1;
   return (((SF_UDItype)tlast) << 32) | ((SF_UDItype)w0);
}

static __FORCE_INLINE __float64 __float64_mul(__float64 a, __float64 b)
{
   __flag aSign, bSign, zSign;
   __int16 aExp, bExp, zExp;
   __bits64 aSig, bSig;

   aSig = __extractFloat64Frac(a);
   aExp = __extractFloat64Exp(a);
   aSign = __extractFloat64Sign(a);
   bSig = __extractFloat64Frac(b);
   bExp = __extractFloat64Exp(b);
   bSign = __extractFloat64Sign(b);
   zSign = aSign ^ bSign;

#ifndef DEFAULT
   __bits8 a_c, b_c, z_c;
   _Bool a_c_zero, b_c_zero, a_c_inf, b_c_inf, a_c_nan, b_c_nan, a_c_normal, b_c_normal;
   a_c_zero = aExp == 0 && aSig == 0;
   a_c_inf = aExp == 0x7FF && aSig == 0;
   a_c_nan = aExp == 0x7FF && aSig != 0;
   a_c_normal = aExp != 0 && aExp != 0x7FF;
   a_c = ((a_c_zero << 1 | a_c_zero) & FP_CLS_ZERO) | ((a_c_normal << 1 | a_c_normal) & FP_CLS_NORMAL) | ((a_c_inf << 1 | a_c_inf) & FP_CLS_INF) | ((a_c_nan << 1 | a_c_nan) & FP_CLS_NAN);

   b_c_zero = bExp == 0 && bSig == 0;
   b_c_inf = bExp == 0x7FF && bSig == 0;
   b_c_nan = bExp == 0x7FF && bSig != 0;
   b_c_normal = bExp != 0 && bExp != 0x7FF;
   b_c = ((b_c_zero << 1 | b_c_zero) & FP_CLS_ZERO) | ((b_c_normal << 1 | b_c_normal) & FP_CLS_NORMAL) | ((b_c_inf << 1 | b_c_inf) & FP_CLS_INF) | ((b_c_nan << 1 | b_c_nan) & FP_CLS_NAN);

   z_c = ((a_c >> 1 | b_c >> 1) << 1) | (((a_c >> 1) & (a_c & 1)) | ((b_c >> 1) & (b_c & 1)) | ((a_c & 1) & (b_c & 1)) | (1 & (~(a_c >> 1)) & ((~a_c) & 1) & (b_c >> 1)) | (1 & (~(b_c >> 1)) & ((~b_c) & 1) & (a_c >> 1)));

#else

   if(aExp == 0x7FF || bExp == 0x7FF || bExp == 0 || aExp == 0)
   {
      if(aExp == 0x7FF)
      {
         if(aSig || ((bExp == 0x7FF) && bSig))
         {
            return __propagateFloat64NaN(a, b);
         }
         if((bExp | bSig) == 0)
         {
            __float_raise(float_flag_invalid);
            return __float64_default_nan;
         }
         return __packFloat64(zSign, 0x7FF, 0);
      }
      if(bExp == 0x7FF)
      {
         if(bSig)
            return __propagateFloat64NaN(a, b);
         if((aExp | aSig) == 0)
         {
            __float_raise(float_flag_invalid);
            return __float64_default_nan;
         }
         return __packFloat64(zSign, 0x7FF, 0);
      }
      if(aExp == 0)
      {
#ifdef NO_SUBNORMALS
         return __packFloat64(zSign, 0, 0);
#else
         if(aSig == 0)
            return __packFloat64(zSign, 0, 0);
         __normalizeFloat64Subnormal(aSig, &aExp, &aSig);
#endif
      }
      if(bExp == 0)
      {
#ifdef NO_SUBNORMALS
         return __packFloat64(zSign, 0, 0);
#else
         if(bSig == 0)
            return __packFloat64(zSign, 0, 0);
         __normalizeFloat64Subnormal(bSig, &bExp, &bSig);
#endif
      }
   }
#endif

#ifndef DEFAULT

   __bits16 expSumPreSub = aExp + bExp;
   __bits16 bias = 1023;
   __bits16 expSum = expSumPreSub - bias;
   __bits16 expPostNorm;
   _Bool norm, expSigOvf0, expSigOvf2;
#ifndef NO_ROUNDING
   _Bool sticky, guard, round, expSigOvf1;
#endif
   __bits64 expSig, expSigPostRound, zSig;
   __bits8 excPostNorm;

   __bits64 sigProdHigh_52, sigProdLow_54, sigProdExtHigh_52, sigProdExtLow_54;
   aSig = (aSig | 0x0010000000000000);
   bSig = (bSig | 0x0010000000000000);

#if 1
   // start multi-part multiplication 53x53=>106
   // karatsuba
   // u = 2^27*u1+u0; //53bit -> u1=(53-K)bit u0=Kbit
   // v = 2^27*v1+v0; //53bit -> v1=(53-K)bit v0=Kbit
   // k0=u1*v1; // k0=2*(53-K)bit
   // k1=u0*v0; // k1=2*Kbit
   // k3=u0+u1; // k3=1+(max(K,53-K))bit
   // k4=v0+v1; // k4=1+(max(K,53-K))bit
   // k5=k3*k4; // k5=2*(1+(max(K,53-K)))bit
   // u*v = 2^54*k0 + k1    + 2^27*(k5 - k0 - k1);
   //       ^105:2*K   ^2*K-1:0   ^2*(1+(max(K,53-K)))-1:K
   // possible values for K=[31:22]
#define KARATSUBA_BITS 23
   __bits64 u1, u0, v1, v0, k0, k1, k2, k3, k4, k5, k6, k7, res_2K_0, res_106_2K;
   u0 = aSig & ((1ULL << KARATSUBA_BITS) - 1);
   u1 = (aSig >> KARATSUBA_BITS) & ((1ULL << (53 - KARATSUBA_BITS)) - 1);
   v0 = bSig & ((1ULL << KARATSUBA_BITS) - 1);
   v1 = (bSig >> KARATSUBA_BITS) & ((1ULL << (53 - KARATSUBA_BITS)) - 1);
   k0 = u1 * v1;
   k1 = u0 * v0;
   k3 = u0 + u1;
   k4 = v0 + v1;
   k5 = k3 * k4;
   k6 = k5 - k0 - k1;
   k7 = (k1 >> KARATSUBA_BITS) + k6;
   res_2K_0 = (k1 & ((1ULL << KARATSUBA_BITS) - 1)) | ((k7 & ((1ULL << KARATSUBA_BITS) - 1)) << KARATSUBA_BITS);
   res_106_2K = (k7 >> KARATSUBA_BITS) + k0;
#if KARATSUBA_BITS == 27
   sigProdLow_54 = res_2K_0;
   sigProdHigh_52 = res_106_2K;
#elif KARATSUBA_BITS < 27
   sigProdLow_54 = ((res_106_2K & ((1ULL << (54 - 2 * KARATSUBA_BITS)) - 1)) << (2 * KARATSUBA_BITS)) | res_2K_0;
   sigProdHigh_52 = res_106_2K >> (54 - 2 * KARATSUBA_BITS);
#else
   sigProdLow_54 = res_2K_0 & ((1ULL << 54) - 1);
   sigProdHigh_52 = (res_106_2K << (2 * KARATSUBA_BITS - 54)) | ((res_2K_0 >> 54) & ((1ULL << (2 * KARATSUBA_BITS - 54)) - 1));
#endif
#else
   __bits64 sigProdHigh, sigProdLow;
   sigProdHigh = _umulh64(aSig, bSig);
   sigProdLow = _umul64(aSig, bSig);
   sigProdLow_54 = sigProdLow & ((1ULL << 54) - 1);
   sigProdHigh_52 = ((sigProdHigh << 10) | ((sigProdLow >> 54) & ((1ULL << 10) - 1))) & ((1ULL << 52) - 1);
#endif
   norm = (sigProdHigh_52 >> (51)) & 1;
   expPostNorm = expSum + norm;
   sigProdExtHigh_52 = (norm ? (sigProdHigh_52 << 1) | ((sigProdLow_54 >> 53) & 1) : (sigProdHigh_52 << 2) | ((sigProdLow_54 >> 52) & 3)) & ((1ULL << 52) - 1);
   sigProdExtLow_54 = (norm ? sigProdLow_54 << 1 : sigProdLow_54 << 2) & ((1ULL << 54) - 1);
   expSig = (((__bits64)expPostNorm) << 52) | sigProdExtHigh_52;
   expSigOvf0 = (expPostNorm >> 12) & 1;
#ifndef NO_ROUNDING
   sticky = (sigProdExtLow_54 >> 53) & 1;
   guard = (sigProdExtLow_54 & ((1ULL << 53) - 1)) != 0;
   round = sticky & ((guard & !(sigProdExtHigh_52 & 1)) | (sigProdExtHigh_52 & 1));

   expSigPostRound = expSig + round;
   expSigOvf1 = round & (expSig == ((__bits64)-1));
   expSigOvf2 = expSigOvf0 ^ expSigOvf1;
#else
   expSigPostRound = expSig;
   expSigOvf2 = expSigOvf0;
#endif
   excPostNorm = (expSigOvf2 << 1) | ((expSigPostRound >> 63) & 1);
   zSig = (((__bits64)zSign) << 63) | (expSigPostRound & ((1ULL << 63) - 1));
   if(z_c == FP_CLS_NORMAL)
      z_c = excPostNorm == 0 ? FP_CLS_NORMAL : (excPostNorm == 1 ? FP_CLS_INF : FP_CLS_ZERO);

   if(z_c == FP_CLS_NORMAL)
      return zSig;
   else if(z_c == FP_CLS_ZERO)
      return __packFloat64(zSign, 0, 0);
   else if(z_c == FP_CLS_NAN)
      return __float64_default_nan;
   else
      return __packFloat64(zSign, 0x7FF, 0);

#else

   __bits64 zSig0, zSig1;
   zExp = aExp + bExp - 0x3FF;
   aSig = (aSig | LIT64(0x0010000000000000)) << 10;
   bSig = (bSig | LIT64(0x0010000000000000)) << 11;
   __mul64To128(aSig, bSig, &zSig0, &zSig1);
   zSig0 |= (zSig1 != 0);
   if(0 <= (__sbits64)(zSig0 << 1))
   {
      zSig0 <<= 1;
      --zExp;
   }
   return __roundAndPackFloat64(zSign, zExp, zSig0);
#endif
}

/*----------------------------------------------------------------------------
| Returns the result of dividing the double-precision floating-point value `a'
| by the corresponding value `b'.  The operation is performed according to
| the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

#ifndef UNROLL_FACTOR_F64_DIV
#define UNROLL_FACTOR_F64_DIV 1
#endif

#define LOOP_BODY_F64_DIV(z, n, data)                  \
   current_sel = (((current >> 51) & 15) << 1) | MsbB; \
   q_i0 = (0xF1FFFF6C >> current_sel) & 1;             \
   q_i1 = (0xFE00FFD0 >> current_sel) & 1;             \
   q_i2 = SELECT_BIT(current_sel, 4);                  \
   nq_i2 = !q_i2;                                      \
   /*q_i = tableR4[current_sel];*/                     \
   q_i = (q_i2 << 2) | (q_i1 << 1) | q_i0;             \
   positive |= (q_i1 << 1) | q_i0;                     \
   positive <<= 2;                                     \
   negative |= q_i2 << 1;                              \
   negative <<= 2;                                     \
   switch(q_i)                                         \
   {                                                   \
      case 1:                                          \
         w = nbSig;                                    \
         break;                                        \
      case 7:                                          \
         w = bSig;                                     \
         break;                                        \
      case 2:                                          \
         w = nbSigx2;                                  \
         break;                                        \
      case 6:                                          \
         w = bSigx2;                                   \
         break;                                        \
      case 3:                                          \
         w = nbSigx3;                                  \
         break;                                        \
      case 5:                                          \
         w = bSigx3;                                   \
         break;                                        \
      default: /*case 0: case 4:*/                     \
         w = 0;                                        \
         break;                                        \
   }                                                   \
   current = (current << 1) + w;                       \
   BIT_RESIZE(current, 54);                            \
   current <<= 1;

static __FORCE_INLINE __float64 __float64_divSRT4(__float64 a, __float64 b)
{
   VOLATILE_DEF _Bool aSign, bSign, zSign, q_i2, q_i1, q_i0, nq_i2;
   VOLATILE_DEF _Bool MsbB = SELECT_BIT(b, 51), correction;
   VOLATILE_DEF __int16 aExp, bExp, zExp;
   VOLATILE_DEF __bits64 aSig, bSig, nbSig, bSigx2, nbSigx2, zSig1, zSig0;
   VOLATILE_DEF __bits64 zExpSig;
   VOLATILE_DEF __bits64 bSigx3, nbSigx3, current, w, positive = 0, negative = 0;
   VOLATILE_DEF __bits8 current_sel, q_i, index;
#ifndef NO_ROUNDING
   VOLATILE_DEF _Bool LSB_bit, Guard_bit, Round_bit, round;
#endif

   aSig = __extractFloat64Frac(a);
   aExp = __extractFloat64Exp(a);
   aSign = __extractFloat64Sign(a);
   bSig = __extractFloat64Frac(b);
   bExp = __extractFloat64Exp(b);
   bSign = __extractFloat64Sign(b);
   zSign = aSign ^ bSign;
   _Bool aExp_null = aExp == 0;
   _Bool bExp_null = bExp == 0;
   __bits8 a_c, b_c, z_c;
   _Bool a_c_zero, b_c_zero, a_c_inf, b_c_inf, a_c_nan, b_c_nan, a_c_normal, b_c_normal;
#ifndef NO_SUBNORMALS
   a_c_zero = aExp_null && aSig == 0;
#else
   a_c_zero = aExp_null;
#endif
   a_c_inf = aExp == 0x7FF && aSig == 0;
   a_c_nan = aExp == 0x7FF && aSig != 0;
   a_c_normal = aExp != 0x7FF && !a_c_zero;
   a_c = ((a_c_zero << 1 | a_c_zero) & FP_CLS_ZERO) | ((a_c_normal << 1 | a_c_normal) & FP_CLS_NORMAL) | ((a_c_inf << 1 | a_c_inf) & FP_CLS_INF) | ((a_c_nan << 1 | a_c_nan) & FP_CLS_NAN);

#ifndef NO_SUBNORMALS
   b_c_zero = bExp_null && bSig == 0;
#else
   b_c_zero = bExp_null;
#endif
   b_c_inf = bExp == 0x7FF && bSig == 0;
   b_c_nan = bExp == 0x7FF && bSig != 0;
   b_c_normal = bExp != 0x7FF && !b_c_zero;
   b_c = ((b_c_zero << 1 | b_c_zero) & FP_CLS_ZERO) | ((b_c_normal << 1 | b_c_normal) & FP_CLS_NORMAL) | ((b_c_inf << 1 | b_c_inf) & FP_CLS_INF) | ((b_c_nan << 1 | b_c_nan) & FP_CLS_NAN);

   z_c = ((a_c >> 1 | (1 & (~(b_c >> 1)) & (~(b_c & 1))) | (1 & (b_c >> 1) & b_c)) << 1) |
         ((1 & (a_c >> 1) & a_c) | (1 & (b_c >> 1) & b_c) | (1 & (a_c >> 1) & (b_c >> 1)) | (1 & a_c & b_c) | (1 & (~(a_c >> 1)) & (~(a_c & 1)) & (~(b_c >> 1)) & (~(b_c & 1))));

#ifndef NO_SUBNORMALS
   if(aExp_null && !a_c_zero)
   {
      unsigned long long int subnormal_lz, mshifted;
      count_leading_zero_macro_lshift(52, aSig, subnormal_lz, mshifted);
      aExp = -subnormal_lz;
      aSig = SELECT_RANGE(mshifted, 50, 0) << 1;
   }
   if(bExp_null && !b_c_zero)
   {
      unsigned long long int subnormal_lz, mshifted;
      count_leading_zero_macro_lshift(52, bSig, subnormal_lz, mshifted);
      bExp = -subnormal_lz;
      bSig = SELECT_RANGE(mshifted, 50, 0) << 1;
   }
#endif
   aSig = aSig | LIT64(0x0010000000000000);
   bSig = bSig | LIT64(0x0010000000000000);
   nbSig = -bSig;
   bSigx2 = bSig * 2;
   nbSigx2 = -bSigx2;
   bSigx3 = bSigx2 + bSig;
   nbSigx3 = -bSigx3;
   current = aSig;
   for(index = 0; index < (28 / UNROLL_FACTOR_F64_DIV); ++index)
   {
      BOOST_PP_REPEAT(UNROLL_FACTOR_F64_DIV, LOOP_BODY_F64_DIV, index);
   }
   BOOST_PP_REPEAT(BOOST_PP_MOD(28, UNROLL_FACTOR_F64_DIV), LOOP_BODY_F64_DIV, index);
   if(current != 0)
   {
      positive |= 2;
      negative |= SELECT_BIT(current, 54) << 1;
   }
   negative <<= 1;
   BIT_RESIZE(negative, 58);
   zSig0 = positive - negative;
   zSig0 >>= 1;
   _Bool ZSig0LSB = SELECT_BIT(zSig0, 0);
   zSig0 = (zSig0 >> 1) | ZSig0LSB;
   BIT_RESIZE(zSig0, 56);
   correction = SELECT_BIT(zSig0, 55);
   if(correction)
      zSig1 = SELECT_RANGE(zSig0, 54, 2) << 1 | (SELECT_BIT(zSig0, 1)) | (SELECT_BIT(zSig0, 0));
   else
      zSig1 = SELECT_RANGE(zSig0, 53, 0);
#ifndef NO_ROUNDING
   LSB_bit = SELECT_BIT(zSig1, 2);
   Guard_bit = SELECT_BIT(zSig1, 1);
   Round_bit = SELECT_BIT(zSig1, 0);
   round = Guard_bit & (LSB_bit | Round_bit);
#endif
   zExp = aExp - bExp + (0x3FE | correction);
   _Bool MSB1zExp = SELECT_BIT(zExp, 12);
   _Bool MSB0zExp = SELECT_BIT(zExp, 11);
   BIT_RESIZE(zExp, 12);
#ifndef NO_ROUNDING
   zExpSig = ((((__bits64)zExp) << 52) | (zSig1 >> 2)) + round;
#else
   zExpSig = ((((__bits64)zExp) << 52) | (zSig1 >> 2));
#endif
   _Bool MSBzExp = SELECT_BIT(zExpSig, 63);
   _Bool ovfCond = ((((MSB0zExp & ((~MSBzExp) & 1)) & 1) ^ MSB1zExp) & 1);
   if(z_c == FP_CLS_NORMAL)
   {
      if(ovfCond)
         return __packFloat64(zSign, 0, 0);
      else if(SELECT_BIT(zExpSig, 63) || zExp == 2047)
         return __packFloat64(zSign, 0x7FF, 0);
      else
         return (((__bits64)zSign) << 63) | SELECT_RANGE(zExpSig, 62, 0);
   }
   else if(z_c == FP_CLS_ZERO)
      return __packFloat64(zSign, 0, 0);
   else if(z_c == FP_CLS_NAN)
      return ((__bits64)((a_c_nan ? aSign : bSign) | (a_c_inf & b_c_inf) | (a_c_zero & b_c_zero))) << 63 | 0x7FF8000000000000 | (a_c_nan ? aSig : (b_c_nan ? bSig : 0));
   else
      return __packFloat64(zSign, 0x7FF, 0);
}

static __FORCE_INLINE __float64 __float64_divG(__float64 a, __float64 b)
{
   __flag aSign, bSign, zSign;
   __int16 aExp, bExp, zExp;
   __bits64 aSig, bSig, zSig;
   __bits64 term0, term1;
   __bits64 c5, c4, c3, c2, c1, c0, shft, p1, bSigsqr1, bSigsqr2, p2, p3, p, ga0, gb0, ga1, gb1, rem0, rem, rnd;

   aSig = __extractFloat64Frac(a);
   aExp = __extractFloat64Exp(a);
   aSign = __extractFloat64Sign(a);
   bSig = __extractFloat64Frac(b);
   bExp = __extractFloat64Exp(b);
   bSign = __extractFloat64Sign(b);
   zSign = aSign ^ bSign;
#ifndef DEFAULT
   __bits8 a_c, b_c, z_c;
   _Bool aExp_null = aExp == 0;
   _Bool bExp_null = bExp == 0;
   _Bool a_c_zero, b_c_zero, a_c_inf, b_c_inf, a_c_nan, b_c_nan, a_c_normal, b_c_normal;
#ifndef NO_SUBNORMALS
   a_c_zero = aExp_null && aSig == 0;
#else
   a_c_zero = aExp_null;
#endif
   a_c_inf = aExp == 0x7FF && aSig == 0;
   a_c_nan = aExp == 0x7FF && aSig != 0;
   a_c_normal = aExp != 0x7FF && !a_c_zero;
   a_c = ((a_c_zero << 1 | a_c_zero) & FP_CLS_ZERO) | ((a_c_normal << 1 | a_c_normal) & FP_CLS_NORMAL) | ((a_c_inf << 1 | a_c_inf) & FP_CLS_INF) | ((a_c_nan << 1 | a_c_nan) & FP_CLS_NAN);

#ifndef NO_SUBNORMALS
   b_c_zero = bExp_null && bSig == 0;
#else
   b_c_zero = bExp_null;
#endif
   b_c_inf = bExp == 0x7FF && bSig == 0;
   b_c_nan = bExp == 0x7FF && bSig != 0;
   b_c_normal = bExp != 0x7FF && !b_c_zero;
   b_c = ((b_c_zero << 1 | b_c_zero) & FP_CLS_ZERO) | ((b_c_normal << 1 | b_c_normal) & FP_CLS_NORMAL) | ((b_c_inf << 1 | b_c_inf) & FP_CLS_INF) | ((b_c_nan << 1 | b_c_nan) & FP_CLS_NAN);

   z_c = ((a_c >> 1 | (1 & (~(b_c >> 1)) & (~(b_c & 1))) | (1 & (b_c >> 1) & b_c)) << 1) |
         ((1 & (a_c >> 1) & a_c) | (1 & (b_c >> 1) & b_c) | (1 & (a_c >> 1) & (b_c >> 1)) | (1 & a_c & b_c) | (1 & (~(a_c >> 1)) & (~(a_c & 1)) & (~(b_c >> 1)) & (~(b_c & 1))));

#ifndef NO_SUBNORMALS
   if(aExp_null && !a_c_zero)
   {
      unsigned long long int subnormal_lz, mshifted;
      count_leading_zero_macro_lshift(52, aSig, subnormal_lz, mshifted);
      aExp = -subnormal_lz;
      aSig = SELECT_RANGE(mshifted, 50, 0) << 1;
   }
   if(bExp_null && !b_c_zero)
   {
      unsigned long long int subnormal_lz, mshifted;
      count_leading_zero_macro_lshift(52, bSig, subnormal_lz, mshifted);
      bExp = -subnormal_lz;
      bSig = SELECT_RANGE(mshifted, 50, 0) << 1;
   }
#endif
#else

   if(aExp == 0x7FF || bExp == 0x7FF || bExp == 0 || aExp == 0)
   {
      if(aExp == 0x7FF)
      {
         if(aSig)
            return __propagateFloat64NaN(a, b);
         if(bExp == 0x7FF)
         {
            if(bSig)
               return __propagateFloat64NaN(a, b);
            __float_raise(float_flag_invalid);
            return __float64_default_nan;
         }
         return __packFloat64(zSign, 0x7FF, 0);
      }
      if(bExp == 0x7FF)
      {
         if(bSig)
            return __propagateFloat64NaN(a, b);
         return __packFloat64(zSign, 0, 0);
      }
      if(bExp == 0)
      {
         if(bSig == 0)
         {
            if((aExp | aSig) == 0)
            {
               __float_raise(float_flag_invalid);
               return __float64_default_nan;
            }
            __float_raise(float_flag_divbyzero);
            return __packFloat64(zSign, 0x7FF, 0);
         }
#ifdef NO_SUBNORMALS
         return __packFloat64(zSign, 0x7FF, 0);
#else
         __normalizeFloat64Subnormal(bSig, &bExp, &bSig);
#endif
      }
      if(aExp == 0)
      {
#ifdef NO_SUBNORMALS
         return __packFloat64(zSign, 0, 0);
#else
         if(aSig == 0)
            return __packFloat64(zSign, 0, 0);
         __normalizeFloat64Subnormal(aSig, &aExp, &aSig);
#endif
      }
   }
#endif
#ifndef DEFAULT
   GOLDSCHMIDT_MANTISSA_DIVISION_64();
   if(z_c == FP_CLS_NORMAL)
      return __roundAndPackFloat64(zSign, zExp, zSig);
   else if(z_c == FP_CLS_ZERO)
      return __packFloat64(zSign, 0, 0);
   else if(z_c == FP_CLS_NAN)
      return ((__bits64)((a_c_nan ? aSign : bSign) | (a_c_inf & b_c_inf) | (a_c_zero & b_c_zero))) << 63 | 0x7FF8000000000000 | (a_c_nan ? aSig : (b_c_nan ? bSig : 0));
   else
      return __packFloat64(zSign, 0x7FF, 0);
#else
   /*On this part it just comments out old code*/
   zExp = aExp - bExp + 0x3FD;
   aSig = (aSig | LIT64(0x0010000000000000)) << 10;
   bSig = (bSig | LIT64(0x0010000000000000)) << 11;
   if(bSig <= (aSig + aSig))
   {
      aSig >>= 1;
      ++zExp;
   }
   zSig = __estimateDiv128To64(aSig, 0, bSig);
   if((zSig & 0x1FF) <= 2)
   {
      __bits64 rem0, rem1;
      __mul64To128(bSig, zSig, &term0, &term1);
      __sub128(aSig, 0, term0, term1, &rem0, &rem1);
      while((__sbits64)rem0 < 0)
      {
         --zSig;
         __add128(rem0, rem1, 0, bSig, &rem0, &rem1);
      }
      zSig |= (rem1 != 0);
   }
   return __roundAndPackFloat64(zSign, zExp, zSig);

#endif
}

/*----------------------------------------------------------------------------
| Returns the remainder of the double-precision floating-point value `a'
| with respect to the corresponding value `b'.  The operation is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float64 __float64_rem(__float64 a, __float64 b)
{
   __flag aSign, bSign, zSign;
   __int16 aExp, bExp, expDiff;
   __bits64 aSig, bSig;
   __bits64 q, alternateASig;
   __sbits64 sigMean;

   aSig = __extractFloat64Frac(a);
   aExp = __extractFloat64Exp(a);
   aSign = __extractFloat64Sign(a);
   bSig = __extractFloat64Frac(b);
   bExp = __extractFloat64Exp(b);
   bSign = __extractFloat64Sign(b);
   if(aExp == 0x7FF)
   {
      if(aSig || ((bExp == 0x7FF) && bSig))
      {
         return __propagateFloat64NaN(a, b);
      }
      __float_raise(float_flag_invalid);
      return __float64_default_nan;
   }
   if(bExp == 0x7FF)
   {
      if(bSig)
         return __propagateFloat64NaN(a, b);
      return a;
   }
   if(bExp == 0)
   {
      if(bSig == 0)
      {
         __float_raise(float_flag_invalid);
         return __float64_default_nan;
      }
      __normalizeFloat64Subnormal(bSig, &bExp, &bSig);
   }
   if(aExp == 0)
   {
      if(aSig == 0)
         return a;
      __normalizeFloat64Subnormal(aSig, &aExp, &aSig);
   }
   expDiff = aExp - bExp;
   aSig = (aSig | LIT64(0x0010000000000000)) << 11;
   bSig = (bSig | LIT64(0x0010000000000000)) << 11;
   if(expDiff < 0)
   {
      if(expDiff < -1)
         return a;
      aSig >>= 1;
   }
   q = (bSig <= aSig);
   if(q)
      aSig -= bSig;
   expDiff -= 64;
   while(0 < expDiff)
   {
      q = __estimateDiv128To64(aSig, 0, bSig);
      q = (2 < q) ? q - 2 : 0;
      aSig = -((bSig >> 2) * q);
      expDiff -= 62;
   }
   expDiff += 64;
   if(0 < expDiff)
   {
      q = __estimateDiv128To64(aSig, 0, bSig);
      q = (2 < q) ? q - 2 : 0;
      q >>= 64 - expDiff;
      bSig >>= 2;
      aSig = ((aSig >> 1) << (expDiff - 1)) - bSig * q;
   }
   else
   {
      aSig >>= 2;
      bSig >>= 2;
   }
   do
   {
      alternateASig = aSig;
      ++q;
      aSig -= bSig;
   } while(0 <= (__sbits64)aSig);
   sigMean = aSig + alternateASig;
   if((sigMean < 0) || ((sigMean == 0) && (q & 1)))
   {
      aSig = alternateASig;
   }
   zSign = ((__sbits64)aSig < 0);
   if(zSign)
      aSig = -aSig;
   return __normalizeRoundAndPackFloat64(aSign ^ zSign, bExp, aSig);
}

/*----------------------------------------------------------------------------
| Returns the square root of the double-precision floating-point value `a'.
| The operation is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __float64 __float64_sqrt(__float64 a)
{
   __flag aSign;
   __int16 aExp, zExp;
   __bits64 aSig, zSig, doubleZSig;
   __bits64 rem0, rem1, term0, term1;
   __float64 z;

   aSig = __extractFloat64Frac(a);
   aExp = __extractFloat64Exp(a);
   aSign = __extractFloat64Sign(a);
   if(aExp == 0x7FF)
   {
      if(aSig)
         return __propagateFloat64NaN(a, a);
      if(!aSign)
         return a;
      __float_raise(float_flag_invalid);
      return __float64_default_nan;
   }
   if(aSign)
   {
      if((aExp | aSig) == 0)
         return a;
      __float_raise(float_flag_invalid);
      return __float64_default_nan;
   }
   if(aExp == 0)
   {
      if(aSig == 0)
         return 0;
      __normalizeFloat64Subnormal(aSig, &aExp, &aSig);
   }
   zExp = ((aExp - 0x3FF) >> 1) + 0x3FE;
   aSig |= LIT64(0x0010000000000000);
   zSig = __estimateSqrt32(aExp, aSig >> 21);
   aSig <<= 9 - (aExp & 1);
   zSig = __estimateDiv128To64(aSig, 0, zSig << 32) + (zSig << 30);
   if((zSig & 0x1FF) <= 5)
   {
      doubleZSig = zSig << 1;
      __mul64To128(zSig, zSig, &term0, &term1);
      __sub128(aSig, 0, term0, term1, &rem0, &rem1);
      while((__sbits64)rem0 < 0)
      {
         --zSig;
         doubleZSig -= 2;
         __add128(rem0, rem1, zSig >> 63, doubleZSig | 1, &rem0, &rem1);
      }
      zSig |= ((rem0 | rem1) != 0);
   }
   return __roundAndPackFloat64(0, zExp, zSig);
}

/*----------------------------------------------------------------------------
| Returns 1 if the double-precision floating-point value `a' is equal to the
| corresponding value `b', and 0 otherwise.  The comparison is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag __float64_eq(__float64 a, __float64 b)
{
   if(((__extractFloat64Exp(a) == 0x7FF) && __extractFloat64Frac(a)) || ((__extractFloat64Exp(b) == 0x7FF) && __extractFloat64Frac(b)))
   {
      if(__float64_is_signaling_nan(a) || __float64_is_signaling_nan(b))
      {
         __float_raise(float_flag_invalid);
      }
      return 0;
   }
   return (a == b) || ((__bits64)((a | b) << 1) == 0);
}

/*----------------------------------------------------------------------------
| Returns 1 if the double-precision floating-point value `a' is less than or
| equal to the corresponding value `b', and 0 otherwise.  The comparison is
| performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag __float64_le(__float64 a, __float64 b)
{
   __flag aSign, bSign;

   if(((__extractFloat64Exp(a) == 0x7FF) && __extractFloat64Frac(a)) || ((__extractFloat64Exp(b) == 0x7FF) && __extractFloat64Frac(b)))
   {
      __float_raise(float_flag_invalid);
      return 0;
   }
   aSign = __extractFloat64Sign(a);
   bSign = __extractFloat64Sign(b);
   if(aSign != bSign)
      return aSign || ((__bits64)((a | b) << 1) == 0);
   return (a == b) || (aSign ^ (a < b));
}

/*----------------------------------------------------------------------------
| Returns 1 if the double-precision floating-point value `a' is less than
| the corresponding value `b', and 0 otherwise.  The comparison is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag __float64_lt(__float64 a, __float64 b)
{
   __flag aSign, bSign;

   if(((__extractFloat64Exp(a) == 0x7FF) && __extractFloat64Frac(a)) || ((__extractFloat64Exp(b) == 0x7FF) && __extractFloat64Frac(b)))
   {
      __float_raise(float_flag_invalid);
      return 0;
   }
   aSign = __extractFloat64Sign(a);
   bSign = __extractFloat64Sign(b);
   if(aSign != bSign)
      return aSign && ((__bits64)((a | b) << 1) != 0);
   return (a != b) && (aSign ^ (a < b));
}

/*----------------------------------------------------------------------------
| Returns 1 if the double-precision floating-point value `a' is equal to the
| corresponding value `b', and 0 otherwise.  The invalid exception is raised
| if either operand is a NaN.  Otherwise, the comparison is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag __float64_eq_signaling(__float64 a, __float64 b)
{
   if(((__extractFloat64Exp(a) == 0x7FF) && __extractFloat64Frac(a)) || ((__extractFloat64Exp(b) == 0x7FF) && __extractFloat64Frac(b)))
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

static __FORCE_INLINE __flag __float64_le_quiet(__float64 a, __float64 b)
{
   __flag aSign, bSign;
   __int16 aExp, bExp;

   if(((__extractFloat64Exp(a) == 0x7FF) && __extractFloat64Frac(a)) || ((__extractFloat64Exp(b) == 0x7FF) && __extractFloat64Frac(b)))
   {
      if(__float64_is_signaling_nan(a) || __float64_is_signaling_nan(b))
      {
         __float_raise(float_flag_invalid);
      }
      return 0;
   }
   aSign = __extractFloat64Sign(a);
   bSign = __extractFloat64Sign(b);
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

static __FORCE_INLINE __flag __float64_lt_quiet(__float64 a, __float64 b)
{
   __flag aSign, bSign;

   if(((__extractFloat64Exp(a) == 0x7FF) && __extractFloat64Frac(a)) || ((__extractFloat64Exp(b) == 0x7FF) && __extractFloat64Frac(b)))
   {
      if(__float64_is_signaling_nan(a) || __float64_is_signaling_nan(b))
      {
         __float_raise(float_flag_invalid);
      }
      return 0;
   }
   aSign = __extractFloat64Sign(a);
   bSign = __extractFloat64Sign(b);
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

static __FORCE_INLINE __int32 __floatx80_to_int32(__floatx80 a)
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

static __FORCE_INLINE __int32 __floatx80_to_int32_round_to_zero(__floatx80 a)
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

static __FORCE_INLINE __int64 __floatx80_to_int64(__floatx80 a)
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

static __FORCE_INLINE __int64 __floatx80_to_int64_round_to_zero(__floatx80 a)
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

static __FORCE_INLINE __float32 __floatx80_to_float32(__floatx80 a)
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
      return __packFloat32(aSign, 0xFF, 0);
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

static __FORCE_INLINE __float64 __floatx80_to_float64(__floatx80 a)
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
      return __packFloat64(aSign, 0x7FF, 0);
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

static __FORCE_INLINE __float128 __floatx80_to_float128(__floatx80 a)
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

static __FORCE_INLINE __floatx80 __floatx80_round_to_int(__floatx80 a)
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

static __FORCE_INLINE __floatx80 floatx80_add(__floatx80 a, __floatx80 b)
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

static __FORCE_INLINE __floatx80 __floatx80_sub(__floatx80 a, __floatx80 b)
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

static __FORCE_INLINE __floatx80 __floatx80_mul(__floatx80 a, __floatx80 b)
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

static __FORCE_INLINE __floatx80 __floatx80_div(__floatx80 a, __floatx80 b)
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

static __FORCE_INLINE __floatx80 __floatx80_rem(__floatx80 a, __floatx80 b)
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
   if(__lt128(alternateASig0, alternateASig1, aSig0, aSig1) || (__eq128(alternateASig0, alternateASig1, aSig0, aSig1) && (q & 1)))
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

static __FORCE_INLINE __floatx80 __floatx80_sqrt(__floatx80 a)
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

static __FORCE_INLINE __flag __floatx80_eq(__floatx80 a, __floatx80 b)
{
   if(((__extractFloatx80Exp(a) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(a) << 1)) || ((__extractFloatx80Exp(b) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(b) << 1)))
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

static __FORCE_INLINE __flag __floatx80_le(__floatx80 a, __floatx80 b)
{
   __flag aSign, bSign;

   if(((__extractFloatx80Exp(a) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(a) << 1)) || ((__extractFloatx80Exp(b) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(b) << 1)))
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

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is
| less than the corresponding value `b', and 0 otherwise.  The comparison
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag __floatx80_lt(__floatx80 a, __floatx80 b)
{
   __flag aSign, bSign;

   if(((__extractFloatx80Exp(a) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(a) << 1)) || ((__extractFloatx80Exp(b) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(b) << 1)))
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

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is equal
| to the corresponding value `b', and 0 otherwise.  The invalid exception is
| raised if either operand is a NaN.  Otherwise, the comparison is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag __floatx80_eq_signaling(__floatx80 a, __floatx80 b)
{
   if(((__extractFloatx80Exp(a) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(a) << 1)) || ((__extractFloatx80Exp(b) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(b) << 1)))
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

static __FORCE_INLINE __flag __floatx80_le_quiet(__floatx80 a, __floatx80 b)
{
   __flag aSign, bSign;

   if(((__extractFloatx80Exp(a) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(a) << 1)) || ((__extractFloatx80Exp(b) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(b) << 1)))
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

static __FORCE_INLINE __flag __floatx80_lt_quiet(__floatx80 a, __floatx80 b)
{
   __flag aSign, bSign;

   if(((__extractFloatx80Exp(a) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(a) << 1)) || ((__extractFloatx80Exp(b) == 0x7FFF) && (__bits64)(__extractFloatx80Frac(b) << 1)))
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

static __FORCE_INLINE __int32 __float128_to_int32(__float128 a)
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

static __FORCE_INLINE __int32 __float128_to_int32_round_to_zero(__float128 a)
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

static __FORCE_INLINE __int64 __float128_to_int64(__float128 a)
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

static __FORCE_INLINE __int64 __float128_to_int64_round_to_zero(__float128 a)
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

static __FORCE_INLINE __float32 __float128_to_float32(__float128 a)
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
      return __packFloat32(aSign, 0xFF, 0);
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

static __FORCE_INLINE __float64 __float128_to_float64(__float128 a)
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
      return __packFloat64(aSign, 0x7FF, 0);
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

static __FORCE_INLINE __floatx80 __float128_to_floatx80(__float128 a)
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

static __FORCE_INLINE __float128 __float128_round_to_int(__float128 a)
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

static __FORCE_INLINE __float128 __float128_add(__float128 a, __float128 b)
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

static __FORCE_INLINE __float128 __float128_sub(__float128 a, __float128 b)
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

static __FORCE_INLINE __float128 __float128_mul(__float128 a, __float128 b)
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

static __FORCE_INLINE __float128 __float128_div(__float128 a, __float128 b)
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

static __FORCE_INLINE __float128 __float128_rem(__float128 a, __float128 b)
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

static __FORCE_INLINE __float128 __float128_sqrt(__float128 a)
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

static __FORCE_INLINE __flag __float128_eq(__float128 a, __float128 b)
{
   if(((__extractFloat128Exp(a) == 0x7FFF) && (__extractFloat128Frac0(a) | __extractFloat128Frac1(a))) || ((__extractFloat128Exp(b) == 0x7FFF) && (__extractFloat128Frac0(b) | __extractFloat128Frac1(b))))
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

static __FORCE_INLINE __flag __float128_le(__float128 a, __float128 b)
{
   __flag aSign, bSign;

   if(((__extractFloat128Exp(a) == 0x7FFF) && (__extractFloat128Frac0(a) | __extractFloat128Frac1(a))) || ((__extractFloat128Exp(b) == 0x7FFF) && (__extractFloat128Frac0(b) | __extractFloat128Frac1(b))))
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

/*----------------------------------------------------------------------------
| Returns 1 if the quadruple-precision floating-point value `a' is less than
| the corresponding value `b', and 0 otherwise.  The comparison is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag __float128_lt(__float128 a, __float128 b)
{
   __flag aSign, bSign;

   if(((__extractFloat128Exp(a) == 0x7FFF) && (__extractFloat128Frac0(a) | __extractFloat128Frac1(a))) || ((__extractFloat128Exp(b) == 0x7FFF) && (__extractFloat128Frac0(b) | __extractFloat128Frac1(b))))
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

/*----------------------------------------------------------------------------
| Returns 1 if the quadruple-precision floating-point value `a' is equal to
| the corresponding value `b', and 0 otherwise.  The invalid exception is
| raised if either operand is a NaN.  Otherwise, the comparison is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static __FORCE_INLINE __flag __float128_eq_signaling(__float128 a, __float128 b)
{
   if(((__extractFloat128Exp(a) == 0x7FFF) && (__extractFloat128Frac0(a) | __extractFloat128Frac1(a))) || ((__extractFloat128Exp(b) == 0x7FFF) && (__extractFloat128Frac0(b) | __extractFloat128Frac1(b))))
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

static __FORCE_INLINE __flag __float128_le_quiet(__float128 a, __float128 b)
{
   __flag aSign, bSign;

   if(((__extractFloat128Exp(a) == 0x7FFF) && (__extractFloat128Frac0(a) | __extractFloat128Frac1(a))) || ((__extractFloat128Exp(b) == 0x7FFF) && (__extractFloat128Frac0(b) | __extractFloat128Frac1(b))))
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

static __FORCE_INLINE __flag __float128_lt_quiet(__float128 a, __float128 b)
{
   __flag aSign, bSign;

   if(((__extractFloat128Exp(a) == 0x7FFF) && (__extractFloat128Frac0(a) | __extractFloat128Frac1(a))) || ((__extractFloat128Exp(b) == 0x7FFF) && (__extractFloat128Frac0(b) | __extractFloat128Frac1(b))))
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
