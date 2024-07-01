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
 *              Copyright (C) 2023-2024 Politecnico di Milano
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

#include <mdpi/mdpi_debug.h>

#if __M_OUT_LVL <= 4 && !defined(NDEBUG)
#define NDEBUG
#endif

#ifndef NDEBUG
#define DEBUG_PARAM(p) p
#else
#define DEBUG_PARAM(p)
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
#include <vector>

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

static std::vector<std::unique_ptr<interface>> __m_interfaces;

void __m_interface_set(uint8_t id, interface* if_manager)
{
   if(__m_interfaces.size() <= id)
   {
      __m_interfaces.resize(id + 1);
   }
   __m_interfaces[id] = std::unique_ptr<interface>(if_manager);
}

void __m_interface_fini()
{
   debug("Finalize interface list.\n");
   __m_interfaces.clear();
}

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
      return ptr_to_bptr(sim_addr);
   }

   ptr_t mapaddr(const bptr_t addr) override
   {
      return reinterpret_cast<ptr_t>(addr);
   }
};
#endif

interface::interface(uint8_t idx) : _idx(idx)
{
}

int interface::state(int data)
{
   if_error("Unknown data required: %d.\n", data);
   return interface::IF_ERROR;
}

class port_interface : public interface
{
   const uint16_t _bitsize;
   const uint64_t _size;
   const bptr_t _data;

 public:
   port_interface(uint8_t idx, void* data, uint16_t bitsize)
       : interface(idx),
         _data(reinterpret_cast<bptr_t>(data)),
         _bitsize(bitsize),
         _size(bitsize / 8 + ((bitsize % 8) ? 1 : 0))
   {
      if_debug("Port interface for " BPTR_FORMAT " %u bits.\n", bptr_to_int(_data), _bitsize);
   }

   int read(bptr_t data, uint16_t bitsize, ptr_t /*addr*/, bool /*shift*/) override
   {
      assert(bitsize == _bitsize && "Bitsize mismatch");
      memcpy(data, _data, _size);
      return 1;
   }

   int write(bptr_t data, uint16_t bitsize, ptr_t /*addr*/, bool /*shift*/) override
   {
      assert(bitsize == _bitsize && "Bitsize mismatch");
      memcpy(_data, data, _size);
      return 1;
   }
};

void __m_interface_port(uint8_t idx, void* bits, uint16_t bitsize)
{
   __m_interface_set(idx, new port_interface(idx, bits, bitsize));
}

void __m_interface_ptr(uint8_t idx, bptr_t* bits, uint16_t bitsize)
{
   const bptr_t addr = *bits;
   const ptr_t dst = __m_mapper->mapaddr(addr);
   if(dst)
   {
      info("Pointer parameter " BPTR_FORMAT " mapped at " PTR_FORMAT "\n", bptr_to_int(addr), dst);
      assert((bitsize == PTR_SIZE || dst < (UINT64_MAX >> (64 - PTR_SIZE))) && "Pointer value overflow.");
      *bits = ptr_to_bptr(dst);
      return __m_interface_set(idx, new port_interface(idx, bits, bitsize));
   }
   error("Unknown parameter %u address mapping for " BPTR_FORMAT "\n", idx, bptr_to_int(addr));
   // TODO: report to simulator
   exit(EXIT_FAILURE);
}

class array_interface : public interface
{
   const bptr_t _base;
   const uint16_t _bitsize;
   const uint8_t _align;
   const uint16_t _esize;
   const uint64_t _size;

 public:
   array_interface(uint8_t idx, void* data, uint16_t bitsize, uint8_t align, uint64_t size)
       : interface(idx),
         _base(reinterpret_cast<bptr_t>(data)),
         _bitsize(bitsize),
         _align(align),
         _esize(bitsize / 8 + ((bitsize % 8 ? 1 : 0))),
         _size(size)
   {
      if_debug("Array interface for " BPTR_FORMAT ", %llu elements of %u bits aligned at %u bytes.\n",
               bptr_to_int(_base), _size, _bitsize, _align);
   }

