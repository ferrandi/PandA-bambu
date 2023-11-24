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
 * @file mdpi_ipc_sig.h
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

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

#define __m_ipc_remote_pid (__m_ipc_file->proc[1 - __LOCAL_ENTITY])
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
         error("Unable to wait for signal.\n");
         perror("sigtimedwait failed");
         abort();
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
   } while(!atomic_compare_exchange_strong(&__m_ipc_file->handle, &expected, MDPI_IPC_STATE_WRITING));
}

#define __ipc_notify()                                                        \
   do                                                                         \
   {                                                                          \
      if(kill(atomic_load(&__m_ipc_remote_pid), __M_IPC_BACKEND_SIGNO) == -1) \
      {                                                                       \
         error("Unable to send signal.\n");                                   \
         perror("kill failed");                                               \
         abort();                                                             \
      }                                                                       \
   } while(0)

static void __ipc_request()
{
#ifndef NDEBUG
   mdpi_ipc_state_t expected = MDPI_IPC_STATE_WRITING;
   atomic_compare_exchange_strong(&__m_ipc_file->handle, &expected, MDPI_IPC_STATE_REQUEST);
   assert(expected == MDPI_IPC_STATE_WRITING && "Illegal IPC commit operation.");
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
      if(expected == MDPI_IPC_STATE_WRITING)
         continue;
   } while(!atomic_compare_exchange_strong(&__m_ipc_file->handle, &expected, MDPI_IPC_STATE_WRITING));
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
   sigset_t sset;

   debug("IPC memory mapping on file " __M_IPC_FILENAME "\n");
   ipc_descriptor = open(__M_IPC_FILENAME, O_RDWR | O_CREAT, 0664);
   if(ipc_descriptor < 0)
   {
      error("Error opening IPC file: %s\n", __M_IPC_FILENAME);
      perror("MDPI library initialization error");
      abort();
   }

   if(init == MDPI_ENTITY_DRIVER)
   {
      // Ensure that the file will hold enough space
      lseek(ipc_descriptor, sizeof(mdpi_ipc_file_t), SEEK_SET);
      if(write(ipc_descriptor, "", 1) < 1)
      {
         error("Error writing IPC file: %s\n", __M_IPC_FILENAME);
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
   atomic_store(&__m_ipc_file->proc[init], getpid());
   sigemptyset(&sset);
   sigaddset(&sset, __M_IPC_BACKEND_SIGNO);
   sigprocmask(SIG_BLOCK, &sset, NULL);

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
      remove(__M_IPC_FILENAME);
   }
}

#endif // __MDPI_IPC_SIG_H