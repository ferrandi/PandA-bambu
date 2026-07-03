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

#ifndef __MDPI_IPC_SIG_H
#define __MDPI_IPC_SIG_H

#define __USE_FILE_OFFSET64
#define _FILE_OFFSET_BITS 64

#include <features.h>

#ifndef __cplusplus
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#else
#include <atomic>
#include <cassert>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#define _Atomic(X) std::atomic<X>
#endif
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#ifndef __M_IPC_BACKEND_SIGNO
#define __M_IPC_BACKEND_SIGNO SIGUSR1
#endif
#ifndef __M_IPC_BACKEND_SIG_TIMEOUT
#define __M_IPC_BACKEND_SIG_TIMEOUT 1
#endif

typedef struct
{
   _Atomic(mdpi_ipc_state_t) handle;
   _Atomic(pid_t) proc[MDPI_ENTITY_COUNT];
   mdpi_op_t operation;
} __attribute__((aligned(8))) mdpi_ipc_file_t;

static mdpi_ipc_file_t* __m_ipc_file = NULL;

#define __m_ipc_remote_pid (__m_ipc_file->proc[1 - __BAMBU_IPC_ENTITY])
#define __m_ipc_operation (__m_ipc_file->operation)

static void __ipc_wait(mdpi_ipc_state_t state)
{
   static struct timespec tv = {__M_IPC_BACKEND_SIG_TIMEOUT, 0};
   int retval;
   sigset_t sset;

   sigemptyset(&sset);
   sigaddset(&sset, __M_IPC_BACKEND_SIGNO);

   while(atomic_load(&__m_ipc_file->handle) != state)
   {
      if(sigtimedwait(&sset, NULL, &tv) == -1)
      {
         if(errno != EAGAIN && errno != EINTR)
         {
            error("Unable to wait for signal.\n");
            perror("sigtimedwait failed");
            abort();
         }
      }
   }
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

#define __ipc_notify()                                                           \
   do                                                                            \
   {                                                                             \
      pid_t __remote_pid = atomic_load(&__m_ipc_remote_pid);                     \
      if(__remote_pid <= 0)                                                       \
      {                                                                          \
         mdpi_ipc_state_t __ipc_state = atomic_load(&__m_ipc_file->handle);      \
         warn("Skipping IPC signal to invalid remote PID %d "                    \
              "(local PID: %d, IPC state: %d, operation type: %d).\n",           \
              __remote_pid, getpid(), __ipc_state, __m_ipc_operation.type);      \
         break;                                                                  \
      }                                                                          \
      do                                                                         \
      {                                                                          \
         if(kill(__remote_pid, __M_IPC_BACKEND_SIGNO) == -1)                     \
         {                                                                       \
            if(errno == EAGAIN || errno == EINTR)                                \
               continue;                                                         \
            error("Failed to signal remote process (PID: %d).\n", __remote_pid); \
            perror("kill failed");                                               \
            abort();                                                             \
         }                                                                       \
         break;                                                                  \
      } while(1);                                                                \
   } while(0)

static void __ipc_request()
{
#ifndef NDEBUG
   mdpi_ipc_state_t expected = MDPI_IPC_STATE_LOCKED;
   atomic_compare_exchange_strong(&__m_ipc_file->handle, &expected, MDPI_IPC_STATE_REQUEST);
   assert(expected == MDPI_IPC_STATE_LOCKED && "Illegal IPC commit operation.");
#else
   atomic_store(&__m_ipc_file->handle, MDPI_IPC_STATE_REQUEST);
#endif
   __ipc_notify();
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
   __ipc_notify();
}

static void __ipc_release()
{
   atomic_store(&__m_ipc_file->handle, MDPI_IPC_STATE_FREE);
   __ipc_notify();
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
   __ipc_notify();
}

#undef __ipc_notify

static void __ipc_init(mdpi_entity_t init)
{
   int ipc_descriptor, i;
   sigset_t sset, old_sset;

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

   if(init == MDPI_ENTITY_DRIVER)
   {
      atomic_store(&__m_ipc_file->handle, MDPI_IPC_STATE_FREE);
      for(i = 0; i < MDPI_ENTITY_COUNT; ++i)
      {
         atomic_store(&__m_ipc_file->proc[i], 0);
      }
      mdpi_op_init(&__m_ipc_operation);
   }
   debug("Set signal block mask for \"%s\".\n", strsignal(__M_IPC_BACKEND_SIGNO));
   sigemptyset(&sset);
   memset(&old_sset, 0, sizeof(sigset_t));
   sigaddset(&sset, __M_IPC_BACKEND_SIGNO);
   if(sigprocmask(SIG_BLOCK, &sset, &old_sset))
   {
      error("An error occurred while setting process signal mask.\n");
      perror("IPC signal initialization error");
      abort();
   }
   if(sigismember(&old_sset, __M_IPC_BACKEND_SIGNO))
   {
      warn("Signal \"%s\" was already blocked.\n", strsignal(__M_IPC_BACKEND_SIGNO));
   }

   atomic_store(&__m_ipc_file->proc[init], getpid());

   close(ipc_descriptor);
   debug("IPC file memory-mapping completed.\n");
}

static void __ipc_init1()
{
   debug("Waiting to sync with simulator process.\n");
   while(!atomic_load(&__m_ipc_remote_pid))
      ;
   debug("Sync completed.\n");
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

#endif // __MDPI_IPC_SIG_H
