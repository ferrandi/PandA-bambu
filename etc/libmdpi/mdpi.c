//    Copyright (C) 2023-2026 Politecnico di Milano
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

#include <mdpi/mdpi.h>

#define __BAMBU_IPC_ENTITY MDPI_ENTITY_SIM

#include <mdpi/mdpi_debug.h>

#if __M_OUT_LVL <= 4
#define NDEBUG
#endif

#include <mdpi/mdpi_ipc.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define byte_offset(i) ((i & 3) << 3)

#define terminate_if_not(op_type)                                                                         \
   if(__m_ipc_operation.type != (op_type))                                                                \
   {                                                                                                      \
      error("Exeception occurred on the host: %s\n", mdpi_state_str(__m_ipc_operation.payload.sc.state)); \
      __fini_trigger = 0;                                                                                 \
      __ipc_fini(__BAMBU_IPC_ENTITY);                                                                     \
      exit(EXIT_FAILURE);                                                                                 \
   }

static int __fini_trigger = 1;
static clock_t __m_runtime;

static void __attribute__((constructor)) __m_init()
{
   debug("Initializing...\n");

   __m_runtime = clock();

   __ipc_init(__BAMBU_IPC_ENTITY);

   debug("Initialization successful\n");
}

static void __attribute__((destructor)) __m_fini()
{
   if(__fini_trigger)
   {
      __ipc_exit(MDPI_IPC_STATE_REQUEST, MDPI_STATE_END, EXIT_SUCCESS);
      __ipc_fini(__BAMBU_IPC_ENTITY);
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
         abort();
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

int m_read(mdpi_idx_t id, svLogicVecVal* data, uint16_t bitsize, ptr_t addr, int8_t cmd)
{
   int retval;
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
      uint16_t i, size = bitsize / 8 + ((bitsize % 8) != 0);
#pragma unroll(4)
      for(i = 0; i < size; ++i)
      {
         byte_t mem = __m_ipc_operation.payload.interface.buffer[i];
         if(i % 4)
         {
            data[i / 4].aval |= (unsigned int)(mem) << byte_offset(i);
         }
         else
         {
            data[i / 4].aval = mem;
            data[i / 4].bval = 0;
         }
      }
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
   return retval;
}

int m_write(mdpi_idx_t id, const svLogicVecVal* data, uint16_t bitsize, ptr_t addr, int8_t cmd)
{
   int retval;
   uint16_t i;
   const uint16_t bsize = (bitsize / 8) + ((bitsize % 8) != 0);
   const mdpi_op_type_t op_type = MDPI_OP_TYPE_IF_WRITE;
   __ipc_reserve();
   __m_ipc_operation.type = op_type;
   __m_ipc_operation.payload.interface.id = id;
   __m_ipc_operation.payload.interface.info = cmd;
   __m_ipc_operation.payload.interface.addr = addr;
   __m_ipc_operation.payload.interface.bitsize = bitsize;
#pragma unroll(4)
   for(i = 0; i < bsize; ++i)
   {
#ifndef NDEBUG
      byte_t bdata_byte = data[i / 4].bval >> byte_offset(i);
      if(bitsize >= (i * 8))
      {
         assert((bdata_byte == 0) && "Memory write data must not contain undefined states X or Z from "
                                     "the simulation");
      }
      else
      {
         byte_t mask = (byte_t)((1 << (bitsize & 7)) - 1);
         assert(((bdata_byte & mask) == 0) && "Memory write data must not contain undefined states X or Z from "
                                              "the simulation");
      }
#endif
      __m_ipc_operation.payload.interface.buffer[i] = data[i / 4].aval >> byte_offset(i);
   }
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

   debug("Interface %u state(%d) -> %d.\n", id, data, retval);

   return retval;
}

void m_builtin_exit(int status)
{
   debug("Emulated exit(%d)\n", status);
   __ipc_reserve();
   __m_ipc_operation.type = MDPI_OP_TYPE_IF_EXIT;
   __m_ipc_operation.payload.interface.id = MDPI_IF_IDX_EMPTY;
   __m_ipc_operation.payload.interface.info = status;
   __m_ipc_operation.payload.interface.bitsize = 32;
   __ipc_request();
   __ipc_wait(MDPI_IPC_STATE_RESPONSE);
   __ipc_release();

   m_fini();
}
