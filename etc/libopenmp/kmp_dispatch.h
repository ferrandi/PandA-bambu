/*! \file */
/*
 * kmp_dispatch.h: dynamic scheduling - iteration initialization and dispatch.
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

#ifndef KMP_DISPATCH_H
#define KMP_DISPATCH_H

#include "kmp.h"
template <typename T>
struct dispatch_private_infoXX_template
{
   typedef typename traits_t<T>::unsigned_t UT;
   typedef typename traits_t<T>::signed_t ST;
   T lb;
   T ub;
   ST st; // signed
   UT tc; // unsigned

   T parm1;
   T parm2;
   T parm3;
   T parm4;

   UT count; // unsigned

   UT ordered_lower; // unsigned
   UT ordered_upper; // unsigned
};

template <typename T>
struct dispatch_private_info_template
{
   // duplicate alignment here, otherwise size of structure is not correct in our
   // compiler
   union private_info_tmpl
   {
      dispatch_private_infoXX_template<T> p;
   } u;
   enum sched_type schedule; /* scheduling algorithm */
   kmp_sched_flags_t flags;  /* flags (e.g., ordered, nomerge, etc.) */
   kmp_uint32 ordered_bumped;
   // to retain the structure size after making order
   // kmp_int32 ordered_dummy[KMP_MAX_ORDERED - 3];
   // dispatch_private_info *next; /* stack of buffers for nest of serial regions */
   kmp_uint32 type_size;
   // enum cons_type pushed_ws;
};

template <typename T>
struct dispatch_shared_infoXX_template
{
   typedef typename traits_t<T>::unsigned_t UT;
   /* chunk index under dynamic, number of idle threads under static-steal;
     iteration index otherwise */
   volatile UT iteration;
   volatile UT num_done;
   volatile UT ordered_iteration;
   // to retain the structure size making ordered_iteration scalar
   // UT ordered_dummy[KMP_MAX_ORDERED - 3];
};

// replaces dispatch_shared_info structure and dispatch_shared_info_t type
template <typename T>
struct dispatch_shared_info_template
{
   typedef typename traits_t<T>::unsigned_t UT;
   // we need union here to keep the structure size
   union shared_info_tmpl
   {
      dispatch_shared_infoXX_template<UT> s;
   } u;
   volatile kmp_uint32 buffer_index;
   volatile kmp_int32 doacross_buf_idx; // teamwise index
   kmp_uint32* doacross_flags;          // array of iteration flags (0/1)
   kmp_int32 doacross_num_done;         // count finished threads
};

template <typename T>
kmp_uint32 __kmp_ge(T value, T checker)
{
   return value >= checker;
}
template <typename T>
kmp_uint32 __kmp_eq(T value, T checker)
{
   return value == checker;
}

