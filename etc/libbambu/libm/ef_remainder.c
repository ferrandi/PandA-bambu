/**
 * Porting of the libm library to the PandA framework
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
 */
/* ef_remainder.c -- float version of e_remainder.c.
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

static const float zero = 0.0;

float __hide_ieee754_remainderf(float x, float p)
{
   int hx, hp;
   unsigned sx;
   float p_half;

   GET_FLOAT_WORD(hx, x);
   GET_FLOAT_WORD(hp, p);
   sx = hx & 0x80000000;
   hp &= 0x7fffffff;
   hx &= 0x7fffffff;

   /* purge off exception values */
   if(FLT_UWORD_IS_ZERO(hp) || !FLT_UWORD_IS_FINITE(hx) || FLT_UWORD_IS_NAN(hp))
      return nanf("");

   if(hp <= FLT_UWORD_HALF_MAX)
      x = __hide_ieee754_fmodf(x, p + p); /* now x < 2p */
   if((hx - hp) == 0)
      return zero * x;
   x = fabsf(x);
   p = fabsf(p);
   if(hp < 0x01000000)
   {
      if(x + x > p)
      {
         x -= p;
         if(x + x >= p)
            x -= p;
      }
   }
   else
   {
      p_half = (float)0.5 * p;
      if(x > p_half)
      {
         x -= p;
         if(x >= p_half)
            x -= p;
      }
   }
   GET_FLOAT_WORD(hx, x);
   SET_FLOAT_WORD(x, hx ^ sx);
   return x;
}
