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

#define __BAMBU_IPC_ENTITY MDPI_ENTITY_SIM

#include <mdpi/mdpi_debug.h>

#if __M_OUT_LVL <= 4 && !defined(NDEBUG)
#define NDEBUG
#endif

#include <mdpi/mdpi_csim.h>
#include <mdpi/mdpi_ipc.h>
#include <mdpi/mdpi_stack.h>

#include <assert.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>

#define terminate_if_not(op_type)                                                                         \
   if(__m_ipc_operation.type != (op_type))                                                                \
   {                                                                                                      \
      error("Exeception occurred on the host: %s\n", mdpi_state_str(__m_ipc_operation.payload.sc.state)); \
      __fini_trigger = 0;                                                                                 \
      __ipc_fini(__BAMBU_IPC_ENTITY);                                                                     \
      exit(EXIT_FAILURE);                                                                                 \
   }

extern const char* __bambu_csim_design_stats_filename;
extern const char* __bambu_csim_results_filename;

static int __fini_trigger = 1;
static clock_t __m_runtime;
static int __m_io_error_report = 0;
static thread_local m_stack_t __module_stack = {.data = NULL, .size = 0, .capacity = 0};

static void __bambu_csim_delay(struct __bambu_csim_function_stats*, unsigned);

static void __m_sig_handler(int __sig)
{
   error("Abrupt exception: %s\n", strsignal(__sig));

   FILE* fd = fopen(__bambu_csim_results_filename, "r+");
   if(fd != NULL)
   {
      int is_running = fseek(fd, -1, SEEK_END) == 0 && (char)fgetc(fd) == '|';
      fseek(fd, 0, SEEK_END);
      fprintf(fd, "%s\nA", is_running ? "A" : "");
      fclose(fd);
   }
   else
   {
      perror("unable to open file");
   }

   debug("Sending abort report to host process.\n");
   __ipc_exit(MDPI_IPC_STATE_REQUEST, MDPI_STATE_ABORT, EXIT_FAILURE);
   fflush(stdout);
#if __M_OUT_LVL < 4
   fflush(stderr);
#endif
   exit(EXIT_FAILURE);
}

static void __attribute__((constructor)) __m_init()
{
   static const int __sigs[] = {SIGINT, SIGABRT, SIGSEGV};
   size_t i;
   struct sigaction csim_sa;
   debug("Initializing...\n");
   __m_runtime = clock();

   __ipc_init(__BAMBU_IPC_ENTITY);

   memset(&csim_sa, 0, sizeof(csim_sa));
   sigemptyset(&csim_sa.sa_mask);
   csim_sa.sa_handler = __m_sig_handler;

   for(i = 0; i < (sizeof(__sigs) / sizeof(*__sigs)); ++i)
   {
      struct sigaction oldsa;
      memset(&oldsa, 0, sizeof(struct sigaction));
      sigaction(__sigs[i], NULL, &oldsa);
      if(((oldsa.sa_flags & SA_SIGINFO) && oldsa.sa_sigaction) || oldsa.sa_handler)
      {
         warn("Signal handler for \"%s\" already registered.\n", strsignal(__sigs[i]));
      }
      else
      {
         sigaction(__sigs[i], &csim_sa, NULL);
         debug("Signal handler registered for \"%s\".\n", strsignal(__sigs[i]));
      }
   }

   if(m_stack_init(&__module_stack, 8))
   {
      error("Unable to initialize CSIM module stack.\n");
      exit(EXIT_FAILURE);
   }

   if(__bambu_csim_design_stats_filename)
   {
      remove(__bambu_csim_design_stats_filename);
   }
   assert(__bambu_csim_results_filename && "CSim results filename always expected.");
   remove(__bambu_csim_results_filename);

   debug("Initialization successful\n");
}

