/**
 * Porting of the libm library to the PandA framework
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
 */
/* sincos -- currently no more efficient than two separate calls to
   sin and cos. */

#include "math_private.h"

#ifndef _DOUBLE_IS_32BITS

void sincos(double x, double* sinx, double* cosx)
{
   int ix;

   /* High word of x. */
   ix = GET_HI(x);

   /* |x| ~< pi/4 */
   ix &= 0x7fffffff;
   if(ix <= 0x3fe921fb)
   {
      *sinx = __hide_kernel_sin(x, 0.0, 0);
      *cosx = __hide_kernel_cos(x, 0.0);
   }
   else if(ix >= 0x7ff00000)
   {
      /* sin(Inf or NaN) is NaN */
      *sinx = __builtin_nanf("");
      *cosx = __builtin_nanf("");
   }
   else
   {
      /* Argument reduction needed.  */
      double y[2];
      int n;

      n = __hide_ieee754_rem_pio2(x, y);
      double temp_sinx = __hide_kernel_sin(y[0], y[1], 1);
      double temp_cosx = __hide_kernel_cos(y[0], y[1]);
      switch(n & 3)
      {
         case 0:
            *sinx = temp_sinx;
            *cosx = temp_cosx;
            break;
         case 1:
            *sinx = temp_cosx;
            *cosx = -temp_sinx;
            break;
         case 2:
            *sinx = -temp_sinx;
            *cosx = -temp_cosx;
            break;
         default:
            *sinx = -temp_cosx;
            *cosx = temp_sinx;
            break;
      }
   }
}

#ifndef __llvm__
double _Complex cexpi(double x)
{
   double _Complex Res;
   int ix;

   /* High word of x. */
   ix = GET_HI(x);

   /* |x| ~< pi/4 */
   ix &= 0x7fffffff;
   if(ix <= 0x3fe921fb)
   {
      __imag__ Res = __hide_kernel_sin(x, 0.0, 0);
      __real__ Res = __hide_kernel_cos(x, 0.0);
   }
   else if(ix >= 0x7ff00000)
   {
      /* sin(Inf or NaN) is NaN */
      __real__ Res = __builtin_nanf("");
      __imag__ Res = __builtin_nanf("");
   }
   else
   {
      /* Argument reduction needed.  */
      double y[2];
      int n;

      n = __hide_ieee754_rem_pio2(x, y);
      double temp_sinx = __hide_kernel_sin(y[0], y[1], 1);
      double temp_cosx = __hide_kernel_cos(y[0], y[1]);
      switch(n & 3)
      {
         case 0:
            __imag__ Res = temp_sinx;
            __real__ Res = temp_cosx;
            break;
         case 1:
            __imag__ Res = temp_cosx;
            __real__ Res = -temp_sinx;
            break;
         case 2:
            __imag__ Res = -temp_sinx;
            __real__ Res = -temp_cosx;
            break;
         default:
            __imag__ Res = -temp_cosx;
            __real__ Res = temp_sinx;
            break;
      }
   }

   return Res;
}
#endif

#endif /* defined(_DOUBLE_IS_32BITS) */
