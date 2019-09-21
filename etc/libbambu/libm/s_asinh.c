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

/* asinh(x)
 * Method :
 *	Based on
 *		asinh(x) = sign(x) * log [ |x| + sqrt(x*x+1) ]
 *	we have
 *	asinh(x) := x  if  1+x*x=1,
 *		 := sign(x)*(log(x)+ln2)) for large |x|, else
 *		 := sign(x)*log(2|x|+1/(|x|+sqrt(x*x+1))) if|x|>2, else
 *		 := sign(x)*log1p(|x| + x^2/(1 + sqrt(1+x^2)))
 */
double asinh(double x)
{
   double t, w;
   int hx, ix;
   hx = GET_HI(x);
   ix = hx & 0x7fffffff;
   if(ix >= 0x7ff00000)
      return x; /* x is inf or NaN */
   if(ix < 0x3e300000)
   { /* |x|<2**-28 */
      if(huge + x > one)
         return x; /* return x inexact except 0 */
   }
   if(ix > 0x41b00000)
   { /* |x| > 2**28 */
      w = __hide_ieee754_log(fabs(x)) + ln2;
   }
   else if(ix > 0x40000000)
   { /* 2**28 > |x| > 2.0 */
      t = fabs(x);
      w = __hide_ieee754_log(2.0 * t + one / (sqrt(x * x + one) + t));
   }
   else
   { /* 2.0 > |x| > 2**-28 */
      t = x * x;
      w = log1p(fabs(x) + t / (one + sqrt(one + t)));
   }
   if(hx > 0)
      return w;
   else
      return -w;
}