/*
    Spin wait loop that pauses between checks.
    Waits until function returns non-zero when called with *spinner and check.
    Does NOT put threads to sleep.
    Arguments:
        UT is unsigned 4- or 8-byte type
        spinner - memory location to check value
        checker - value which spinner is >, <, ==, etc.
        pred - predicate function to perform binary comparison of some sort
#if USE_ITT_BUILD
        obj -- is higher-level synchronization object to report to ittnotify. It
        is used to report locks consistently. For example, if lock is acquired
        immediately, its address is reported to ittnotify via
        KMP_FSYNC_ACQUIRED(). However, it lock cannot be acquired immediately
        and lock routine calls to KMP_WAIT(), the later should report the
        same address, not an address of low-level spinner.
#endif // USE_ITT_BUILD
    TODO: make inline function (move to header file for icl)
*/
template <typename UT>
static UT __kmp_wait(volatile UT* spinner, UT checker, kmp_uint32 (*pred)(UT, UT))
{
   // note: we may not belong to a team at this point
   volatile UT* spin = spinner;
   UT check = checker;
   kmp_uint32 spins;
   kmp_uint32 (*f)(UT, UT) = pred;
   UT r;

   KMP_FSYNC_SPIN_INIT(obj, CCAST(UT*, spin));
   KMP_INIT_YIELD(spins);
   // main wait spin loop
   while(!f(r = *spin, check))
   {
      KMP_FSYNC_SPIN_PREPARE(obj);
      /* GEH - remove this since it was accidentally introduced when kmp_wait was
       split.
       It causes problems with infinite recursion because of exit lock */
      /* if ( TCR_4(__kmp_global.g.g_done) && __kmp_global.g.g_abort)
        __kmp_abort_thread(); */
      // If oversubscribed, or have waited a bit then yield.
      KMP_YIELD_OVERSUB_ELSE_SPIN(spins);
   }
   KMP_FSYNC_SPIN_ACQUIRED(obj);
   return r;
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

template <typename UT>
void __kmp_dispatch_deo(int* gtid_ref, int* cid_ref, ident_t* loc_ref)
{
   dispatch_private_info_template<UT>* pr;

   int gtid = *gtid_ref;
   //    int  cid = *cid_ref;
   kmp_info_t* th = __kmp_threads[gtid];

   if(!th->th.th_team->t.t_serialized)
   {
      dispatch_shared_info_template<UT>* sh =
          reinterpret_cast<dispatch_shared_info_template<UT>*>(th->th.th_dispatch->th_dispatch_sh_current);
      UT lower;

      lower = pr->u.p.ordered_lower;

      __kmp_wait<UT>(&sh->u.s.ordered_iteration, lower, __kmp_ge<UT>);
   }
}

template <typename UT>
void __kmp_dispatch_dxo(int* gtid_ref, int* cid_ref, ident_t* loc_ref)
{
   typedef typename traits_t<UT>::signed_t ST;
   dispatch_private_info_template<UT>* pr;

   int gtid = *gtid_ref;
   //    int  cid = *cid_ref;
   kmp_info_t* th = __kmp_threads[gtid];

   if(!th->th.th_team->t.t_serialized)
   {
      dispatch_shared_info_template<UT>* sh =
          reinterpret_cast<dispatch_shared_info_template<UT>*>(th->th.th_dispatch->th_dispatch_sh_current);

      KMP_FSYNC_RELEASING(CCAST(UT*, &sh->u.s.ordered_iteration));

      pr->ordered_bumped += 1;

      /* TODO use general release procedure? */
      test_then_inc<ST>((volatile ST*)&sh->u.s.ordered_iteration);
   }
}

/* Computes and returns x to the power of y, where y must a non-negative integer
 */
template <typename UT>
static __forceinline long double __kmp_pow(long double x, UT y)
{
   long double s = 1.0L;

   // KMP_DEBUG_ASSERT(y >= 0); // y is unsigned
   while(y)
   {
      if(y & 1)
         s *= x;
      x *= x;
      y >>= 1;
   }
   return s;
}

/* Computes and returns the number of unassigned iterations after idx chunks
   have been assigned
   (the total number of unassigned iterations in chunks with index greater than
   or equal to idx).
   __forceinline seems to be broken so that if we __forceinline this function,
   the behavior is wrong
   (one of the unit tests, sch_guided_analytical_basic.cpp, fails)
*/
template <typename T>
static __inline typename traits_t<T>::unsigned_t
__kmp_dispatch_guided_remaining(T tc, typename traits_t<T>::floating_t base, typename traits_t<T>::unsigned_t idx)
{
   /* Note: On Windows* OS on IA-32 architecture and Intel(R) 64, at
     least for ICL 8.1, long double arithmetic may not really have
     long double precision, even with /Qlong_double.  Currently, we
     workaround that in the caller code, by manipulating the FPCW for
     Windows* OS on IA-32 architecture.  The lack of precision is not
     expected to be a correctness issue, though.
  */
   typedef typename traits_t<T>::unsigned_t UT;

   long double x = tc * __kmp_pow<UT>(base, idx);
   UT r = (UT)x;
   if(x == r)
      return r;
   return r + 1;
}

// Parameters of the guided-iterative algorithm:
//   p2 = n * nproc * ( chunk + 1 )  // point of switching to dynamic
//   p3 = 1 / ( n * nproc )          // remaining iterations multiplier
// by default n = 2. For example with n = 3 the chunks distribution will be more
// flat.
// With n = 1 first chunk is the same as for static schedule, e.g. trip / nproc.
static const int guided_int_param = 2;
static const double guided_flt_param = 0.5; // = 1.0 / guided_int_param;

#endif // KMP_DISPATCH_H
