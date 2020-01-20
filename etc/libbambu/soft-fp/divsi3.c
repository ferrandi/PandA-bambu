/* Specific functions for bambu architecture.
   Copyright (C) 2014-2020 Politecnico di Milano (Italy).
   This file is part of the HLS-FP Library.

   author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>

   The HLS-FP Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The HLS-FP Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include "bambu-arch.h"
#include <stdlib.h>

#define K 4

typedef int SItype __attribute__((mode(SI)));
typedef unsigned int USItype __attribute__((mode(SI)));
typedef unsigned int UDItype __attribute__((mode(DI)));
/* Define ALIASNAME as a strong alias for NAME.  */
#define strong_alias(name, aliasname) _strong_alias(name, aliasname)
#ifdef __APPLE__
#define _strong_alias(name, aliasname)
#else
#define _strong_alias(name, aliasname) extern __typeof(name) aliasname __attribute__((alias(#name)));
#endif

static USItype lsl(USItype a, USItype b)
{
   return a << b;
}

static USItype lsr(USItype a, USItype b)
{
   return a >> b;
}

static USItype mul(USItype a, USItype b)
{
   return a * b;
}

static inline USItype __udivmodsi4(USItype x, USItype y, USItype* res)
{
   static unsigned char unrt[256] = {-2,   -4,   -6,   -8,   -10,  -12,  -14,  -16,  -18,  -20,  -22,  -23, -25, -27, -29, -31, -32, -34, -36, -38, -39, -41, -43, -44, -46,  -48,  -49,  -51,  -53,  -54,  -56,  -57,  -59,  -61,  -62,  -64,  -65,
                                     -67,  -68,  -70,  -71,  -73,  -74,  -76,  -77,  -78,  -80,  -81,  -83, -84, -86, -87, -88, -90, -91, -92, -94, -95, -96, -98, -99, -100, -102, -103, -104, -105, -107, -108, -109, -110, -112, -113, -114, -115,
                                     -117, -118, -119, -120, -121, -122, -124, -125, -126, -127, -128, 127, 126, 125, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113,  112,  111,  110,  109,  108,  107,  106,  105,  104,  103,  102,  101,
                                     100,  99,   98,   97,   96,   95,   94,   93,   92,   91,   90,   89,  88,  88,  87,  86,  85,  84,  83,  82,  81,  80,  80,  79,  78,   77,   76,   75,   74,   74,   73,   72,   71,   70,   70,   69,   68,
                                     67,   66,   66,   65,   64,   63,   62,   62,   61,   60,   59,   59,  58,  57,  56,  56,  55,  54,  53,  53,  52,  51,  50,  50,  49,   48,   48,   47,   46,   46,   45,   44,   43,   43,   42,   41,   41,
                                     40,   39,   39,   38,   37,   37,   36,   35,   35,   34,   33,   33,  32,  32,  31,  30,  30,  29,  28,  28,  27,  27,  26,  25,  25,   24,   24,   23,   22,   22,   21,   21,   20,   19,   19,   18,   18,
                                     17,   17,   16,   15,   15,   14,   14,   13,   13,   12,   12,   11,  10,  10,  9,   9,   8,   8,   7,   7,   6,   6,   5,   5,   4,    4,    3,    3,    2,    2,    1,    1,    0,    0};
   // table lookup start
   unsigned char k = clz(y);
   // prescaling
   USItype lshifted_y = lsl(y, k);
   /// remove the leading 1
   lshifted_y = lshifted_y << 1;
   USItype ty = lsr(lshifted_y, __WORDSIZE - 8);
   // table lookup
   USItype t = unrt[ty] | 256;
   // postscaling
   USItype z = lsr(lsl(t, __WORDSIZE - 9), __WORDSIZE - k - 1);
   // z recurrence, 2 iterations
   USItype my = 0 - y;
   USItype z0, d0, q0, q1;
   USItype q;
   z0 = mul(my, z);
   umul_ppmm(d0, q0, z, z0);
   z = z + d0;
   z0 = mul(my, z);
   umul_ppmm(d0, q0, z, z0);
   z = z + d0;

   // q estimate
   umul_ppmm(q, q1, x, z);
   USItype r = x - mul(y, q);
   // q refinement
   if(r >= y)
   {
      r = r - y;
      q = q + 1;
      if(r >= y)
      {
         r = r - y;
         q = q + 1;
      }
   }

   if(res != 0)
      *res = r;
   return q;
}

SItype __divsi3(SItype u, SItype v)
{
   _Bool c = 0;
   SItype w;

   if(u < 0)
   {
      c = !c;
      u = -u;
   }
   if(v < 0)
   {
      c = !c;
      v = -v;
   }
   w = __udivmodsi4(u, v, NULL);
   if(c)
      w = -w;
   return w;
}
strong_alias(__divsi3, __divsi3_internal)

    SItype __modsi3(SItype u, SItype v)
{
   _Bool c = 0;
   SItype w;

   if(u < 0)
   {
      c = !c;
      u = -u;
   }
   if(v < 0)
      v = -v;
   __udivmodsi4(u, v, (USItype*)&w);
   if(c)
      w = -w;
   return w;
}
strong_alias(__modsi3, __modsi3_internal)

    USItype __udivsi3(USItype u, USItype v)
{
   return __udivmodsi4(u, v, NULL);
}
strong_alias(__udivsi3, __udivsi3_internal)

    USItype __umodsi3(USItype u, USItype v)
{
   USItype w;

   __udivmodsi4(u, v, &w);
   return w;
}
strong_alias(__umodsi3, __umodsi3_internal)
