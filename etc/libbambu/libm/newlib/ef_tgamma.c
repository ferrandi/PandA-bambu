/* ef_tgamma.c -- float version of e_tgamma.c.
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
 *
 */

/* __ieee754_tgammaf(x)
 * Float version the Gamma function. Returns gamma(x)
 *
 * Method: See __ieee754_lgammaf_r
 */

#include "fdlibm.h"

#ifdef __STDC__
	float __ieee754_tgammaf(float x)
#else
	float __ieee754_tgammaf(x)
#endif
{
	int signgam_local;
	float y;
	__uint32_t hx;
	GET_FLOAT_WORD(hx,x);
	
	if(hx == 0) return __math_oflowf(0);
	if(((hx >> 31) & 1) && rintf(x) == x) return __math_invalidf(x);
	if(((hx >> 23) & 0xff) == 0xff) return x;
	
	y = __ieee754_expf(__ieee754_lgammaf_r(x, &signgam_local));
	if (signgam_local < 0)
		y = -y;
	return y;
}
