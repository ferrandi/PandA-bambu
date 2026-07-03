//    Copyright (C) 2023-2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//    This file is part of the PandA/Bambu MDPI Library.
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
#ifndef __MDPI_DEBUG_H
#define __MDPI_DEBUG_H

#include "mdpi_types.h"

#include <pthread.h>

#ifdef __cplusplus
#include <cstdio>
#else
#include <stdio.h>
#endif

#ifndef __BAMBU_IPC_ENTITY
#error Must define __BAMBU_IPC_ENTITY for debug prints
#endif

#ifndef __M_OUT_LVL
#define __M_OUT_LVL 4
#endif

#if __M_OUT_LVL >= 3
#define info(str, ...) fprintf(stdout, "%s: " str, mdpi_entity_str(__BAMBU_IPC_ENTITY), ##__VA_ARGS__)
#define info_append(str, ...) fprintf(stdout, str, ##__VA_ARGS__)
#else
#define info(...)
#define info_append(...)
#endif

#if __M_OUT_LVL > 4
#define debug(str, ...) fprintf(stdout, "%s %10s: " str, mdpi_entity_str(__BAMBU_IPC_ENTITY), __func__, ##__VA_ARGS__)
#define debug_append(str, ...) fprintf(stdout, str, ##__VA_ARGS__)
#define warn(str, ...) debug("WARNING: " str, ##__VA_ARGS__)
#define error(str, ...) debug("ERROR: " str, ##__VA_ARGS__)
#else
#define debug(...)
#define debug_append(...)
#define warn(str, ...) fprintf(stderr, "WARNING: %s: " str, mdpi_entity_str(__BAMBU_IPC_ENTITY), ##__VA_ARGS__)
#define error(str, ...) fprintf(stderr, "ERROR: %s: " str, mdpi_entity_str(__BAMBU_IPC_ENTITY), ##__VA_ARGS__)
#endif

#endif // __MDPI_DEBUG_H