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

#define __MDPI_INTERNAL
#define NDEBUG

#include "mdpi_debug.h"
#include "mdpi_types.h"
#include "mdpi_wrapper.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <pthread.h>

#define FORCE_INLINE __attribute__((always_inline)) inline

#define byte_offset(i) ((i & 3) << 3)

EXTERN_C int m_cosim_main(int argc, const char** argv);

#ifdef __M_COSIM_ARGV
#include __M_COSIM_ARGV
#else
static const char* __m_cosim_argv[] = {"m_cosim_main"};
#endif

volatile pthread_t __m_main_tid;
static pthread_t __m_cosim_thread;

static enum mdpi_state __m_state[MDPI_ENTITY_COUNT];
static pthread_mutex_t __m_lock;
static pthread_cond_t __m_sig;

static mdpi_params_t __m_params;
static std::map<uint8_t, size_t> __m_params_size;
static std::map<ptr_t, bptr_t> __m_mmu;

static void __m_signal_init()
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

static void* __m_cosim_main(void*)
{
   int retval = -1;
   debug("Thread started\n");

   enum mdpi_state sim_state = __m_wait_for(MDPI_ENTITY_COSIM);
   if(sim_state == MDPI_COSIM_INIT)
   {
      info("Co-simulation started\n");
      retval = m_cosim_main(sizeof(__m_cosim_argv) / sizeof(*__m_cosim_argv), __m_cosim_argv);
      info("Co-simulation finished\n");
   }
   else
   {
      error("Co-simulation startup failed. Unexpected state recived from simulator: %s\n", mdpi_state_str(sim_state));
   }

   __m_signal_to(MDPI_ENTITY_SIM, MDPI_COSIM_END);
   return reinterpret_cast<void*>(static_cast<long>(((retval & 0xFF) << 8) | MDPI_COSIM_END));
}

EXTERN_C void __m_exit(int __status)
{
   enum mdpi_state state;
   info("Exit called with value %d\n", __status);
   debug("Simulator reported state: %s\n", mdpi_state_str(state));
   __m_signal_to(MDPI_ENTITY_SIM, MDPI_COSIM_END);
   pthread_exit(reinterpret_cast<void*>(static_cast<long>(((__status & 0xFF) << 8) | MDPI_COSIM_END)));
}

EXTERN_C void __m_abort()
{
   enum mdpi_state state;
   error("Co-simulation called abort\n");
   debug("Simulator reported state: %s\n", mdpi_state_str(state));
   __m_signal_to(MDPI_ENTITY_SIM, MDPI_COSIM_END);
   pthread_exit(reinterpret_cast<void*>(static_cast<long>(MDPI_COSIM_ABORT)));
}

EXTERN_C void __m_assert_fail(const char* __assertion, const char* __file, unsigned int __line, const char* __function)
{
   error("%s: %d: %s: Assertion `%s' failed.\n", __file, __line, __function, __assertion);
   __m_abort();
}

void m_init()
{
   int retval;

   __m_main_tid = pthread_self();

   __m_signal_init();

   retval = pthread_create(&__m_cosim_thread, NULL, __m_cosim_main, NULL);
   if(retval)
   {
      error("An error occurred on co-simulation thread creation.\n");
      perror("MDPI library initialization error");
      exit(EXIT_FAILURE);
   }

   debug("Initialization successful\n");
}

unsigned int m_next(unsigned int state)
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

int m_fini()
{
   long retval = 0;
   pthread_join(__m_cosim_thread, (void**)&retval);

   debug("Finalization successful\n");
   return static_cast<int>(retval);
}

void m_getarg(svLogicVecVal* data, unsigned int index)
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
   for(i = 0; i < byte_count; ++i)
   {
      uint8_t mem = prm[i];
      if(i % 4)
      {
         data[i / 4].aval |= static_cast<unsigned int>(mem) << byte_offset(i);
      }
      else
      {
         data[i / 4].aval = mem;
         data[i / 4].bval = 0;
      }
   }
}

void m_setarg(CONSTARG svLogicVecVal* data, unsigned int index)
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
      prm[i] = data[i / 4].aval >> byte_offset(i);
   }
}

unsigned int m_getptrargsize(unsigned int index)
{
   return __m_param_size(index);
}

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
   info("Parameter %u is %u bits at " BPTR_FORMAT "\n", index, bitsize, bptr_to_int(bits));
   __m_params.prms[index].bits = static_cast<bptr_t>(bits);
   __m_params.prms[index].bitsize = bitsize;
}

static ptr_t __m_reverse_memmap(const bptr_t addr)
{
   std::map<ptr_t, bptr_t>::iterator curr = __m_mmu.begin(), prev;
   do
   {
      if(curr->second)
      {
         prev = curr++;
      }
      else
      {
         prev = ++curr;
         ++curr;
      }
   } while(prev != __m_mmu.end() && (addr < (prev->first + prev->second) || addr >= (curr->first + prev->second)));
   if(prev == __m_mmu.end())
   {
      return 0;
   }
   return addr - prev->second;
}

