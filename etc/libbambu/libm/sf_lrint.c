/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
*/
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice 
 * is preserved.
 * ====================================================
 */

/*
 * lrintf(x)
 * Return x rounded to integral value according to the prevailing
 * rounding mode.
 * Method:
 *	Using floating addition.
 * Exception:
 *	Inexact flag raised if x not equal to lrintf(x).
 */

#include "math_privatef.h"

static const float
/* Adding a float, x, to 2^23 will cause the result to be rounded based on
   the fractional part of x, according to the implementation's current rounding
   mode.  2^23 is the smallest float that can be represented using all 23 significant
   digits. */
TWO23[2]={
  8.3886080000e+06, /* 0x4b000000 */
 -8.3886080000e+06, /* 0xcb000000 */
};

long int lrintf(float x)
{
  int j0,sx;
  unsigned i0;
  float t;
  float w;
  long int result;

  GET_FLOAT_WORD(i0,x);

  /* Extract sign bit. */
  sx = (i0 >> 31);

  /* Extract exponent field. */
  j0 = ((i0 & 0x7f800000) >> 23) - 127;
  
  if (j0 < (int)(sizeof (long int) * 8) - 1)
    {
      if (j0 < -1)
        return 0;
      else if (j0 >= 23)
        result = (long int) ((i0 & 0x7fffff) | 0x800000) << (j0 - 23);
      else
        {
          w = TWO23[sx] + x;
          t = w - TWO23[sx];
          GET_FLOAT_WORD (i0, t);
          /* Detect the all-zeros representation of plus and
             minus zero, which fails the calculation below. */
          if ((i0 & ~(1L << 31)) == 0)
              return 0;
          j0 = ((i0 >> 23) & 0xff) - 0x7f;
          i0 &= 0x7fffff;
          i0 |= 0x800000;
          result = i0 >> (23 - j0);
        }
    }
  else
    {
      return (long int) x;
    }
  return sx ? -result : result;
}

#ifdef _DOUBLE_IS_32BITS

long int lrint(double x)
{
  return lrintf((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
