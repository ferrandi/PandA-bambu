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
 * wrapper log(x)
 */
double __builtin_log(double x)		/* wrapper log */
{
#ifdef _IEEE_LIBM
	return __hide_ieee754_log(x);
#else
	double z;
	z = __hide_ieee754_log(x);
	if(_LIB_VERSION == _IEEE_ || __builtin_isnan(x) || x > 0.0) return z;
	if(x==0.0)
	    return __hide_kernel_standard(x,x,16); /* log(0) */
	else 
	    return __hide_kernel_standard(x,x,17); /* log(x<0) */
#endif
}
