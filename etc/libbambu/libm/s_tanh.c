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

/* Tanh(x)
 * Return the Hyperbolic Tangent of x
 *
 * Method :
 *				       x    -x
 *				      e  - e
 *	0. tanh(x) is defined to be -----------
 *				       x    -x
 *				      e  + e
 *	1. reduce x to non-negative by tanh(-x) = -tanh(x).
 *	2.  0      <= x <= 2**-55 : tanh(x) := x*(one+x)
 *					        -t
 *	    2**-55 <  x <=  1     : tanh(x) := -----; t = expm1(-2x)
 *					       t + 2
 *						     2
 *	    1      <= x <=  22.0  : tanh(x) := 1-  ----- ; t=expm1(2x)
 *						   t + 2
 *	    22.0   <  x <= INF    : tanh(x) := 1.
 *
 * Special cases:
 *	tanh(NaN) is NaN;
 *	only tanh(0)=0 is exact for finite argument.
 */
double tanh(double x)
{
   double t, z;
   int jx, ix;
   unsigned long long ux;

   /* High word of |x|. */
   jx = GET_HI(x);
   ix = jx & 0x7fffffff;
   ux = __hide_get_uint64(x) & 0x7fffffffffffffffL;

   /* x is INF or NaN */
   if(ux >= 0x7ff0000000000000L)
   {
      /* tanh(+-inf)=+-1 */
      /* tanh(NaN) = NaN */
      return ux == 0x7ff0000000000000L ? copysign(one, x) : __builtin_nan("");
   }

   /* |x| < 22 */
   if(ix < 0x40360000)
   {                           /* |x|<22 */
      if(ix < 0x3c800000)      /* |x|<2**-55 */
         return x * (one + x); /* tanh(small) = small */
      if(ix >= 0x3ff00000)
      { /* |x|>=1  */
         t = expm1(two * fabs(x));
         z = one - two / (t + two);
      }
      else
      {
         t = expm1(-two * fabs(x));
         z = -t / (t + two);
      }
      /* |x| > 22, return +-1 */
   }
   else
   {
      z = one - tiny; /* raised inexact flag */
   }
   return (jx >= 0) ? z : -z;
}
