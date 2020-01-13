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

static USItype half(USItype a)
{
   return a >> 1;
}

static USItype tsubsh(USItype a, USItype b)
{
   return a >= b ? lsl(a - b, 1) + 1 : lsl(a, 1);
}

static inline USItype __udivmodsi4(USItype x, USItype y, USItype* res)
{
   USItype r = x;
   USItype q = 0;
   if(y <= r)
   {
      unsigned char k = clz(y) - clz(r);
      BIT_RESIZE(k, 6);
      y = lsl(y, k); // align y
      if(r >= y)
      {
         r = r - y; // special first iter
         q = lsl(1, k);
      }
      if(k != 0)
      {
         y = half(y);
         unsigned char i = k;
         do
         {
            r = tsubsh(r, y);
            i = i - 1;
         } while(i != 0);   // k iters
         q = q + r;         // combine with first cycle quotient bit
         r = lsr(r, k);     // extract remainder
         q = q - lsl(r, k); // leave just quotient
      }
   }
   if(res)
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
