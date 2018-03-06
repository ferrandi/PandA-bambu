/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
*/
/* Copyright (C) 2002 by  Red Hat, Incorporated. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * is freely granted, provided that this notice is preserved.
 */

#include "math_privatef.h"

float fmaf(float x, float y, float z)
{
  /* NOTE:  The floating-point exception behavior of this is not as
   * required.  But since the basic function is not really done properly,
   * it is not worth bothering to get the exceptions right, either.  */
  /* Let the implementation handle this. */ /* <= NONSENSE! */
  /* In floating-point implementations in which double is larger than float,
   * computing as double should provide the desired function.  Otherwise,
   * the behavior will not be as specified in the standards.  */
  return (float) (((double) x * (double) y) + (double) z);
}

#ifdef _DOUBLE_IS_32BITS

double fma(double x, double y, double z)
{
  return (double) fmaf((float) x, (float) y, (float) z);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
