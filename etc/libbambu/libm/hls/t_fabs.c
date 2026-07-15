// Copyright (C) 2024-2026 Politecnico di Milano
//
// Part of the PandA/Bambu libm_hls IP Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//


#include "kt_fabs.h"
#include <softfloat/softfloat_features.h>

unsigned long long __MATH_TF(fabs)(unsigned long long x, unsigned char __exp_bits, unsigned char __frac_bits,
                                   int __exp_bias, unsigned char __rnd, unsigned char __exc, bool __one, bool __subnorm,
                                   signed char __sign)
{
   return __kernel_fabs(x, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}
