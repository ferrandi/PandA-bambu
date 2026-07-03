//    Copyright (C) 2024-2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//    This file is part of the PandA/Bambu libm_hls IP Library.
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

#include "kt_copysign.h"
#include <softfloat/softfloat_features.h>

unsigned long long __MATH_TF(copysign)(unsigned long long x, unsigned long long y, unsigned char __exp_bits,
                                       unsigned char __frac_bits, int __exp_bias, unsigned char __rnd,
                                       unsigned char __exc, bool __one, bool __subnorm, signed char __sign)
{
   return __kernel_copysign(x, y, __exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}