static void __attribute__((destructor)) __m_fini()
{
   if(__fini_trigger)
   {
      __ipc_exit(MDPI_IPC_STATE_REQUEST, MDPI_STATE_END, __m_io_error_report ? EXIT_SUCCESS : EXIT_FAILURE);
      __ipc_fini(__BAMBU_IPC_ENTITY);
      m_stack_fini(&__module_stack);
      debug("Finalization successful\n");
      info("Runtime: %.3f\n", (double)(clock() - __m_runtime) / CLOCKS_PER_SEC);
   }
}

int m_fini()
{
   int retval;

   // NOTE: here it is "safe" to read from __m_ipc_operation without lock since this should be the last communication
   assert(__m_ipc_operation.type == MDPI_OP_TYPE_STATE_CHANGE && "Unexpected cosim end state.");
   retval = ((uint16_t)(__m_ipc_operation.payload.sc.retval) << 8) | (__m_ipc_operation.payload.sc.state & 0xFF);

   __fini_trigger = 0;
   __ipc_fini(__BAMBU_IPC_ENTITY);
   m_stack_fini(&__module_stack);

   debug("Finalization successful\n");
   info("Runtime: %.3f\n", (double)(clock() - __m_runtime) / CLOCKS_PER_SEC);
   return retval;
}

unsigned int m_next(unsigned int state)
{
   mdpi_state_t state_next = MDPI_STATE_UNDEFINED;
   uint16_t retval = 0;

   debug("Current state: %s\n", mdpi_state_str((mdpi_state_t)(state)));
   switch(state)
   {
      case MDPI_STATE_READY:
         do
         {
            __ipc_reserve();
            __m_ipc_operation.type = MDPI_OP_TYPE_STATE_CHANGE;
            __m_ipc_operation.payload.sc.state = (mdpi_state_t)(state);
            __ipc_request();
            debug("Next state required\n");
            __ipc_wait(MDPI_IPC_STATE_RESPONSE);
            state_next = __m_ipc_operation.payload.sc.state;
            retval = __m_ipc_operation.payload.sc.retval;
            __ipc_release();
         } while(state_next == state);
         break;
      default:
         error("Unexpected state received from simulator: %s (%u)\n", mdpi_state_str((mdpi_state_t)(state)), state);
         exit(EXIT_FAILURE);
         break;
   }
   debug("Next state: %s\n", mdpi_state_str((mdpi_state_t)(state_next)));
   if(state_next == MDPI_STATE_ERROR)
   {
      state_next = MDPI_STATE_END;
   }

   assert((state_next == MDPI_STATE_SETUP || state_next == MDPI_STATE_END || state_next == MDPI_STATE_ABORT) &&
          "Unexpected state required.");

   return (retval << 8) | (state_next & 0xFF);
}

int m_read(mdpi_idx_t id, byte_t data[], uint16_t bitsize, ptr_t addr, int8_t cmd)
{
   int retval;
   struct __bambu_csim_function_stats* s;
   const mdpi_op_type_t op_type = MDPI_OP_TYPE_IF_READ;
   __ipc_reserve();
   __m_ipc_operation.type = op_type;
   __m_ipc_operation.payload.interface.id = id;
   __m_ipc_operation.payload.interface.info = cmd;
   __m_ipc_operation.payload.interface.addr = addr;
   __m_ipc_operation.payload.interface.bitsize = bitsize;
   __ipc_request();
   __ipc_wait(MDPI_IPC_STATE_RESPONSE);

   terminate_if_not(op_type);

   retval = __m_ipc_operation.payload.interface.info;
   debug("Interface %u read state -> %d.\n", id, retval);

   if(__m_ipc_operation.payload.interface.id == id)
   {
      size_t size = bitsize / 8 + ((bitsize % 8) != 0);
      memcpy(data, __m_ipc_operation.payload.interface.buffer, size);
   }
   else if(__m_ipc_operation.payload.interface.id == MDPI_IF_IDX_EMPTY)
   {
      debug("Fake pipelined read operation on interface %u.\n", id);
   }
   else
   {
      error("Read operation on uninitialized interface %u.\n", id);
      abort();
   }
   __ipc_release();
   if(retval < 0)
   {
      __m_io_error_report = EXIT_FAILURE;
   }
   if(!m_stack_front(&__module_stack, (void**)&s))
   {
      __bambu_csim_delay(s, __bambu_artificial_delay_read[id]);
   }
   return retval;
}

