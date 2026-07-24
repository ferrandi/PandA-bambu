// Copyright (C) 2014-2026 Politecnico di Milano
//
// Part of the PandA/Bambu libsoftint IP Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
//


#include "softint.h"

#define DIV_NUM_BIT 64

#include "common_core_nonrestoring_sdiv.h"

__int64_t __divdi36432(__int64_t u, __int32_t v)
{
   __int64_t rem;
   return non_restoring_sdiv(u, v, &rem, 1);
}

__int64_t __moddi36432(__int64_t u, __int32_t v)
{
   __int64_t rem;
   non_restoring_sdiv(u, v, &rem, 1);
   return rem;
}

__uint64_t __udivdi36432(__uint64_t u, __uint32_t v)
{
   __int64_t rem;
   return non_restoring_sdiv(u, v, &rem, 0);
}

__uint64_t __umoddi36432(__uint64_t u, __uint32_t v)
{
   __uint64_t rem;
   non_restoring_sdiv(u, v, &rem, 0);
   return rem;
}