   int read(bptr_t data, uint16_t DEBUG_PARAM(bitsize), ptr_t addr, bool /*shift*/) override
   {
      assert(bitsize == _bitsize && "Bitsize mismatch");
      if(addr >= _size)
      {
         if_error("Access out of bounds: " PTR_FORMAT " > %llu\n", addr, _size);
         return IF_ERROR;
      }
      memcpy(data, _base + (_align * addr), _esize);
      return IF_OK;
   }

   int write(bptr_t data, uint16_t DEBUG_PARAM(bitsize), ptr_t addr, bool /*shift*/) override
   {
      assert(bitsize == _bitsize && "Bitsize mismatch");
      if(addr >= _size)
      {
         if_error("Access out of bounds: " PTR_FORMAT " > %llu\n", addr, _size);
         return IF_ERROR;
      }
      memcpy(_base + (_align * addr), data, _esize);
      return IF_OK;
   }
};

void __m_interface_array(uint8_t idx, void* base, uint16_t bitsize, uint8_t align, uint64_t size)
{
   __m_interface_set(idx, new array_interface(idx, base, bitsize, align, size));
}

class fifo_interface : public interface
{
   bptr_t _base;
   const bptr_t _end;
   const uint16_t _bitsize;
   const uint8_t _align;
   const uint16_t _esize;

   inline int _size()
   {
      return std::distance(_base, _end) / _align;
   }

 public:
   fifo_interface(uint8_t idx, void* data, uint16_t bitsize, uint8_t align, uint64_t size)
       : interface(idx),
         _base(reinterpret_cast<bptr_t>(data)),
         _end(_base + (align * size)),
         _bitsize(bitsize),
         _align(align),
         _esize(bitsize / 8 + ((bitsize % 8 ? 1 : 0)))
   {
      if_debug("FIFO interface for " BPTR_FORMAT ", %llu elements of %u bits aligned at %u bytes.\n",
               bptr_to_int(_base), size, _bitsize, _align);
   }

   int read(bptr_t data, uint16_t DEBUG_PARAM(bitsize), ptr_t /*addr*/, bool shift) override
   {
      assert(bitsize == _bitsize && "Bitsize mismatch");
      if(_base == _end)
      {
         if(shift)
         {
            if_error("Read on empty FIFO.\n");
            return IF_EMPTY;
         }
         return 0;
      }
      if(shift)
      {
         _base += _align;
         if_debug("Item pop (%u left).\n", _size());
      }
      memcpy(data, _base, _esize);
      return _size();
   }

   int write(bptr_t data, uint16_t DEBUG_PARAM(bitsize), ptr_t /*addr*/, bool shift) override
   {
      assert(bitsize == _bitsize && "Bitsize mismatch");
      if(_base == _end)
      {
         if_error("Write on full FIFO.\n");
         return IF_FULL;
      }
      memcpy(_base, data, _esize);
      if(shift)
      {
         _base += _align;
         if_debug("Item push (%u free).\n", _size());
      }
      return _size();
   }

   int state(int data) override
   {
      if(data == MDPI_OP_TYPE_IF_READ || data == MDPI_OP_TYPE_IF_WRITE)
      {
         return _size();
      }
      return interface::state(data);
   }
};

void __m_interface_fifo(uint8_t idx, void* base, uint16_t bitsize, uint8_t align, uint64_t size)
{
   __m_interface_set(idx, new fifo_interface(idx, base, bitsize, align, size));
}

class mem_interface : public interface
{
 public:
   mem_interface(uint8_t idx) : interface(idx)
   {
      if_debug("Memory interface.\n");
   }

   int read(bptr_t data, uint16_t bitsize, ptr_t addr, bool /*shift*/) override
   {
      assert((bitsize % 8) == 0 && "Expected byte-aligned memory address");
      bptr_t __addr = __m_mapper->addrmap(addr);
      if(__addr)
      {
         if_debug("Read %u bytes at " PTR_FORMAT "->" BPTR_FORMAT "\n", bitsize / 8, addr, bptr_to_int(__addr));
         memcpy(data, __addr, bitsize / 8);
      }
      else
      {
         if_error("Read to non-mapped address " PTR_FORMAT ".\n", addr);
         return IF_ERROR;
      }
      return IF_OK;
   }

