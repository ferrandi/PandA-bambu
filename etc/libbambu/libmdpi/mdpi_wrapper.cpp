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
static std::map<ptr_t, bptr_t> __m_mmu;

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
   debug("Parameter %u is %u bits at " BPTR_FORMAT "\n", index, bitsize, bptr_to_int(bits));
   __m_params.prms[index].bits = static_cast<bptr_t>(bits);
   __m_params.prms[index].bitsize = bitsize;
}

struct addr_search_s
{
   const bptr_t _addr;
   addr_search_s(const bptr_t addr) : _addr(addr)
   {
   }
   bool operator()(std::pair<const ptr_t, bptr_t>& _t)
   {
      return (_t.second + _t.first) == _addr;
   }
};

void __m_setptrarg(uint8_t index, bptr_t* bits, uint16_t bitsize)
{
   assert((bitsize == PTR_SIZE) && "Unexpected pointer size");
   const bptr_t addr = *bits;
   const std::map<ptr_t, bptr_t>::iterator mmu_it = std::find_if(__m_mmu.begin(), __m_mmu.end(), addr_search_s(addr));
   if(mmu_it != __m_mmu.end())
   {
      debug("Pointer parameter " BPTR_FORMAT " mapped at " PTR_FORMAT "\n", bptr_to_int(addr), mmu_it->first);
      *bits = ptr_to_bptr(mmu_it->first);
      return __m_setarg(index, reinterpret_cast<bptr_t>(bits), PTR_SIZE);
   }
   error("Unknown parameter address mapping for " BPTR_FORMAT "\n", bptr_to_int(addr));
   exit(EXIT_FAILURE);
}

void __m_memmap_init()
{
   debug("Initializing co-simulation MMU\n");
   __m_mmu.clear();
   __m_mmu[0] = NULL;
}

void __m_memmap(ptr_t dst, void* _bits)
{
   bptr_t bits = reinterpret_cast<bptr_t>(_bits);
   debug("Address " BPTR_FORMAT " mapped at " PTR_FORMAT "\n", bptr_to_int(bits), dst);
   __m_mmu[dst] = bits - dst;
}

bptr_t __m_memaddr(ptr_t sim_addr)
{
   const std::map<ptr_t, bptr_t>::iterator mmu_it = --__m_mmu.upper_bound(sim_addr);
   if(mmu_it != __m_mmu.begin())
   {
      bptr_t addr = mmu_it->second + sim_addr;
      debug("Address " PTR_FORMAT " mapped back at " BPTR_FORMAT "\n", sim_addr, bptr_to_int(addr));
      return addr;
   }
   error("Unknown address mapping for " PTR_FORMAT "\n", sim_addr);
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

void __m_alloc_param(uint8_t idx, size_t size)
{
   if(!__m_params_size.count(idx))
   {
      __m_params_size[idx] = size;
      debug("Memory size for parameter %u set to %zu bytes.\n", idx, size);
   }
}

void m_alloc_param(uint8_t idx, size_t size)
{
   info("Memory size for parameter %u set to %zu bytes.\n", idx, size);
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
