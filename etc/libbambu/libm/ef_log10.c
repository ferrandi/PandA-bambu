/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
*/
/* ef_log10.c -- float version of e_log10.c.
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

/* |(log(1+s)-log(1-s))/s - Lg(s)| < 2**-34.24 (~[-4.95e-11, 4.97e-11]). */
static const float
Lg1 = 0xaaaaaa.0p-24, /* 0.66666662693 */
Lg2 = 0xccce13.0p-25, /* 0.40000972152 */
Lg3 = 0x91e9ee.0p-25, /* 0.28498786688 */
Lg4 = 0xf89e26.0p-26; /* 0.24279078841 */

static inline float __hide_log1pf(float f)
{
	float_t hfsq,s,z,R,w,t1,t2;

	s = f/(2.0f + f);
	z = s*s;
	w = z*z;
	t1 = w*(Lg2+w*Lg4);
	t2 = z*(Lg1+w*Lg3);
	R = t2+t1;
	hfsq = 0.5f * f * f;
	return s*(hfsq+R);
}

static const float
two25     =  3.3554432000e+07, /* 0x4c000000 */
ivln10hi  =  4.3432617188e-01, /* 0x3ede6000 */
ivln10lo  = -3.1689971365e-05, /* 0xb804ead9 */
log10_2hi =  3.0102920532e-01, /* 0x3e9a2080 */
log10_2lo =  7.9034151668e-07; /* 0x355427db */

float __hide_ieee754_log10f(float x)
{
	float f,hfsq,hi,lo,r,y;
	int i,k,hx;

	GET_FLOAT_WORD(hx, x);

	k = 0;
	if (hx < 0x00800000) {  /* x < 2**-126  */
		if ((hx&0x7fffffff) == 0)
			return -__builtin_inff();  /* log(+-0)=-inf */
		if (hx < 0)
			return __builtin_nansf("");   /* log(-#) = NaN */
		/* subnormal number, scale up x */
		k -= 25;
		x *= two25;
		GET_FLOAT_WORD(hx, x);
	}
	if (hx >= 0x7f800000)
		return x;
	if (hx == 0x3f800000)
		return 0.0f;  /* log(1) = +0 */
	k += (hx>>23) - 127;
	hx &= 0x007fffff;
	i = (hx+(0x4afb0d))&0x800000;
	SET_FLOAT_WORD(x, hx|(i^0x3f800000));  /* normalize x or x/2 */
	k += i>>23;
	y = (float)k;
	f = x - 1.0f;
	hfsq = 0.5f * f * f;
	r = __hide_log1pf(f);

	hi = f - hfsq;
	GET_FLOAT_WORD(hx, hi);
	SET_FLOAT_WORD(hi, hx&0xfffff000);
	lo = (f - hi) - hfsq + r;
	return y*log10_2lo + (lo+hi)*ivln10lo + lo*ivln10hi +
	        hi*ivln10hi + y*log10_2hi;
}
