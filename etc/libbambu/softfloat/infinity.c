//    Copyright (C) 2024-2026 Politecnico di Milano
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

#include "builtins.h"

__tfloat_t __MATH_TF(infinity)(__uint8_t __exp_bits, __uint8_t __frac_bits, __int32_t __exp_bias, __rnd_mode_t __rnd,
                               __exc_mode_t __exc, bool __one, bool __subnorm, __int8_t __sign)
{
   return __kernel_inf(__exp_bits, __frac_bits, __exp_bias, __rnd, __exc, __one, __subnorm, __sign);
}
