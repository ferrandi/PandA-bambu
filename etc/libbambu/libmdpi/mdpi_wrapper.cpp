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
 * @file mdpi_wrapper.cpp
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "mdpi_wrapper.h"

#include "mdpi_debug.h"
#include "mdpi_types.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <pthread.h>

static enum mdpi_state __m_state[MDPI_ENTITY_COUNT];
static pthread_mutex_t __m_lock;
static pthread_cond_t __m_sig;

mdpi_params_t __m_params;
static std::map<uint8_t, size_t> __m_params_size;
static std::map<ptr_t, ptr_t> __m_mmu;

void __m_arg_init(uint8_t argcount)
{
   debug("Initializing shared memory for %u parameters\n", argcount);
   __m_params.prms = (mdpi_parm_t*)malloc(sizeof(mdpi_parm_t) * argcount);
   if(!__m_params.prms)
   {
      exit(EXIT_FAILURE);
   }
   __m_params.size = argcount;
}

void __m_arg_fini()
{
   debug("Finalizing parameters' shared memory\n");
   free(__m_params.prms);
   __m_params.size = 0;
}

void __m_setarg(uint8_t index, void* bits, uint16_t bitsize)
{
   if(index >= __m_params.size)
   {
      error("Parameter index out of bounds %u\n", (unsigned int)index);
      exit(EXIT_FAILURE);
   }
#if __WORDSIZE == 64
   debug("Parameter %u is %u bits at 0x%016llX\n", index, bitsize, (ptr_t)bits);
#else
   debug("Parameter %u is %u bits at 0x%08X\n", index, bitsize, (ptr_t)bits);
#endif
   __m_params.prms[index].bits = bits;
   __m_params.prms[index].bitsize = bitsize;
}

struct addr_search_s
{
   const ptr_t _addr;
   addr_search_s(ptr_t addr) : _addr(addr)
   {
   }
   bool operator()(std::pair<const ptr_t, ptr_t>& _t)
   {
      return _t.first + _t.second == _addr;
   }
};

void __m_setptrarg(uint8_t index, void* bits, uint16_t bitsize)
{
   assert((bitsize == (sizeof(ptr_t) * 8)) && "Unexpected pointer size");
   const ptr_t addr = *static_cast<ptr_t*>(bits);
   const std::map<ptr_t, ptr_t>::iterator mmu_it = std::find_if(__m_mmu.begin(), __m_mmu.end(), addr_search_s(addr));
   if(mmu_it != __m_mmu.end())
   {
#if __WORDSIZE == 64
      debug("Pointer parameter 0x%016llX mapped at 0x%016llX\n", addr, mmu_it->first);
#else
      debug("Pointer parameter 0x%08X mapped at 0x%08X\n", addr, mmu_it->first);
#endif
      *static_cast<ptr_t*>(bits) = mmu_it->first;
      return __m_setarg(index, bits, sizeof(ptr_t) * 8);
   }
#if __WORDSIZE == 64
   error("Unknown parameter address mapping for 0x%016llX\n", addr);
#else
   error("Unknown parameter address mapping for 0x%08X\n", addr);
#endif
   exit(EXIT_FAILURE);
}

void __m_memmap_init()
{
   debug("Initializing co-simulation MMU\n");
   __m_mmu.clear();
   __m_mmu[0] = 0;
}

void __m_memmap(ptr_t dst, void* bits)
{
#if __WORDSIZE == 64
   debug("Address 0x%016llX mapped at 0x%016llX\n", reinterpret_cast<ptr_t>(bits), dst);
#else
   debug("Address 0x%08X mapped at 0x%08X\n", reinterpret_cast<ptr_t>(bits), dst);
#endif
   __m_mmu[dst] = reinterpret_cast<ptr_t>(bits) - dst;
}

