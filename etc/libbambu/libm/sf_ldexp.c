/**
 * Porting of the libm library to the PandA framework
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
 */
/* sf_ldexp.c -- float version of s_ldexp.c.
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
#ifdef WITH_ERRNO
#include <errno.h>
#endif

float ldexpf(float value, int exp)
{
   if(!finitef(value) || value == (float)0.0)
      return value;
   value = scalbnf(value, exp);
#ifdef WITH_ERRNO
   if(!finitef(value) || value == (float)0.0)
      errno = ERANGE;
#else
   if(!finitef(value) || value == (float)0.0)
      return (value - value) / (value - value);
#endif
   return value;
}

#ifdef _DOUBLE_IS_32BITS

double ldexp(double value, int exp)
{
   return (double)ldexpf((float)value, exp);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
