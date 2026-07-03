//    Copyright (C) 2016-2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//    This file is part of the PandA/Bambu libm_hls IP Library.
//
//    author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
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

/* In this file the single precision sine and cosine functions are implemented
   following the HOTBM method published by
   Jeremie Detrey and Florent de Dinechin, "Floating-point Trigonometric Functions for FPGAs" FPL 2007.
*/

#include "kf_sincos.c"

#ifndef TEST_PREFIX
#define TEST_PREFIX
#endif

float TEST_PREFIX(tanf)(float x)
{
   float sinx;
   float cosx;
   float _Complex res = __cexpif(x);
   sinx = __imag__ res;
   cosx = __real__ res;
   return sinx / cosx;
}
