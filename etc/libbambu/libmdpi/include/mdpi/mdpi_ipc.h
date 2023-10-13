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
 * @file mdpi_ipc.h
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef __MDPI_IPC_H
#define __MDPI_IPC_H

#ifndef __M_IPC_FILENAME
#define __M_IPC_FILENAME "/tmp/panda_ipc_mmap"
#endif

#define __USE_FILE_OFFSET64
#define _FILE_OFFSET_BITS 64

#define IPC_STRUCT_ATTR __attribute__((aligned(8), packed))

#include "mdpi_debug.h"
#include "mdpi_types.h"

#ifndef __cplusplus
#include <assert.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#else
#include <atomic>
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#define _Atomic(X) std::atomic<X>
#endif
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

// clang-format off
typedef enum
{
   MDPI_STATE_UNDEFINED = 0,
   MDPI_STATE_READY     = 1,
   MDPI_STATE_SETUP     = 1 << 1,
   MDPI_STATE_RUNNING   = 1 << 2,
   MDPI_STATE_END       = 1 << 3,
   MDPI_STATE_ERROR     = 1 << 4,
   MDPI_STATE_ABORT     = 1 << 5,
} mdpi_state_t;
// clang-format on

#define mdpi_state_str(s)               \
   s == MDPI_STATE_READY ?              \
       "READY" :                        \
       (s == MDPI_STATE_SETUP ?         \
            "SETUP" :                   \
            (s == MDPI_STATE_RUNNING ?  \
                 "RUNNING" :            \
                 (s == MDPI_STATE_END ? \
                      "END" :           \
                      (s == MDPI_STATE_ERROR ? "ERROR" : (s == MDPI_STATE_ABORT ? "ABORT" : "UNDEFINED")))))

typedef enum
{
   MDPI_IPC_STATE_FREE = 0,
   MDPI_IPC_STATE_WRITING,
   MDPI_IPC_STATE_REQUEST,
   MDPI_IPC_STATE_DONE
} mdpi_ipc_state_t;

typedef enum
{
   MDPI_OP_TYPE_NONE = 0,
   MDPI_OP_TYPE_STATE_CHANGE = 1,
   MDPI_OP_TYPE_MEM_READ,
   MDPI_OP_TYPE_MEM_WRITE,
   MDPI_OP_TYPE_ARG_READ,
   MDPI_OP_TYPE_ARG_WRITE,
   MDPI_OP_TYPE_PARAM_INFO
} mdpi_op_type_t;

typedef struct
{
   ptr_t addr;
   uint16_t size; // Size in bytes
   byte_t buffer[4096];
} IPC_STRUCT_ATTR mdpi_op_mem_t;

typedef struct
{
   uint8_t index;
   uint16_t bitsize;
   byte_t buffer[4096];
} IPC_STRUCT_ATTR mdpi_op_arg_t;

typedef struct
{
   uint8_t index;
   uint64_t size;
} IPC_STRUCT_ATTR mdpi_op_param_t;

typedef struct
{
   mdpi_state_t state;
   uint8_t retval;
} IPC_STRUCT_ATTR mdpi_op_state_change_t;

typedef struct
{
   _Atomic(mdpi_ipc_state_t) handle;
   mdpi_op_type_t type;
   union
   {
      mdpi_op_state_change_t sc;
      mdpi_op_param_t param;
      mdpi_op_mem_t mem;
      mdpi_op_arg_t arg;
   } payload;
} IPC_STRUCT_ATTR mdpi_ipc_op_t;

typedef struct
{
   mdpi_ipc_op_t operation[MDPI_ENTITY_COUNT];
} IPC_STRUCT_ATTR mdpi_ipc_file_t;

static mdpi_ipc_file_t* __m_ipc_file = NULL;

#define __get_operation(entity) (__m_ipc_file->operation[entity])

static void __ipc_wait(mdpi_entity_t entity, mdpi_ipc_state_t state)
{
   while(atomic_load(&__get_operation(entity).handle) != state)
      ;
}

static void __ipc_reserve(mdpi_entity_t entity)
{
   mdpi_ipc_state_t expected;
   do
   {
      expected = MDPI_IPC_STATE_FREE;
      __ipc_wait(entity, expected);
   } while(!atomic_compare_exchange_strong(&__get_operation(entity).handle, &expected, MDPI_IPC_STATE_WRITING));
}

