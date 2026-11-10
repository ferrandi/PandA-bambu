// Copyright (C) 2023-2026 Politecnico di Milano
//
// Part of the PandA/Bambu MDPI Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//    author Jesun Firoz <jesun.firoz@pnnl.gov>
//

#ifndef __MDPI_H
#define __MDPI_H

#include "svdpi.h"

#include "mdpi_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

   DPI_DLLESPEC unsigned int m_next(unsigned int state);
   DPI_DLLESPEC int m_fini();

   DPI_DLLESPEC int m_read(mdpi_idx_t id, svLogicVecVal* data, unsigned short bitsize, ptr_t addr, signed char cmd);
   DPI_DLLESPEC int m_write(mdpi_idx_t id, const svLogicVecVal* data, unsigned short bitsize, ptr_t addr,
                            signed char cmd);
   DPI_DLLESPEC int m_state(mdpi_idx_t id, int data);
   DPI_DLLESPEC void m_builtin_exit(int status);

   // Non-blocking memory operations for Ramulator2 integration
   DPI_DLLESPEC int m_read_request(unsigned char id, unsigned short bitsize, ptr_t addr, signed char cmd);
   DPI_DLLESPEC int m_read_complete(unsigned char id, svLogicVecVal* data);
   DPI_DLLESPEC int m_read_complete_ch(unsigned char id, unsigned char channel, svLogicVecVal* data);
   DPI_DLLESPEC int m_write_request(unsigned char id, const svLogicVecVal* data, unsigned short bitsize, ptr_t addr,
                                    unsigned char shift);

   extern double mdpi_clock_period;

#ifdef __cplusplus
}
#endif

#endif // __MDPI_H
