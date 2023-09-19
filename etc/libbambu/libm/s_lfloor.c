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
 * floor(x)
 * Return x rounded toward -inf to integral value
 * Method:
 *	Bit twiddling.
 * Exception:
 *	Inexact flag raised if x not equal to floor(x).
 */

// static const double huge = 1.0e300;

long int lfloor(double x)
{
   int i0, i1, j0;
   unsigned i, j;
   i0 = GET_HI(x);
   i1 = GET_LO(x);
   j0 = ((i0 >> 20) & 0x7ff) - 0x3ff;
   if(j0 < 20)
   {
      if(j0 < 0)
      {
         math_force_eval(huge + x); /* raise inexact flag */ /* return 0*sign(x) if |x|<1 */
         if(i0 >= 0)
         {
            i0 = i1 = 0;
         }
         else if(((i0 & 0x7fffffff) | i1) != 0)
         {
            i0 = 0xbff00000;
            i1 = 0;
         }
      }
      else
      {
         i = (0x000fffff) >> j0;
         if(((i0 & i) | i1) == 0)
            return x;               /* x is integral */
         math_force_eval(huge + x); /* raise inexact flag */
         if(i0 < 0)
            i0 += (0x00100000) >> j0;
         i0 &= (~i);
         i1 = 0;
      }
   }
   else if(j0 > 51)
   {
      if(j0 == 0x400)
         return X_PLUS_X(x); /* inf or NaN */
      else
         return x; /* x is integral */
   }
   else
   {
      i = ((unsigned)(0xffffffff)) >> (j0 - 20);
      if((i1 & i) == 0)
         return x;               /* x is integral */
      math_force_eval(huge + x); /* raise inexact flag */
      if(i0 < 0)
      {
         if(j0 == 20)
            i0 += 1;
         else
         {
            j = i1 + (1 << (52 - j0));
            if(j < i1)
               i0 += 1; /* got a carry */
            i1 = j;
         }
      }
      i1 &= (~i);
   }
   INSERT_WORDS(x, i0, i1);
   return (long int)x;
}
