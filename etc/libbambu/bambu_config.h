// Copyright (C) 2013-2026 Politecnico di Milano
//
// Part of the PandA/Bambu libbambu IP Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
//    author Michele Fiorito <michele.fiorito@polimi.it>
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
