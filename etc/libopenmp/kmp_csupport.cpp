/*! \file */
/*
 * kmp_csupport.cpp -- kfront linkage support for OpenMP.
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

/*!
@ingroup THREAD_STATES
@param loc Source location information.
@return The global thread index of the active thread.

This function can be called in any context.

If the runtime has ony been entered at the outermost level from a
single (necessarily non-OpenMP<sup>*</sup>) thread, then the thread number is
that which would be returned by omp_get_thread_num() in the outermost
active parallel construct. (Or zero if there is no active parallel
construct, since the master thread is necessarily thread zero).

If multiple non-OpenMP threads all enter an OpenMP construct then this
will be a unique thread identifier among all the threads created by
the OpenMP runtime (but the value cannot be defined in terms of
OpenMP thread ids returned by omp_get_thread_num()).
*/
__attribute__((always_inline)) kmp_int32 __kmpc_global_thread_num(ident_t* loc)
{
   kmp_int32 gtid = __kmp_entry_gtid();
   return gtid;
}

/*!
@ingroup THREAD_STATES
@param loc Source location information.
@return The number of threads in the innermost active parallel construct.
*/
__attribute__((always_inline)) kmp_int32 __kmpc_bound_num_threads(ident_t* loc)
{
   return KMP_T_NPROC();
}

/*!
@ingroup PARALLEL
@param loc source location information
@param global_tid global thread number
@param num_threads number of threads requested for this parallel construct

Set the number of threads to be used by the next fork spawned by this thread.
This call is only required if the parallel construct has a `num_threads` clause.
*/
__attribute__((always_inline)) void __kmpc_push_num_threads(ident_t* loc, kmp_int32 global_tid, kmp_int32 num_threads)
{
   __kmp_push_num_threads(loc, global_tid, num_threads);
}

/* -------------------------------------------------------------------------- */
/*!
@ingroup SYNCHRONIZATION
@param loc source location information
@param global_tid thread id.

Execute a barrier.
*/
__attribute__((always_inline)) void __kmpc_barrier(ident_t* loc, kmp_int32 global_tid)
{
   //__kmp_resume_if_soft_paused();//required in case we are going to support omp 5: omp_pause_resource

   // TODO: explicit barrier_wait_id:
   //   this function is called when 'barrier' directive is present or
   //   implicit barrier at the end of a worksharing construct.
   // 1) better to add a per-thread barrier counter to a thread data structure
   // 2) set to 0 when a new team is created
   // 4) no sync is required
   __kmp_barrier(bs_plain_barrier, global_tid, FALSE, 0, NULL, NULL);
}

__attribute__((always_inline)) void __kmpc_critical(ident_t*, kmp_int32, kmp_critical_name* loc)
{
   KMP_CRITICAL(loc);
}

__attribute__((always_inline)) void __kmpc_end_critical(ident_t*, kmp_int32, kmp_critical_name* loc)
{
   KMP_END_CRITICAL(loc);
}

__attribute__((always_inline)) void __kmpc_critical_with_hint(ident_t*, kmp_int32, kmp_critical_name* loc, unsigned int)
{
   KMP_CRITICAL(loc);
}

/*!
@ingroup WORK_SHARING
@param loc Source location
@param global_tid Global thread id

Mark the end of a statically scheduled loop.
*/
__attribute__((always_inline)) void __kmpc_for_static_fini(ident_t* loc, kmp_int32 global_tid)
{
}

