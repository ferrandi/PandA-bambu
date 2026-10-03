// Copyright (C) 2024-2026 Politecnico di Milano
//
// Part of the PandA/Bambu libm_hls IP Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//


#include <bambu_config.h>
#include <softfloat/builtins.h>

int __fpclassifyd(double d)
{
   return __kernel_fpclassify(asuint64(d), IEEE64_SPEC_ARGS);
}

bool _llvm_is_fpclass_d(double d, int mask)
{
   return __llvm_kernel_fpclassify(asuint64(d), mask, IEEE64_SPEC_ARGS);

}
