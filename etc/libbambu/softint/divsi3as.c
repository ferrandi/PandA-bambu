// Copyright (C) 2014-2026 Politecnico di Milano
//
// Part of the PandA/Bambu libsoftint IP Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
//


#include "softint.h"

static __FORCE_INLINE __uint32_t lsl(__uint32_t a, __uint32_t b)
{
   return a << b;
}

static __FORCE_INLINE __uint32_t lsr(__uint32_t a, __uint32_t b)
{
   return a >> b;
}

static __FORCE_INLINE __uint32_t half(__uint32_t a)
{
   return a >> 1;
}

static __FORCE_INLINE __uint32_t tsubsh(__uint32_t a, __uint32_t b)
{
   return a >= b ? lsl(a - b, 1) + 1 : lsl(a, 1);
}

static __FORCE_INLINE __uint32_t __udivmodsi4(__uint32_t x, __uint32_t y, __uint32_t* res)
{
   __uint32_t r = x;
   __uint32_t q = 0;
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

__int32_t __divsi3(__int32_t u, __int32_t v)
{
   bool c = 0;
   __int32_t w;

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

__int32_t __modsi3(__int32_t u, __int32_t v)
{
   bool c = 0;
   __int32_t w;

   if(u < 0)
   {
      c = !c;
      u = -u;
   }
   if(v < 0)
      v = -v;
   __udivmodsi4(u, v, (__uint32_t*)&w);
   if(c)
      w = -w;
   return w;
}

__uint32_t __udivsi3(__uint32_t u, __uint32_t v)
{
   return __udivmodsi4(u, v, NULL);
}

__uint32_t __umodsi3(__uint32_t u, __uint32_t v)
{
   __uint32_t w;

   __udivmodsi4(u, v, &w);
   return w;
}
