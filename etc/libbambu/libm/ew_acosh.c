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
 *
 */
#include "math_private.h"

/* __hide_ieee754_acosh(x)
 * Method :
 *	Based on
 *		acosh(x) = log [ x + sqrt(x*x-1) ]
 *	we have
 *		acosh(x) := log(x)+ln2,	if x is large; else
 *		acosh(x) := log(2x-1/(sqrt(x*x-1)+x)) if x>2; else
 *		acosh(x) := log1p(t+sqrt(2.0*t+t*t)); where t=x-1.
 *
 * Special cases:
 *	acosh(x) is NaN with signal if x<1.
 *	acosh(NaN) is NaN without signal.
 */
double __hide_ieee754_acosh(double x)
{
   double t;
   int hx;
   hx = GET_HI(x);
   if(hx < 0x3ff00000)
   { /* x < 1 */
      return __builtin_nans("");
   }
   else if(hx >= 0x41b00000)
   { /* x > 2**28 */
      if(hx >= 0x7ff00000)
      { /* x is inf or NaN */
         return x;
      }
      else
         return __hide_ieee754_log(x) + ln2; /* acosh(huge)=log(2x) */
   }
   else if(((hx - 0x3ff00000) | GET_LO(x)) == 0)
   {
      return 0.0; /* acosh(1) = 0 */
   }
   else if(hx > 0x40000000)
   { /* 2**28 > x > 2 */
      t = x * x;
      return __hide_ieee754_log(2.0 * x - one / (x + sqrt(t - one)));
   }
   else
   { /* 1<x<2 */
      t = x - one;
      return log1p(t + sqrt(2.0 * t + t * t));
   }
}
/*
 * wrapper acosh(x)
 */

double acosh(double x) /* wrapper acosh */
{
#ifdef _IEEE_LIBM
   return __hide_ieee754_acosh(x);
#else
   double z;
   z = __hide_ieee754_acosh(x);
   if(_LIB_VERSION == _IEEE_ || isnan(x))
      return z;
   if(x < 1.0)
   {
      return __hide_kernel_standard(x, x, 29); /* acosh(x<1) */
   }
   else
      return z;
#endif
}
