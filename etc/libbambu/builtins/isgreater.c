// Copyright (C) 2024-2026 Politecnico di Milano
//
// Part of the PandA/Bambu libbambu IP Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//


#include <bambu_config.h>
#include <softfloat/builtins.h>

int BUILTIN(isgreater)(double x, double y)
{
   return __kernel_isgreater(asuint64(x), asuint64(y), IEEE64_SPEC_ARGS);
}

int BUILTIN(isgreaterf)(float x, float y)
{
   return __kernel_isgreater(asuint(x), asuint(y), IEEE32_SPEC_ARGS);
}
