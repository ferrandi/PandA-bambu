/**
 * Porting of the libm library to the PandA framework
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
 */
/* wf_log10.c -- float version of w_log10.c.
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
 * wrapper log10f(X)
 */

#include "math_privatef.h"
#ifndef _IEEE_LIBM
#include <errno.h>
#endif

float log10f(float x) /* wrapper log10f */
{
#ifdef _IEEE_LIBM
   return __hide_ieee754_log10f(x);
#else
   float z;
   struct exception exc;
   z = __hide_ieee754_log10f(x);
   if(_LIB_VERSION == _IEEE_ || isnanf(x))
      return z;
   if(x <= (float)0.0)
   {
#ifndef HUGE_VAL
#define HUGE_VAL inf
      double inf = 0.0;

      SET_HIGH_WORD(inf, 0x7ff00000); /* set inf to infinite */
#endif
      exc.name = "log10f";
      exc.err = 0;
      exc.arg1 = exc.arg2 = (double)x;
      if(_LIB_VERSION == _SVID_)
         exc.retval = -HUGE;
      else
         exc.retval = -HUGE_VAL;
      if(x == (float)0.0)
      {
         /* log10f(0) */
         exc.type = SING;
         if(_LIB_VERSION == _POSIX_)
            errno = ERANGE;
         else if(!matherr(&exc))
         {
            errno = ERANGE;
         }
      }
      else
      {
         /* log10f(x<0) */
         exc.type = DOMAIN;
         if(_LIB_VERSION == _POSIX_)
            errno = EDOM;
         else if(!matherr(&exc))
         {
            errno = EDOM;
         }
         exc.retval = __builtin_nan("");
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

double log10(double x)
{
   return (double)log10f((float)x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