int m_write(mdpi_idx_t id, const byte_t data[], uint16_t bitsize, ptr_t addr, int8_t cmd)
{
   int retval;
   struct __bambu_csim_function_stats* s;
   const size_t bsize = (bitsize / 8) + ((bitsize % 8) != 0);
   const mdpi_op_type_t op_type = MDPI_OP_TYPE_IF_WRITE;
   __ipc_reserve();
   __m_ipc_operation.type = op_type;
   __m_ipc_operation.payload.interface.id = id;
   __m_ipc_operation.payload.interface.info = cmd;
   __m_ipc_operation.payload.interface.addr = addr;
   __m_ipc_operation.payload.interface.bitsize = bitsize;
   memcpy(__m_ipc_operation.payload.interface.buffer, data, bsize);
   __ipc_request();
   __ipc_wait(MDPI_IPC_STATE_RESPONSE);

   terminate_if_not(op_type);

   retval = __m_ipc_operation.payload.interface.info;
   debug("Interface %u write state -> %d.\n", id, retval);

   if(__m_ipc_operation.payload.interface.id == MDPI_IF_IDX_EMPTY)
   {
      debug("Fake pipelined write operation on interface %u.\n", id);
   }
   else if(__m_ipc_operation.payload.interface.id != id)
   {
      error("Write operation on uninitialized interface %u.\n", id);
      abort();
   }
   __ipc_release();
   if(retval < 0)
   {
      __m_io_error_report = EXIT_FAILURE;
   }
   if(!m_stack_front(&__module_stack, (void**)&s))
   {
      __bambu_csim_delay(s, __bambu_artificial_delay_write[id]);
   }
   return retval;
}

int m_state(mdpi_idx_t id, int data)
{
   int retval;
   __ipc_reserve();
   __m_ipc_operation.type = MDPI_OP_TYPE_IF_INFO;
   __m_ipc_operation.payload.interface.id = id;
   __m_ipc_operation.payload.interface.info = data;
   __ipc_request();
   __ipc_wait(MDPI_IPC_STATE_RESPONSE);

   terminate_if_not(MDPI_OP_TYPE_IF_INFO);

   retval = __m_ipc_operation.payload.interface.info;

   if(__m_ipc_operation.payload.interface.id == MDPI_IF_IDX_EMPTY)
   {
      debug("Fake state operation on interface %u.\n", id);
   }
   else if(__m_ipc_operation.payload.interface.id != id)
   {
      error("State operation on uninitialized interface %u.\n", id);
      abort();
   }
   __ipc_release();
   if(retval < 0)
   {
      __m_io_error_report = EXIT_FAILURE;
   }

   debug("Interface %u state(%d) -> %d.\n", id, data, retval);

   return retval;
}

static void __bambu_csim_bb_stats_reset(struct __bambu_csim_bb_stats* _s)
{
   _s->exec = 0;
}

static void __bambu_csim_function_stats_reset(struct __bambu_csim_function_stats* _s)
{
   unsigned i;
   for(i = 0; i < _s->bb_exec_size; ++i)
   {
      __bambu_csim_bb_stats_reset(&_s->bb_exec[i]);
   }
   _s->last_cycle = 0;
}

static int __bambu_csim_function_stats_cmp(const void* a, const void* b)
{
   const struct __bambu_csim_function_stats* _a = (struct __bambu_csim_function_stats*)a;
   const struct __bambu_csim_function_stats* _b = (struct __bambu_csim_function_stats*)b;
   return _a->id - _b->id;
}

