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
#include <mdpi/mdpi_memmap.h>
#include <mdpi/mdpi_types.h>
#include <mdpi/mdpi_user.h>

#include <algorithm>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef MDPI_PARALLEL_VERIFICATION
#include <pthread.h>
#endif

#define FORCE_INLINE __attribute__((always_inline)) inline

#define byte_offset(i) ((i & 3) << 3)

#if(__WORDSIZE == 32 && !defined(__M64)) || (__WORDSIZE == 64 && defined(__M64))
#define ENABLE_SHARED_MEMORY 1
#else
#define ENABLE_SHARED_MEMORY 0
#endif

static void* __m_driver_loop(void*);

static pid_t __m_sim_pid = 0;
static pthread_t __m_ipc_driver = 0;

static mdpi_params_t __m_params;
static std::map<uint8_t, size_t> __m_params_size;
static std::unique_ptr<memmap> __m_mapper;

class device_memmap : public memmap
{
 private:
   std::map<ptr_t, bptr_t> __m_mmu;

 public:
   device_memmap()
   {
      __m_mmu[0] = NULL;
   }

   int map(ptr_t dst, void* src, size_t bytes) override
   {
      bptr_t bits = reinterpret_cast<bptr_t>(src);
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
               error("Uncorrelated memory spaces overlap: " PTR_FORMAT "(" BPTR_FORMAT ") over " PTR_FORMAT
                     "(" BPTR_FORMAT ").\n",
                     front->first, bptr_to_int(front->second + front->first), it->first,
                     bptr_to_int(it->second + it->first));
               return 1;
            }
         }
         __m_mmu.erase(next, back);
      }
      return 0;
   }

   bptr_t addrmap(ptr_t sim_addr) override
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

   ptr_t mapaddr(const bptr_t addr) override
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
};

#if ENABLE_SHARED_MEMORY
class shared_memmap : public memmap
{
 public:
   int map(ptr_t dst, void* src, size_t bytes) override
   {
      return 0;
   }

   bptr_t addrmap(ptr_t sim_addr) override
   {
      return reinterpret_cast<bptr_t>(sim_addr);
   }

   ptr_t mapaddr(const bptr_t addr) override
   {
      return reinterpret_cast<ptr_t>(addr);
   }
};
#endif

static FORCE_INLINE void __ipc_exit(mdpi_state_t state, uint8_t retval)
{
   __ipc_exit(__LOCAL_ENTITY, MDPI_IPC_STATE_DONE, state, retval);
}

static void __ipc_abort()
{
   __ipc_exit(MDPI_STATE_ABORT, EXIT_FAILURE);
   // TODO: write sim report file
   exit(EXIT_FAILURE);
}

void __m_sig_handler(int __sig)
{
   if(__sig == SIGCHLD)
   {
      int status;
      if(!__m_sim_pid)
      {
         return;
      }
      __m_sim_pid = 0;
      if(wait(&status) == -1)
      {
         error("Error waiting for simulation process.\n");
         perror("wait failed");
      }
      else
      {
         if(WIFEXITED(status) && !WEXITSTATUS(status))
         {
            debug("Simulation process exited with code %d.\n", WEXITSTATUS(status));
            return;
         }
         error("Simulation process terminated with error.\n");
      }
   }
   else
   {
      error("Abrupt exception: %u\n", __sig);
   }
   __ipc_exit(MDPI_STATE_ABORT, EXIT_FAILURE);
#if __M_OUT_LVL > 4
   fflush(stdout);
#else
   fflush(stderr);
#endif
   exit(EXIT_FAILURE);
}

void __m_exit(int __status)
{
   info("Exit called with value %d\n", __status);
   __ipc_exit(MDPI_STATE_END, __status);
   exit(__status);
}

void __m_abort()
{
   error("Co-simulation called abort\n");
   __ipc_abort();
}