void __m_setptrarg(uint8_t index, bptr_t* bits, uint16_t bitsize)
{
   assert((bitsize == PTR_SIZE) && "Unexpected pointer size");
   const bptr_t addr = *bits;
   const ptr_t dst = __m_reverse_memmap(addr);
   if(dst)
   {
      info("Pointer parameter " BPTR_FORMAT " mapped at " PTR_FORMAT "\n", bptr_to_int(addr), dst);
      *bits = ptr_to_bptr(dst);
      return __m_setarg(index, reinterpret_cast<bptr_t>(bits), PTR_SIZE);
   }
   error("Unknown parameter %u address mapping for " BPTR_FORMAT "\n", index, bptr_to_int(addr));
   exit(EXIT_FAILURE);
}

void __m_memmap_init()
{
   debug("Initializing co-simulation MMU\n");
   __m_mmu.clear();
   __m_mmu[0] = NULL;
}

int __m_memmap(ptr_t dst, void* _bits, size_t bytes)
{
   bptr_t bits = reinterpret_cast<bptr_t>(_bits);
   info("Address " BPTR_FORMAT " mapped at " PTR_FORMAT " (%zu bytes)\n", bptr_to_int(bits), dst, bytes);

   const std::pair<std::map<ptr_t, bptr_t>::iterator, bool> base = __m_mmu.insert(std::make_pair(dst, bits - dst));
   const std::pair<std::map<ptr_t, bptr_t>::iterator, bool> top =
       __m_mmu.insert(std::make_pair<ptr_t, bptr_t>(dst + bytes, 0));
   std::map<ptr_t, bptr_t>::iterator front = base.first, prev = base.first;
   --prev;
   std::map<ptr_t, bptr_t>::iterator back = top.first, next = top.first;
   ++next;
   if(!base.second)
   {
      if(!front->second)
      {
         front->second = bits - dst;
         if(prev->second == front->second)
         {
            __m_mmu.erase(front);
            front = prev;
            --prev;
         }
      }
      else if(front->second != (bits - dst))
      {
         error("Uncorrelated memory spaces overlap: " PTR_FORMAT "(" BPTR_FORMAT ") over " PTR_FORMAT "(" BPTR_FORMAT
               ").\n",
               dst, bptr_to_int(bits + dst), front->first, bptr_to_int(front->second + front->first));
         return 1;
      }
   }
   else if(prev->second)
   {
      if(prev->second != front->second)
      {
         error("Uncorrelated memory spaces overlap: " PTR_FORMAT "(" BPTR_FORMAT ") over " PTR_FORMAT "(" BPTR_FORMAT
               ").\n",
               front->first, bptr_to_int(front->second + front->first), prev->first,
               bptr_to_int(prev->second + prev->first));
         return 1;
      }
      front = prev;
   }
   if(top.second && next != __m_mmu.end() && !next->second)
   {
      back = next;
   }
   next = front;
   ++next;
   if(next != back)
   {
      std::map<ptr_t, bptr_t>::iterator it = next;
      for(; it != back; ++it)
      {
         if(it->second && it->second != front->second)
         {
            error("Uncorrelated memory spaces overlap: " PTR_FORMAT "(" BPTR_FORMAT ") over " PTR_FORMAT "(" BPTR_FORMAT
                  ").\n",
                  front->first, bptr_to_int(front->second + front->first), it->first,
                  bptr_to_int(it->second + it->first));
            return 1;
         }
      }
      __m_mmu.erase(next, back);
   }
   return 0;
}

