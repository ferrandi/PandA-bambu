//    Copyright (C) 2020-2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//    This file is part of the PandA/Bambu OpenMP Library.
//
//    author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
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
#ifndef KMP_BAMBU_H
#define KMP_BAMBU_H

#include "kmp_os.h"

#include "kmp_bambu_names.h"

#ifdef __cplusplus
extern "C"
{
#endif
   // get a thread id from the global thread id
   extern int KMP_GET_TID_FROM_GTID(int gtid) __attribute__((pure));
   // get a global thread id from bambu context switch scheduler
   extern int KMP_CS_GET_GTID() __attribute__((pure));
   // get a thread id from bambu context switch scheduler
   extern int KMP_CS_GET_TID() __attribute__((pure));
   // get the master thread id from the global thread id
   extern kmp_uint32 __kmp_bambu_t_master_tid(int global_tid);
   // set the thread local reduce_data pointer to thread tid
   extern void KMP_SET_REDUCE_DATA(char tid, void* reduce_data);
   // get the thread local reduce_data pointer from thread tid
   extern void* KMP_GET_REDUCE_DATA(char tid);
   // the thread reached the barrier
   extern void KMP_BARRIER_REACHED(int tid);
   // initialize/reset the barrier
   extern void KMP_WAIT_ALL_THREADS();
   // begin critical section
   extern void KMP_CRITICAL(kmp_critical_name* loc);
   // end critical section
   extern void KMP_END_CRITICAL(kmp_critical_name* loc);
   // number of threads in team
   extern kmp_uint32 KMP_T_NPROC() __attribute__((pure));
   // is not an active worksharing construct? //controlled by
   // __kmpc_serialized_parallel,__kmpc_end_serialized_parallel,__kmpc_fork_call
   inline int __kmp_bambu_t_serialized()
   {
      return KMP_T_NPROC() == 1;
   }
   // set the number of thread for gtid
   extern void KMP_TH_SET_NPROC(int gtid, int num_threads);

   /*!
   @ingroup PARALLEL
   @param argc  total number of arguments in the ellipsis
   @param microtask  pointer to callback routine consisting of outlined parallel
   construct
   @param ...  pointers to shared variables that aren't global
   Do the actual fork and call the microtask in the relevant number of threads.
   */
   extern void KMP_FORK_CALL(kmp_int32 nargs, kmpc_micro microtask, ...);

/// we assume that
/// parameter %0 of lambda function .omp_outlined.xx represents the address of gtid
/// parameter %1 of lambda function .omp_outlined.xx represents the address of tid
/// all parameter >= %2 are the parameter passed to __kmpc_fork_call
#ifdef __cplusplus
}
#endif
#endif /* KMP_BAMBU_H */
