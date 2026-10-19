// Copyright (C) 2023-2026 Politecnico di Milano
//
// Part of the PandA/Bambu MDPI Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
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