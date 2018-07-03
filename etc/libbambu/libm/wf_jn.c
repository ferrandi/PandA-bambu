/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
*/
/* wf_jn.c -- float version of w_jn.c.
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
#ifndef _IEEE_LIBM
#include <errno.h>
#endif

float jnf(int n, float x)	/* wrapper jnf */
{
#ifdef _IEEE_LIBM
	return __hide_ieee754_jnf(n,x);
#else
	float z;
	struct exception exc;
	z = __hide_ieee754_jnf(n,x);
    if(_LIB_VERSION == _IEEE_ || isnan(x) ) return z;
    if(fabsf(x)>(float)X_TLOSS) {
	    /* jnf(|x|>X_TLOSS) */
            exc.type = TLOSS;
            exc.name = "jnf";
	    exc.err = 0;
	    exc.arg1 = (double)n;
	    exc.arg2 = (double)x;
            exc.retval = 0.0;
            if (_LIB_VERSION == _POSIX_)
                errno = ERANGE;
            else if (!matherr(&exc)) {
               errno = ERANGE;
            }        
	    if (exc.err != 0)
	       errno = exc.err;
            return exc.retval; 
	} else
	    return z;
#endif
}

float ynf(int n, float x)	/* wrapper ynf */
{
#ifdef _IEEE_LIBM
	return __hide_ieee754_ynf(n,x);
#else
	float z;
	struct exception exc;
	z = __hide_ieee754_ynf(n,x);
    if(_LIB_VERSION == _IEEE_ || isnan(x) ) return z;
        if(x <= (float)0.0){
	    /* ynf(n,0) = -inf or ynf(x<0) = NaN */
#ifndef HUGE_VAL 
#define HUGE_VAL inf
	    double inf = 0.0;

	    SET_HIGH_WORD(inf,0x7ff00000);	/* set inf to infinite */
#endif
	    exc.type = DOMAIN;	/* should be SING for IEEE */
	    exc.name = "ynf";
	    exc.err = 0;
	    exc.arg1 = (double)n;
	    exc.arg2 = (double)x;
	    if (_LIB_VERSION == _SVID_)
	        exc.retval = -HUGE;
	    else
	        exc.retval = -HUGE_VAL;
	    if (_LIB_VERSION == _POSIX_)
	        errno = EDOM;
        else if (!matherr(&exc)) {
	        errno = EDOM;
	    }
	    if (exc.err != 0)
	       errno = exc.err;
            return (float)exc.retval; 
        }
	if(x>(float)X_TLOSS) {
	    /* ynf(x>X_TLOSS) */
            exc.type = TLOSS;
            exc.name = "ynf";
	    exc.err = 0;
	    exc.arg1 = (double)n;
	    exc.arg2 = (double)x;
            exc.retval = 0.0;
            if (_LIB_VERSION == _POSIX_)
                errno = ERANGE;
            else if (!matherr(&exc)) {
                errno = ERANGE;
            }        
	    if (exc.err != 0)
	       errno = exc.err;
            return (float)exc.retval; 
	} else
	    return z;
#endif
}

#ifdef _DOUBLE_IS_32BITS

double jn(int n, double x)
{
    return (double) jnf(n, (float) x);
}

double yn(int n, double x)
{
    return (double) ynf(n, (float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
