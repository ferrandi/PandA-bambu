/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
*/
/* wrf_gamma.c -- float version of wr_gamma.c.
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
 * wrapper float gammaf_r(float x, int *signgamp)
 */

#include "math_privatef.h"
#ifndef _IEEE_LIBM
#include <errno.h>
#endif

float __builtin_gammaf_r(float x, int *signgamp) /* wrapper lgammaf_r */
{
#ifdef _IEEE_LIBM
	return __hide_ieee754_lgammaf_r(x,signgamp);
#else
        float y;
	struct exception exc;
        y = __hide_ieee754_lgammaf_r(x,signgamp);
        if(_LIB_VERSION == _IEEE_) return y;
        if(!__finitef(y)&&__finitef(x)) {
#ifndef HUGE_VAL 
#define HUGE_VAL inf
	    double inf = 0.0;

	    SET_HIGH_WORD(inf,0x7ff00000);	/* set inf to infinite */
#endif
	    exc.name = "gammaf";
	    exc.err = 0;
	    exc.arg1 = exc.arg2 = (double)x;
            if (_LIB_VERSION == _SVID_)
                exc.retval = HUGE;
            else
                exc.retval = HUGE_VAL;
            if(__builtin_floorf(x)==x&&x<=(float)0.0) {
		/* gammaf(-integer) or gamma(0) */
		exc.type = SING;
		if (_LIB_VERSION == _POSIX_)
		  errno = EDOM;
		else if (!__builtin_matherr(&exc)) {
		  errno = EDOM;
		}
            } else {
		/* gammaf(finite) overflow */
		exc.type = OVERFLOW;
                if (_LIB_VERSION == _POSIX_)
		  errno = ERANGE;
                else if (!__builtin_matherr(&exc)) {
                  errno = ERANGE;
                }
            }
	    if (exc.err != 0)
	       errno = exc.err;
	    return (float)exc.retval; 
        } else
            return y;
#endif
}
