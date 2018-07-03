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

/* ilogb(double x)
 * return the binary exponent of non-zero x
 * ilogb(0) = 0x80000000
 * ilogb(inf) = 0x7fffffff (no signal is raised)
 * ilogb(NaN) = 0x80000000 (no signal is raised)
 */

int ilogb(double x)
{
	int hx,lx,ix;

	hx  = (GET_HI(x))&0x7fffffff;	/* high word of x */
	if(hx<0x00100000) {
	    lx = GET_LO(x);
	    if((hx|lx)==0) 
		return 0x80000000;	/* ilogb(0) = 0x80000000 */
	    else			/* subnormal x */
		if(hx==0) {
		    for (ix = -1043; lx>0; lx<<=1) ix -=1;
		} else {
		    for (ix = -1022,hx<<=11; hx>0; hx<<=1) ix -=1;
		}
	    return ix;
	}
	else if (hx<0x7ff00000) return (hx>>20)-1023;
	else {
	    lx = GET_LO(x);
	    if(hx==0x7ff00000 && lx==0) return 0x7fffffff;
	    else return 0x80000000;
	}
}
