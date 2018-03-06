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
 * modf(double x, double *iptr) 
 * return fraction part of x, and return x's integral part in *iptr.
 * Method:
 *	Bit twiddling.
 *
 * Exception:
 *	No exception.
 */

//static const double one = 1.0;

double modf(double x, double *iptr)
{
	int i0,i1,j0;
	unsigned i;
	i0 =  GET_HI(x);		/* high x */
	i1 =  GET_LO(x);		/* low  x */
	j0 = ((i0>>20)&0x7ff)-0x3ff;	/* exponent of x */
	if(j0<20) {			/* integer part in high x */
	    if(j0<0) {			/* |x|<1 */
		INSERT_WORDS(*iptr, i0&0x80000000, 0);		/* *iptr = +-0 */
		return x;
	    } else {
		i = (0x000fffff)>>j0;
		if(((i0&i)|i1)==0) {		/* x is integral */
		    *iptr = x;
		    INSERT_WORDS(x, GET_HI(x) & 0x80000000, 0);	/* return +-0 */
		    return x;
		} else {
		    INSERT_WORDS(*iptr, i0&(~i), 0);
		    return x - *iptr;
		}
	    }
	} else if (j0>51) {		/* no fraction part */
	    *iptr = x*one;
	    /* We must handle NaNs separately.  */
	    if (j0 == 0x400 && ((i0 & 0xfffff) | i1))
          return __builtin_nan("");
	    INSERT_WORDS(x, GET_HI(x) & 0x80000000, 0);	/* return +-0 */
	    return x;
	} else {			/* fraction part in low x */
	    i = ((unsigned)(0xffffffff))>>(j0-20);
	    if((i1&i)==0) { 		/* x is integral */
		*iptr = x;
		INSERT_WORDS(x, GET_HI(x) & 0x80000000, 0);	/* return +-0 */
		return x;
	    } else {
		INSERT_WORDS(*iptr, i0, i1&(~i));
		return x - *iptr;
	    }
	}
}
