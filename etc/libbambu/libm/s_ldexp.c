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
#ifdef WITH_ERRNO
#include <errno.h>
#endif

double ldexp(double value, int exp)
{
   if(!finite(value) || value == 0.0)
      return value;
   value = scalbn(value, exp);
#ifdef WITH_ERRNO
   if(!finite(value) || value == 0.0)
      errno = ERANGE;
#else
   if(!finite(value) || value == 0.0)
      return (value - value) / (value - value);
#endif
   return value;
}
