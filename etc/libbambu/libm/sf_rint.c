/**
 * Porting of the libm library to the PandA framework
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
 */
/* sf_rint.c -- float version of s_rint.c.
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

static const float TWO23[2] = {
    8.3886080000e+06,  /* 0x4b000000 */
    -8.3886080000e+06, /* 0xcb000000 */
};

float rintf(float x)
{
   int i0, j0, sx;
   unsigned i, i1, ix;
   float t;
   float w;
   GET_FLOAT_WORD(i0, x);
   sx = (i0 >> 31) & 1;
   ix = (i0 & 0x7fffffff);
   j0 = (ix >> 23) - 0x7f;
   if(j0 < 23)
   {
      if(FLT_UWORD_IS_ZERO(ix))
         return x;
      if(j0 < 0)
      {
         i1 = (i0 & 0x07fffff);
         i0 &= 0xfff00000;
         i0 |= ((i1 | -i1) >> 9) & 0x400000;
         SET_FLOAT_WORD(x, i0);
         w = TWO23[sx] + x;
         t = w - TWO23[sx];
         GET_FLOAT_WORD(i0, t);
         SET_FLOAT_WORD(t, (i0 & 0x7fffffff) | (sx << 31));
         return t;
      }
      else
      {
         i = (0x007fffff) >> j0;
         if((i0 & i) == 0)
            return x; /* x is integral */
         i >>= 1;
         if((i0 & i) != 0)
            i0 = (i0 & (~i)) | ((0x200000) >> j0);
      }
   }
   else
   {
      if(!FLT_UWORD_IS_FINITE(ix))
         return x; /* inf or NaN */
      else
         return x; /* x is integral */
   }
   SET_FLOAT_WORD(x, i0);
   w = TWO23[sx] + x;
   return w - TWO23[sx];
}

#ifdef _DOUBLE_IS_32BITS

double rint(double x)
{
   return (double)rintf((float)x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
