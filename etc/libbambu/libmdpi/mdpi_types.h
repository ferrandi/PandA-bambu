/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (C) 2023 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/**
 * @file mdpi_types.h
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
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
#include <stdint.h>

struct ab_uint8_t
{
   uint8_t aval, bval;
};

struct mdpi_parm_t
{
   void* bits;
   uint16_t bitsize;
};

struct mdpi_params_t
{
   struct mdpi_parm_t* prms;
   uint8_t size;
};

enum mdpi_state
{
   MDPI_UNDEFINED = 0,
   MDPI_SIM_READY,
   MDPI_COSIM_INIT,
   MDPI_SIM_SETUP,
   MDPI_SIM_RUNNING,
   MDPI_SIM_END,
   MDPI_COSIM_END,
   MDPI_COSIM_ABORT,
};

#define mdpi_state_str(s)                                       \
   s == MDPI_SIM_READY ?                                        \
       "SIM_READY" :                                            \
       (s == MDPI_COSIM_INIT ?                                  \
            "COSIM_INIT" :                                      \
            (s == MDPI_SIM_SETUP ?                              \
                 "SIM_SETUP" :                                  \
                 (s == MDPI_SIM_RUNNING ?                       \
                      "SIM_RUNNING" :                           \
                      (s == MDPI_SIM_END ?                      \
                           "SIM_END" :                          \
                           (s == MDPI_COSIM_END ? "COSIM_END" : \
                                                  (s == MDPI_COSIM_ABORT ? "COSIM_ABORT" : "UNDEFINED"))))))

enum mdpi_entity
{
   MDPI_ENTITY_SIM = 0,
   MDPI_ENTITY_COSIM,
   MDPI_ENTITY_COUNT
};

#define mdpi_entity_str(s) s == MDPI_ENTITY_SIM ? "Sim" : (s == MDPI_ENTITY_COSIM ? "Co-sim" : "Unknown")

#if defined(VERILATOR) // Verilator
typedef long long sv_longint_t;
typedef unsigned long long sv_longint_unsigned_t;
#define NO_SHORTREAL
#define CONSTARG const
#define EXPORT
#elif defined(MODEL_TECH) || defined(XILINX_SIMULATOR) // ModelSim or XSim
typedef int64_t sv_longint_t;
typedef uint64_t sv_longint_unsigned_t;
#define CONSTARG const
#define EXPORT DPI_DLLESPEC
#else
#error "Unknown simulator for DPI"
#endif

#if __WORDSIZE == 32
#define ptr_t unsigned int
#else
#define ptr_t sv_longint_unsigned_t
#endif

#endif // __MDPI_TYPES_H