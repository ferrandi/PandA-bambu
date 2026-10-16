/* sf_ldexp.c -- float version of s_ldexp.c.
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
#ifndef _IEEE_LIBM
#include <errno.h>
#endif

#ifdef __STDC__
	float ldexpf(float value, int exp)
#else
	float ldexpf(value, exp)
	float value; int exp;
#endif
{
	if(!finitef(value)||value==(float)0.0) return value;
	value = scalbnf(value,exp);
#ifndef _IEEE_LIBM
	if(!finitef(value)||value==(float)0.0) errno = ERANGE;
#endif
	return value;
}

#ifdef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double ldexp(double value, int exp)
#else
	double ldexp(value, exp)
	double value; int exp;
#endif
{
	return (double) ldexpf((float) value, exp);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
