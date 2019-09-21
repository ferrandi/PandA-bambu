/**
 * Porting of the libm library to the PandA framework
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
 */
/* wf_log.c -- float version of w_log.c.
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
 * wrapper logf(x)
 */

#include "math_privatef.h"
#ifndef _IEEE_LIBM
#include <errno.h>
#endif

float logf(float x) /* wrapper logf */
{
#ifdef _IEEE_LIBM
   return __hide_ieee754_logf(x);
#else
   float z;
   struct exception exc;
   z = __hide_ieee754_logf(x);
   if(_LIB_VERSION == _IEEE_ || isnan(x) || x > (float)0.0)
      return z;
#ifndef HUGE_VAL
#define HUGE_VAL inf
   double inf = 0.0;

   SET_HIGH_WORD(inf, 0x7ff00000); /* set inf to infinite */
#endif
   exc.name = "logf";
   exc.err = 0;
   exc.arg1 = exc.arg2 = (double)x;
   if(_LIB_VERSION == _SVID_)
      exc.retval = -HUGE;
   else
      exc.retval = -HUGE_VAL;
   if(x == (float)0.0)
   {
      /* logf(0) */
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
      /* logf(x<0) */
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
#endif
}

#ifdef _DOUBLE_IS_32BITS

double log(double x)
{
   return (double)logf((float)x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
