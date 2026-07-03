//    Copyright (C) 2025-2026 Politecnico di Milano
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
