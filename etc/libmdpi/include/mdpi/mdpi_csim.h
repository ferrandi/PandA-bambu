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
#ifndef __MDPI_CSIM_H
#define __MDPI_CSIM_H

#include "mdpi_bbp.h"
#include "mdpi_pp.h"
#include "mdpi_types.h"

extern struct __bambu_csim_design_stats __bambu_csim_current_run;
extern const unsigned* __bambu_artificial_delay_read;
extern const unsigned* __bambu_artificial_delay_write;

void __bambu_csim_results_append(const char* _res_filename, const char* format, ...);

unsigned int m_next(unsigned int state);
int m_fini();

int m_read(mdpi_idx_t id, byte_t data[], uint16_t bitsize, ptr_t addr, int8_t cmd);
int m_write(mdpi_idx_t id, const byte_t data[], uint16_t bitsize, ptr_t addr, int8_t cmd);
int m_state(mdpi_idx_t id, int data);

#define if_setup_default(idx, var, bitsize) var = bambu_artificial_ParmMgr_Read(idx, bitsize, NULL)
#define if_setup_ptr(idx, var, bitsize) var = (void*)bambu_artificial_ParmMgr_Read(idx, sizeof(bptr_t) * 8, NULL)
#define if_setup_null(idx, var, bitsize) var = NULL

#define if_setup_acknowledge(...) if_setup_null(__VA_ARGS__)
#define if_setup_array(...) if_setup_null(__VA_ARGS__)
#define if_setup_axis(...) if_setup_null(__VA_ARGS__)
#define if_setup_channel(...) if_setup_null(__VA_ARGS__)
#define if_setup_fifo(...) if_setup_null(__VA_ARGS__)
#define if_setup_handshake(...) if_setup_null(__VA_ARGS__)
#define if_setup_none(...) if_setup_null(__VA_ARGS__)
#define if_setup_ovalid(...) if_setup_null(__VA_ARGS__)
#define if_setup_valid(...) if_setup_null(__VA_ARGS__)

#define if_setup_m_axi(...) if_setup_ptr(__VA_ARGS__)

#define if_done_retval(idx, var, bitsize) bambu_artificial_ParmMgr_Write(idx, bitsize, var, NULL)

#endif // __MDPI_CSIM_H
