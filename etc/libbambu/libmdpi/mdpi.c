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
 * @file mdpi.c
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include <mdpi/mdpi.h>

#define __LOCAL_ENTITY MDPI_ENTITY_SIM

#include <mdpi/mdpi_debug.h>

#if __M_OUT_LVL <= 4
#define NDEBUG
#endif

#include <mdpi/mdpi_ipc.h>

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define byte_offset(i) ((i & 3) << 3)

static int __fini_trigger = 1;

void __attribute__((constructor)) __m_init()
{
   debug("Initializing...\n");

   __ipc_init(__LOCAL_ENTITY);

   debug("Initialization successful\n");
}

void __attribute__((destructor)) __m_fini()
{
   if(__fini_trigger)
   {
      __ipc_exit(MDPI_IPC_STATE_REQUEST, MDPI_STATE_END, EXIT_SUCCESS);
      __ipc_fini(__LOCAL_ENTITY);
      debug("Finalization successful\n");
   }
}

int m_fini()
{
   int retval;

   assert(__m_ipc_operation.type == MDPI_OP_TYPE_STATE_CHANGE && "Unexpected cosim end state.");
   retval = ((uint16_t)(__m_ipc_operation.payload.sc.retval) << 8) | (__m_ipc_operation.payload.sc.state & 0xFF);

   __fini_trigger = 0;
   __ipc_fini(__LOCAL_ENTITY);

   debug("Finalization successful\n");
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

int m_read(uint8_t id, svLogicVecVal* data, uint16_t bitsize, ptr_t addr, uint8_t shift)
{
   int retval;
   __ipc_reserve();
   __m_ipc_operation.type = shift ? MDPI_OP_TYPE_IF_POP : MDPI_OP_TYPE_IF_READ;
   __m_ipc_operation.payload.interface.id = id;
   __m_ipc_operation.payload.interface.info = 0;
   __m_ipc_operation.payload.interface.addr = addr;
   __m_ipc_operation.payload.interface.bitsize = bitsize;
   __ipc_request();
   __ipc_wait(MDPI_IPC_STATE_RESPONSE);

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

int m_write(uint8_t id, CONSTARG svLogicVecVal* data, uint16_t bitsize, ptr_t addr, uint8_t shift)
{
   int retval;
   uint16_t i;
   const uint16_t bsize = (bitsize / 8) + ((bitsize % 8) != 0);
   __ipc_reserve();
   __m_ipc_operation.type = shift ? MDPI_OP_TYPE_IF_PUSH : MDPI_OP_TYPE_IF_WRITE;
   __m_ipc_operation.payload.interface.id = id;
   __m_ipc_operation.payload.interface.info = 0;
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

int m_state(uint8_t id, int data)
{
   int retval;
   __ipc_reserve();
   __m_ipc_operation.type = MDPI_OP_TYPE_IF_INFO;
   __m_ipc_operation.payload.interface.id = id;
   __m_ipc_operation.payload.interface.info = data;
   __ipc_request();
   __ipc_wait(MDPI_IPC_STATE_RESPONSE);

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
   __ipc_reserve();
   __m_ipc_operation.type = MDPI_OP_TYPE_IF_EXIT;
   __m_ipc_operation.payload.interface.id = MDPI_IF_IDX_EMPTY;
   __m_ipc_operation.payload.interface.info = status;
   __m_ipc_operation.payload.interface.bitsize = 32;
   __ipc_request();
   __ipc_wait(MDPI_IPC_STATE_RESPONSE);

   debug("Interface %u exit state -> %d.\n", __m_ipc_operation.payload.interface.id,
         __m_ipc_operation.payload.interface.info);

   if(__m_ipc_operation.payload.interface.id != MDPI_IF_IDX_EMPTY)
   {
      error("Builtin exit operation failed.\n");
      abort();
   }
   __ipc_release();
}
