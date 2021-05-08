/**
 * Porting of the libm library to the PandA framework
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
 */
/* wf_acosh.c -- float version of w_acosh.c.
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
 *
 */

/*
 * wrapper acoshf(x)
 */

#include "math_privatef.h"
#ifndef _IEEE_LIBM
#include <errno.h>
#endif

float acoshf(float x) /* wrapper acoshf */
{
#ifdef _IEEE_LIBM
   return __hide_ieee754_acoshf(x);
#else
   float z;
   struct exception exc;
   z = __hide_ieee754_acoshf(x);
   if(_LIB_VERSION == _IEEE_ || isnanf(x))
      return z;
   if(x < (float)1.0)
   {
      /* acoshf(x<1) */
      exc.type = DOMAIN;
      exc.name = "acoshf";
      exc.err = 0;
      exc.arg1 = exc.arg2 = (double)x;
      exc.retval = 0.0 / 0.0;
      if(_LIB_VERSION == _POSIX_)
         errno = EDOM;
      else if(!matherr(&exc))
      {
         errno = EDOM;
      }
      if(exc.err != 0)
         errno = exc.err;
      return (float)exc.retval;
   }
   else
      return z;
#endif
}

#ifdef _DOUBLE_IS_32BITS

double acosh(double x)
{
   return (double)acoshf((float)x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
