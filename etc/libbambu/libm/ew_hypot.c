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

/* __ieee754_hypot(x,y)
 *
 * Method :
 *	If (assume round-to-nearest) z=x*x+y*y
 *	has error less than sqrt(2)/2 ulp, than
 *	sqrt(z) has error less than 1 ulp (exercise).
 *
 *	So, compute sqrt(x*x+y*y) with some care as
 *	follows to get the error below 1 ulp:
 *
 *	Assume x>y>0;
 *	(if possible, set rounding to round-to-nearest)
 *	1. if x > 2y  use
 *		x1*x1+(y*y+(x2*(x+x1))) for x*x+y*y
 *	where x1 = x with lower 32 bits cleared, x2 = x-x1; else
 *	2. if x <= 2y use
 *		t1*y1+((x-y)*(x-y)+(t1*y2+t2*y))
 *	where t1 = 2x with lower 32 bits cleared, t2 = 2x-t1,
 *	y1= y with lower 32 bits chopped, y2 = y-y1.
 *
 *	NOTE: scaling may be necessary if some argument is too
 *	      large or too tiny
 *
 * Special cases:
 *	hypot(x,y) is INF if x or y is +INF or -INF; else
 *	hypot(x,y) is NAN if x or y is NAN.
 *
 * Accuracy:
 * 	hypot(x,y) returns sqrt(x^2+y^2) with error less
 * 	than 1 ulps (units in the last place)
 */
double __hide_ieee754_hypot(double x, double y)
{
   double a = x, b = y, t1, t2, y1, y2, w;
   int j, k, ha, hb;

   ha = GET_HI(x) & 0x7fffffff; /* high word of  x */
   hb = GET_HI(y) & 0x7fffffff; /* high word of  y */
   if(hb > ha)
   {
      a = y;
      b = x;
      j = ha;
      ha = hb;
      hb = j;
   }
   else
   {
      a = x;
      b = y;
   }
   SET_HIGH_WORD(a, ha); /* a <- |a| */
   SET_HIGH_WORD(b, hb); /* b <- |b| */
   if((ha - hb) > 0x3c00000)
   {
      return a + b;
   } /* x/y > 2**60 */
   k = 0;
   if(ha > 0x5f300000)
   { /* a>2**500 */
      if(ha >= 0x7ff00000)
      {                         /* Inf or NaN */
         w = __builtin_nan(""); /* for sNaN */
         if(((ha & 0xfffff) | GET_LO(a)) == 0)
            w = a;
         if(((hb ^ 0x7ff00000) | GET_LO(b)) == 0)
            w = b;
         return w;
      }
      /* scale a and b by 2**-600 */
      ha -= 0x25800000;
      hb -= 0x25800000;
      k += 600;
      SET_HIGH_WORD(a, ha);
      SET_HIGH_WORD(b, hb);
   }
   if(hb < 0x20b00000)
   { /* b < 2**-500 */
      if(hb <= 0x000fffff)
      { /* subnormal b or 0 */
         if((hb | (GET_LO(b))) == 0)
            return a;
         t1 = 0;
         SET_HIGH_WORD(t1, 0x7fd00000); /* t1=2^1022 */
         b *= t1;
         a *= t1;
         k -= 1022;
      }
      else
      {                    /* scale a and b by 2^600 */
         ha += 0x25800000; /* a *= 2^600 */
         hb += 0x25800000; /* b *= 2^600 */
         k -= 600;
         SET_HIGH_WORD(a, ha);
         SET_HIGH_WORD(b, hb);
      }
   }
   /* medium size a and b */
   w = a - b;
   if(w > b)
   {
      t1 = 0;
      SET_HIGH_WORD(t1, ha);
      t2 = a - t1;
      w = sqrt(t1 * t1 - (b * (-b) - t2 * (a + t1)));
   }
   else
   {
      a = a + a;
      y1 = 0;
      SET_HIGH_WORD(y1, hb);
      y2 = b - y1;
      t1 = 0;
      SET_HIGH_WORD(t1, ha + 0x00100000);
      t2 = a - t1;
      w = sqrt(t1 * y1 - (w * (-w) - (t1 * y2 + t2 * b)));
   }
   if(k != 0)
   {
      t1 = 1.0;
      SET_HIGH_WORD(t1, (GET_HI(t1) + (k << 20)));
      return t1 * w;
   }
   else
      return w;
}

/*
 * wrapper hypot(x,y)
 */
double hypot(double x, double y) /* wrapper hypot */
{
#ifdef _IEEE_LIBM
   return __hide_ieee754_hypot(x, y);
#else
   double z;
   z = __hide_ieee754_hypot(x, y);
   if(_LIB_VERSION == _IEEE_)
      return z;
   if((!finite(z)) && finite(x) && finite(y))
      return __hide_kernel_standard(x, y, 4); /* hypot overflow */
   else
      return z;
#endif
}