   int write(bptr_t data, uint16_t bitsize, ptr_t addr, bool /*shift*/) override
   {
      bptr_t __addr = __m_mapper->addrmap(addr);
      if(__addr)
      {
         if_debug("Write %u bits at " PTR_FORMAT "->" BPTR_FORMAT "\n", bitsize, addr, bptr_to_int(__addr));
         auto floor_bytes = bitsize / 8;
         memcpy(__addr, data, floor_bytes);
         auto spare = bitsize % 8;
         if(spare)
         {
            byte_t mask = 0xFF << spare;
            __addr[floor_bytes] = (__addr[floor_bytes] & mask) | (data[floor_bytes] & ~mask);
         }
      }
      else
      {
         if_error("Write to non-mapped address " PTR_FORMAT ".\n", addr);
         return IF_ERROR;
      }
      return IF_OK;
   }
};

void __m_interface_mem(uint8_t idx)
{
   __m_interface_set(idx, new mem_interface(idx));
}

static FORCE_INLINE void __ipc_exit(mdpi_state_t state, uint8_t retval)
{
   __ipc_exit(MDPI_IPC_STATE_RESPONSE, state, retval);
}

static void __ipc_abort()
{
   __ipc_exit(MDPI_STATE_ABORT, EXIT_FAILURE);
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
      error("Abrupt exception: %s\n", strsignal(__sig));
      if(__m_sim_pid)
      {
         pid_t sim_pid = __m_sim_pid;
         __m_sim_pid = 0;
         debug("Sending abort report to simulation process.\n");
         __ipc_exit(MDPI_STATE_ABORT, EXIT_FAILURE);
         sleep(2);
         do
         {
            info("Killing simulation process (PID: %d).\n", sim_pid);
            if(kill(sim_pid, SIGTERM) == -1)
            {
               if(errno == EAGAIN || errno == EINTR)
                  continue;
               error("Unable to kill simulation process (PID: %d).\n", sim_pid);
               perror("kill failed");
            }
            break;
         } while(1);
         if(wait(NULL) == -1)
         {
            error("Error waiting for simulation process.\n");
            perror("wait failed");
         }
      }
   }
   fflush(stdout);
#if __M_OUT_LVL < 4
   fflush(stderr);
#endif
   exit(EXIT_FAILURE);
}

