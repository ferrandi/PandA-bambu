/*
+--------------------------------------------------------------------------+
| CHStone : a suite of benchmark programs for C-based High-Level Synthesis |
| ======================================================================== |
|                                                                          |
| * Collected and Modified : Y. Hara, H. Tomiyama, S. Honda,               |
|                            H. Takada and K. Ishii                        |
|                            Nagoya University, Japan                      |
|                                                                          |
| * Remark :                                                               |
|    1. This source code is modified to unify the formats of the benchmark |
|       programs in CHStone.                                               |
|    2. Test vectors are added for CHStone.                                |
|    3. If "main_result" is 0 at the end of the program, the program is    |
|       correctly executed.                                                |
|    4. Please follow the copyright of each benchmark program.             |
+--------------------------------------------------------------------------+
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

/*----------------------------------------------------------------------------
| Floating-point rounding mode, extended double-precision rounding precision,
| and exception flags.
*----------------------------------------------------------------------------*/
int8 float_rounding_mode = float_round_nearest_even;
int8 float_exception_flags = 0;

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
| Returns the fraction bits of the double-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

INLINE bits64
extractFloat64Frac (float64 a)
{

  return a & LIT64 (0x000FFFFFFFFFFFFF);

}

/*----------------------------------------------------------------------------
| Returns the exponent bits of the double-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

INLINE int16
extractFloat64Exp (float64 a)
{

  return (a >> 52) & 0x7FF;

}

/*----------------------------------------------------------------------------
| Returns the sign bit of the double-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

INLINE flag
extractFloat64Sign (float64 a)
{

  return a >> 63;

}

/*----------------------------------------------------------------------------
| Normalizes the subnormal double-precision floating-point value represented
| by the denormalized significand `aSig'.  The normalized exponent and
| significand are stored at the locations pointed to by `zExpPtr' and
| `zSigPtr', respectively.
*----------------------------------------------------------------------------*/

