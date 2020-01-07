/* 64-bit non-restoring division and modulus
   Copyright (C) 2014-2020 Politecnico di Milano (Italy).
   This file is part of the HLS-FP Library.

   author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>

   The HLS-DIV Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The HLS-DIV Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include "bambu-arch.h"

typedef unsigned int UQItype __attribute__((mode(QI)));
typedef int SItype __attribute__((mode(SI)));
typedef unsigned int USItype __attribute__((mode(SI)));
typedef int DItype __attribute__((mode(DI)));
typedef unsigned int UDItype __attribute__((mode(DI)));
#define Wtype SItype
#define DWtype DItype
#define UWtype USItype
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

#define DIV_NUM_BIT 64

#include "common_core_nonrestoring_sdiv.h"

DWtype __divdi36432(DWtype u, SItype v)
{
   DWtype rem;
   return non_restoring_sdiv(u, v, &rem, 1);
}
strong_alias(__divdi36432, __divdi36432_internal)

    DWtype __moddi36432(DWtype u, SItype v)
{
   DWtype rem;
   non_restoring_sdiv(u, v, &rem, 1);
   return rem;
}
strong_alias(__moddi36432, __moddi36432_internal)

    UDWtype __udivdi36432(UDWtype u, USItype v)
{
   DWtype rem;
   return non_restoring_sdiv(u, v, &rem, 0);
}
strong_alias(__udivdi36432, __udivdi36432_internal)

    UDWtype __umoddi36432(UDWtype u, USItype v)
{
   UDWtype rem;
   non_restoring_sdiv(u, v, &rem, 0);
   return rem;
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
