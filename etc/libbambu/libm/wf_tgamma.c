/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
*/
/* w_gammaf.c -- float version of w_gamma.c.
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
#include "erf_lgamma.c"

float tgammaf(float x)
{
        float y;
	int local_signgam=0;
	y = __hide_ieee754_expf(__hide_ieee754_lgammaf_r(x,&local_signgam));
	if (local_signgam < 0) y = -y;
#ifdef _IEEE_LIBM
	return y;
#else
	if(_LIB_VERSION == _IEEE_) return y;

	if(!__finitef(y)&&__finitef(x)) {
      if(floorf(x)==x&&x<=(float)0.0)
	    /* tgammaf pole */
	    return (float)__hide_kernel_standard((double)x,(double)x,141);
	  else
	    /* tgammaf overflow */
	    return (float)__hide_kernel_standard((double)x,(double)x,140);
	}
	return y;
#endif
}
