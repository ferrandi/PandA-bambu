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

/* __ieee754_atanh(x)
 * Method :
 *    1.Reduced x to positive by atanh(-x) = -atanh(x)
 *    2.For x>=0.5
 *                  1              2x                          x
 *	atanh(x) = --- * log(1 + -------) = 0.5 * log1p(2 * --------)
 *                  2             1 - x                      1 - x
 *
 * 	For x<0.5
 *	atanh(x) = 0.5*log1p(2x+2x*x/(1-x))
 *
 * Special cases:
 *	atanh(x) is NaN if |x| > 1 with signal;
 *	atanh(NaN) is that NaN with no signal;
 *	atanh(+-1) is +-INF with signal.
 *
 */
double __hide_ieee754_atanh(double x)
{
   double t;
   int hx, ix;
   unsigned lx;
   hx = GET_HI(x); /* high word */
   lx = GET_LO(x); /* low word */
   ix = hx & 0x7fffffff;
   if((ix | ((lx | (-lx)) >> 31)) > 0x3ff00000) /* |x|>1 */
      return __builtin_nans("");
   if(ix == 0x3ff00000)
      return (hx >> 31) ? -__builtin_inf() : __builtin_inf();
   if(ix < 0x3e300000)
   {
      math_force_eval(huge + x);
      return x; /* x<2**-28 */
   }
   SET_HIGH_WORD(x, ix); /* x <- |x| */
   if(ix < 0x3fe00000)
   { /* x < 0.5 */
      t = x + x;
      t = 0.5 * log1p(t + t * x / (one - x));
   }
   else
      t = 0.5 * log1p((x + x) / (one - x));
   if(hx >> 31)
      return -t;
   else
      return t;
}

/*
 * wrapper atanh(x)
 */
double atanh(double x) /* wrapper atanh */
{
#ifdef _IEEE_LIBM
   return __hide_ieee754_atanh(x);
#else
   double z, y;
   z = __hide_ieee754_atanh(x);
   if(_LIB_VERSION == _IEEE_ || isnan(x))
      return z;
   y = fabs(x);
   if(y >= 1.0)
   {
      if(y > 1.0)
         return __hide_kernel_standard(x, x, 30); /* atanh(|x|>1) */
      else
         return __hide_kernel_standard(x, x, 31); /* atanh(|x|==1) */
   }
   else
      return z;
#endif
}
