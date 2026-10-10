// Copyright (C) 2021-2026 Politecnico di Milano
//
// Part of the PandA/Bambu libm_hls IP Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//


#include <bambu_config.h>
#include <softfloat/softfloat_features.h>

static __FORCE_INLINE __tfloat_t __kernel_copysign(__tfloat_t x, __tfloat_t y, __uint8_t __exp_bits,
                                                   __uint8_t __frac_bits, __int32_t __exp_bias, __rnd_mode_t __rnd,
                                                   __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   if(__sign == -1)
   {
      return (x & ((1ULL << (__exp_bits + __frac_bits)) - 1)) | (y & (1ULL << (__exp_bits + __frac_bits)));
   }
   return x;
}
