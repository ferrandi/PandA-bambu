/* Specific functions for bambu architecture.
   Copyright (C) 2016-2020 Politecnico di Milano (Italy).
   This specific code has been derived from libgcc from GCC.
   The GCC licence and its exception applies.
*/
/* Copyright (C) 1989-2018 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */

typedef union {
   double value;
   unsigned long long int u_value;
} local_double_shape_type;

static inline unsigned long long int __local_get_uint64(double x)
{
   local_double_shape_type val;
   val.value = x;
   return val.u_value;
}
static inline double __local_get_double(unsigned long long int x)
{
   local_double_shape_type val;
   val.u_value = x;
   return val.value;
}

#define isinf(x) ((__local_get_uint64(x) & -1ULL >> 1) == 0x7ffULL << 52)
#define isnan(x) ((__local_get_uint64(x) & -1ULL >> 1) > 0x7ffULL << 52)
#define isfinite(x) ((__local_get_uint64(x) & -1ULL >> 1) < 0x7ffULL << 52)

static inline double __local_copysign(double x, double y)
{
   unsigned long long int ix, iy, high;
   unsigned int high_ix, high_iy, low;
   ix = __local_get_uint64(x);
   iy = __local_get_uint64(y);
   high_ix = ix >> 32;
   high_iy = iy >> 32;
   low = ix;
   high = (high_ix & 0x7fffffff) | (high_iy & 0x80000000);
   high = high << 32;
   x = __local_get_double(high | low);
   return x;
}

__complex__ double ___divdc3(double a, double b, double c, double d)
{
   double denom, ratio, x, y;
   __complex__ double res;

   if(fabs(c) < fabs(d))
   {
      ratio = c / d;
      denom = (c * ratio) + d;
      x = ((a * ratio) + b) / denom;
      y = ((b * ratio) - a) / denom;
   }
   else
   {
      ratio = d / c;
      denom = (d * ratio) + c;
      x = ((b * ratio) + a) / denom;
      y = (b - (a * ratio)) / denom;
   }

   /* Recover infinities and zeros that computed as NaN+iNaN; the only cases
      are nonzero/zero, infinite/finite, and finite/infinite.  */
   if(isnan(x) && isnan(y))
   {
      if(c == 0.0 && d == 0.0 && (!isnan(a) || !isnan(b)))
      {
         x = __local_copysign(__builtin_inf(), c) * a;
         y = __local_copysign(__builtin_inf(), c) * b;
      }
      else if((isinf(a) || isinf(b)) && isfinite(c) && isfinite(d))
      {
         a = __local_copysign(isinf(a) ? 1 : 0, a);
         b = __local_copysign(isinf(b) ? 1 : 0, b);
         x = __builtin_inf() * (a * c + b * d);
         y = __builtin_inf() * (b * c - a * d);
      }
      else if((isinf(c) || isinf(d)) && isfinite(a) && isfinite(b))
      {
         c = __local_copysign(isinf(c) ? 1 : 0, c);
         d = __local_copysign(isinf(d) ? 1 : 0, d);
         x = 0.0 * (a * c + b * d);
         y = 0.0 * (b * c - a * d);
      }
   }

   __real__ res = x;
   __imag__ res = y;
   return res;
}