static void __ipc_commit(mdpi_entity_t entity)
{
#ifndef NDEBUG
   mdpi_ipc_state_t expected = MDPI_IPC_STATE_WRITING;
   atomic_compare_exchange_strong(&__get_operation(entity).handle, &expected, MDPI_IPC_STATE_REQUEST);
   assert(expected == MDPI_IPC_STATE_WRITING && "Illegal IPC commit operation.");
#else
   atomic_store(&__get_operation(entity).handle, MDPI_IPC_STATE_REQUEST);
#endif
}

static void __ipc_complete(mdpi_entity_t entity)
{
#ifndef NDEBUG
   mdpi_ipc_state_t expected = MDPI_IPC_STATE_REQUEST;
   atomic_compare_exchange_strong(&__get_operation(entity).handle, &expected, MDPI_IPC_STATE_DONE);
   assert(expected == MDPI_IPC_STATE_REQUEST && "Illegal IPC complete operation.");
#else
   atomic_store(&__get_operation(entity).handle, MDPI_IPC_STATE_DONE);
#endif
}

static void __ipc_release(mdpi_entity_t entity)
{
   atomic_store(&__get_operation(entity).handle, MDPI_IPC_STATE_FREE);
}

static void __ipc_exit(mdpi_entity_t entity, mdpi_ipc_state_t ipc_state, mdpi_state_t state, uint8_t retval)
{
   mdpi_ipc_state_t expected;
   do
   {
      expected = atomic_load(&__get_operation(entity).handle);
      if(expected == MDPI_IPC_STATE_WRITING)
         continue;
   } while(!atomic_compare_exchange_strong(&__get_operation(entity).handle, &expected, MDPI_IPC_STATE_WRITING));
   __get_operation(entity).type = MDPI_OP_TYPE_STATE_CHANGE;
   __get_operation(entity).payload.sc.state = state;
   __get_operation(entity).payload.sc.retval = retval;
   atomic_store(&__get_operation(entity).handle, ipc_state);
}

static void __ipc_init()
{
   unsigned init = 1;
   int ipc_descriptor;

   if(__m_ipc_file != NULL)
   {
      debug("IPC file memory-mapping already initialized.");
      return;
   }

   debug("IPC memory mapping on file %s\n", __M_IPC_FILENAME);
   ipc_descriptor = open(__M_IPC_FILENAME, O_RDWR | O_CREAT | O_EXCL, 0664);
   if(ipc_descriptor < 0 && errno == EEXIST)
   {
      init = 0;
      debug("IPC file exists already.\n");
      ipc_descriptor = open(__M_IPC_FILENAME, O_RDWR | O_CREAT, 0664);
   }
   else if(ipc_descriptor < 0)
   {
      error("Error opening IPC file: %s\n", __M_IPC_FILENAME);
      perror("MDPI library initialization error");
      exit(EXIT_FAILURE);
   }

   if(init)
   {
      // Ensure that the file will hold enough space
      if(ftruncate(ipc_descriptor, sizeof(mdpi_ipc_file_t)) == -1)
      {
         error("Error writing IPC file: %s\n", __M_IPC_FILENAME);
         perror("MDPI library initialization error");
         exit(EXIT_FAILURE);
      }
   }

   __m_ipc_file =
       (mdpi_ipc_file_t*)mmap(NULL, sizeof(mdpi_ipc_file_t), PROT_READ | PROT_WRITE, MAP_SHARED, ipc_descriptor, 0);

   if(__m_ipc_file == MAP_FAILED)
   {
      error("An error occurred while mapping IPC address range.\n");
      perror("MDPI library initialization error");
      exit(EXIT_FAILURE);
   }
   debug("IPC file memory-mapping completed.\n");

   close(ipc_descriptor);
}

static void __ipc_fini()
{
   if(munmap(__m_ipc_file, sizeof(mdpi_ipc_file_t)))
   {
      error("An error occurred while unmapping IPC address range.\n");
      perror("MDPI library finalization error");
   }

   // TODO: remove ipc file
}

#endif // __MDPI_IPC_H