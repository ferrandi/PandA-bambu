/**
 * Porting of the libm library to the PandA framework
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
 */
/* sf_modf.c -- float version of s_modf.c.
 * Conversion to float by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
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

#include "math_privatef.h"

static const float one = 1.0;

float modff(float x, float* iptr)
{
   int i0, j0;
   unsigned i;
   GET_FLOAT_WORD(i0, x);
   j0 = ((i0 >> 23) & 0xff) - 0x7f; /* exponent of x */
   if(j0 < 23)
   { /* integer part in x */
      if(j0 < 0)
      {                                          /* |x|<1 */
         SET_FLOAT_WORD(*iptr, i0 & 0x80000000); /* *iptr = +-0 */
         return x;
      }
      else
      {
         i = (0x007fffff) >> j0;
         if((i0 & i) == 0)
         { /* x is integral */
            unsigned ix;
            *iptr = x;
            GET_FLOAT_WORD(ix, x);
            SET_FLOAT_WORD(x, ix & 0x80000000); /* return +-0 */
            return x;
         }
         else
         {
            SET_FLOAT_WORD(*iptr, i0 & (~i));
            return x - *iptr;
         }
      }
   }
   else
   { /* no fraction part */
      unsigned ix;
      *iptr = x * one;
      /* We must handle NaNs separately.  */
      if(j0 == 0x80 && (i0 & 0x7fffff))
         return __builtin_nanf("");
      GET_FLOAT_WORD(ix, x);
      SET_FLOAT_WORD(x, ix & 0x80000000); /* return +-0 */
      return x;
   }
}

#ifdef _DOUBLE_IS_32BITS

double modf(double x, double* iptr)
{
   return (double)modff((float)x, (float*)iptr);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
