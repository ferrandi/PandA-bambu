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
 * @file mdpi_driver.cpp
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#define __LOCAL_ENTITY MDPI_ENTITY_DRIVER
#define __REMOTE_ENTITY MDPI_ENTITY_SIM
#define __local_operation __get_operation(__LOCAL_ENTITY)
#define __remote_operation __get_operation(__REMOTE_ENTITY)

#include <mdpi/mdpi_debug.h>

#if __M_OUT_LVL <= 4
#define NDEBUG
#endif

#include <mdpi/mdpi_driver.h>
#include <mdpi/mdpi_ipc.h>
#include <mdpi/mdpi_types.h>
#include <mdpi/mdpi_user.h>

#include <algorithm>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <map>

#ifdef MDPI_PARALLEL_VERIFICATION
#include <pthread.h>
#endif

#define FORCE_INLINE __attribute__((always_inline)) inline

#define byte_offset(i) ((i & 3) << 3)

static void* __m_driver_loop(void*);

static pthread_t __m_ipc_driver = 0;

static mdpi_params_t __m_params;
static std::map<uint8_t, size_t> __m_params_size;
static std::map<ptr_t, bptr_t> __m_mmu;

static void __ipc_exit(mdpi_state_t state)
{
   mdpi_ipc_state_t expected;
   do
   {
      expected = atomic_load(&__local_operation.handle);
      if(expected == MDPI_IPC_STATE_WRITING)
         continue;
   } while(!atomic_compare_exchange_strong(&__local_operation.handle, &expected, MDPI_IPC_STATE_WRITING));
   __local_operation.type = MDPI_OP_TYPE_STATE_CHANGE;
   __local_operation.payload.sc.state = state;
   __local_operation.payload.sc.retval = EXIT_FAILURE;
   __ipc_complete(__LOCAL_ENTITY);
   // TODO: write sim report file
   exit(EXIT_FAILURE);
}

void __m_sim_start()
{
   debug("Waiting for simulator state report...\n");
   __ipc_wait(__LOCAL_ENTITY, MDPI_IPC_STATE_REQUEST);
   assert(__local_operation.type == MDPI_OP_TYPE_STATE_CHANGE && "Unexpected simulator request.");
   assert(__local_operation.payload.sc.state == MDPI_STATE_READY && "Unexpected simulator state.");
   __local_operation.payload.sc.state = MDPI_STATE_SETUP;
   __ipc_complete(__LOCAL_ENTITY);
   debug("Simulator state: %s (%u)\n", mdpi_state_str(__local_operation.payload.sc.state),
         __local_operation.payload.sc.retval);
   debug("Launch simulation\n");

#ifdef MDPI_PARALLEL_VERIFICATION
   int error = pthread_create(&__m_ipc_driver, NULL, __m_driver_loop, NULL);
   if(error)
   {
      error("An error occurred on co-simulation thread creation.\n");
      errno = error;
      perror("pthread_create");
      __ipc_exit(MDPI_STATE_ERROR);
   }
#else
   __m_driver_loop(NULL);
#endif
}

unsigned int __m_sim_end()
{
   unsigned int retval;

   debug("Waiting for simulator state report...\n");
#ifdef MDPI_PARALLEL_VERIFICATION
   int error = pthread_join(__m_ipc_driver, NULL);
   if(error)
   {
      error("An error occurred on co-simulation thread join.\n");
      errno = error;
      perror("pthread_join");
      exit(EXIT_FAILURE);
   }
#endif

   assert(__local_operation.type == MDPI_OP_TYPE_STATE_CHANGE && "Unexpected simulator request.");
   retval = __local_operation.payload.sc.retval;
   debug("Simulator state: %s (%u)\n", mdpi_state_str(__local_operation.payload.sc.state), retval);
   return retval;
}

