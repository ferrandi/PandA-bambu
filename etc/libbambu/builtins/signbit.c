// Copyright (C) 2024-2026 Politecnico di Milano
//
// Part of the PandA/Bambu libbambu IP Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//


#include <bambu_config.h>
#include <softfloat/builtins.h>

int BUILTIN(signbit)(double d)
{
   return __kernel_signbit(asuint64(d), IEEE64_SPEC_ARGS);
}

int BUILTIN(signbitf)(float f)
{
   return __kernel_signbit(asuint(f), IEEE32_SPEC_ARGS);
}
