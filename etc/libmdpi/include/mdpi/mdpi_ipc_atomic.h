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

/*
 * Never include this file directly; use <mdpi/mdpi_ipc.h> instead.
 */

#ifndef __MDPI_IPC_ATOMIC_H
#define __MDPI_IPC_ATOMIC_H

#define __USE_FILE_OFFSET64
#define _FILE_OFFSET_BITS 64

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

typedef struct
{
   _Atomic(mdpi_ipc_state_t) handle;
   mdpi_op_t operation;
} __attribute__((aligned(8))) mdpi_ipc_file_t;

static mdpi_ipc_file_t* __m_ipc_file = NULL;

#define __m_ipc_operation (__m_ipc_file->operation)

static void __ipc_wait(mdpi_ipc_state_t state)
{
   while(atomic_load(&__m_ipc_file->handle) != state)
      ;
}

static void __ipc_reserve()
{
   mdpi_ipc_state_t expected;
   do
   {
      expected = MDPI_IPC_STATE_FREE;
      __ipc_wait(expected);
   } while(!atomic_compare_exchange_strong(&__m_ipc_file->handle, &expected, MDPI_IPC_STATE_LOCKED));
}

static void __ipc_request()
{
#ifndef NDEBUG
   mdpi_ipc_state_t expected = MDPI_IPC_STATE_LOCKED;
   atomic_compare_exchange_strong(&__m_ipc_file->handle, &expected, MDPI_IPC_STATE_REQUEST);
   assert(expected == MDPI_IPC_STATE_LOCKED && "Illegal IPC commit operation.");
#else
   atomic_store(&__m_ipc_file->handle, MDPI_IPC_STATE_REQUEST);
#endif
}

static void __ipc_response()
{
#ifndef NDEBUG
   mdpi_ipc_state_t expected = MDPI_IPC_STATE_REQUEST;
   atomic_compare_exchange_strong(&__m_ipc_file->handle, &expected, MDPI_IPC_STATE_RESPONSE);
   assert(expected == MDPI_IPC_STATE_REQUEST && "Illegal IPC complete operation.");
#else
   atomic_store(&__m_ipc_file->handle, MDPI_IPC_STATE_RESPONSE);
#endif
}

static void __ipc_release()
{
   atomic_store(&__m_ipc_file->handle, MDPI_IPC_STATE_FREE);
}

static void __ipc_exit(mdpi_ipc_state_t ipc_state, mdpi_state_t state, uint8_t retval)
{
   mdpi_ipc_state_t expected;
   do
   {
      expected = atomic_load(&__m_ipc_file->handle);
      if(expected == MDPI_IPC_STATE_LOCKED)
         continue;
   } while(!atomic_compare_exchange_strong(&__m_ipc_file->handle, &expected, MDPI_IPC_STATE_LOCKED));
   __m_ipc_operation.type = MDPI_OP_TYPE_STATE_CHANGE;
   __m_ipc_operation.payload.sc.state = state;
   __m_ipc_operation.payload.sc.retval = retval;
   atomic_store(&__m_ipc_file->handle, ipc_state);
}

static void __ipc_init(mdpi_entity_t init)
{
   int ipc_descriptor;

   const char* ipc_filename = __ipc_filename();

   debug("IPC memory mapping on file %s\n", ipc_filename);
   ipc_descriptor = open(ipc_filename, O_RDWR | O_CREAT, 0664);
   if(ipc_descriptor < 0)
   {
      error("Error opening IPC file: %s\n", ipc_filename);
      perror("MDPI library initialization error");
      abort();
   }

   if(init == MDPI_ENTITY_DRIVER)
   {
      // Ensure that the file will hold enough space
      lseek(ipc_descriptor, sizeof(mdpi_ipc_file_t), SEEK_SET);
      if(write(ipc_descriptor, "", 1) < 1)
      {
         error("Error writing IPC file: %s\n", ipc_filename);
         perror("MDPI library initialization error");
         abort();
      }
      lseek(ipc_descriptor, 0, SEEK_SET);
   }

   __m_ipc_file =
       (mdpi_ipc_file_t*)mmap(NULL, sizeof(mdpi_ipc_file_t), PROT_READ | PROT_WRITE, MAP_SHARED, ipc_descriptor, 0);

   if(__m_ipc_file == MAP_FAILED)
   {
      error("An error occurred while mapping IPC address range.\n");
      perror("MDPI library initialization error");
      abort();
   }
   debug("IPC file memory-mapping completed.\n");

   if(init == MDPI_ENTITY_DRIVER)
   {
      atomic_store(&__m_ipc_file->handle, MDPI_IPC_STATE_FREE);
      mdpi_op_init(&__m_ipc_operation);
   }

   close(ipc_descriptor);
}

static void __ipc_init1()
{
}

static void __ipc_fini(__attribute__((unused)) mdpi_entity_t init)
{
   if(munmap(__m_ipc_file, sizeof(mdpi_ipc_file_t)))
   {
      error("An error occurred while unmapping IPC address range.\n");
      perror("MDPI library finalization error");
   }
   if(init == MDPI_ENTITY_DRIVER)
   {
      remove(__ipc_filename());
   }
}

#endif // __MDPI_IPC_ATOMIC_H