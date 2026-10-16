/**
 * The complex division implementation was derived from Annex G of the
 * C99 specification (ISO/IEC 9899:1999) and improved using Knuth's
 * Algorithm D to minimize overflow/underflow errors.
 *
 * author Michele Fiorito <michele.fiorito@polimi.it>
 *
 * Politecnico di Milano (November 7th, 2024)
 */
/* Public domain.  */

#include <complex.h>
#include <math.h>

__complex double __divdc3(double a, double b, double c, double d)
{
   double scale, denom, x, y;
   __complex double res;

   if(fabs(c) > fabs(d))
   {
      scale = d / c;
      denom = c + d * scale;
   }
   else
   {
      scale = c / d;
      denom = d + c * scale;
   }

   x = (a + b * scale) / denom;
   y = (b - a * scale) / denom;

   /* Recover infinities and zeros that computed as NaN+iNaN;           */
   /* the only cases are nonzero/zero, infinite/finite, and finite/infinite, ...   */

   if(isnan(x) && isnan(y))
   {
      if((denom == 0.0) && (!isnan(a) || !isnan(b)))
      {
         x = copysign(__builtin_inf(), c) * a;
         y = copysign(__builtin_inf(), c) * b;
      }
      else if((isinf(a) || isinf(b)) && isfinite(c) && isfinite(d))
      {
         a = copysign(isinf(a) ? 1.0 : 0.0, a);
         b = copysign(isinf(b) ? 1.0 : 0.0, b);
         x = __builtin_inf() * (a * c + b * d);
         y = __builtin_inf() * (b * c - a * d);
      }
      else if((isinf(c) || isinf(d)) && isfinite(a) && isfinite(b))
      {
         c = copysign(isinf(c) ? 1.0 : 0.0, c);
         d = copysign(isinf(d) ? 1.0 : 0.0, d);
         x = 0.0 * (a * c + b * d);
         y = 0.0 * (b * c - a * d);
      }
   }
   __real__ res = x;
   __imag__ res = y;
   return res;
}
