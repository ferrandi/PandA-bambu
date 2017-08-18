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

#ifndef _IEEE_LIBM
#include <errno.h>
#endif

/*
 * wrapper scalb(double x, double fn) is provide for
 * passing various standard test suite. One 
 * should use scalbn() instead.
 */
#ifdef _SCALB_INT
	double __builtin_scalb(double x, int fn)		/* wrapper scalb */
#else
	double __builtin_scalb(double x, double fn)	/* wrapper scalb */
#endif
{
#ifdef _IEEE_LIBM
	return __hide_ieee754_scalb(x,fn);
#else
	double z;
	z = __hide_ieee754_scalb(x,fn);
	if(_LIB_VERSION == _IEEE_) return z;
	if(!(__finite(z)||__builtin_isnan(z))&&__finite(x)) {
	    return __hide_kernel_standard(x,(double)fn,32); /* scalb overflow */
	}
	if(z==0.0&&z!=x) {
	    return __hide_kernel_standard(x,(double)fn,33); /* scalb underflow */
	} 
#ifndef _SCALB_INT
	if(!__finite(fn)) errno = ERANGE;
#endif
	return z;
#endif 
}
