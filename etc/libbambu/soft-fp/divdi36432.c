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

/* Allow bambu-arch to have its own byte order definitions. */
#ifndef __BYTE_ORDER
#ifdef _LIBC
#include <endian.h>
#else
#error "endianness not defined by sfp-machine.h"
#endif
#endif
#include <stdlib.h>

#if __WORDSIZE != 32
#error This is for 32-bit targets only
#endif

typedef unsigned int UQItype __attribute__((mode(QI)));
typedef int SItype __attribute__((mode(SI)));
typedef unsigned int USItype __attribute__((mode(SI)));
typedef int DItype __attribute__((mode(DI)));
typedef unsigned int UDItype __attribute__((mode(DI)));

/* Prototypes of exported functions.  */
extern DItype __divdi36432(DItype u, SItype v);
extern DItype __moddi36432(DItype u, SItype v);
extern UDItype __udivdi36432(UDItype u, USItype v);
extern UDItype __umoddi36432(UDItype u, USItype v);
/* Define ALIASNAME as a strong alias for NAME.  */
#define strong_alias(name, aliasname) _strong_alias(name, aliasname)
#ifdef __APPLE__
#define _strong_alias(name, aliasname)
#else
#define _strong_alias(name, aliasname) extern __typeof(name) aliasname __attribute__((alias(#name)));
#endif

static UDItype mul64(UDItype u, UDItype v)
{
#if 1
   UDItype t;
   USItype u0, u1, v0, v1, k;
   USItype w0, w1;
   USItype tlast;
   u1 = u >> 32;
   u0 = u;
   v1 = v >> 32;
   v0 = v;
   t = (UDItype)u0 * v0;
   w0 = t;
   k = t >> 32;
   tlast = u1 * v0 + k;
   w1 = tlast;
   tlast = u0 * v1 + w1;
   return (((UDItype)tlast) << 32) | ((UDItype)w0);
#else
   return u * v;
#endif
}

static UDItype umulh64(UDItype u, UDItype v)
{
   register USItype u0, v0, k, u1, v1, w1, w2;
   register UDItype t, t0, t1;
   u1 = u >> 32;
   u0 = u;
   v1 = v >> 32;
   v0 = v;
   t = (UDItype)u0 * v0;
   k = t >> 32;
   t0 = (UDItype)u1 * v0;
   t = t0 + (UDItype)k;
   w1 = t;
   w2 = t >> 32;
   t0 = (UDItype)u0 * v1;
   t = t0 + (UDItype)w1;
   k = t >> 32;
   t0 = (UDItype)u1 * v1;
   t1 = (UDItype)w2 + (UDItype)k;
   return t0 + t1;
}

static UDItype lsl64(UDItype a, UDItype b)
{
   return a << b;
}

static UDItype lsr64(UDItype a, UDItype b)
{
   return a >> b;
}

static inline DItype __udivmoddi4(UDItype x, UDItype y, DItype* res)
{
   static unsigned char unrt[256] = {-2,   -4,   -6,   -8,   -10,  -12,  -14,  -16,  -18,  -20,  -22,  -23, -25, -27, -29, -31, -32, -34, -36, -38, -39, -41, -43, -44, -46,  -48,  -49,  -51,  -53,  -54,  -56,  -57,  -59,  -61,  -62,  -64,  -65,
                                     -67,  -68,  -70,  -71,  -73,  -74,  -76,  -77,  -78,  -80,  -81,  -83, -84, -86, -87, -88, -90, -91, -92, -94, -95, -96, -98, -99, -100, -102, -103, -104, -105, -107, -108, -109, -110, -112, -113, -114, -115,
                                     -117, -118, -119, -120, -121, -122, -124, -125, -126, -127, -128, 127, 126, 125, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113,  112,  111,  110,  109,  108,  107,  106,  105,  104,  103,  102,  101,
                                     100,  99,   98,   97,   96,   95,   94,   93,   92,   91,   90,   89,  88,  88,  87,  86,  85,  84,  83,  82,  81,  80,  80,  79,  78,   77,   76,   75,   74,   74,   73,   72,   71,   70,   70,   69,   68,
                                     67,   66,   66,   65,   64,   63,   62,   62,   61,   60,   59,   59,  58,  57,  56,  56,  55,  54,  53,  53,  52,  51,  50,  50,  49,   48,   48,   47,   46,   46,   45,   44,   43,   43,   42,   41,   41,
                                     40,   39,   39,   38,   37,   37,   36,   35,   35,   34,   33,   33,  32,  32,  31,  30,  30,  29,  28,  28,  27,  27,  26,  25,  25,   24,   24,   23,   22,   22,   21,   21,   20,   19,   19,   18,   18,
                                     17,   17,   16,   15,   15,   14,   14,   13,   13,   12,   12,   11,  10,  10,  9,   9,   8,   8,   7,   7,   6,   6,   5,   5,   4,    4,    3,    3,    2,    2,    1,    1,    0,    0};
   BIT_RESIZE(y, 32);
   // table lookup start
#define W 64
   unsigned char k32 = clz(y);
   UDItype lshifted_y = ((UDItype)(((USItype)y) << k32)) << 32;
   /// remove the leading 1
   lshifted_y = lshifted_y << 1;
   UDItype ty = lsr64(lshifted_y, W - 8);                  // prescaling
   UDItype t = unrt[ty] | 256;                             // table lookup
   UDItype z = lsr64(lsl64(t, W - 9), (W - 32 - 1) - k32); // postscaling
                                                           // z recurrence
   UDItype my = 0 - y;
//#define NR_UNROLLED
#ifdef NR_UNROLLED
   z = z + umulh64(z, mul64(my, z));
   z = z + umulh64(z, mul64(my, z));
   z = z + umulh64(z, mul64(my, z));
#else
   unsigned int index;
   for(index = 0; index < 3; ++index)
   {
      UDItype zd = umulh64(z, mul64(my, z));
      // if (zd == 0) break;
      z = z + zd;
   }
#endif
   // q estimate
   UDItype q = umulh64(x, z);
   UDItype r = x - mul64(y, q);
   // q refinement
   if(r >= y)
   {
      r = r - y;
      q = q + 1;
      if(r >= y)
      {
         r = r - y;
         q = q + 1;
         if(r >= y)
         { // add this in case of three iterations
            r = r - y;
            q = q + 1;
         }
      }
   }
   if(res != 0)
      *res = r;
   return q;
}

DItype __divdi36432(DItype u, SItype v)
{
   _Bool c = 0;
   DItype w, _v = v;

   if(u < 0)
   {
      c = !c;
      u = -u;
   }
   if(v < 0)
   {
      c = !c;
      _v = -_v;
   }
   w = __udivmoddi4(u, _v, NULL);
   if(c)
      w = -w;
   return w;
}
strong_alias(__divdi36432, __divdi36432_internal)

    DItype __moddi36432(DItype u, SItype v)
{
   _Bool c = 0;
   DItype w, _v = v;

   if(u < 0)
   {
      c = !c;
      u = -u;
   }
   if(v < 0)
      _v = -_v;
   __udivmoddi4(u, _v, (UDItype*)&w);
   if(c)
      w = -w;
   return w;
}
strong_alias(__moddi36432, __moddi36432_internal)

    UDItype __udivdi36432(UDItype u, USItype v)
{
   return __udivmoddi4(u, v, NULL);
}
strong_alias(__udivdi36432, __udivdi36432_internal)

    UDItype __umoddi36432(UDItype u, USItype v)
{
   UDItype w;

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
compat_symbol(libc, __umoddi36432, __umoddi36432, GLIBC_2_0);
#endif
