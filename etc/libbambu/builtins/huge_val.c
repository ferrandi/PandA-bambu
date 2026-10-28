// Copyright (C) 2024-2026 Politecnico di Milano
//
// Part of the PandA/Bambu libbambu IP Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//


#include <bambu_config.h>
#include <softfloat/builtins.h>

double BUILTIN(huge_val)()
{
   return asdouble(__kernel_inf(IEEE64_SPEC_ARGS));
}

float BUILTIN(huge_valf)()
{
   return asfloat(__kernel_inf(IEEE32_SPEC_ARGS));
}
