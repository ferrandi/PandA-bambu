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
 * for non-zero x
 *	x = frexp(arg,&exp);
 * return a double fp quantity x such that 0.5 <= |x| <1.0
 * and the corresponding binary exponent "exp". That is
 *	arg = x*2^exp.
 * If arg is inf, 0.0, or NaN, then frexp(arg,&exp) returns arg
 * with *exp=0.
 */

// static const double
// two54 =  1.80143985094819840000e+16; /* 0x43500000, 0x00000000 */

double frexp(double x, int* eptr)
{
   int hx, ix, lx;
   hx = GET_HI(x);
   ix = 0x7fffffff & hx;
   lx = GET_LO(x);
   *eptr = 0;
   if(ix >= 0x7ff00000 || ((ix | lx) == 0))
      return x; /* 0,inf,nan */
   if(ix < 0x00100000)
   { /* subnormal */
      x *= two54;
      hx = GET_HI(x);
      ix = hx & 0x7fffffff;
      *eptr = -54;
   }
   *eptr += (ix >> 20) - 1022;
   hx = (hx & 0x800fffff) | 0x3fe00000;
   SET_HIGH_WORD(x, hx);
   return x;
}
