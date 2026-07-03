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
#ifndef __MDPI_TYPES_H
#define __MDPI_TYPES_H

#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif
#endif

#include <bits/wordsize.h>
#include <inttypes.h>

#if defined(VERILATOR) // Verilator
typedef long long sv_longint_t;
typedef unsigned long long sv_longint_unsigned_t;
#define NO_SHORTREAL
#elif defined(MODEL_TECH) || defined(XILINX_SIMULATOR) || defined(VCS) || defined(BAMBU_CSIM) // ModelSim, XSim or VCS
typedef int64_t sv_longint_t;
typedef uint64_t sv_longint_unsigned_t;
#else
#error "Unknown simulator for DPI"
#endif

typedef uint8_t byte_t;
typedef byte_t* bptr_t;
typedef uint16_t mdpi_idx_t;
#if __WORDSIZE == 32
#define bptr_to_int(v) reinterpret_cast<unsigned>(v)
#define ptr_to_bptr(v) reinterpret_cast<bptr_t>(static_cast<unsigned>(v))
#define BPTR_FORMAT "0x%08X"
#else
#define bptr_to_int(v) reinterpret_cast<unsigned long long>(v)
#define ptr_to_bptr(v) reinterpret_cast<bptr_t>(static_cast<unsigned long long>(v))
#define BPTR_FORMAT "0x%016llX"
#endif

#ifdef __M64
typedef sv_longint_unsigned_t ptr_t;
#if defined(MODEL_TECH) || defined(XILINX_SIMULATOR) || defined(VCS)
#define PTR_FORMAT "0x%016zX"
#else
#define PTR_FORMAT "0x%016llX"
#endif
#else
typedef unsigned int ptr_t;
#define PTR_FORMAT "0x%08X"
#endif
#define PTR_SIZE (sizeof(ptr_t) * 8)

typedef struct
{
   bptr_t bits;
   uint16_t bitsize;
} mdpi_parm_t;

typedef enum
{
   MDPI_ENTITY_SIM = 0,
   MDPI_ENTITY_DRIVER,
   MDPI_ENTITY_COUNT
} mdpi_entity_t;

typedef enum
{
   none = 0,
   block,
   cyclic,
   complete
} partition_kind_t;

typedef struct
{
   partition_kind_t kind; // none/block/cyclic/complete
   uint32_t factor;     // for block/cyclic; ignored for complete/none
} partition_desc_t;

#define mdpi_entity_str(s) s == MDPI_ENTITY_DRIVER ? "MDPI driver" : (s == MDPI_ENTITY_SIM ? "Sim" : "Undefined")

#endif // __MDPI_TYPES_H
