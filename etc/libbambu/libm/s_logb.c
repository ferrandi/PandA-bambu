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

/*
 * double logb(x)
 * IEEE 754 logb. Included to pass IEEE test suite. Not recommend.
 * Use ilogb instead.
 */

double logb(double x)
{
   int lx, ix;
   ix = (GET_HI(x)) & 0x7fffffff; /* high |x| */
   lx = GET_LO(x);                /* low x */
   if((ix | lx) == 0)
      return -1.0 / fabs(x);
   if(ix >= 0x7ff00000)
      return fabs(x);
   if((ix >>= 20) == 0) /* IEEE 754 logb */
      return -1022.0;
   else
      return (double)(ix - 1023);
}