ptr_t __m_memaddr(ptr_t sim_addr)
{
   const std::map<ptr_t, ptr_t>::iterator mmu_it = --__m_mmu.upper_bound(sim_addr);
   if(mmu_it != __m_mmu.begin())
   {
      const ptr_t addr = sim_addr + mmu_it->second;
#if __WORDSIZE == 64
      debug("Address 0x%016llX mapped back at 0x%016llX\n", sim_addr, addr);
#else
      debug("Address 0x%08X mapped back at 0x%08X\n", sim_addr, addr);
#endif
      return addr;
   }
#if __WORDSIZE == 64
   error("Unknown address mapping for 0x%016llX\n", sim_addr);
#else
   error("Unknown address mapping for 0x%08X\n", sim_addr);
#endif
   return 0;
}

size_t __m_param_size(uint8_t idx)
{
   const std::map<uint8_t, size_t>::iterator mps_it = __m_params_size.find(idx);
   if(mps_it != __m_params_size.end())
   {
      return mps_it->second;
   }
   error("Parameter size for parameter %u has not been set.\n", idx);
   exit(EXIT_FAILURE);
   return 0;
}

void m_alloc_param(uint8_t idx, size_t size)
{
   debug("Memory parameter size for parameter %u set to %u bytes.\n", idx, size);
   __m_params_size[idx] = size;
}

void __m_signal_init()
{
   int error;
   for(size_t i = 0; i < MDPI_ENTITY_COUNT; ++i)
   {
      __m_state[i] = MDPI_UNDEFINED;
   }
   error = pthread_mutex_init(&__m_lock, NULL);
   if(error)
   {
      perror("pthread_mutex_init error");
      exit(EXIT_FAILURE);
   }
   error = pthread_cond_init(&__m_sig, NULL);
   if(error)
   {
      perror("pthread_cond_init error");
      exit(EXIT_FAILURE);
   }
}

void __m_signal_to(enum mdpi_entity entity, enum mdpi_state state)
{
   assert(entity < MDPI_ENTITY_COUNT && "Entity not valid");
   assert(state != MDPI_UNDEFINED && "Signal state must be a valid state");
   int error;
   error = pthread_mutex_lock(&__m_lock);
   if(error)
   {
      perror("pthread_mutex_lock error");
      exit(EXIT_FAILURE);
   }
   debug("%s <- %s\n", mdpi_entity_str(entity), mdpi_state_str(state));
   if(__m_state[entity] != MDPI_UNDEFINED)
   {
      debug("Overwriting signal state.\n");
   }
   fflush(stdout);
   __m_state[entity] = state;
   error = pthread_cond_signal(&__m_sig);
   if(error)
   {
      perror("pthread_cond_signal error");
      exit(EXIT_FAILURE);
   }
   error = pthread_mutex_unlock(&__m_lock);
   if(error)
   {
      perror("pthread_mutex_unlock error");
      exit(EXIT_FAILURE);
   }
}

enum mdpi_state __m_wait_for(enum mdpi_entity entity)
{
   assert(entity < MDPI_ENTITY_COUNT && "Entity not valid");
   int error;
   enum mdpi_state state_read;
   error = pthread_mutex_lock(&__m_lock);
   if(error)
   {
      perror("pthread_mutex_lock error");
      exit(EXIT_FAILURE);
   }
   if(__m_state[entity] == MDPI_UNDEFINED)
   {
      error = pthread_cond_wait(&__m_sig, &__m_lock);
      if(error)
      {
         perror("pthread_cond_wait error");
         exit(EXIT_FAILURE);
      }
   }
   assert(__m_state[entity] != MDPI_UNDEFINED && "State is undefined after condition signal.");
   state_read = __m_state[entity];
   __m_state[entity] = MDPI_UNDEFINED;
   error = pthread_mutex_unlock(&__m_lock);
   if(error)
   {
      perror("pthread_mutex_unlock error");
      exit(EXIT_FAILURE);
   }
   return state_read;
}
