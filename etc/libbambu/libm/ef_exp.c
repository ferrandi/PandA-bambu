/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
*/
/* ef_exp.c -- float version of e_exp.c.
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

static const float
half[2] = {0.5f,-0.5f},
ln2hi   = 6.9314575195e-1f,  /* 0x3f317200 */
ln2lo   = 1.4286067653e-6f,  /* 0x35bfbe8e */
invln2  = 1.4426950216e+0f,  /* 0x3fb8aa3b */
/*
 * Domain [-0.34568, 0.34568], range ~[-4.278e-9, 4.447e-9]:
 * |x*(exp(x)+1)/(exp(x)-1) - p(x)| < 2**-27.74
 */
P1 =  1.6666625440e-1f, /*  0xaaaa8f.0p-26 */
P2 = -2.7667332906e-3f; /* -0xb55215.0p-32 */

float __hide_ieee754_expf(float x)
{
	float hi, lo, c, xx;
	int k, sign;
	unsigned int hx;

	GET_FLOAT_WORD(hx, x);
	sign = hx >> 31;   /* sign bit of x */
	hx &= 0x7fffffff;  /* high word of |x| */

	/* special cases */
	if (hx >= 0x42b17218) {  /* if |x| >= 88.722839f or NaN */
		if (hx > 0x7f800000)  /* NaN */
			return x;
		if (!sign) {
			/* overflow if x!=inf */
			STRICT_ASSIGN(float, x, x * 0x1p127f);
			return x;
		}
		if (hx == 0x7f800000)  /* -inf */
			return 0;
		if (hx >= 0x42cff1b5) { /* x <= -103.972084f */
			/* underflow */
			STRICT_ASSIGN(float, x, 0x1p-100f*0x1p-100f);
			return x;
		}
	}

	/* argument reduction */
	if (hx > 0x3eb17218) {  /* if |x| > 0.5 ln2 */
		if (hx > 0x3f851592)  /* if |x| > 1.5 ln2 */
			k = invln2*x + half[sign];
		else
			k = 1 - sign - sign;
		hi = x - k*ln2hi;  /* k*ln2hi is exact here */
		lo = k*ln2lo;
		STRICT_ASSIGN(float, x, hi - lo);
	} else if (hx > 0x39000000) {  /* |x| > 2**-14 */
		k = 0;
		hi = x;
		lo = 0;
	} else {
		/* raise inexact */
		return 1 + x;
	}

	/* x is now in primary range */
	xx = x*x;
	c = x - xx*(P1+xx*P2);
	x = 1 + (x*c/(2-c) - lo + hi);
	if (k == 0)
		return x;
    return scalbnf(x, k);
}
