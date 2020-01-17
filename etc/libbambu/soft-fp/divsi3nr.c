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
typedef int DItype __attribute__((mode(DI)));
typedef unsigned int UDItype __attribute__((mode(DI)));
/* Define ALIASNAME as a strong alias for NAME.  */
#define strong_alias(name, aliasname) _strong_alias(name, aliasname)
#ifdef __APPLE__
#define _strong_alias(name, aliasname)
#else
#define _strong_alias(name, aliasname) extern __typeof(name) aliasname __attribute__((alias(#name)));
#endif

#define DIV_NUM_BIT 32
#include "common_core_nonrestoring_sdiv.h"

SItype __divsi3(SItype u, SItype v)
{
   DItype rem;
   SItype res = non_restoring_sdiv(u, v, &rem, 1);
   return res;
}
strong_alias(__divsi3, __divsi3_internal)

    SItype __modsi3(SItype u, SItype v)
{
   DItype rem;
   non_restoring_sdiv(u, v, &rem, 1);
   SItype res = rem;
   return res;
}
strong_alias(__modsi3, __modsi3_internal)

    USItype __udivsi3(USItype u, USItype v)
{
   DItype rem;
   USItype res = non_restoring_sdiv(u, v, &rem, 0);
   return res;
}
strong_alias(__udivsi3, __udivsi3_internal)

    USItype __umodsi3(USItype u, USItype v)
{
   DItype rem;
   non_restoring_sdiv(u, v, &rem, 0);
   USItype res = rem;
   return res;
}
strong_alias(__umodsi3, __umodsi3_internal)
