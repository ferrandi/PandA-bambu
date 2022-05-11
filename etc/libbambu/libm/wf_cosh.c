/**
 * Porting of the libm library to the PandA framework
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
 */
/* wf_cosh.c -- float version of w_cosh.c.
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
 * wrapper coshf(x)
 */

#include "math_privatef.h"
#ifndef _IEEE_LIBM
#include <errno.h>
#endif

float coshf(float x) /* wrapper coshf */
{
#ifdef _IEEE_LIBM
   return __hide_ieee754_coshf(x);
#else
   float z;
   struct exception exc;
   z = __hide_ieee754_coshf(x);
   if(_LIB_VERSION == _IEEE_ || isnanf(x))
      return z;
   if(fabsf(x) > (float)8.9415985107e+01)
   {
      /* coshf(finite) overflow */
#ifndef HUGE_VAL
#define HUGE_VAL inf
      double inf = 0.0;

      SET_HIGH_WORD(inf, 0x7ff00000); /* set inf to infinite */
#endif
      exc.type = OVERFLOW;
      exc.name = "coshf";
      exc.err = 0;
      exc.arg1 = exc.arg2 = (double)x;
      if(_LIB_VERSION == _SVID_)
         exc.retval = HUGE;
      else
         exc.retval = HUGE_VAL;
      if(_LIB_VERSION == _POSIX_)
         errno = ERANGE;
      else if(!matherr(&exc))
      {
         errno = ERANGE;
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

#ifdef __STDC__
double cosh(double x)
#else
double cosh(x) double x;
#endif
{
   return (double)coshf((float)x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
