// Copyright (C) 2024-2026 Politecnico di Milano
//
// Part of the PandA/Bambu libsoftfloat IP Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//


#include "builtins.h"

int __MATH_TF(isnormal)(__tfloat_t x, __uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias,
                        __rnd_mode_t __rnd, __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   return __kernel_isnormal(x, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}
