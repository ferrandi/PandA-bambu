/*! \file */
/*
 * kmp_sched.cpp -- static scheduling -- iteration initialization
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

template <typename T>
__attribute__((always_inline)) static void
__kmp_for_static_init(ident_t* loc, kmp_int32 global_tid, kmp_int32 schedtype, kmp_int32* plastiter, T* plower,
                      T* pupper, typename traits_t<T>::signed_t* pstride, typename traits_t<T>::signed_t incr,
                      typename traits_t<T>::signed_t chunk)
{
   typedef typename traits_t<T>::unsigned_t UT;
   typedef typename traits_t<T>::signed_t ST;
   /*  this all has to be changed back to TID and such.. */
   kmp_uint32 tid;
   kmp_uint32 nth;
   UT trip_count;

   /* special handling for zero-trip loops */
   if(incr > 0 ? (*pupper < *plower) : (*plower < *pupper))
   {
      if(plastiter != NULL)
         *plastiter = FALSE;
      /* leave pupper and plower set to entire iteration space */
      *pstride = incr; /* value should never be used */
                       // *plower = *pupper - incr;
                       // let compiler bypass the illegal loop (like for(i=1;i<10;i--))
                       // THE LINE COMMENTED ABOVE CAUSED shape2F/h_tests_1.f TO HAVE A FAILURE
                       // ON A ZERO-TRIP LOOP (lower=1, upper=0,stride=1) - JPH June 23, 2009.
      return;
   }

   // Although there are schedule enumerations above kmp_ord_upper which are not
   // schedules for "distribute", the only ones which are useful are dynamic, so
   // cannot be seen here, since this codepath is only executed for static
   // schedules.
   if(schedtype > kmp_ord_upper)
   {
      // we are in DISTRIBUTE construct
      schedtype += kmp_sch_static - kmp_distribute_static; // AC: convert to usual schedule type
      tid = __kmp_bambu_t_master_tid(global_tid);
   }
   else
   {
      tid = __kmp_tid_from_gtid(global_tid);
   }

   /* determine if "for" loop is an active worksharing construct */
   if(__kmp_bambu_t_serialized())
   {
      /* serialized parallel, each thread executes whole iteration space */
      if(plastiter != NULL)
         *plastiter = TRUE;
      /* leave pupper and plower set to entire iteration space */
      *pstride = (incr > 0) ? (*pupper - *plower + 1) : (-(*plower - *pupper + 1));

      return;
   }
   nth = KMP_T_NPROC();
   if(nth == 1)
   {
      if(plastiter != NULL)
         *plastiter = TRUE;
      *pstride = (incr > 0) ? (*pupper - *plower + 1) : (-(*plower - *pupper + 1));

      return;
   }

   /* compute trip count */
   if(incr == 1)
   {
      trip_count = *pupper - *plower + 1;
   }
   else if(incr == -1)
   {
      trip_count = *plower - *pupper + 1;
   }
   else if(incr > 0)
   {
      // upper-lower can exceed the limit of signed type
      trip_count = (UT)(*pupper - *plower) / incr + 1;
   }
   else
   {
      trip_count = (UT)(*plower - *pupper) / (-incr) + 1;
   }

   /* compute remaining parameters */
   switch(schedtype)
   {
      case kmp_sch_static:
      {
         if(trip_count < nth)
         {
            if(tid < trip_count)
            {
               *pupper = *plower = *plower + tid * incr;
            }
            else
            {
               *plower = *pupper + incr;
            }
            if(plastiter != NULL)
               *plastiter = (tid == trip_count - 1);
         }
         else
         {
            if(__kmp_static == kmp_sch_static_balanced)
            {
               UT small_chunk = trip_count / nth;
               UT extras = trip_count % nth;
               *plower += incr * (tid * small_chunk + (tid < extras ? tid : extras));
               *pupper = *plower + small_chunk * incr - (tid < extras ? 0 : incr);
               if(plastiter != NULL)
                  *plastiter = (tid == nth - 1);
            }
            else
            {
               T big_chunk_inc_count = (trip_count / nth + ((trip_count % nth) ? 1 : 0)) * incr;
               T old_upper = *pupper;

               // Unknown static scheduling type.

               *plower += tid * big_chunk_inc_count;
               *pupper = *plower + big_chunk_inc_count - incr;
               if(incr > 0)
               {
                  if(*pupper < *plower)
                     *pupper = traits_t<T>::max_value;
                  if(plastiter != NULL)
                     *plastiter = *plower <= old_upper && *pupper > old_upper - incr;
                  if(*pupper > old_upper)
                     *pupper = old_upper; // tracker C73258
               }
               else
               {
                  if(*pupper > *plower)
                     *pupper = traits_t<T>::min_value;
                  if(plastiter != NULL)
                     *plastiter = *plower >= old_upper && *pupper < old_upper - incr;
                  if(*pupper < old_upper)
                     *pupper = old_upper; // tracker C73258
               }
            }
         }
         *pstride = trip_count;
         break;
      }
      case kmp_sch_static_chunked:
      {
         ST span;
         if(chunk < 1)
         {
            chunk = 1;
         }
         span = chunk * incr;
         *pstride = span * nth;
         *plower = *plower + (span * tid);
         *pupper = *plower + span - incr;
         if(plastiter != NULL)
            *plastiter = (tid == ((trip_count - 1) / (UT)chunk) % nth);
         break;
      }
      case kmp_sch_static_balanced_chunked:
      {
         T old_upper = *pupper;
         // round up to make sure the chunk is enough to cover all iterations
         UT span = (trip_count + nth - 1) / nth;

         // perform chunk adjustment
         chunk = (span + chunk - 1) & ~(chunk - 1);

         span = chunk * incr;
         *plower = *plower + (span * tid);
         *pupper = *plower + span - incr;
         if(incr > 0)
         {
            if(*pupper > old_upper)
               *pupper = old_upper;
         }
         else if(*pupper < old_upper)
            *pupper = old_upper;

         if(plastiter != NULL)
            *plastiter = (tid == ((trip_count - 1) / (UT)chunk));
         break;
      }
      default:
         break;
   }

   return;
}

