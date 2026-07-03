//    Copyright (C) 2014-2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//    This file is part of the PandA/Bambu libsoftint IP Library.
//
//    author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
//
// Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "softint.h"

static __FORCE_INLINE __uint64_t lsl64(__uint64_t a, __uint64_t b)
{
   return a << b;
}

static __FORCE_INLINE __uint64_t lsr64(__uint64_t a, __uint64_t b)
{
   return a >> b;
}

static __FORCE_INLINE __uint64_t half64(__uint64_t a)
{
   return a >> 1;
}

static __FORCE_INLINE __uint64_t tsubsh64(__uint64_t a, __uint64_t b)
{
   return a >= b ? lsl64(a - b, 1) + 1 : lsl64(a, 1);
}

static __FORCE_INLINE __uint64_t __udivmoddi4(__uint64_t x, __uint64_t y, __uint64_t* res)
{
   __uint64_t r = x;
   __uint64_t q = 0;
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

__int64_t __divdi36432(__int64_t u, __int32_t v)
{
   bool c = 0;
   __int64_t w;

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

__int64_t __moddi36432(__int64_t u, __int32_t v)
{
   bool c = 0;
   __int64_t w;

   if(u < 0)
   {
      c = !c;
      u = -u;
   }
   if(v < 0)
      v = -v;
   __udivmoddi4(u, v, (__uint64_t*)&w);
   if(c)
      w = -w;
   return w;
}

__uint64_t __udivdi36432(__uint64_t u, __uint32_t v)
{
   return __udivmoddi4(u, v, NULL);
}

__uint64_t __umoddi36432(__uint64_t u, __uint32_t v)
{
   __uint64_t w;

   __udivmoddi4(u, v, &w);
   return w;
}
