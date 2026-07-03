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

#define DIV_NUM_BIT 32

#include "common_core_nonrestoring_sdiv.h"

__int32_t __divsi3(__int32_t u, __int32_t v)
{
   __int64_t rem;
   return non_restoring_sdiv(u, v, &rem, 1);
}

__int32_t __modsi3(__int32_t u, __int32_t v)
{
   __int64_t rem;
   non_restoring_sdiv(u, v, &rem, 1);
   return rem;
}

__uint32_t __udivsi3(__uint32_t u, __uint32_t v)
{
   __int64_t rem;
   return non_restoring_sdiv(u, v, &rem, 0);
}

__uint32_t __umodsi3(__uint32_t u, __uint32_t v)
{
   __int64_t rem;
   non_restoring_sdiv(u, v, &rem, 0);
   return rem;
}
