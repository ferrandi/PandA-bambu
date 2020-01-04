/* 64-bit division and modulus
   Copyright (C) 2004-2020 Politecnico di Milano
   This file is part of the HLS-FP Library.

   author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include "bambu-arch.h"

#include <stdlib.h>

typedef unsigned int UQItype __attribute__((mode(QI)));
typedef int SItype __attribute__((mode(SI)));
typedef unsigned int USItype __attribute__((mode(SI)));
typedef int DItype __attribute__((mode(DI)));
typedef unsigned int UDItype __attribute__((mode(DI)));
#define Wtype SItype
#define HWtype SItype
#define DWtype DItype
#define UWtype USItype
#define UHWtype USItype
#define UDWtype UDItype

/* Prototypes of exported functions.  */
extern DWtype __divdi36432(DWtype u, SItype v);
extern DWtype __moddi36432(DWtype u, SItype v);
extern UDWtype __udivdi36432(UDWtype u, USItype v);
extern UDWtype __umoddi36432(UDWtype u, USItype v);
/* Define ALIASNAME as a strong alias for NAME.  */
#define strong_alias(name, aliasname) _strong_alias(name, aliasname)
#ifdef __APPLE__
#define _strong_alias(name, aliasname)
#else
#define _strong_alias(name, aliasname) extern __typeof(name) aliasname __attribute__((alias(#name)));
#endif

static UDItype lsl64(UDItype a, UDItype b)
{
   return a << b;
}

static UDItype lsr64(UDItype a, UDItype b)
{
   return a >> b;
}

static UDItype half64(UDItype a)
{
   return a >> 1;
}

static UDItype tsubsh64(UDItype a, UDItype b)
{
   return a >= b ? lsl64(a - b, 1) + 1 : lsl64(a, 1);
}

static UDWtype __udivmoddi4(UDWtype x, UDWtype y, UDWtype* res)
{
   UDWtype r = x;
   UDWtype q = 0;
   BIT_RESIZE(y, 32);
   if(y <= r)
   {
      unsigned char k = ((1 << 5) | clz(y)) - clzll(r);
      BIT_RESIZE(k, 6);
      y = lsl64(y, k); // align y
      if(r >= y)
      {
         r = r - y; // special first iter
         q = lsl64(1, k);
      }
      if(k != 0)
      {
         y = half64(y);
         unsigned char i = k;
         do
         {
            r = tsubsh64(r, y);
            i = i - 1;
         } while(i != 0);     // k iters
         q = q + r;           // combine with first cycle quotient bit
         r = lsr64(r, k);     // extract remainder
         q = q - lsl64(r, k); // leave just quotient
      }
   }
   if(res)
      *res = r;
   return q;
}

DWtype __attribute__((optimize("O2"))) __attribute__((optimize("-finline-functions"))) __divdi36432(DWtype u, SItype v)
{
   _Bool c = 0;
   DWtype w;

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
   w = __udivmoddi4(u, v, NULL);
   if(c)
      w = -w;
   return w;
}
strong_alias(__divdi36432, __divdi36432_internal)

    DWtype __attribute__((optimize("O2"))) __attribute__((optimize("-finline-functions"))) __moddi36432(DWtype u, SItype v)
{
   _Bool c = 0;
   DWtype w;

   if(u < 0)
   {
      c = !c;
      u = -u;
   }
   if(v < 0)
      v = -v;
   __udivmoddi4(u, v, (UDWtype*)&w);
   if(c)
      w = -w;
   return w;
}
strong_alias(__moddi36432, __moddi36432_internal)

    UDWtype __attribute__((optimize("O2"))) __attribute__((optimize("-finline-functions"))) __udivdi36432(UDWtype u, USItype v)
{
   return __udivmoddi4(u, v, NULL);
}
strong_alias(__udivdi36432, __udivdi36432_internal)

    UDWtype __attribute__((optimize("O2"))) __attribute__((optimize("-finline-functions"))) __umoddi36432(UDWtype u, USItype v)
{
   UDWtype w;

   __udivmoddi4(u, v, &w);
   return w;
}
strong_alias(__umoddi36432, __umoddi36432_internal)

/* We declare these with compat_symbol so that they are not visible at
   link time.  Programs must use the functions from libgcc.  */
#if defined SHARED && defined DO_VERSIONING
#include <shlib-compat.h>
    compat_symbol(libc, __divdi36432, __divdi36432, GLIBC_2_0);
compat_symbol(libc, __moddi36432, __moddi36432, GLIBC_2_0);
compat_symbol(libc, __udivdi36432, __udivdi36432, GLIBC_2_0);
compat_symbol(libc, __umoddi3, __umoddi3, GLIBC_2_0);
#endif