void __m_assert_fail(const char* __assertion, const char* __file, unsigned int __line, const char* __function)
{
#if __M_OUT_LVL > 4
   fprintf(stdout,
#else
   fprintf(stderr,
#endif
           "%s: %d: %s: Assertion `%s' failed.\n", __file, __line, __function, __assertion);
   __m_abort();
}

void __attribute__((constructor)) __mdpi_driver_init()
{
   static const int __sigs[] = {SIGINT, SIGABRT, SIGSEGV, SIGCHLD};
   int error;
   size_t i;
   char* sim_argv[4] = {"bash", "-c", NULL, NULL};

   debug("Loading MDPI library...\n");

   __ipc_init(MDPI_ENTITY_COUNT);

   for(i = 0; i < (sizeof(__sigs) / sizeof(*__sigs)); ++i)
   {
      signal(__sigs[i], __m_sig_handler);
   }

   __m_sim_pid = fork();
   if(__m_sim_pid == -1)
   {
      error("Error forking simulation process.\n");
      perror("fork");
      exit(EXIT_FAILURE);
   }
   else if(!__m_sim_pid)
   {
      sim_argv[2] = getenv(__M_IPC_SIM_CMD_ENV);
      if(!sim_argv[2])
      {
         error("Simulation command line environment variable not set.\n");
         _exit(EXIT_FAILURE);
      }
      error = unsetenv("LD_PRELOAD");
      if(error)
      {
         error("Failed to unset LD_PRELOAD.\n");
         perror("unsetenv");
         _exit(EXIT_FAILURE);
      }
      debug("Simulation process command line: \"%s\"", sim_argv[2]);
      error = execvp("bash", sim_argv);
      error("Failed to launch simulation process.\n");
      perror("execv");
      _exit(EXIT_FAILURE);
   }
   debug("Launched simulation process with PID %d.\n", __m_sim_pid);

   debug("Loading completed.\n");
}

void __attribute__((destructor)) __mdpi_driver_fini()
{
   __ipc_exit(MDPI_STATE_END, EXIT_SUCCESS);
   __ipc_fini();
   if(__m_sim_pid)
   {
      int status;
      __m_sim_pid = 0;
      if(wait(&status) == -1)
      {
         error("Error waiting for simulation process.\n");
         perror("wait failed");
      }
      else if(!WIFEXITED(status) || WEXITSTATUS(status))
      {
         error("Simulation process terminated with error.\n");
      }
   }
   debug("Finalization completed.\n");
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
      __ipc_abort();
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

void __m_setptrarg(uint8_t index, bptr_t* bits, uint16_t bitsize)
{
   const bptr_t addr = *bits;
   const ptr_t dst = __m_mapper->mapaddr(addr);
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

void __m_memmap_init(int map_mode)
{
   debug("Initializing co-simulation MMU\n");
   if(map_mode == MDPI_MEMMAP_DEVICE)
   {
      __m_mapper = std::unique_ptr<memmap>(new device_memmap());
   }
#if ENABLE_SHARED_MEMORY
   else if(map_mode == MDPI_MEMMAP_SHARED)
   {
      __m_mapper = std::unique_ptr<memmap>(new shared_memmap());
   }
#endif
   else
   {
      error("Unsupported memory mapping mode '%d'.", map_mode);
      exit(EXIT_FAILURE);
   }
}

int __m_memmap(ptr_t dst, void* src, size_t bytes)
{
   return __m_mapper->map(dst, src, bytes);
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
   bptr_t __addr = __m_mapper->addrmap(addr);
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
   bptr_t __addr = __m_mapper->addrmap(addr);
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

static uint16_t __arg_read(uint8_t* index, bptr_t buffer)
{
   debug("Parameter %u read\n", *index);
   if(__m_params.size == 0)
   {
      error("Parameter read on uninitialized parameters' list.\n");
      *index = MDPI_ARG_IDX_EMPTY;
      return 0;
   }
   else if(*index >= __m_params.size)
   {
      error("Parameter index out of bounds: %u\n", *index);
      *index = MDPI_ARG_IDX_OUT_OF_BOUNDS;
      return 0;
   }
   mdpi_parm_t* p = &__m_params.prms[*index];
   uint16_t byte_count = (p->bitsize / 8) + ((p->bitsize % 8) != 0);
   memcpy(buffer, p->bits, byte_count);
   return p->bitsize;
}

static void __arg_write(uint8_t* index, bptr_t buffer)
{
   debug("Parameter %u write\n", *index);
   if(__m_params.size == 0)
   {
      error("Parameter write on uninitialized parameters' list.\n");
      *index = MDPI_ARG_IDX_EMPTY;
      return;
   }
   else if(*index >= __m_params.size)
   {
      error("Parameter index out of bounds: %u\n", *index);
      *index = MDPI_ARG_IDX_OUT_OF_BOUNDS;
      return;
   }
   mdpi_parm_t* p = &__m_params.prms[*index];
   uint16_t byte_count = (p->bitsize / 8) + ((p->bitsize % 8) != 0);
   memcpy(p->bits, buffer, byte_count);
}

static uint64_t __param_size(uint8_t* index)
{
   debug("Parameter %u size\n", *index);
   if(__m_params_size.empty())
   {
      error("Parameter size request on uninitialized parameters' list.\n");
      *index = MDPI_ARG_IDX_EMPTY;
      return 0;
   }
   const std::map<uint8_t, size_t>::iterator mps_it = __m_params_size.find(*index);
   if(mps_it == __m_params_size.end())
   {
      error("Parameter index out of bounds: %u\n", *index);
      *index = MDPI_ARG_IDX_OUT_OF_BOUNDS;
      return 0;
   }
   return mps_it->second;
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
                __arg_read(&__local_operation.payload.arg.index, __local_operation.payload.arg.buffer);
            __ipc_complete(__LOCAL_ENTITY);
            break;
         case MDPI_OP_TYPE_ARG_WRITE:
            __arg_write(&__local_operation.payload.arg.index, __local_operation.payload.arg.buffer);
            __ipc_complete(__LOCAL_ENTITY);
            break;
         case MDPI_OP_TYPE_PARAM_INFO:
            __local_operation.payload.param.size = __param_size(&__local_operation.payload.param.index);
            __ipc_complete(__LOCAL_ENTITY);
            break;
         case MDPI_OP_TYPE_NONE:
         default:
            error("Unexpected transaction type: %u\n", __local_operation.type);
            __ipc_abort();
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
