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
#define __REMOTE_ENTITY MDPI_ENTITY_DRIVER
#define __remote_operation __get_operation(__REMOTE_ENTITY)

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

void __attribute__((constructor)) m_init()
{
   debug("Initializing...\n");

   __ipc_init();

   atomic_store(&__remote_operation.handle, MDPI_IPC_STATE_FREE);

   debug("Initialization successful\n");
}

int m_fini()
{
   int retval;

   assert(__remote_operation.type == MDPI_OP_TYPE_STATE_CHANGE && "Unexpected cosim end state.");
   retval = ((uint16_t)(__remote_operation.payload.sc.retval) << 8) | (__remote_operation.payload.sc.state & 0xFF);

   __ipc_fini();

   debug("Finalization successful\n");
   return retval;
}

unsigned int m_next(unsigned int state)
{
   mdpi_state_t state_next = MDPI_STATE_UNDEFINED;

   debug("Current state: %s\n", mdpi_state_str((mdpi_state_t)(state)));
   switch(state)
   {
      case MDPI_STATE_READY:
         do
         {
            __ipc_reserve(__REMOTE_ENTITY);
            __remote_operation.type = MDPI_OP_TYPE_STATE_CHANGE;
            __remote_operation.payload.sc.state = (mdpi_state_t)(state);
            __ipc_commit(__REMOTE_ENTITY);
            debug("Next state required\n");
            __ipc_wait(__REMOTE_ENTITY, MDPI_IPC_STATE_DONE);
            state_next = __remote_operation.payload.sc.state;
            __ipc_release(__REMOTE_ENTITY);
         } while(state_next == state);
         break;
      default:
         error("Unexpected state received from simulator: %s (%u)\n", mdpi_state_str((mdpi_state_t)(state)), state);
         exit(EXIT_FAILURE);
         break;
   }
   if(state_next == MDPI_STATE_ERROR || state_next == MDPI_STATE_ABORT)
   {
      state_next = MDPI_STATE_END;
   }

   assert((state_next == MDPI_STATE_READY || state_next == MDPI_STATE_END) && "Unexpected state required.");

   return state_next;
}

unsigned int m_getptrargsize(unsigned int index)
{
   uint16_t bitsize;
   __ipc_reserve(__REMOTE_ENTITY);
   __remote_operation.type = MDPI_OP_TYPE_ARG_SIZE;
   __remote_operation.payload.arg.index = index;
   __ipc_commit(__REMOTE_ENTITY);
   __ipc_wait(__REMOTE_ENTITY, MDPI_IPC_STATE_DONE);
   if(__remote_operation.payload.arg.index != index)
   {
      __ipc_release(__REMOTE_ENTITY);
      error("Parameter %u size read failed.\n", index);
      abort();
   }
   bitsize = __remote_operation.payload.arg.bitsize;
   __ipc_release(__REMOTE_ENTITY);
   return bitsize;
}

void m_getarg(svLogicVecVal* data, unsigned int index)
{
   uint16_t bitsize, byte_count, i;
   debug("Parameter %u read\n", index);
   __ipc_reserve(__REMOTE_ENTITY);
   __remote_operation.type = MDPI_OP_TYPE_ARG_READ;
   __remote_operation.payload.arg.index = index;
   __ipc_commit(__REMOTE_ENTITY);
   __ipc_wait(__REMOTE_ENTITY, MDPI_IPC_STATE_DONE);
   if(__remote_operation.payload.arg.index != index)
   {
      __ipc_release(__REMOTE_ENTITY);
      error("Parameter %u read failed.\n", index);
      abort();
   }

   bitsize = __remote_operation.payload.arg.bitsize;
   byte_count = (bitsize / 8) + ((bitsize % 8) != 0);
   for(i = 0; i < byte_count; ++i)
   {
      uint8_t mem = __remote_operation.payload.arg.buffer[i];
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

   __ipc_release(__REMOTE_ENTITY);
}

void m_setarg(CONSTARG svLogicVecVal* data, unsigned int index)
{
   uint16_t bitsize, byte_count, i;

   debug("Parameter %u write\n", index);

   bitsize = m_getptrargsize(index);

   __ipc_reserve(__REMOTE_ENTITY);
   __remote_operation.type = MDPI_OP_TYPE_ARG_WRITE;
   __remote_operation.payload.arg.index = index;
   __remote_operation.payload.arg.bitsize = bitsize;
   byte_count = (bitsize / 8) + ((bitsize % 8) != 0);
   for(i = 0; i < byte_count; ++i)
   {
      assert((data[i / 4].bval == 0) && "Memory write data must not contain undefined states X or Z from "
                                        "the simulation");
      __remote_operation.payload.arg.buffer[i] = data[i / 4].aval >> byte_offset(i);
   }
   __ipc_commit(__REMOTE_ENTITY);
   __ipc_wait(__REMOTE_ENTITY, MDPI_IPC_STATE_DONE);
   if(__remote_operation.payload.arg.index != index)
   {
      error("Parameter %u write failed.\n", index);
      abort();
   }
   __ipc_release(__REMOTE_ENTITY);
}

static void __attribute__((noinline)) __m_read(const uint16_t size, svLogicVecVal* data, ptr_t addr)
{
   __ipc_reserve(__REMOTE_ENTITY);
   __remote_operation.type = MDPI_OP_TYPE_MEM_READ;
   __remote_operation.payload.mem.addr = addr;
   __remote_operation.payload.mem.size = size;
   __ipc_commit(__REMOTE_ENTITY);
   __ipc_wait(__REMOTE_ENTITY, MDPI_IPC_STATE_DONE);

   if(__remote_operation.payload.mem.addr == addr)
   {
#pragma unroll(4)
      for(uint16_t i = 0; i < size; ++i)
      {
         byte_t mem = __remote_operation.payload.mem.buffer[i];
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
   else
   {
      error("Read to non-mapped address " PTR_FORMAT ".\n", addr);
      abort();
   }
   __ipc_release(__REMOTE_ENTITY);
}

static void __attribute__((noinline))
__m_write(const uint16_t max_bsize, uint16_t size, CONSTARG svLogicVecVal* data, ptr_t addr)
{
   __ipc_reserve(__REMOTE_ENTITY);
   __remote_operation.type = MDPI_OP_TYPE_MEM_WRITE;
   __remote_operation.payload.mem.addr = addr;
   __remote_operation.payload.mem.size = size;
   const uint16_t bsize = (size / 8) + ((size % 8) != 0);
   assert((max_bsize * 8) >= size && "Memory write bitsize must be smaller than bus size");
#pragma unroll(4)
   for(uint16_t i = 0; i < bsize; ++i)
   {
#ifndef NDEBUG
      byte_t bdata_byte = data[i / 4].bval >> byte_offset(i);
      if(size >= (i * 8))
      {
         assert((bdata_byte == 0) && "Memory write data must not contain undefined states X or Z from "
                                     "the simulation");
      }
      else
      {
         byte_t mask = (byte_t)((1 << (size & 7)) - 1);
         assert(((bdata_byte & mask) == 0) && "Memory write data must not contain undefined states X or Z from "
                                              "the simulation");
      }
#endif
      __remote_operation.payload.mem.buffer[i] = data[i / 4].aval >> byte_offset(i);
   }
   __ipc_commit(__REMOTE_ENTITY);
   __ipc_wait(__REMOTE_ENTITY, MDPI_IPC_STATE_DONE);

   if(__remote_operation.payload.mem.addr != addr)
   {
      error("Write to non-mapped address " PTR_FORMAT ".\n", addr);
      abort();
   }
   __ipc_release(__REMOTE_ENTITY);
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