static void
normalizeFloat64Subnormal (bits64 aSig, int16 * zExpPtr, bits64 * zSigPtr)
{
  int8 shiftCount;

  shiftCount = countLeadingZeros64 (aSig) - 11;
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

INLINE float64
packFloat64 (flag zSign, int16 zExp, bits64 zSig)
{

  return (((bits64) zSign) << 63) + (((bits64) zExp) << 52) + zSig;

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

static float64
roundAndPackFloat64 (flag zSign, int16 zExp, bits64 zSig)
{
  int8 roundingMode;
  flag roundNearestEven, isTiny;
  int16 roundIncrement, roundBits;

  roundingMode = float_rounding_mode;
  roundNearestEven = (roundingMode == float_round_nearest_even);
  roundIncrement = 0x200;
  if (!roundNearestEven)
    {
      if (roundingMode == float_round_to_zero)
	{
	  roundIncrement = 0;
	}
      else
	{
	  roundIncrement = 0x3FF;
	  if (zSign)
	    {
	      if (roundingMode == float_round_up)
		roundIncrement = 0;
	    }
	  else
	    {
	      if (roundingMode == float_round_down)
		roundIncrement = 0;
	    }
	}
    }
  roundBits = zSig & 0x3FF;
  if (0x7FD <= (bits16) zExp)
    {
      if ((0x7FD < zExp)
	  || ((zExp == 0x7FD) && ((sbits64) (zSig + roundIncrement) < 0)))
	{
	  float_raise (float_flag_overflow | float_flag_inexact);
	  return packFloat64 (zSign, 0x7FF, 0) - (roundIncrement == 0);
	}
      if (zExp < 0)
	{
	  isTiny = (float_detect_tininess == float_tininess_before_rounding)
	    || (zExp < -1)
	    || (zSig + roundIncrement < LIT64 (0x8000000000000000));
	  shift64RightJamming (zSig, -zExp, &zSig);
	  zExp = 0;
	  roundBits = zSig & 0x3FF;
	  if (isTiny && roundBits)
	    float_raise (float_flag_underflow);
	}
    }
  if (roundBits)
    float_exception_flags |= float_flag_inexact;
  zSig = (zSig + roundIncrement) >> 10;
  zSig &= ~(((roundBits ^ 0x200) == 0) & roundNearestEven);
  if (zSig == 0)
    zExp = 0;
  return packFloat64 (zSign, zExp, zSig);

}

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent `zExp',
| and significand `zSig', and returns the proper double-precision floating-
| point value corresponding to the abstract input.  This routine is just like
| `roundAndPackFloat64' except that `zSig' does not have to be normalized.
| Bit 63 of `zSig' must be zero, and `zExp' must be 1 less than the ``true''
| floating-point exponent.
*----------------------------------------------------------------------------*/

static float64
normalizeRoundAndPackFloat64 (flag zSign, int16 zExp, bits64 zSig)
{
  int8 shiftCount;

  shiftCount = countLeadingZeros64 (zSig) - 1;
  return roundAndPackFloat64 (zSign, zExp - shiftCount, zSig << shiftCount);

}

/*----------------------------------------------------------------------------
| Returns the result of converting the 32-bit two's complement integer `a'
| to the double-precision floating-point format.  The conversion is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

float64
int32_to_float64 (int32 a)
{
  flag zSign;
  uint32 absA;
  int8 shiftCount;
  bits64 zSig;

  if (a == 0)
    return 0;
  zSign = (a < 0);
  absA = zSign ? -a : a;
  shiftCount = countLeadingZeros32 (absA) + 21;
  zSig = absA;
  return packFloat64 (zSign, 0x432 - shiftCount, zSig << shiftCount);

}

/*----------------------------------------------------------------------------
| Returns the result of adding the absolute values of the double-precision
| floating-point values `a' and `b'.  If `zSign' is 1, the sum is negated
| before being returned.  `zSign' is ignored if the result is a NaN.
| The addition is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static float64
addFloat64Sigs (float64 a, float64 b, flag zSign)
{
  int16 aExp, bExp, zExp;
  bits64 aSig, bSig, zSig;
  int16 expDiff;

  aSig = extractFloat64Frac (a);
  aExp = extractFloat64Exp (a);
  bSig = extractFloat64Frac (b);
  bExp = extractFloat64Exp (b);
  expDiff = aExp - bExp;
  aSig <<= 9;
  bSig <<= 9;
  if (0 < expDiff)
    {
      if (aExp == 0x7FF)
	{
	  if (aSig)
	    return propagateFloat64NaN (a, b);
	  return a;
	}
      if (bExp == 0)
	--expDiff;
      else
	bSig |= LIT64 (0x2000000000000000);
      shift64RightJamming (bSig, expDiff, &bSig);
      zExp = aExp;
    }
  else if (expDiff < 0)
    {
      if (bExp == 0x7FF)
	{
	  if (bSig)
	    return propagateFloat64NaN (a, b);
	  return packFloat64 (zSign, 0x7FF, 0);
	}
      if (aExp == 0)
	++expDiff;
      else
	{
	  aSig |= LIT64 (0x2000000000000000);
	}
      shift64RightJamming (aSig, -expDiff, &aSig);
      zExp = bExp;
    }
  else
    {
      if (aExp == 0x7FF)
	{
	  if (aSig | bSig)
	    return propagateFloat64NaN (a, b);
	  return a;
	}
      if (aExp == 0)
	return packFloat64 (zSign, 0, (aSig + bSig) >> 9);
      zSig = LIT64 (0x4000000000000000) + aSig + bSig;
      zExp = aExp;
      goto roundAndPack;
    }
  aSig |= LIT64 (0x2000000000000000);
  zSig = (aSig + bSig) << 1;
  --zExp;
  if ((sbits64) zSig < 0)
    {
      zSig = aSig + bSig;
      ++zExp;
    }
roundAndPack:
  return roundAndPackFloat64 (zSign, zExp, zSig);

}

/*----------------------------------------------------------------------------
| Returns the result of subtracting the absolute values of the double-
| precision floating-point values `a' and `b'.  If `zSign' is 1, the
| difference is negated before being returned.  `zSign' is ignored if the
| result is a NaN.  The subtraction is performed according to the IEC/IEEE
| Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

static float64
subFloat64Sigs (float64 a, float64 b, flag zSign)
{
  int16 aExp, bExp, zExp;
  bits64 aSig, bSig, zSig;
  int16 expDiff;

  aSig = extractFloat64Frac (a);
  aExp = extractFloat64Exp (a);
  bSig = extractFloat64Frac (b);
  bExp = extractFloat64Exp (b);
  expDiff = aExp - bExp;
  aSig <<= 10;
  bSig <<= 10;
  if (0 < expDiff)
    goto aExpBigger;
  if (expDiff < 0)
    goto bExpBigger;
  if (aExp == 0x7FF)
    {
      if (aSig | bSig)
	return propagateFloat64NaN (a, b);
      float_raise (float_flag_invalid);
      return float64_default_nan;
    }
  if (aExp == 0)
    {
      aExp = 1;
      bExp = 1;
    }
  if (bSig < aSig)
    goto aBigger;
  if (aSig < bSig)
    goto bBigger;
  return packFloat64 (float_rounding_mode == float_round_down, 0, 0);
bExpBigger:
  if (bExp == 0x7FF)
    {
      if (bSig)
	return propagateFloat64NaN (a, b);
      return packFloat64 (zSign ^ 1, 0x7FF, 0);
    }
  if (aExp == 0)
    ++expDiff;
  else
    aSig |= LIT64 (0x4000000000000000);
  shift64RightJamming (aSig, -expDiff, &aSig);
  bSig |= LIT64 (0x4000000000000000);
bBigger:
  zSig = bSig - aSig;
  zExp = bExp;
  zSign ^= 1;
  goto normalizeRoundAndPack;
aExpBigger:
  if (aExp == 0x7FF)
    {
      if (aSig)
	return propagateFloat64NaN (a, b);
      return a;
    }
  if (bExp == 0)
    --expDiff;
  else
    bSig |= LIT64 (0x4000000000000000);
  shift64RightJamming (bSig, expDiff, &bSig);
  aSig |= LIT64 (0x4000000000000000);
aBigger:
  zSig = aSig - bSig;
  zExp = aExp;
normalizeRoundAndPack:
  --zExp;
  return normalizeRoundAndPackFloat64 (zSign, zExp, zSig);

}

/*----------------------------------------------------------------------------
| Returns the result of adding the double-precision floating-point values `a'
| and `b'.  The operation is performed according to the IEC/IEEE Standard for
| Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

float64
float64_add (float64 a, float64 b)
{
  flag aSign, bSign;

  aSign = extractFloat64Sign (a);
  bSign = extractFloat64Sign (b);
  if (aSign == bSign)
    return addFloat64Sigs (a, b, aSign);
  else
    return subFloat64Sigs (a, b, aSign);

}

/*----------------------------------------------------------------------------
| Returns the result of multiplying the double-precision floating-point values
| `a' and `b'.  The operation is performed according to the IEC/IEEE Standard
| for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

float64
float64_mul (float64 a, float64 b)
{
  flag aSign, bSign, zSign;
  int16 aExp, bExp, zExp;
  bits64 aSig, bSig, zSig0, zSig1;

  aSig = extractFloat64Frac (a);
  aExp = extractFloat64Exp (a);
  aSign = extractFloat64Sign (a);
  bSig = extractFloat64Frac (b);
  bExp = extractFloat64Exp (b);
  bSign = extractFloat64Sign (b);
  zSign = aSign ^ bSign;
  if (aExp == 0x7FF)
    {
      if (aSig || ((bExp == 0x7FF) && bSig))
	return propagateFloat64NaN (a, b);
      if ((bExp | bSig) == 0)
	{
	  float_raise (float_flag_invalid);
	  return float64_default_nan;
	}
      return packFloat64 (zSign, 0x7FF, 0);
    }
  if (bExp == 0x7FF)
    {
      if (bSig)
	return propagateFloat64NaN (a, b);
      if ((aExp | aSig) == 0)
	{
	  float_raise (float_flag_invalid);
	  return float64_default_nan;
	}
      return packFloat64 (zSign, 0x7FF, 0);
    }
  if (aExp == 0)
    {
      if (aSig == 0)
	return packFloat64 (zSign, 0, 0);
      normalizeFloat64Subnormal (aSig, &aExp, &aSig);
    }
  if (bExp == 0)
    {
      if (bSig == 0)
	return packFloat64 (zSign, 0, 0);
      normalizeFloat64Subnormal (bSig, &bExp, &bSig);
    }
  zExp = aExp + bExp - 0x3FF;
  aSig = (aSig | LIT64 (0x0010000000000000)) << 10;
  bSig = (bSig | LIT64 (0x0010000000000000)) << 11;
  mul64To128 (aSig, bSig, &zSig0, &zSig1);
  zSig0 |= (zSig1 != 0);
  if (0 <= (sbits64) (zSig0 << 1))
    {
      zSig0 <<= 1;
      --zExp;
    }
  return roundAndPackFloat64 (zSign, zExp, zSig0);

}

/*----------------------------------------------------------------------------
| Returns the result of dividing the double-precision floating-point value `a'
| by the corresponding value `b'.  The operation is performed according to
| the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

float64
float64_div (float64 a, float64 b)
{
  flag aSign, bSign, zSign;
  int16 aExp, bExp, zExp;
  bits64 aSig, bSig, zSig;
  bits64 rem0, rem1, term0, term1;

  aSig = extractFloat64Frac (a);
  aExp = extractFloat64Exp (a);
  aSign = extractFloat64Sign (a);
  bSig = extractFloat64Frac (b);
  bExp = extractFloat64Exp (b);
  bSign = extractFloat64Sign (b);
  zSign = aSign ^ bSign;
  if (aExp == 0x7FF)
    {
      if (aSig)
	return propagateFloat64NaN (a, b);
      if (bExp == 0x7FF)
	{
	  if (bSig)
	    return propagateFloat64NaN (a, b);
	  float_raise (float_flag_invalid);
	  return float64_default_nan;
	}
      return packFloat64 (zSign, 0x7FF, 0);
    }
  if (bExp == 0x7FF)
    {
      if (bSig)
	return propagateFloat64NaN (a, b);
      return packFloat64 (zSign, 0, 0);
    }
  if (bExp == 0)
    {
      if (bSig == 0)
	{
	  if ((aExp | aSig) == 0)
	    {
	      float_raise (float_flag_invalid);
	      return float64_default_nan;
	    }
	  float_raise (float_flag_divbyzero);
	  return packFloat64 (zSign, 0x7FF, 0);
	}
      normalizeFloat64Subnormal (bSig, &bExp, &bSig);
    }
  if (aExp == 0)
    {
      if (aSig == 0)
	return packFloat64 (zSign, 0, 0);
      normalizeFloat64Subnormal (aSig, &aExp, &aSig);
    }
  zExp = aExp - bExp + 0x3FD;
  aSig = (aSig | LIT64 (0x0010000000000000)) << 10;
  bSig = (bSig | LIT64 (0x0010000000000000)) << 11;
  if (bSig <= (aSig + aSig))
    {
      aSig >>= 1;
      ++zExp;
    }
  zSig = estimateDiv128To64 (aSig, 0, bSig);
  if ((zSig & 0x1FF) <= 2)
    {
      mul64To128 (bSig, zSig, &term0, &term1);
      sub128 (aSig, 0, term0, term1, &rem0, &rem1);
      while ((sbits64) rem0 < 0)
	{
	  --zSig;
	  add128 (rem0, rem1, 0, bSig, &rem0, &rem1);
	}
      zSig |= (rem1 != 0);
    }
  return roundAndPackFloat64 (zSign, zExp, zSig);

}

/*----------------------------------------------------------------------------
| Returns 1 if the double-precision floating-point value `a' is less than or
| equal to the corresponding value `b', and 0 otherwise.  The comparison is
| performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

flag
float64_le (float64 a, float64 b)
{
  flag aSign, bSign;

  if (((extractFloat64Exp (a) == 0x7FF) && extractFloat64Frac (a))
      || ((extractFloat64Exp (b) == 0x7FF) && extractFloat64Frac (b)))
    {
      float_raise (float_flag_invalid);
      return 0;
    }
  aSign = extractFloat64Sign (a);
  bSign = extractFloat64Sign (b);
  if (aSign != bSign)
    return aSign || ((bits64) ((a | b) << 1) == 0);
  return (a == b) || (aSign ^ (a < b));

}

flag
float64_ge (float64 a, float64 b)
{
  return float64_le (b, a);
}

// added by hiroyuki@acm.org
float64
float64_neg (float64 x)
{
  return (((~x) & 0x8000000000000000ULL) | (x & 0x7fffffffffffffffULL));
}
