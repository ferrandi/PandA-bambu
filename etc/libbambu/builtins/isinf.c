//    Copyright (C) 2024-2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//    This file is part of the PandA/Bambu libbambu IP Library.
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

#include <bambu_config.h>
#include <softfloat/builtins.h>

int BUILTIN(isinf)(double d)
{
   return __kernel_isinf(asuint64(d), IEEE64_SPEC_ARGS);
}

int BUILTIN(isinff)(float f)
{
   return __kernel_isinf(asuint(f), IEEE32_SPEC_ARGS);
}