void __m_abrupt_exit(int)
{
   mdpi_ipc_state_t expected;
   do
   {
      expected = atomic_load(&__local_operation.handle);
      if(expected == MDPI_IPC_STATE_WRITING)
         continue;
   } while(!atomic_compare_exchange_strong(&__local_operation.handle, &expected, MDPI_IPC_STATE_WRITING));
   __local_operation.type = MDPI_OP_TYPE_STATE_CHANGE;
   __local_operation.payload.sc.state = MDPI_STATE_ABORT;
   __local_operation.payload.sc.retval = EXIT_FAILURE;
   atomic_store(&__local_operation.handle, MDPI_IPC_STATE_DONE);
   error("Abrupt exception notification.\n");
#if __M_OUT_LVL > 4
   fflush(stdout);
#else
   fflush(stderr);
#endif
}

void __attribute__((constructor)) __mdpi_driver_init()
{
   static const int __sigs[] = {SIGINT, SIGABRT, SIGIOT};
   int error;
   size_t i;

   debug("Loading...\n");

   __ipc_init();

   struct sigaction sa;
   memset(&sa, 0, sizeof(sa));
   sa.sa_handler = __m_abrupt_exit;
   error = sigfillset(&sa.sa_mask);
   for(i = 0; i < (sizeof(__sigs) / sizeof(*__sigs)); ++i)
   {
      error = sigaction(__sigs[i], &sa, NULL);
      if(error)
      {
         error("Cannot install signal %d handler: %s.\n", __sigs[i], strerror(errno));
         exit(EXIT_FAILURE);
      }
   }
   debug("Loading completed.\n");
}

void __attribute__((destructor)) __mdpi_driver_fini()
{
   mdpi_ipc_state_t expected;
   do
   {
      expected = atomic_load(&__local_operation.handle);
      if(expected == MDPI_IPC_STATE_WRITING)
         continue;
   } while(!atomic_compare_exchange_strong(&__local_operation.handle, &expected, MDPI_IPC_STATE_WRITING));
   __local_operation.type = MDPI_OP_TYPE_STATE_CHANGE;
   __local_operation.payload.sc.state = MDPI_STATE_END;
   __local_operation.payload.sc.retval = EXIT_SUCCESS;
   __ipc_complete(__LOCAL_ENTITY);

   __ipc_fini();

   debug("Finalization completed.\n");
}

void __m_exit(int __status)
{
   info("Exit called with value %d\n", __status);
   __ipc_exit(MDPI_STATE_END);
}

void __m_abort()
{
   error("Co-simulation called abort\n");
   __ipc_exit(MDPI_STATE_ABORT);
}