static bptr_t __m_memaddr(ptr_t sim_addr)
{
   std::map<ptr_t, bptr_t>::iterator mmu_it = --__m_mmu.upper_bound(sim_addr);
   if(mmu_it != __m_mmu.begin() && mmu_it->second)
   {
      bptr_t addr = mmu_it->second + sim_addr;
      return addr;
   }
   std::map<ptr_t, bptr_t>::iterator mmu_base;
   if(mmu_it == __m_mmu.begin())
   {
      mmu_base = ++mmu_it;
      ++mmu_it;
   }
   else
   {
      mmu_base = mmu_it;
      --mmu_base;
   }
   error("Nearest memory space is [" PTR_FORMAT ", " PTR_FORMAT "] -> [" BPTR_FORMAT ", " BPTR_FORMAT
         "] (%zu bytes).\n",
         mmu_base->first, bptr_to_int(mmu_base->second + mmu_base->first), mmu_it->first,
         bptr_to_int(mmu_base->second + mmu_it->first), static_cast<size_t>(mmu_it->first - mmu_base->first));
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

void __m_param_alloc(uint8_t idx, size_t size)
{
   if(!__m_params_size.count(idx))
   {
      __m_params_size[idx] = size;
      debug("Memory size for parameter %u set to %zu bytes.\n", idx, size);
   }
}

void m_param_alloc(uint8_t idx, size_t size)
{
   info("Memory size for parameter %u set to %zu bytes.\n", idx, size);
   __m_params_size[idx] = size;
}

static FORCE_INLINE uint8_t load(bptr_t addr)
{
   return *addr;
}

static void __attribute__((noinline)) __m_read(const uint16_t bsize, svLogicVecVal* data, ptr_t addr)
{
   bptr_t __addr = __m_memaddr(addr);
   if(__addr)
   {
      debug("Read %u bytes at " PTR_FORMAT "->" BPTR_FORMAT "\n", bsize, addr, bptr_to_int(__addr));
#pragma unroll(4)
      for(uint16_t i = 0; i < bsize; ++i)
      {
         uint8_t mem = load(__addr + i);
         if(i % 4)
         {
            data[i / 4].aval |= static_cast<unsigned int>(mem) << byte_offset(i);
         }
         else
         {
            data[i / 4].aval = mem;
            data[i / 4].bval = 0;
         }
      }
   }
   else
   {
      error("Read to non-mapped address " PTR_FORMAT ".\n", addr);
      abort();
   }
}

static FORCE_INLINE void store(bptr_t addr, uint8_t val)
{
   *addr = val;
}

static void __attribute__((noinline))
__m_write(const uint16_t max_bsize, uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   bptr_t __addr = __m_memaddr(addr);
   if(__addr)
   {
      debug("Write %u bits at " PTR_FORMAT "->" BPTR_FORMAT "\n", size, addr, bptr_to_int(__addr));
      assert((max_bsize * 8) >= size && "Memory write bitsize must be smaller than bus size");
      const uint16_t bsize = (size / 8) + ((size % 8) != 0);
#pragma unroll(4)
      for(uint16_t i = 0; i < bsize; ++i)
      {
         const uint8_t data_byte = data[i / 4].aval >> byte_offset(i);
         const uint8_t bdata_byte = data[i / 4].bval >> byte_offset(i);
         if(size >= (i * 8))
         {
            assert((bdata_byte == 0) && "Memory write data must not contain undefined states X or Z from "
                                        "the simulation");
            store(__addr + i, data[i / 4].aval >> byte_offset(i));
         }
         else
         {
            const uint8_t mask = static_cast<uint8_t>((1 << (size & 7)) - 1);
            assert(((bdata_byte & mask) == 0) && "Memory write data must not contain undefined states X or Z from "
                                                 "the simulation");
            const uint8_t mem_val = (load(__addr + i) & ~mask) | (data_byte & mask);
            store(__addr + i, mem_val);
         }
      }
   }
   else
   {
      error("Write to non-mapped address " PTR_FORMAT ".\n", addr);
      abort();
   }
}

void m_read8(svLogicVecVal* data, ptr_t addr)
{
   __m_read(1, data, addr);
}
void m_read16(svLogicVecVal* data, ptr_t addr)
{
   __m_read(2, data, addr);
}
void m_read32(svLogicVecVal* data, ptr_t addr)
{
   __m_read(4, data, addr);
}
void m_read64(svLogicVecVal* data, ptr_t addr)
{
   __m_read(8, data, addr);
}
void m_read128(svLogicVecVal* data, ptr_t addr)
{
   __m_read(16, data, addr);
}
void m_read256(svLogicVecVal* data, ptr_t addr)
{
   __m_read(32, data, addr);
}
void m_read512(svLogicVecVal* data, ptr_t addr)
{
   __m_read(64, data, addr);
}
void m_read1024(svLogicVecVal* data, ptr_t addr)
{
   __m_read(128, data, addr);
}
void m_read2048(svLogicVecVal* data, ptr_t addr)
{
   __m_read(256, data, addr);
}
void m_read4096(svLogicVecVal* data, ptr_t addr)
{
   __m_read(512, data, addr);
}

void m_write8(uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   __m_write(1, size, data, addr);
}
void m_write16(uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   __m_write(2, size, data, addr);
}
void m_write32(uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   __m_write(4, size, data, addr);
}
void m_write64(uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   __m_write(8, size, data, addr);
}
void m_write128(uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   __m_write(16, size, data, addr);
}
void m_write256(uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   __m_write(32, size, data, addr);
}
void m_write512(uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   __m_write(64, size, data, addr);
}
void m_write1024(uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   __m_write(128, size, data, addr);
}
void m_write2048(uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   __m_write(256, size, data, addr);
}
void m_write4096(uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   __m_write(512, size, data, addr);
}

EXTERN_C float m_float_distancef(float a, float b)
{
   return m_float_distance<float>(a, b);
}

EXTERN_C double m_float_distance(double a, double b)
{
   return m_float_distance<double>(a, b);
}

EXTERN_C long double m_float_distancel(long double a, long double b)
{
   return m_float_distance<long double>(a, b);
}

EXTERN_C float m_floats_distancef(float a, float b)
{
   return m_floats_distance<float>(a, b);
}

EXTERN_C double m_floats_distance(double a, double b)
{
   return m_floats_distance<double>(a, b);
}

EXTERN_C long double m_floats_distancel(long double a, long double b)
{
   return m_floats_distance<long double>(a, b);
}