/* 2.a.i. Reduce Block without a terminating barrier */
/*!
@ingroup SYNCHRONIZATION
@param loc source location information
@param global_tid global thread number
@param num_vars number of items (variables) to be reduced
@param reduce_size size of data in bytes to be reduced
@param reduce_data pointer to data to be reduced
@param reduce_func callback function providing reduction operation on two
operands and returning result of reduction in lhs_data
@param lck pointer to the unique lock data structure
@result 1 for the master thread, 0 for all other team threads, 2 for all team
threads if atomic reduction needed

The nowait version is used for a reduce clause with the nowait argument.
*/
__attribute__((always_inline)) kmp_int32 __kmpc_reduce_nowait(ident_t* loc, kmp_int32 global_tid, kmp_int32 num_vars,
                                                              size_t reduce_size, void* reduce_data,
                                                              void (*reduce_func)(void* lhs_data, void* rhs_data),
                                                              kmp_critical_name* lck)
{
   int retval = 0;
   PACKED_REDUCTION_METHOD_T packed_reduction_method;
   //  kmp_info_t *th;
   //  kmp_team_t *team;
   //  int teams_swapped = 0, task_state;

   //  __kmp_resume_if_soft_paused();

   //  th = __kmp_thread_from_gtid(global_tid);
   //  teams_swapped = __kmp_swap_teams_for_teams_reduction(th, &team, &task_state);

   packed_reduction_method =
       __kmp_determine_reduction_method(loc, global_tid, num_vars, reduce_size, reduce_data, reduce_func, lck);

   if(packed_reduction_method == empty_reduce_block)
   {
      // usage: if team size == 1, no synchronization is required
      retval = 1;
   }
   else
   {
      // AT: performance issue: a real barrier here
      // AT:     (if master goes slow, other threads are blocked here waiting for the
      // master to come and release them)
      // AT:     (it's not what a customer might expect specifying NOWAIT clause)
      // AT:     (specifying NOWAIT won't result in improvement of performance, it'll
      // be confusing to a customer)
      // AT: another implementation of *barrier_gather*nowait() (or some other design)
      // might go faster and be more in line with sense of NOWAIT
      // AT: TO DO: do epcc test and compare times

      // this barrier should be invisible to a customer and to the threading profile
      // tool (it's neither a terminating barrier nor customer's code, it's
      // used for an internal purpose)
#ifdef TREE_REDUCTION
      retval = __kmp_barrier(bp_tree_bar, global_tid, FALSE, reduce_size, reduce_data, reduce_func);
#else
      retval = __kmp_barrier(bs_plain_barrier, global_tid, FALSE, reduce_size, reduce_data, reduce_func);
#endif
   }
   //  if (teams_swapped) {
   //    __kmp_restore_swapped_teams(th, team, task_state);
   //  }

   return retval;
}

/*!
@ingroup SYNCHRONIZATION
@param loc source location information
@param global_tid global thread id.
@param lck pointer to the unique lock data structure

Finish the execution of a reduce nowait.
*/
__attribute__((always_inline)) void __kmpc_end_reduce_nowait(ident_t* loc, kmp_int32 global_tid, kmp_critical_name* lck)
{
   return;
}

/* 2.a.ii. Reduce Block with a terminating barrier */

/*!
@ingroup SYNCHRONIZATION
@param loc source location information
@param global_tid global thread number
@param num_vars number of items (variables) to be reduced
@param reduce_size size of data in bytes to be reduced
@param reduce_data pointer to data to be reduced
@param reduce_func callback function providing reduction operation on two
operands and returning result of reduction in lhs_data
@param lck pointer to the unique lock data structure
@result 1 for the master thread, 0 for all other team threads, 2 for all team
threads if atomic reduction needed

A blocking reduce that includes an implicit barrier.
        */
__attribute__((always_inline)) kmp_int32 __kmpc_reduce(ident_t* loc, kmp_int32 global_tid, kmp_int32 num_vars,
                                                       size_t reduce_size, void* reduce_data,
                                                       void (*reduce_func)(void* lhs_data, void* rhs_data),
                                                       kmp_critical_name* lck)
{
   int retval = 0;
   PACKED_REDUCTION_METHOD_T packed_reduction_method;

   // why do we need this initialization here at all?
   // Reduction clause can not be a stand-alone directive.

   // do not call __kmp_serial_initialize(), it will be called by
   // __kmp_parallel_initialize() if needed
   // possible detection of false-positive race by the threadchecker ???

   // check correctness of reduce block nesting

   packed_reduction_method =
       __kmp_determine_reduction_method(loc, global_tid, num_vars, reduce_size, reduce_data, reduce_func, lck);

   if(packed_reduction_method == empty_reduce_block)
   {
      // usage: if team size == 1, no synchronization is required ( Intel
      // platforms only )
      retval = 1;
   }
   else
   {
      // case tree_reduce_block:
      // this barrier should be visible to a customer and to the threading profile
      // tool (it's a terminating barrier on constructs if NOWAIT not specified)
#ifdef TREE_REDUCTION
      retval = __kmp_barrier(bp_tree_bar, global_tid, TRUE, reduce_size, reduce_data, reduce_func);
#else
      retval = __kmp_barrier(bs_plain_barrier, global_tid, TRUE, reduce_size, reduce_data, reduce_func);
#endif

      // all other workers except master should do this pop here
      // ( none of other workers except master will enter __kmpc_end_reduce() )
   }

   return retval;
}

/*!
@ingroup SYNCHRONIZATION
@param loc source location information
@param global_tid global thread id.
@param lck pointer to the unique lock data structure

Finish the execution of a blocking reduce.
    The <tt>lck</tt> pointer must be the same as that used in the corresponding
            start function.
                */
__attribute__((always_inline)) void __kmpc_end_reduce(ident_t* loc, kmp_int32 global_tid, kmp_critical_name* lck)
{
   return;
}
