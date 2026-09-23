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

__complex float __divsc3(float a, float b, float c, float d)
{
   float scale, denom, x, y;
   __complex float res;

   if(fabsf(c) > fabsf(d))
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

   if(isnanf(x) && isnanf(y))
   {
      if((denom == 0.0f) && (!isnanf(a) || !isnanf(b)))
      {
         x = copysignf(__builtin_inff(), c) * a;
         y = copysignf(__builtin_inff(), c) * b;
      }
      else if((isinff(a) || isinff(b)) && finitef(c) && finitef(d))
      {
         a = copysignf(isinff(a) ? 1.0f : 0.0f, a);
         b = copysignf(isinff(b) ? 1.0f : 0.0f, b);
         x = __builtin_inff() * (a * c + b * d);
         y = __builtin_inff() * (b * c - a * d);
      }
      else if((isinff(c) || isinff(d)) && finitef(a) && finitef(b))
      {
         c = copysignf(isinff(c) ? 1.0f : 0.0f, c);
         d = copysignf(isinff(d) ? 1.0f : 0.0f, d);
         x = 0.0f * (a * c + b * d);
         y = 0.0f * (b * c - a * d);
      }
   }
   __real__ res = x;
   __imag__ res = y;
   return res;
}