void __m_assert_fail(const char* __assertion, const char* __file, unsigned int __line, const char* __function)
{
   error("%s: %d: %s: Assertion `%s' failed.\n", __file, __line, __function, __assertion);
   __m_abort();
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
   const bptr_t addr = *bits;
   const ptr_t dst = __m_reverse_memmap(addr);
   if(dst)
   {
      info("Pointer parameter " BPTR_FORMAT " mapped at " PTR_FORMAT "\n", bptr_to_int(addr), dst);
      assert((bitsize == PTR_SIZE || dst < (UINT64_MAX >> (64 - PTR_SIZE))) && "Pointer value overflow.");
      *bits = ptr_to_bptr(dst);
      return __m_setarg(index, reinterpret_cast<bptr_t>(bits), PTR_SIZE);
   }
   error("Unknown parameter %u address mapping for " BPTR_FORMAT "\n", index, bptr_to_int(addr));
   // TODO: report to simulator
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
         mmu_base->first, mmu_it->first, bptr_to_int(mmu_base->second + mmu_base->first),
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

static void __mem_read(const uint16_t size, bptr_t data, ptr_t addr)
{
   bptr_t __addr = __m_memaddr(addr);
   if(__addr)
   {
      debug("Read %u bytes at " PTR_FORMAT "->" BPTR_FORMAT "\n", size, addr, bptr_to_int(__addr));
      memcpy(data, __addr, size);
   }
   else
   {
      error("Read to non-mapped address " PTR_FORMAT ".\n", addr);
      // TODO: report abort to simulator
      abort();
   }
}

static void __mem_write(const uint16_t size, bptr_t data, ptr_t addr)
{
   bptr_t __addr = __m_memaddr(addr);
   if(__addr)
   {
      debug("Write %u bits at " PTR_FORMAT "->" BPTR_FORMAT "\n", size, addr, bptr_to_int(__addr));
      auto floor_bytes = size / 8;
      memcpy(__addr, data, floor_bytes);
      auto spare = size % 8;
      if(spare)
      {
         byte_t mask = 0xFF << spare;
         __addr[floor_bytes] = (__addr[floor_bytes] & mask) | (data[floor_bytes] & ~mask);
      }
   }
   else
   {
      error("Write to non-mapped address " PTR_FORMAT ".\n", addr);
      // TODO: report abort to simulator
      abort();
   }
}

static uint16_t __arg_read(unsigned int index, bptr_t buffer)
{
   debug("Parameter %u read\n", index);
   if(index >= __m_params.size)
   {
      error("Parameter index out of bounds: %u\n", index);
      __ipc_exit(MDPI_STATE_ERROR);
   }
   mdpi_parm_t* p = &__m_params.prms[index];
   uint16_t byte_count = (p->bitsize / 8) + ((p->bitsize % 8) != 0);
   memcpy(buffer, __m_params.prms[index].bits, byte_count);
   return p->bitsize;
}

static void __arg_write(unsigned int index, bptr_t buffer)
{
   debug("Parameter %u write\n", index);
   if(index >= __m_params.size)
   {
      error("Parameter index out of bounds: %u\n", index);
      __ipc_exit(MDPI_STATE_ERROR);
   }
   mdpi_parm_t* p = &__m_params.prms[index];
   uint16_t byte_count = (p->bitsize / 8) + ((p->bitsize % 8) != 0);
   memcpy(p->bits, buffer, byte_count);
}

static uint16_t __arg_size(unsigned int index)
{
   debug("Parameter %u size\n", index);
   if(index >= __m_params.size)
   {
      error("Parameter index out of bounds: %u\n", index);
      __ipc_exit(MDPI_STATE_ERROR);
   }
   return __m_params.prms[index].bitsize;
}

static void* __m_driver_loop(void*)
{
   debug("IPC thread started.\n");

   while(true)
   {
      __ipc_wait(__LOCAL_ENTITY, MDPI_IPC_STATE_REQUEST);
      switch(__local_operation.type)
      {
         case MDPI_OP_TYPE_STATE_CHANGE:
            return NULL;
            break;
         case MDPI_OP_TYPE_MEM_READ:
            __mem_read(__local_operation.payload.mem.size, __local_operation.payload.mem.buffer,
                       __local_operation.payload.mem.addr);
            __ipc_complete(__LOCAL_ENTITY);
            break;
         case MDPI_OP_TYPE_MEM_WRITE:
            __mem_write(__local_operation.payload.mem.size, __local_operation.payload.mem.buffer,
                        __local_operation.payload.mem.addr);
            __ipc_complete(__LOCAL_ENTITY);
            break;
         case MDPI_OP_TYPE_ARG_READ:
            __local_operation.payload.arg.bitsize =
                __arg_read(__local_operation.payload.arg.index, __local_operation.payload.arg.buffer);
            __ipc_complete(__LOCAL_ENTITY);
            break;
         case MDPI_OP_TYPE_ARG_WRITE:
            __arg_write(__local_operation.payload.arg.index, __local_operation.payload.arg.buffer);
            __ipc_complete(__LOCAL_ENTITY);
            break;
         case MDPI_OP_TYPE_ARG_SIZE:
            __local_operation.payload.arg.bitsize = __arg_size(__local_operation.payload.arg.index);
            __ipc_complete(__LOCAL_ENTITY);
            break;
         case MDPI_OP_TYPE_NONE:
         default:
            error("Unexpected transaction type: %u\n", __local_operation.type);
            // TODO: notify simulator for error
            __ipc_exit(MDPI_STATE_ERROR);
            break;
      }
   }
   return NULL;
}

void m_param_alloc(uint8_t idx, size_t size)
{
   info("Memory size for parameter %u set to %zu bytes.\n", idx, size);
   __m_params_size[idx] = size;
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
