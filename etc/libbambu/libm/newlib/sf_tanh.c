/* sf_tanh.c -- float version of s_tanh.c.
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

#include "fdlibm.h"

#ifdef __STDC__
static const RAISE_VOLATILE float one=1.0, two=2.0, tiny = 1.0e-30;
#else
static RAISE_VOLATILE float one=1.0, two=2.0, tiny = 1.0e-30;
#endif

#ifdef __STDC__
	float tanhf(float x)
#else
	float tanhf(x)
	float x;
#endif
{
	float t,z;
	__int32_t jx,ix;

	GET_FLOAT_WORD(jx,x);
	ix = jx&0x7fffffff;

    /* x is INF or NaN */
	if(!FLT_UWORD_IS_FINITE(ix)) {
#ifndef NO_RAISE_EXCEPTIONS
	    if (jx>=0) return one/x+one;    /* tanh(+-inf)=+-1 */
	    else       return one/x-one;    /* tanh(NaN) = NaN */
#else
		return FLT_UWORD_IS_INFINITE(ix) ? (((jx >> 31)&1)?-one:one) : __math_invalidf(x);
#endif
	}

    /* |x| < 22 */
	if (ix < 0x41b00000) {		/* |x|<22 */
	    if (ix<0x24000000) 		/* |x|<2**-55 */
		return x*(one+x);    	/* tanh(small) = small */
	    if (ix>=0x3f800000) {	/* |x|>=1  */
		t = expm1f(two*fabsf(x));
		z = one - two/(t+two);
	    } else {
	        t = expm1f(-two*fabsf(x));
	        z= -t/(t+two);
	    }
    /* |x| > 22, return +-1 */
	} else {
	    z = __math_raise(one - tiny, one);		/* raised inexact flag */
	}
	return (jx>=0)? z: -z;
}

#ifdef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double tanh(double x)
#else
	double tanh(x)
	double x;
#endif
{
	return (double) tanhf((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
