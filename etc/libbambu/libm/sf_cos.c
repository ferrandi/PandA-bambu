/**
 * Porting of the libm library to the PandA framework
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
 */
/* sf_cos.c -- float version of s_cos.c.
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

// static const float one=1.0;

static inline float __hide_local_cosf(float x)
{
   float y[2], z = 0.0;
   int n, ix;
   GET_FLOAT_WORD(ix, x);
   /* |x| ~< pi/4 */
   ix &= 0x7fffffff;
   if(ix <= 0x3f490fd8)
      return __hide_kernel_cosf(x, z);
   /* cos(Inf or NaN) is NaN */
   else if(!FLT_UWORD_IS_FINITE(ix))
      return nanf("");
   /* argument reduction needed */
   else
   {
      n = __hide_ieee754_rem_pio2f(x, y);
      switch(n & 3)
      {
         case 0:
            return __hide_kernel_cosf(y[0], y[1]);
         case 1:
            return -__hide_kernel_sinf(y[0], y[1], 1);
         case 2:
            return -__hide_kernel_cosf(y[0], y[1]);
         default:
            return __hide_kernel_sinf(y[0], y[1], 1);
      }
   }
}

float cosf(float x)
{
   return __hide_local_cosf(x);
}
#ifdef _DOUBLE_IS_32BITS

double cos(double x)
{
   return (double)cosf((float)x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