void __bambu_csim_design_stats_trace(struct __bambu_csim_function_stats* _s, unsigned bbi, int csteps)
{
   unsigned long long delay = (long long)csteps;
   if(!csteps)
   {
      if(bbi >= _s->bb_exec_size)
      {
         error("BB%u not found in function %u.\n", bbi, _s->id);
         return exit(EXIT_FAILURE);
      }
      _s->bb_exec[bbi].exec += 1;
      delay = _s->bb_exec[bbi].csteps;
   }
   debug("Trace %u:BB%u:%llu\n", _s->id, bbi, delay);
   _s->last_cycle += delay;
}

static void __bambu_csim_delay(struct __bambu_csim_function_stats* _s, unsigned csteps)
{
   debug("Add %u cycles delay\n", csteps);
   _s->last_cycle += csteps;
}

struct __bambu_csim_function_stats* __bambu_csim_get_stats(struct __bambu_csim_design_stats* _s, unsigned fid)
{
   struct __bambu_csim_function_stats key, *it;
   key.id = fid;
   it = (struct __bambu_csim_function_stats*)bsearch(&key, _s->functions, _s->functions_size,
                                                     sizeof(struct __bambu_csim_function_stats),
                                                     __bambu_csim_function_stats_cmp);
   if(it == NULL)
   {
      error("Required function id not found: %u\n", fid);
      exit(EXIT_FAILURE);
   }
   return it;
}

void __bambu_csim_design_stats_rotate(struct __bambu_csim_design_stats* _s, const char* _prof_filename)
{
   if(_prof_filename)
   {
      FILE* fd;
      unsigned i, j;
      fd = fopen(_prof_filename, "a");
      if(fd == NULL)
      {
         perror("unable to open file");
         exit(EXIT_FAILURE);
      }
      fprintf(fd, "RUN %u\n", _s->run_id);
      for(i = 0; i < _s->functions_size; ++i)
      {
         struct __bambu_csim_function_stats* fstats = &_s->functions[i];
         fprintf(fd, "FUNC %u", fstats->id);
         for(j = 0; j < fstats->bb_exec_size; ++j)
         {
            fprintf(fd, " %llu", fstats->bb_exec[j].exec);
         }
         fprintf(fd, "\n");
         __bambu_csim_function_stats_reset(fstats);
      }
      fclose(fd);
   }
   _s->run_id += 1;
}

void __bambu_csim_results_append(const char* _res_filename, const char* format, ...)
{
   va_list va;
   FILE* fd = fopen(_res_filename, "a");
   if(fd == NULL)
   {
      perror("unable to open file");
      exit(EXIT_FAILURE);
   }
   va_start(va, format);
   vfprintf(fd, format, va);
   va_end(va);
   fclose(fd);
}

void __bambu_csim_module_push(struct __bambu_csim_function_stats* _called, int cstep_fix)
{
   struct __bambu_csim_function_stats* current;
   unsigned long long last_cycle = cstep_fix;
   if(!m_stack_front(&__module_stack, (void**)&current))
   {
      last_cycle += current->last_cycle;
      debug("%u -> %u at cycle %llu.\n", current->id, _called->id, last_cycle);
   }
   _called->last_cycle = last_cycle;
   m_stack_push(&__module_stack, _called);
}

void __bambu_csim_module_pop(unsigned long count)
{
   unsigned long i;
   struct __bambu_csim_function_stats *current, *prev;
   unsigned long long later_cycle = 0;
   for(i = 0; i < count; ++i)
   {
      m_stack_pop(&__module_stack, (void**)&prev);
      later_cycle = later_cycle > prev->last_cycle ? later_cycle : prev->last_cycle;
   }
   debug("Pop %lu modules at cycle %llu.\n", count, later_cycle);
   if(!m_stack_is_empty(&__module_stack))
   {
      m_stack_front(&__module_stack, (void**)&current);
      debug("Update %u module last cycle: %llu -> %llu.\n", current->id, current->last_cycle, later_cycle);
      assert(current->last_cycle <= later_cycle && "Going 88 on a DeLorean?");
      current->last_cycle = later_cycle;
   }
}