void __m_exit(int __status)
{
   info("Exit called with value %d\n", __status);
   __ipc_exit(MDPI_STATE_END, __status);
   _exit(__status);
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
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wwrite-strings"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
   char* sim_argv[4] = {"bash", "-c", NULL, NULL};
#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif

   debug("Loading MDPI library...\n");

   __ipc_init(__LOCAL_ENTITY);

   for(i = 0; i < (sizeof(__sigs) / sizeof(*__sigs)); ++i)
   {
      signal(__sigs[i], __m_sig_handler);
   }

   sim_argv[2] = getenv(__M_IPC_SIM_CMD_ENV);
   if(sim_argv[2] && strlen(sim_argv[2]))
   {
      fflush(stdout);
      __m_sim_pid = fork();
      if(__m_sim_pid == -1)
      {
         error("Error forking simulation process.\n");
         perror("fork");
         exit(EXIT_FAILURE);
      }
      else if(!__m_sim_pid)
      {
         // Unset LD_PRELOAD to prevent the new process from loading mdpi_driver.so
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
   }
   else
   {
      info("Simulation command line found empty, expecting simulator to be launched by others.\n");
   }

   __ipc_init1();

   debug("Loading completed.\n");
}

void __attribute__((destructor)) __mdpi_driver_fini()
{
   __ipc_exit(MDPI_STATE_END, EXIT_SUCCESS);
   __ipc_fini(__LOCAL_ENTITY);
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
   __ipc_wait(MDPI_IPC_STATE_REQUEST);
   assert(__m_ipc_operation.type == MDPI_OP_TYPE_STATE_CHANGE && "Unexpected simulator request.");
   assert(__m_ipc_operation.payload.sc.state == MDPI_STATE_READY && "Unexpected simulator state.");
   __m_ipc_operation.payload.sc.state = MDPI_STATE_SETUP;
   __ipc_response();
   debug("Simulator state: %s (%u)\n", mdpi_state_str(__m_ipc_operation.payload.sc.state),
         __m_ipc_operation.payload.sc.retval);
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

   assert(__m_ipc_operation.type == MDPI_OP_TYPE_STATE_CHANGE && "Unexpected simulator request.");
   retval = __m_ipc_operation.payload.sc.retval;
   debug("Simulator state: %s (%u)\n", mdpi_state_str(__m_ipc_operation.payload.sc.state), retval);
   return retval;
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

static void* __m_driver_loop(void*)
{
   debug("IPC thread started.\n");

   while(true)
   {
      __ipc_wait(MDPI_IPC_STATE_REQUEST);
      switch(__m_ipc_operation.type)
      {
         case MDPI_OP_TYPE_STATE_CHANGE:
            return NULL;
            break;
         case MDPI_OP_TYPE_IF_READ:
         case MDPI_OP_TYPE_IF_WRITE:
         case MDPI_OP_TYPE_IF_POP:
         case MDPI_OP_TYPE_IF_PUSH:
         case MDPI_OP_TYPE_IF_INFO:
            if(__m_interfaces.empty())
            {
               error("Operation on uninitialized interfaces' list.\n");
               __m_ipc_operation.payload.interface.id = MDPI_IF_IDX_EMPTY;
            }
            else if(__m_interfaces.size() <= __m_ipc_operation.payload.interface.id)
            {
               error("Interface id out of bounds: %u.\n", __m_ipc_operation.payload.interface.id);
               __m_ipc_operation.payload.interface.id = MDPI_IF_IDX_OUT_OF_BOUNDS;
            }
            else
            {
               debug("Interface %u operation: ", __m_ipc_operation.payload.interface.id);
               if(__m_ipc_operation.type & MDPI_OP_TYPE_IF_READ)
               {
                  debug_append("read %u bits at " PTR_FORMAT ".\n", __m_ipc_operation.payload.interface.bitsize,
                               __m_ipc_operation.payload.interface.addr);
                  __m_ipc_operation.payload.interface.info =
                      __m_interfaces.at(__m_ipc_operation.payload.interface.id)
                          ->read(__m_ipc_operation.payload.interface.buffer,
                                 __m_ipc_operation.payload.interface.bitsize, __m_ipc_operation.payload.interface.addr,
                                 (__m_ipc_operation.type & MDPI_OP_TYPE_IF_POP) == MDPI_OP_TYPE_IF_POP);
               }
               else if(__m_ipc_operation.type & MDPI_OP_TYPE_IF_WRITE)
               {
                  debug_append("write %u bits at " PTR_FORMAT ".\n", __m_ipc_operation.payload.interface.bitsize,
                               __m_ipc_operation.payload.interface.addr);
                  __m_ipc_operation.payload.interface.info =
                      __m_interfaces.at(__m_ipc_operation.payload.interface.id)
                          ->write(__m_ipc_operation.payload.interface.buffer,
                                  __m_ipc_operation.payload.interface.bitsize, __m_ipc_operation.payload.interface.addr,
                                  (__m_ipc_operation.type & MDPI_OP_TYPE_IF_PUSH) == MDPI_OP_TYPE_IF_PUSH);
               }
               else
               {
                  debug_append("state (data: %u).\n", __m_ipc_operation.payload.interface.info);
                  __m_ipc_operation.payload.interface.info = __m_interfaces.at(__m_ipc_operation.payload.interface.id)
                                                                 ->state(__m_ipc_operation.payload.interface.info);
               }
            }
            __ipc_response();
            break;
         case MDPI_OP_TYPE_IF_EXIT:
            assert(__m_interfaces.size() == 1 && "Expected single interface on builtin exit/abort.");
            __m_interfaces.at(0)->write(reinterpret_cast<bptr_t>(&__m_ipc_operation.payload.interface.info),
                                        sizeof(__m_ipc_operation.payload.interface.info) * 8, 0, 0);
            __m_ipc_operation.payload.interface.id = MDPI_IF_IDX_EMPTY;
            __m_ipc_operation.payload.interface.info = 0;
            __ipc_response();
            return NULL;
         case MDPI_OP_TYPE_NONE:
         default:
            error("Unexpected transaction type: %u\n", __m_ipc_operation.type);
            __ipc_abort();
            break;
      }
   }
   debug("IPC thread completed.\n");
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
