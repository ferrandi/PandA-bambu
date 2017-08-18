/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
*/
/* sf_cbrt.c -- float version of s_cbrt.c.
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

#include "math_privatef.h"

/* cbrtf(x)
 * Return cube root of x
 */
static const unsigned 
	B1 = 709958130, /* B1 = (84+2/3-0.03306235651)*2**23 */
	B2 = 642849266; /* B2 = (76+2/3-0.03306235651)*2**23 */

static const float
C =  5.4285717010e-01, /* 19/35     = 0x3f0af8b0 */
D = -7.0530611277e-01, /* -864/1225 = 0xbf348ef1 */
E =  1.4142856598e+00, /* 99/70     = 0x3fb50750 */
F =  1.6071428061e+00, /* 45/28     = 0x3fcdb6db */
G =  3.5714286566e-01; /* 5/14      = 0x3eb6db6e */

float __builtin_cbrtf(float x) 
{
	int	hx;
	float r,s,t;
	unsigned sign;
	unsigned high;

	GET_FLOAT_WORD(hx,x);
	sign=hx&0x80000000; 		/* sign= sign(x) */
	hx  ^=sign;
	if(!FLT_UWORD_IS_FINITE(hx))
	    return(x);		/* cbrt(NaN,INF) is itself */
	if(FLT_UWORD_IS_ZERO(hx))
	    return(x);			/* cbrt(0) is itself */

	SET_FLOAT_WORD(x,hx);	/* x <- |x| */
    /* rough cbrt to 5 bits */
	if(FLT_UWORD_IS_SUBNORMAL(hx)) 		/* subnormal number */
	  {SET_FLOAT_WORD(t,0x4b800000); /* set t= 2**24 */
	   t*=x; GET_FLOAT_WORD(high,t); SET_FLOAT_WORD(t,high/3+B2);
	  }
	else
	  SET_FLOAT_WORD(t,hx/3+B1);


    /* new cbrt to 23 bits */
	r=t*t/x;
	s=C+r*t;
	t*=G+F/(s+E+D/s);	

    /* retore the sign bit */
	GET_FLOAT_WORD(high,t);
	SET_FLOAT_WORD(t,high|sign);
	return(t);
}

#ifdef _DOUBLE_IS_32BITS

double __builtin_cbrt(double x)
{
	return (double) __builtin_cbrtf((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