//------------------------------------------------------------------------------
extern "C"
{
   /*!
   @ingroup WORK_SHARING
   @param    loc       Source code location
   @param    gtid      Global thread id of this thread
   @param    schedtype  Scheduling type
   @param    plastiter Pointer to the "last iteration" flag
   @param    plower    Pointer to the lower bound
   @param    pupper    Pointer to the upper bound
   @param    pstride   Pointer to the stride
   @param    incr      Loop increment
   @param    chunk     The chunk size

   Each of the four functions here are identical apart from the argument types.

   The functions compute the upper and lower bounds and stride to be used for the
   set of iterations to be executed by the current thread from the statically
   scheduled loop that is described by the initial values of the bounds, stride,
   increment and chunk size.

   @{
   */
   __attribute__((always_inline)) void __kmpc_for_static_init_4(ident_t* loc, kmp_int32 gtid, kmp_int32 schedtype,
                                                                kmp_int32* plastiter, kmp_int32* plower,
                                                                kmp_int32* pupper, kmp_int32* pstride, kmp_int32 incr,
                                                                kmp_int32 chunk)
   {
      __kmp_for_static_init<kmp_int32>(loc, gtid, schedtype, plastiter, plower, pupper, pstride, incr, chunk);
   }

   /*!
    See @ref __kmpc_for_static_init_4
    */
   __attribute__((always_inline)) void __kmpc_for_static_init_4u(ident_t* loc, kmp_int32 gtid, kmp_int32 schedtype,
                                                                 kmp_int32* plastiter, kmp_uint32* plower,
                                                                 kmp_uint32* pupper, kmp_int32* pstride, kmp_int32 incr,
                                                                 kmp_int32 chunk)
   {
      __kmp_for_static_init<kmp_uint32>(loc, gtid, schedtype, plastiter, plower, pupper, pstride, incr, chunk);
   }

   /*!
    See @ref __kmpc_for_static_init_4
    */
   __attribute__((always_inline)) void __kmpc_for_static_init_8(ident_t* loc, kmp_int32 gtid, kmp_int32 schedtype,
                                                                kmp_int32* plastiter, kmp_int64* plower,
                                                                kmp_int64* pupper, kmp_int64* pstride, kmp_int64 incr,
                                                                kmp_int64 chunk)
   {
      __kmp_for_static_init<kmp_int64>(loc, gtid, schedtype, plastiter, plower, pupper, pstride, incr, chunk);
   }

   /*!
    See @ref __kmpc_for_static_init_4
    */
   __attribute__((always_inline)) void __kmpc_for_static_init_8u(ident_t* loc, kmp_int32 gtid, kmp_int32 schedtype,
                                                                 kmp_int32* plastiter, kmp_uint64* plower,
                                                                 kmp_uint64* pupper, kmp_int64* pstride, kmp_int64 incr,
                                                                 kmp_int64 chunk)
   {
      __kmp_for_static_init<kmp_uint64>(loc, gtid, schedtype, plastiter, plower, pupper, pstride, incr, chunk);
   }
   /*!
   @}
   */

} // extern "C"
