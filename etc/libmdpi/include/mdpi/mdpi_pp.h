// Copyright (C) 2025-2026 Politecnico di Milano
//
// Part of the PandA/Bambu MDPI Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//

#ifndef __MDPI_PP_H
#define __MDPI_PP_H

#include <inttypes.h>
#include <stdbool.h>

unsigned long long bambu_artificial_ParmMgr_Read(uint8_t idx, uint16_t bitsize, void* addr);

void bambu_artificial_ParmMgr_Write(uint8_t idx, uint16_t bitsize, unsigned long long data, void* addr);

unsigned long long bambu_artificial_ParmMgr(uint8_t idx, bool readWrite, uint16_t bitsize, unsigned long long data,
                                            void* addr);

#define aligned_bambu_artificial_ParmMgr(idx, rw, bitsize, data, addr, align) \
   bambu_artificial_ParmMgr(idx, rw, bitsize, data, (void*)((long)addr / align))

#endif // __MDPI_BBP_H
