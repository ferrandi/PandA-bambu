// Copyright (C) 2024-2026 Politecnico di Milano
//
// Part of the PandA/Bambu libbambu IP Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//


#include <bambu_config.h>
#include <softfloat/builtins.h>

double BUILTIN(nan)(const char* __tagb)
{
   return asdouble(__kernel_nan(__tagb, IEEE64_SPEC_ARGS));
}

float BUILTIN(nanf)(const char* __tagb)
{
   return asfloat(__kernel_nan(__tagb, IEEE32_SPEC_ARGS));
}
