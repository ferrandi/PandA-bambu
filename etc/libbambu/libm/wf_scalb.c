/**
 * Porting of the libm library to the PandA framework
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
 */
/* wf_scalb.c -- float version of w_scalb.c.
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
 * wrapper scalbf(float x, float fn) is provide for
 * passing various standard test suite. One
 * should use scalbn() instead.
 */

#include "math_privatef.h"
#ifndef _IEEE_LIBM
#include <errno.h>
#endif

#ifdef _SCALB_INT
float scalbf(float x, int fn) /* wrapper scalbf */
#else
float scalbf(float x, float fn) /* wrapper scalbf */
#endif
{
#ifdef _IEEE_LIBM
   return __hide_ieee754_scalbf(x, fn);
#else
   float z;
#ifndef HUGE_VAL
#define HUGE_VAL inf
   double inf = 0.0;

   SET_HIGH_WORD(inf, 0x7ff00000); /* set inf to infinite */
#endif
   struct exception exc;
   z = __hide_ieee754_scalbf(x, fn);
   if(_LIB_VERSION == _IEEE_)
      return z;
   if(!(finitef(z) || isnanf(z)) && finitef(x))
   {
      /* scalbf overflow; SVID also returns +-HUGE_VAL */
      exc.type = OVERFLOW;
      exc.name = "scalbf";
      exc.err = 0;
      exc.arg1 = (double)x;
      exc.arg2 = (double)fn;
      exc.retval = x > 0.0 ? HUGE_VAL : -HUGE_VAL;
      if(_LIB_VERSION == _POSIX_)
         errno = ERANGE;
      else if(!matherr(&exc))
      {
         errno = ERANGE;
      }
      if(exc.err != 0)
         errno = exc.err;
      return exc.retval;
   }
   if(z == (float)0.0 && z != x)
   {
      /* scalbf underflow */
      exc.type = UNDERFLOW;
      exc.name = "scalbf";
      exc.err = 0;
      exc.arg1 = (double)x;
      exc.arg2 = (double)fn;
      exc.retval = copysign(0.0, x);
      if(_LIB_VERSION == _POSIX_)
         errno = ERANGE;
      else if(!matherr(&exc))
      {
         errno = ERANGE;
      }
      if(exc.err != 0)
         errno = exc.err;
      return exc.retval;
   }
#ifndef _SCALB_INT
   if(!finitef(fn))
      errno = ERANGE;
#endif
   return z;
#endif
}

#ifdef _DOUBLE_IS_32BITS

#ifdef _SCALB_INT
double scalb(double x, int fn)
#else
double scalb(double x, double fn)
#endif
{
#ifdef _SCALB_INT
   return (double)scalbf((float)x, fn);
#else
   return (double)scalbf((float)x, (float)fn);
#endif
}

#endif /* defined(_DOUBLE_IS_32BITS) */
