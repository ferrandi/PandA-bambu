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
 * Copyright (C) 2004 by Sun Microsystems, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice 
 * is preserved.
 * ====================================================
 */

#include "math_private.h"

/* 
 * wrapper exp(x)
 */
double exp(double x)		/* wrapper exp */
{
#ifdef _IEEE_LIBM
	return __hide_ieee754_exp(x);
#else
	double z;
	z = __ieee754_exp(x);
	if(_LIB_VERSION == _IEEE_) return z;
	if(__finite(x)) {
	    if(x>o_threshold)
	        return __hide_kernel_standard(x,x,6); /* exp overflow */
	    else if(x<u_threshold)
	        return __hide_kernel_standard(x,x,7); /* exp underflow */
	} 
	return z;
#endif
}
