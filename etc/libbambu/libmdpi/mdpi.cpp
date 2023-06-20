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
 * @file mdpi.cpp
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "mdpi.h"

#include "mdpi_cosim.h"
#include "mdpi_types.h"
#include "mdpi_wrapper.h"
#include "segvcatch/segvcatch.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <pthread.h>

#define FORCE_INLINE __attribute__((always_inline)) inline

volatile pthread_t __m_main_tid;

#define info(str, ...) fprintf(stdout, "%s: " str, __m_main_tid == pthread_self() ? "Sim" : "Co-sim", ##__VA_ARGS__)

#ifndef NDEBUG
#define debug(str, ...) \
   fprintf(stdout, "%s %10s: " str, __m_main_tid == pthread_self() ? "Sim" : "Co-sim", __func__, ##__VA_ARGS__)
#define error(str, ...) debug("ERROR: " str, ##__VA_ARGS__)
#else
#define debug(...)
#define error(str, ...) \
   fprintf(stderr, "ERROR: %s: " str, __m_main_tid == pthread_self() ? "Sim" : "Co-sim", ##__VA_ARGS__)
#endif

extern mdpi_params_t __m_params;

static pthread_t __m_cosim_thread;

EXTERN_C void m_init()
{
   int retval;

   __m_main_tid = pthread_self();

   __m_signal_init();

   retval = pthread_create(&__m_cosim_thread, NULL, __m_cosim_main, NULL);
   if(retval)
   {
      perror("MDPI library initialization error");
      exit(EXIT_FAILURE);
   }

   segvcatch::init_segv();

   debug("Initialization successful\n");
}

EXTERN_C unsigned int m_next(unsigned int state)
{
   enum mdpi_state state_next = MDPI_UNDEFINED;

   switch(state)
   {
      case MDPI_COSIM_INIT:
         debug("Next state required\n");
         __m_signal_to(MDPI_ENTITY_COSIM, MDPI_COSIM_INIT);
         state_next = __m_wait_for(MDPI_ENTITY_SIM);
         break;
      default:
         error("Sim: Unexpected state received from simulator: %s (%u)\n", mdpi_state_str(state), state);
         exit(EXIT_FAILURE);
         break;
   }

   return state_next;
}

EXTERN_C int m_fini()
{
   int retval = 0;
   pthread_join(__m_cosim_thread, (void**)&retval);

   debug("Finalization successful\n");
   return retval;
}

static FORCE_INLINE ab_uint8_t load(bptr_t addr)
{
   ab_uint8_t mem;
   try
   {
      mem.aval = *addr;
      mem.bval = 0;
   }
   catch(std::exception& e)
   {
      error("Memory load exception: illegal access at " BPTR_FORMAT "\n", bptr_to_int(addr));
      mem.aval = 0xFF;
      mem.bval = 0xFF;
   }
   return mem;
}

EXTERN_C void m_getarg(svLogicVecVal* data, unsigned int index)
{
   debug("Simulator required parameter %u read\n", index);
   if(index >= __m_params.size)
   {
      error("Simulator required parameter index out of bounds: %u\n", index);
      exit(EXIT_FAILURE);
   }
   mdpi_parm_t* p = &__m_params.prms[index];
   uint8_t* prm = (uint8_t*)__m_params.prms[index].bits;
   uint16_t i, byte_count = (p->bitsize / 8) + ((p->bitsize % 8) != 0);
   for(uint8_t i = 0; i < byte_count; ++i)
   {
      uint8_t mem = prm[i];
      if(i % 4)
      {
         data[i / 4].aval |= static_cast<unsigned int>(mem) << (8 * (i % 4));
      }
      else
      {
         data[i / 4].aval = mem;
         data[i / 4].bval = 0;
      }
   }
}

EXTERN_C void m_setarg(CONSTARG svLogicVecVal* data, unsigned int index)
{
   debug("Simulator required parameter %u write\n", index);
   if(index >= __m_params.size)
   {
      error("Simulator required parameter index out of bounds: %u\n", index);
      exit(EXIT_FAILURE);
   }
   mdpi_parm_t* p = &__m_params.prms[index];
   uint8_t* prm = (uint8_t*)__m_params.prms[index].bits;
   uint16_t i, byte_count = (p->bitsize / 8) + ((p->bitsize % 8) != 0);
   for(i = 0; i < byte_count; ++i)
   {
      assert((data[i / 4].bval == 0) && "Memory write data must not contain undefined states X or Z from "
                                        "the simulation");
      prm[i] = data[i / 4].aval >> (8 * (i % 4));
   }
}

EXTERN_C unsigned int m_getptrargsize(unsigned int index)
{
   return __m_param_size(index);
}

static FORCE_INLINE void store(bptr_t addr, uint8_t val)
{
   try
   {
      *addr = val;
   }
   catch(std::exception& e)
   {
      error("Memory store exception: illegal access at " BPTR_FORMAT "\n", bptr_to_int(addr));
      abort();
   }
}

static FORCE_INLINE void __m_read(uint16_t bsize, svLogicVecVal* data, ptr_t addr)
{
   debug("Read %u bytes at " PTR_FORMAT "\n", bsize, addr);
   bptr_t __addr = __m_memaddr(addr);
   if(__addr)
   {
      for(uint16_t i = 0; i < bsize; ++i)
      {
         ab_uint8_t mem = load(__addr + i);
         if(i % 4)
         {
            data[i / 4].aval |= static_cast<unsigned int>(mem.aval) << (8 * (i % 4));
            data[i / 4].bval |= static_cast<unsigned int>(mem.bval) << (8 * (i % 4));
         }
         else
         {
            data[i / 4].aval = mem.aval;
            data[i / 4].bval = mem.bval;
         }
      }
   }
   else
   {
      error("Read to invalid address skipped.\n");
   }
}

static FORCE_INLINE void __m_write(uint16_t max_bsize, uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   debug("Write %u bits at " PTR_FORMAT "\n", size, addr);
   bptr_t __addr = __m_memaddr(addr);
   if(__addr)
   {
      assert(max_bsize >= (size / 8) && "Memory write bitsize must be smaller than bus size");
      max_bsize = size / 8;
      for(uint16_t i = 0; i < max_bsize; ++i)
      {
         const uint8_t data_byte = data[i / 4].aval >> (8 * (i % 4));
         const uint8_t bdata_byte = data[i / 4].bval >> (8 * (i % 4));
         if(size >= (i * 8))
         {
            assert((bdata_byte == 0) && "Memory write data must not contain undefined states X or Z from "
                                        "the simulation");
            store(__addr + i, data[i / 4].aval >> (8 * (i % 4)));
         }
         else
         {
            const uint8_t mask = static_cast<uint8_t>((1 << (size % 8)) - 1);
            assert(((bdata_byte & mask) == 0) && "Memory write data must not contain undefined states X or Z from "
                                                 "the simulation");
            const uint8_t mem_val = (load(__addr + i).aval & ~mask) | (data_byte & mask);
            store(__addr + i, mem_val);
         }
      }
   }
   else
   {
      error("Write to invalid address skipped.\n");
   }
}

#pragma GCC push_options
#pragma GCC optimize("unroll-loops")

EXTERN_C void m_read8(svLogicVecVal* data, ptr_t addr)
{
   __m_read(1, data, addr);
}
EXTERN_C void m_read16(svLogicVecVal* data, ptr_t addr)
{
   __m_read(2, data, addr);
}
EXTERN_C void m_read32(svLogicVecVal* data, ptr_t addr)
{
   __m_read(4, data, addr);
}
EXTERN_C void m_read64(svLogicVecVal* data, ptr_t addr)
{
   __m_read(8, data, addr);
}
EXTERN_C void m_read128(svLogicVecVal* data, ptr_t addr)
{
   __m_read(16, data, addr);
}
EXTERN_C void m_read256(svLogicVecVal* data, ptr_t addr)
{
   __m_read(32, data, addr);
}
EXTERN_C void m_read512(svLogicVecVal* data, ptr_t addr)
{
   __m_read(64, data, addr);
}
EXTERN_C void m_read1024(svLogicVecVal* data, ptr_t addr)
{
   __m_read(128, data, addr);
}
EXTERN_C void m_read2048(svLogicVecVal* data, ptr_t addr)
{
   __m_read(256, data, addr);
}
EXTERN_C void m_read4096(svLogicVecVal* data, ptr_t addr)
{
   __m_read(512, data, addr);
}

EXTERN_C void m_write8(uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   __m_write(1, size, data, addr);
}
EXTERN_C void m_write16(uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   __m_write(2, size, data, addr);
}
EXTERN_C void m_write32(uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   __m_write(4, size, data, addr);
}
EXTERN_C void m_write64(uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   __m_write(8, size, data, addr);
}
EXTERN_C void m_write128(uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   __m_write(16, size, data, addr);
}
EXTERN_C void m_write256(uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   __m_write(32, size, data, addr);
}
EXTERN_C void m_write512(uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   __m_write(64, size, data, addr);
}
EXTERN_C void m_write1024(uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   __m_write(128, size, data, addr);
}
EXTERN_C void m_write2048(uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   __m_write(256, size, data, addr);
}
EXTERN_C void m_write4096(uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   __m_write(512, size, data, addr);
}

#pragma GCC pop_options
