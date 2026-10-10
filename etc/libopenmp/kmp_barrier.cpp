/*! \file */
/*
 * kmp_barrier.cpp
 */
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/////////////////////////////////////////////////////////////////////////////////////////////
//    bambu customized OpenMP runtime library
//
//    Author Fabrizio Ferrandi - Politecnico di Milano
//
//    November 6, 2020.
//    The original library has been taken from https://github.com/llvm/llvm-project/tree/master/openmp
//    For the additional notes check the associated README.md.
/////////////////////////////////////////////////////////////////////////////////////////////

#include "kmp.h"
#include <stdlib.h>
// Linear Barrier
__attribute__((always_inline)) static bool
__kmp_linear_barrier_gather_template(int gtid, int tid, void (*reduce)(void*, void*), void* reduce_data)
{
   // We now perform a linear reduction to signal that all of the threads have
   // arrived.
   if(!KMP_MASTER_TID(tid))
   {
      // not master thread
      // Mark arrival to master thread and wait for its arrival
      KMP_BARRIER_REACHED(tid);
      KMP_WAIT_ALL_THREADS();
   }
   else if(reduce == NULL)
   {
      KMP_BARRIER_REACHED(0);
      KMP_WAIT_ALL_THREADS();
   }
   else
   {
      int nproc = KMP_T_NPROC();
      int i;

      // Collect all the worker team member threads.
      KMP_BARRIER_REACHED(0);
      KMP_WAIT_ALL_THREADS();

#pragma nounroll
      for(i = 1; i < nproc; ++i)
      {
         (*reduce)(reduce_data, KMP_GET_REDUCE_DATA(i));
      }
   }
   return false;
}

__attribute__((always_inline)) static void __kmp_linear_barrier_gather(int gtid, int tid, void (*reduce)(void*, void*),
                                                                       void* reduce_data)
{
   __kmp_linear_barrier_gather_template(gtid, tid, reduce, reduce_data);
}

// Tree barrier
__attribute__((always_inline)) static void __kmp_tree_barrier_gather(enum barrier_type bt, int gtid, int tid,
                                                                     void (*reduce)(void*, void*), void* reduce_data)
{
   kmp_uint32 nproc = KMP_T_NPROC();
   kmp_uint32 branch_bits = __kmp_barrier_gather_branch_bits[bt];
   kmp_uint32 branch_factor = 1 << branch_bits;
   kmp_uint32 child_tid;

   // Perform tree gather to wait until all threads have arrived; reduce any
   // required data as we go
   child_tid = (tid << branch_bits) + 1;
   if(child_tid < nproc)
   {
      kmp_uint32 child;
      // Parent threads wait for all their children to arrive
      child = 1;
      do
      {
         // Wait for child to arrive
         // KMP_WAIT_THREAD(child_tid); Needed but currently not implemented TO DO rewrite with atomic
         if(reduce)
         {
            (*reduce)(reduce_data, KMP_GET_REDUCE_DATA(child_tid));
         }
         child++;
         child_tid++;
      } while(child <= branch_factor && child_tid < nproc);
   }
   KMP_BARRIER_REACHED(tid);
}

// Returns 0 if master thread, 1 if worker thread.
__attribute__((always_inline)) int __kmp_barrier(enum barrier_type bt, int gtid, int is_split, size_t reduce_size,
                                                 void* reduce_data, void (*reduce)(void*, void*))
{
   int tid = __kmp_tid_from_gtid(gtid);
   int status = 0;

   if(!__kmp_bambu_t_serialized())
   {
      /*    if (__kmp_tasking_mode == tskm_extra_barrier) {
            __kmp_tasking_barrier(team, this_thr, gtid);
          }
      */

      // we are managing reduce data outside
      if(reduce != NULL)
      {
         KMP_SET_REDUCE_DATA(tid, reduce_data);
      }

      // if (KMP_MASTER_TID(tid) /*&& __kmp_tasking_mode != tskm_immediate_exec*/)
      // use 0 to only setup the current team if nthreads > 1
      //__kmp_task_team_setup(this_thr, team, 0);

      if(reduce == NULL)
      {
         __kmp_linear_barrier_gather(gtid, tid, reduce, reduce_data);
      }
      else
      {
         switch(__kmp_barrier_gather_pattern[bt])
         {
            case bp_hyper_bar:
            {
               // don't set branch bits to 0; use linear
               abort(); //|        __kmp_hyper_barrier_gather(bt, gtid, tid, reduce );
               break;
            }
            case bp_hierarchical_bar:
            {
               abort(); //|        __kmp_hierarchical_barrier_gather( bt, gtid, tid, reduce );
               break;
            }
            case bp_tree_bar:
            {
               // don't set branch bits to 0; use linear
               __kmp_tree_barrier_gather(bt, gtid, tid, reduce, reduce_data);
               break;
            }
            default:
            {
               __kmp_linear_barrier_gather(gtid, tid, reduce, reduce_data);
            }
         }
      }

      if(KMP_MASTER_TID(tid))
      {
         // With the contexts switching the barrier_reset and the wait_all_threads must be atomic
         // as thread 0 could be restarted after other threads had already reached the barrier
         // The only way to do it is to force the reset to happen when the wait_all signal is given,
         // and so the barrier_reset component is not necessary anymore.

         status = 1;
      }
      else
      { // Team is serialized.
         status = 0;
      }
   }

   return status;
}
