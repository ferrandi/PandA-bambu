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
extern DWtype __divdi3(DWtype u, DWtype v);
extern DWtype __moddi3(DWtype u, DWtype v);
extern UDWtype __udivdi3(UDWtype u, UDWtype v);
extern UDWtype __umoddi3(UDWtype u, UDWtype v);
/* Define ALIASNAME as a strong alias for NAME.  */
#define strong_alias(name, aliasname) _strong_alias(name, aliasname)
#ifdef __APPLE__
#define _strong_alias(name, aliasname)
#else
#define _strong_alias(name, aliasname) extern __typeof(name) aliasname __attribute__((alias(#name)));
#endif

#include "common_core_nonrestoring_sdiv.h"

DWtype __divdi3(DWtype u, DWtype v)
{
   DWtype rem;
   return non_restoring_sdiv(u, v, &rem, 1);
}
strong_alias(__divdi3, __divdi3_internal)

    DWtype __moddi3(DWtype u, DWtype v)
{
   DWtype rem;
   non_restoring_sdiv(u, v, &rem, 1);
   return rem;
}
strong_alias(__moddi3, __moddi3_internal)

    UDWtype __udivdi3(UDWtype u, UDWtype v)
{
   DWtype rem;
   return non_restoring_sdiv(u, v, &rem, 0);
}
strong_alias(__udivdi3, __udivdi3_internal)

    UDWtype __umoddi3(UDWtype u, UDWtype v)
{
   UDWtype rem;
   non_restoring_sdiv(u, v, &rem, 0);
   return rem;
}
strong_alias(__umoddi3, __umoddi3_internal)

/* We declare these with compat_symbol so that they are not visible at
   link time.  Programs must use the functions from libgcc.  */
#if defined SHARED && defined DO_VERSIONING
#include <shlib-compat.h>
    compat_symbol(libc, __divdi3, __divdi3, GLIBC_2_0);
compat_symbol(libc, __moddi3, __moddi3, GLIBC_2_0);
compat_symbol(libc, __udivdi3, __udivdi3, GLIBC_2_0);
compat_symbol(libc, __umoddi3, __umoddi3, GLIBC_2_0);
#endif
