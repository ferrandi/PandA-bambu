/**
 * Porting of the libm library to the PandA framework
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
 */
/* Adapted for Newlib, 2009.  (Allow for int < 32 bits; return *quo=0 during
 * errors to make test scripts easier.)  */
/*-
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#include "math_privatef.h"

/* For quotient, return either all 31 bits that can from calculation (using
 * int32_t), or as many as can fit into an int that is smaller than 32 bits.  */
#if INT_MAX > 0x7FFFFFFFL
#define QUO_MASK 0x7FFFFFFF
#else
#define QUO_MASK INT_MAX
#endif

static const float Zero[] = {
    0.0,
    -0.0,
};

/*
 * Return the IEEE remainder and set *quo to the last n bits of the
 * quotient, rounded to the nearest integer.  We choose n=31--if that many fit--
 * we wind up computing all the integer bits of the quotient anyway as
 * a side-effect of computing the remainder by the shift and subtract
 * method.  In practice, this is far more bits than are needed to use
 * remquo in reduction algorithms.
 */
float remquof(float x, float y, int* quo)
{
   int n, hx, hy, hz, ix, iy, sx, i;
   unsigned q, sxy;

   GET_FLOAT_WORD(hx, x);
   GET_FLOAT_WORD(hy, y);
   sxy = (hx ^ hy) & 0x80000000;
   sx = hx & 0x80000000; /* sign of x */
   hx ^= sx;             /* |x| */
   hy &= 0x7fffffff;     /* |y| */

   /* purge off exception values */
   if(hy == 0 || hx >= 0x7f800000 || hy > 0x7f800000)
   {            /* y=0,NaN;or x not finite */
      *quo = 0; /* Not necessary, but return consistent value */
      return (x * y) / (x * y);
   }
   if(hx < hy)
   {
      q = 0;
      goto fixup; /* |x|<|y| return x or x-y */
   }
   else if(hx == hy)
   {
      *quo = 1;
      return Zero[(unsigned)sx >> 31]; /* |x|=|y| return x*0*/
   }

   /* determine ix = ilogb(x) */
   if(hx < 0x00800000)
   { /* subnormal x */
      for(ix = -126, i = (hx << 8); i > 0; i <<= 1)
         ix -= 1;
   }
   else
      ix = (hx >> 23) - 127;

   /* determine iy = ilogb(y) */
   if(hy < 0x00800000)
   { /* subnormal y */
      for(iy = -126, i = (hy << 8); i > 0; i <<= 1)
         iy -= 1;
   }
   else
      iy = (hy >> 23) - 127;

   /* set up {hx,lx}, {hy,ly} and align y to x */
   if(ix >= -126)
      hx = 0x00800000 | (0x007fffff & hx);
   else
   { /* subnormal x, shift x to normal */
      n = -126 - ix;
      hx <<= n;
   }
   if(iy >= -126)
      hy = 0x00800000 | (0x007fffff & hy);
   else
   { /* subnormal y, shift y to normal */
      n = -126 - iy;
      hy <<= n;
   }

   /* fix point fmod */
   n = ix - iy;
   q = 0;
   while(n--)
   {
      hz = hx - hy;
      if(hz < 0)
         hx = hx << 1;
      else
      {
         hx = hz << 1;
         q++;
      }
      q <<= 1;
   }
   hz = hx - hy;
   if(hz >= 0)
   {
      hx = hz;
      q++;
   }

   /* convert back to floating value and restore the sign */
   if(hx == 0)
   { /* return sign(x)*0 */
      *quo = (sxy ? -q : q);
      return Zero[(unsigned)sx >> 31];
   }
   while(hx < 0x00800000)
   { /* normalize x */
      hx <<= 1;
      iy -= 1;
   }
   if(iy >= -126)
   { /* normalize output */
      hx = ((hx - 0x00800000) | ((iy + 127) << 23));
   }
   else
   { /* subnormal output */
      n = -126 - iy;
      hx >>= n;
   }
fixup:
   SET_FLOAT_WORD(x, hx);
   y = fabsf(y);
   if(y < 0x1p-125f)
   {
      if(x + x > y || (x + x == y && (q & 1)))
      {
         q++;
         x -= y;
      }
   }
   else if(x > 0.5f * y || (x == 0.5f * y && (q & 1)))
   {
      q++;
      x -= y;
   }
   GET_FLOAT_WORD(hx, x);
   SET_FLOAT_WORD(x, hx ^ sx);
   q &= 0x7fffffff;
   *quo = (sxy ? -q : q);
   return x;
}
