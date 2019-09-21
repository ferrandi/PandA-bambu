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
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#include "math_private.h"

/*
 * rint(x)
 * Return x rounded to integral value according to the prevailing
 * rounding mode.
 * Method:
 *	Using floating addition.
 * Exception:
 *	Inexact flag raised if x not equal to rint(x).
 */

static const double TWO52[2] = {
    4.50359962737049600000e+15,  /* 0x43300000, 0x00000000 */
    -4.50359962737049600000e+15, /* 0xC3300000, 0x00000000 */
};

double rint(double x)
{
   int i0, j0, sx;
   unsigned i, i1;
   double w, t;
   i0 = GET_HI(x);
   sx = (i0 >> 31) & 1;
   i1 = GET_LO(x);
   j0 = ((i0 >> 20) & 0x7ff) - 0x3ff;
   if(j0 < 20)
   {
      if(j0 < 0)
      {
         if(((i0 & 0x7fffffff) | i1) == 0)
            return x;
         i1 |= (i0 & 0x0fffff);
         i0 &= 0xfffe0000;
         i0 |= ((i1 | -i1) >> 12) & 0x80000;
         SET_HIGH_WORD(x, i0);
         w = TWO52[sx] + x;
         t = w - TWO52[sx];
         i0 = GET_HI(t);
         SET_HIGH_WORD(t, (i0 & 0x7fffffff) | (sx << 31));
         return t;
      }
      else
      {
         i = (0x000fffff) >> j0;
         if(((i0 & i) | i1) == 0)
            return x; /* x is integral */
         i >>= 1;
         if(((i0 & i) | i1) != 0)
         {
            if(j0 == 19)
               i1 = 0x40000000;
            else
               i0 = (i0 & (~i)) | ((0x20000) >> j0);
         }
      }
   }
   else if(j0 > 51)
   {
      if(j0 == 0x400)
         return x; /* inf or NaN */
      else
         return x; /* x is integral */
   }
   else
   {
      i = ((unsigned)(0xffffffff)) >> (j0 - 20);
      if((i1 & i) == 0)
         return x; /* x is integral */
      i >>= 1;
      if((i1 & i) != 0)
         i1 = (i1 & (~i)) | ((0x40000000) >> (j0 - 20));
   }
   INSERT_WORDS(x, i0, i1);
   w = TWO52[sx] + x;
   return w - TWO52[sx];
}
