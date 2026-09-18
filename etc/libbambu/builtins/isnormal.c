// Copyright (C) 2024-2026 Politecnico di Milano
//
// Part of the PandA/Bambu libbambu IP Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//


#include <bambu_config.h>
#include <softfloat/builtins.h>

int BUILTIN(isnormal)(double d)
{
   return __kernel_isnormal(asuint64(d), IEEE64_SPEC_ARGS);
}

int BUILTIN(isnormalf)(float f)
{
   return __kernel_isnormal(asuint(f), IEEE32_SPEC_ARGS);
}
