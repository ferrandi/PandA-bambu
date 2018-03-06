/** 
 * Porting of the libm library to the PandA framework 
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
*/
#include "math_privatef.h"

void sincosf(float x, float *sinx, float *cosx)
{
  int ix;

  /* High word of x. */
  GET_FLOAT_WORD (ix, x);

  /* |x| ~< pi/4 */
  ix &= 0x7fffffff;
  if (ix <= 0x3f490fd8)
    {
      *sinx = __hide_kernel_sinf (x, 0.0, 0);
      *cosx = __hide_kernel_cosf (x, 0.0);
    }
  else if (!FLT_UWORD_IS_FINITE(ix))
    {
      /* sin(Inf or NaN) is NaN */
      *sinx = *cosx =  nanf("");
    }
  else
    {
      /* Argument reduction needed.  */
      float y[2];
      int n;

      n = __hide_ieee754_rem_pio2f (x, y);
      switch (n & 3)
	{
	case 0:
	  *sinx = __hide_kernel_sinf (y[0], y[1], 1);
	  *cosx = __hide_kernel_cosf (y[0], y[1]);
	  break;
	case 1:
	  *sinx = __hide_kernel_cosf (y[0], y[1]);
	  *cosx = -__hide_kernel_sinf (y[0], y[1], 1);
	  break;
	case 2:
	  *sinx = -__hide_kernel_sinf (y[0], y[1], 1);
	  *cosx = -__hide_kernel_cosf (y[0], y[1]);
	  break;
	default:
	  *sinx = -__hide_kernel_cosf (y[0], y[1]);
	  *cosx = __hide_kernel_sinf (y[0], y[1], 1);
	  break;
	}
    }

}

#ifdef _DOUBLE_IS_32BITS

void sincos(double x, double *sinx, double *cosx)
{
  int ix;

  /* High word of x. */
  GET_FLOAT_WORD (ix, x);

  /* |x| ~< pi/4 */
  ix &= 0x7fffffff;
  if (ix <= 0x3f490fd8)
    {
      *sinx = __hide_kernel_sinf (x, 0.0, 0);
      *cosx = __hide_kernel_cosf (x, 0.0);
    }
  else if (!FLT_UWORD_IS_FINITE(ix))
    {
      /* sin(Inf or NaN) is NaN */
      *sinx = *cosx =  nanf("");
    }
  else
    {
      /* Argument reduction needed.  */
      float y[2];
      int n;

      n = __hide_ieee754_rem_pio2f (x, y);
      switch (n & 3)
	{
	case 0:
	  *sinx = __hide_kernel_sinf (y[0], y[1], 1);
	  *cosx = __hide_kernel_cosf (y[0], y[1]);
	  break;
	case 1:
	  *sinx = __hide_kernel_cosf (y[0], y[1]);
	  *cosx = -__hide_kernel_sinf (y[0], y[1], 1);
	  break;
	case 2:
	  *sinx = -__hide_kernel_sinf (y[0], y[1], 1);
	  *cosx = -__hide_kernel_cosf (y[0], y[1]);
	  break;
	default:
	  *sinx = -__hide_kernel_cosf (y[0], y[1]);
	  *cosx = __hide_kernel_sinf (y[0], y[1], 1);
	  break;
	}
    }
}
#endif /* defined(_DOUBLE_IS_32BITS) */

#ifndef __llvm__
float _Complex
cexpif (float x)
{
  float _Complex  Res;
  int ix;

  /* High word of x. */
  GET_FLOAT_WORD (ix, x);

  /* |x| ~< pi/4 */
  ix &= 0x7fffffff;
  if (ix <= 0x3f490fd8)
    {
      __imag__ Res = __hide_kernel_sinf (x, 0.0, 0);
      __real__ Res = __hide_kernel_cosf (x, 0.0);
    }
  else if (!FLT_UWORD_IS_FINITE(ix))
    {
      /* sin(Inf or NaN) is NaN */
      __imag__ Res =  nanf("");
      __real__ Res = nanf("");
    }
  else
    {
      /* Argument reduction needed.  */
      float y[2];
      int n;

      n = __hide_ieee754_rem_pio2f (x, y);
      switch (n & 3)
	{
	case 0:
	  __imag__ Res = __hide_kernel_sinf (y[0], y[1], 1);
	  __real__ Res = __hide_kernel_cosf (y[0], y[1]);
	  break;
	case 1:
	  __imag__ Res = __hide_kernel_cosf (y[0], y[1]);
	  __real__ Res = -__hide_kernel_sinf (y[0], y[1], 1);
	  break;
	case 2:
	  __imag__ Res = -__hide_kernel_sinf (y[0], y[1], 1);
	  __real__ Res = -__hide_kernel_cosf (y[0], y[1]);
	  break;
	default:
	  __imag__ Res = -__hide_kernel_cosf (y[0], y[1]);
	  __real__ Res = __hide_kernel_sinf (y[0], y[1], 1);
	  break;
	}
    }

  return Res;
}
#endif
