/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
*/
/* wf_acos.c -- float version of w_acos.c.
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

/*
 * wrap_acosf(x)
 */

#include "math_privatef.h"
#ifndef _IEEE_LIBM
#include <errno.h>
#endif

float __builtin_acosf(float x)		/* wrapper acosf */
{
#ifdef _IEEE_LIBM
	return __hide_ieee754_acosf(x);
#else
	float z;
	struct exception exc;
	z = __hide_ieee754_acosf(x);
	if(_LIB_VERSION == _IEEE_ || __builtin_isnan(x)) return z;
	if(__builtin_fabsf(x)>(float)1.0) {
	    /* acosf(|x|>1) */
	    exc.type = DOMAIN;
	    exc.name = "acosf";
	    exc.err = 0;
	    exc.arg1 = exc.arg2 = (double)x;
	    exc.retval = nan("");
	    if (_LIB_VERSION == _POSIX_)
	       errno = EDOM;
	    else if (!__builtin_matherr(&exc)) {
	       errno = EDOM;
            }
            if (exc.err != 0)
	       errno = exc.err;
	    return (float)exc.retval; 
	} else
	    return z;
#endif
}

#ifdef _DOUBLE_IS_32BITS

double __builtin_acos(double x)
{
	return (double) __builtin_acosf((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
