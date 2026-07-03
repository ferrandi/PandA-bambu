//    Copyright (C) 2021-2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//    This file is part of the PandA/Bambu libsoftfloat IP Library.
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
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
#ifndef _SOFTFLOAT_FEATURES_H
#define _SOFTFLOAT_FEATURES_H

#include <sys/types.h>

typedef __uint32_t __float32_t;
typedef __uint64_t __float64_t;
typedef __uint64_t __tfloat_t;

typedef __uint8_t __rnd_mode_t;
typedef __uint8_t __exc_mode_t;

#define FLOAT_EXC_OVF 0
#define FLOAT_EXC_STD 1
#define FLOAT_EXC_SAT 2
#define FLOAT_EXC_NONAN 4

#define FLOAT_RND_NONE 0
#define FLOAT_RND_NEVN 1

#define IEEE16_FRAC_BITS 10
#define IEEE16_EXP_BITS 5
#define IEEE16_EXP_BIAS -15
#define IEEE32_FRAC_BITS 23
#define IEEE32_EXP_BITS 8
#define IEEE32_EXP_BIAS -127
#define IEEE64_FRAC_BITS 52
#define IEEE64_EXP_BITS 11
#define IEEE64_EXP_BIAS -1023
#define IEEE_RND FLOAT_RND_NEVN
#define IEEE_EXC FLOAT_EXC_STD
#define IEEE_ONE 1
#ifdef _FLT_NO_DENORMALS
#define IEEE_SUBNORM 0
#else
#define IEEE_SUBNORM 1
#endif
#define IEEE_SIGN -1

#define IEEE16_EXTRACT_FRAC IEEE16_FRAC_BITS
#define IEEE16_EXTRACT_EXP IEEE16_EXP_BITS, IEEE16_FRAC_BITS
#define IEEE16_EXTRACT_SIGN IEEE16_EXP_BITS, IEEE16_FRAC_BITS, IEEE_SIGN
#define IEEE16_SPEC_ARGS \
   IEEE16_EXP_BITS, IEEE16_FRAC_BITS, IEEE16_EXP_BIAS, IEEE_RND, IEEE_EXC, IEEE_ONE, IEEE_SUBNORM, IEEE_SIGN
#define IEEE16_PACK IEEE16_EXP_BITS, IEEE16_FRAC_BITS

#define IEEE32_EXTRACT_FRAC IEEE32_FRAC_BITS
#define IEEE32_EXTRACT_EXP IEEE32_EXP_BITS, IEEE32_FRAC_BITS
#define IEEE32_EXTRACT_SIGN IEEE32_EXP_BITS, IEEE32_FRAC_BITS, IEEE_SIGN
#define IEEE32_SPEC_ARGS \
   IEEE32_EXP_BITS, IEEE32_FRAC_BITS, IEEE32_EXP_BIAS, IEEE_RND, IEEE_EXC, IEEE_ONE, IEEE_SUBNORM, IEEE_SIGN
#define IEEE32_PACK IEEE32_EXP_BITS, IEEE32_FRAC_BITS

#define IEEE64_EXTRACT_FRAC IEEE64_FRAC_BITS
#define IEEE64_EXTRACT_EXP IEEE64_EXP_BITS, IEEE64_FRAC_BITS
#define IEEE64_EXTRACT_SIGN IEEE64_EXP_BITS, IEEE64_FRAC_BITS, IEEE_SIGN
#define IEEE64_SPEC_ARGS \
   IEEE64_EXP_BITS, IEEE64_FRAC_BITS, IEEE64_EXP_BIAS, IEEE_RND, IEEE_EXC, IEEE_ONE, IEEE_SUBNORM, IEEE_SIGN
#define IEEE64_PACK IEEE64_EXP_BITS, IEEE64_FRAC_BITS

#ifndef __MATH_TF
#define __MATH_TF(func) tf_##func
#endif

#ifndef INLINE_PREFIX
#define INLINE_PREFIX
#endif

#endif // _SOFTFLOAT_FEATURES_H