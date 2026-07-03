//    Copyright (C) 2013-2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//    This file is part of the PandA/Bambu libbambu IP Library.
//
//    author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
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
#ifndef _BAMBU_CONFIG_H
#define _BAMBU_CONFIG_H

#include <stdbool.h>
#include <sys/types.h>

#define __FORCE_INLINE __attribute__((always_inline)) inline

#ifdef __clang__
#define BUILTIN(name) name
#else
#define BUILTIN(name) __builtin_##name
#endif

// clang-format off
#define asuint64(f) ((union{double _f; __uint64_t _i;}){f})._i

#define asdouble(i) ((union{__uint64_t _i; double _f;}){i})._f

#define asuint(f) ((union{float _f; __uint32_t _i;}){f})._i

#define asfloat(i) ((union{__uint32_t _i; float _f;}){i})._f
// clang-format on

#endif // _BAMBU_CONFIG_H
