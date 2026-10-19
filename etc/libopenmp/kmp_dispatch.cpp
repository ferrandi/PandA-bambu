/*! \file */
/*
 * kmp_dispatch.cpp: dynamic scheduling - iteration initialization and dispatch.
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

/* Dynamic scheduling initialization and dispatch.
 *
 * NOTE: __kmp_nth is a constant inside of any dispatch loop, however
 *       it may change values between parallel regions.  __kmp_max_nth
 *       is the largest value __kmp_nth may take, 1 is the smallest.
 */

#include "kmp_dispatch.h"
#include "kmp.h"

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

void __kmp_dispatch_deo_error(int* gtid_ref, int* cid_ref, ident_t* loc_ref)
{
}

void __kmp_dispatch_dxo_error(int* gtid_ref, int* cid_ref, ident_t* loc_ref)
{
}

// Returns either SCHEDULE_MONOTONIC or SCHEDULE_NONMONOTONIC
static inline int __kmp_get_monotonicity(enum sched_type schedule, bool use_hier = false)
{
   // Pick up the nonmonotonic/monotonic bits from the scheduling type
   int monotonicity;
   // default to monotonic
   monotonicity = SCHEDULE_MONOTONIC;
   if(SCHEDULE_HAS_NONMONOTONIC(schedule))
      monotonicity = SCHEDULE_NONMONOTONIC;
   else if(SCHEDULE_HAS_MONOTONIC(schedule))
      monotonicity = SCHEDULE_MONOTONIC;
   return monotonicity;
}

// Initialize a dispatch_private_info_template<T> buffer for a particular
// type of schedule,chunk.  The loop description is found in lb (lower bound),
// ub (upper bound), and st (stride).  nproc is the number of threads relevant
// to the scheduling (often the number of threads in a team, but not always if
// hierarchical scheduling is used).  tid is the id of the thread calling
// the function within the group of nproc threads.  It will have a value
// between 0 and nproc - 1.  This is often just the thread id within a team, but
// is not necessarily the case when using hierarchical scheduling.
// loc is the source file location of the corresponding loop
// gtid is the global thread id
template <typename T>
void __kmp_dispatch_init_algorithm(ident_t* loc, int gtid, dispatch_private_info_template<T>* pr,
                                   enum sched_type schedule, T lb, T ub, typename traits_t<T>::signed_t st,
                                   typename traits_t<T>::signed_t chunk, T nproc, T tid)
{
   typedef typename traits_t<T>::unsigned_t UT;
   typedef typename traits_t<T>::floating_t DBL;

   int active;
   T tc;
   kmp_info_t* th;
   kmp_team_t* team;
   int monotonicity;
   bool use_hier;

   /* setup data */
   th = __kmp_threads[gtid];
   team = th->th.th_team;
   active = !team->t.t_serialized;

   use_hier = false;

   /* Pick up the nonmonotonic/monotonic bits from the scheduling type */
   monotonicity = __kmp_get_monotonicity(schedule, use_hier);
   schedule = SCHEDULE_WITHOUT_MODIFIERS(schedule);

   /* Pick up the nomerge/ordered bits from the scheduling type */
   if((schedule >= kmp_nm_lower) && (schedule < kmp_nm_upper))
   {
      pr->flags.nomerge = TRUE;
      schedule = (enum sched_type)(((int)schedule) - (kmp_nm_lower - kmp_sch_lower));
   }
   else
   {
      pr->flags.nomerge = FALSE;
   }
   pr->type_size = traits_t<T>::type_size; // remember the size of variables
   if(kmp_ord_lower & schedule)
   {
      pr->flags.ordered = TRUE;
      schedule = (enum sched_type)(((int)schedule) - (kmp_ord_lower - kmp_sch_lower));
   }
   else
   {
      pr->flags.ordered = FALSE;
   }
   // Ordered overrides nonmonotonic
   if(pr->flags.ordered)
   {
      monotonicity = SCHEDULE_MONOTONIC;
   }

   if(schedule == kmp_sch_static)
   {
      schedule = __kmp_static;
   }
   else
   {
      if(schedule == kmp_sch_runtime)
      {
         // Use the scheduling specified by OMP_SCHEDULE (or __kmp_sch_default if
         // not specified)
         schedule = team->t.t_sched.r_sched_type;
         monotonicity = __kmp_get_monotonicity(schedule, use_hier);
         schedule = SCHEDULE_WITHOUT_MODIFIERS(schedule);
         // Detail the schedule if needed (global controls are differentiated
         // appropriately)
         if(schedule == kmp_sch_guided_chunked)
         {
            schedule = __kmp_guided;
         }
         else if(schedule == kmp_sch_static)
         {
            schedule = __kmp_static;
         }
         // Use the chunk size specified by OMP_SCHEDULE (or default if not
         // specified)
         chunk = team->t.t_sched.chunk;
      }
      else
      {
         if(schedule == kmp_sch_guided_chunked)
         {
            schedule = __kmp_guided;
         }
         if(chunk <= 0)
         {
            chunk = KMP_DEFAULT_CHUNK;
         }
      }

      if(schedule == kmp_sch_auto)
      {
         // mapping and differentiation: in the __kmp_do_serial_initialize()
         schedule = __kmp_auto;
      }
      /* guided analytical not safe for too many threads */
      if(schedule == kmp_sch_guided_analytical_chunked && nproc > 1 << 20)
      {
         schedule = kmp_sch_guided_iterative_chunked;
      }
      if(schedule == kmp_sch_runtime_simd)
      {
         // compiler provides simd_width in the chunk parameter
         schedule = team->t.t_sched.r_sched_type;
         monotonicity = __kmp_get_monotonicity(schedule, use_hier);
         schedule = SCHEDULE_WITHOUT_MODIFIERS(schedule);
         // Detail the schedule if needed (global controls are differentiated
         // appropriately)
         if(schedule == kmp_sch_static || schedule == kmp_sch_auto || schedule == __kmp_static)
         {
            schedule = kmp_sch_static_balanced_chunked;
         }
         else
         {
            if(schedule == kmp_sch_guided_chunked || schedule == __kmp_guided)
            {
               schedule = kmp_sch_guided_simd;
            }
            chunk = team->t.t_sched.chunk * chunk;
         }
      }
      pr->u.p.parm1 = chunk;
   }

   pr->u.p.count = 0;

   // compute trip count
   if(st == 1)
   { // most common case
      if(ub >= lb)
      {
         tc = ub - lb + 1;
      }
      else
      {          // ub < lb
         tc = 0; // zero-trip
      }
   }
   else if(st < 0)
   {
      if(lb >= ub)
      {
         // AC: cast to unsigned is needed for loops like (i=2B; i>-2B; i-=1B),
         // where the division needs to be unsigned regardless of the result type
         tc = (UT)(lb - ub) / (-st) + 1;
      }
      else
      {          // lb < ub
         tc = 0; // zero-trip
      }
   }
   else
   { // st > 0
      if(ub >= lb)
      {
         // AC: cast to unsigned is needed for loops like (i=-2B; i<2B; i+=1B),
         // where the division needs to be unsigned regardless of the result type
         tc = (UT)(ub - lb) / st + 1;
      }
      else
      {          // ub < lb
         tc = 0; // zero-trip
      }
   }

   pr->u.p.lb = lb;
   pr->u.p.ub = ub;
   pr->u.p.st = st;
   pr->u.p.tc = tc;

   /* NOTE: only the active parallel region(s) has active ordered sections */

   if(active)
   {
      if(pr->flags.ordered)
      {
         pr->ordered_bumped = 0;
         pr->u.p.ordered_lower = 1;
         pr->u.p.ordered_upper = 0;
      }
   }

   switch(schedule)
   {
      case kmp_sch_static_balanced:
      {
         T init, limit;

         if(nproc > 1)
         {
            T id = tid;

            if(tc < nproc)
            {
               if(id < tc)
               {
                  init = id;
                  limit = id;
                  pr->u.p.parm1 = (id == tc - 1); /* parm1 stores *plastiter */
               }
               else
               {
                  pr->u.p.count = 1; /* means no more chunks to execute */
                  pr->u.p.parm1 = FALSE;
                  break;
               }
            }
            else
            {
               T small_chunk = tc / nproc;
               T extras = tc % nproc;
               init = id * small_chunk + (id < extras ? id : extras);
               limit = init + small_chunk - (id < extras ? 0 : 1);
               pr->u.p.parm1 = (id == nproc - 1);
            }
         }
         else
         {
            if(tc > 0)
            {
               init = 0;
               limit = tc - 1;
               pr->u.p.parm1 = TRUE;
            }
            else
            {
               // zero trip count
               pr->u.p.count = 1; /* means no more chunks to execute */
               pr->u.p.parm1 = FALSE;
               break;
            }
         }
         if(st == 1)
         {
            pr->u.p.lb = lb + init;
            pr->u.p.ub = lb + limit;
         }
         else
         {
            // calculated upper bound, "ub" is user-defined upper bound
            T ub_tmp = lb + limit * st;
            pr->u.p.lb = lb + init * st;
            // adjust upper bound to "ub" if needed, so that MS lastprivate will match
            // it exactly
            if(st > 0)
            {
               pr->u.p.ub = (ub_tmp + st > ub ? ub : ub_tmp);
            }
            else
            {
               pr->u.p.ub = (ub_tmp + st < ub ? ub : ub_tmp);
            }
         }
         if(pr->flags.ordered)
         {
            pr->u.p.ordered_lower = init;
            pr->u.p.ordered_upper = limit;
         }
         break;
      } // case
      case kmp_sch_static_balanced_chunked:
      {
         // similar to balanced, but chunk adjusted to multiple of simd width
         T nth = nproc;
         schedule = kmp_sch_static_greedy;
         if(nth > 1)
            pr->u.p.parm1 = ((tc + nth - 1) / nth + chunk - 1) & ~(chunk - 1);
         else
            pr->u.p.parm1 = tc;
         break;
      } // case
      case kmp_sch_guided_simd:
      case kmp_sch_guided_iterative_chunked:
      {
         if(nproc > 1)
         {
            if((2L * chunk + 1) * nproc >= tc)
            {
               /* chunk size too large, switch to dynamic */
               schedule = kmp_sch_dynamic_chunked;
            }
            else
            {
               // when remaining iters become less than parm2 - switch to dynamic
               pr->u.p.parm2 = guided_int_param * nproc * (chunk + 1);
               *(double*)&pr->u.p.parm3 = guided_flt_param / nproc; // may occupy parm3 and parm4
            }
         }
         else
         {
            schedule = kmp_sch_static_greedy;
            /* team->t.t_nproc == 1: fall-through to kmp_sch_static_greedy */
            pr->u.p.parm1 = tc;
         } // if
      }    // case
      break;
      case kmp_sch_guided_analytical_chunked:
      {
         if(nproc > 1)
         {
            if((2L * chunk + 1) * nproc >= tc)
            {
               /* chunk size too large, switch to dynamic */
               schedule = kmp_sch_dynamic_chunked;
            }
            else
            {
               /* commonly used term: (2 nproc - 1)/(2 nproc) */
               DBL x;

               /* value used for comparison in solver for cross-over point */
               long double target = ((long double)chunk * 2 + 1) * nproc / tc;

               /* crossover point--chunk indexes equal to or greater than
           this point switch to dynamic-style scheduling */
               UT cross;

               /* commonly used term: (2 nproc - 1)/(2 nproc) */
               x = (long double)1.0 - (long double)0.5 / nproc;

               /* save the term in thread private dispatch structure */
               *(DBL*)&pr->u.p.parm3 = x;

               /* solve for the crossover point to the nearest integer i for which C_i
           <= chunk */
               {
                  UT left, right, mid;
                  long double p;

                  /* estimate initial upper and lower bound */

                  /* doesn't matter what value right is as long as it is positive, but
             it affects performance of the solver */
                  right = 229;
                  p = __kmp_pow<UT>(x, right);
                  if(p > target)
                  {
                     do
                     {
                        p *= p;
                        right <<= 1;
                     } while(p > target && right < (1 << 27));
                     /* lower bound is previous (failed) estimate of upper bound */
                     left = right >> 1;
                  }
                  else
                  {
                     left = 0;
                  }

                  /* bisection root-finding method */
                  while(left + 1 < right)
                  {
                     mid = (left + right) / 2;
                     if(__kmp_pow<UT>(x, mid) > target)
                     {
                        left = mid;
                     }
                     else
                     {
                        right = mid;
                     }
                  } // while
                  cross = right;
               }

               /* save the crossover point in thread private dispatch structure */
               pr->u.p.parm2 = cross;

// C75803
#if((KMP_OS_LINUX || KMP_OS_WINDOWS) && KMP_ARCH_X86) && (!defined(KMP_I8))
#define GUIDED_ANALYTICAL_WORKAROUND (*(DBL*)&pr->u.p.parm3)
#else
#define GUIDED_ANALYTICAL_WORKAROUND (x)
#endif
               /* dynamic-style scheduling offset */
               pr->u.p.count =
                   tc - __kmp_dispatch_guided_remaining(tc, GUIDED_ANALYTICAL_WORKAROUND, cross) - cross * chunk;
            } // if
         }
         else
         {
            schedule = kmp_sch_static_greedy;
            /* team->t.t_nproc == 1: fall-through to kmp_sch_static_greedy */
            pr->u.p.parm1 = tc;
         } // if
      }    // case
      break;
      case kmp_sch_static_greedy:
         pr->u.p.parm1 = (nproc > 1) ? (tc + nproc - 1) / nproc : tc;
         break;
      case kmp_sch_static_chunked:
      case kmp_sch_dynamic_chunked:
         if(pr->u.p.parm1 <= 0)
         {
            pr->u.p.parm1 = KMP_DEFAULT_CHUNK;
         }
         break;
      case kmp_sch_trapezoidal:
      {
         /* TSS: trapezoid self-scheduling, minimum chunk_size = parm1 */

         T parm1, parm2, parm3, parm4;

         parm1 = chunk;

         /* F : size of the first cycle */
         parm2 = (tc / (2 * nproc));

         if(parm2 < 1)
         {
            parm2 = 1;
         }

         /* L : size of the last cycle.  Make sure the last cycle is not larger
       than the first cycle. */
         if(parm1 < 1)
         {
            parm1 = 1;
         }
         else if(parm1 > parm2)
         {
            parm1 = parm2;
         }

         /* N : number of cycles */
         parm3 = (parm2 + parm1);
         parm3 = (2 * tc + parm3 - 1) / parm3;

         if(parm3 < 2)
         {
            parm3 = 2;
         }

         /* sigma : decreasing incr of the trapezoid */
         parm4 = (parm3 - 1);
         parm4 = (parm2 - parm1) / parm4;

         // pointless check, because parm4 >= 0 always
         // if ( parm4 < 0 ) {
         //    parm4 = 0;
         //}

         pr->u.p.parm1 = parm1;
         pr->u.p.parm2 = parm2;
         pr->u.p.parm3 = parm3;
         pr->u.p.parm4 = parm4;
      } // case
      break;

      default:
      {
      }
      break;
   } // switch
   pr->schedule = schedule;
}

// UT - unsigned flavor of T, ST - signed flavor of T,
// DBL - double if sizeof(T)==4, or long double if sizeof(T)==8
template <typename T>
static void __kmp_dispatch_init(ident_t* loc, int gtid, enum sched_type schedule, T lb, T ub,
                                typename traits_t<T>::signed_t st, typename traits_t<T>::signed_t chunk, int push_ws)
{
   typedef typename traits_t<T>::unsigned_t UT;

   int active;
   kmp_info_t* th;
   kmp_team_t* team;
   kmp_uint32 my_buffer_index;
   dispatch_private_info_template<T>* pr;
   dispatch_shared_info_template<T> volatile* sh;

   // if (!TCR_4(__kmp_init_parallel))
   //  __kmp_parallel_initialize();

   // __kmp_resume_if_soft_paused();

   /* setup data */
   th = __kmp_threads[gtid];
   team = th->th.th_team;
   active = !__kmp_bambu_t_serialized();
   th->th.th_ident = loc;

   if(!active)
   {
      pr = reinterpret_cast<dispatch_private_info_template<T>*>(
          th->th.th_dispatch->th_disp_buffer); /* top of the stack */
   }
   else
   {
      my_buffer_index = th->th.th_dispatch->th_disp_index++;

      /* What happens when number of threads changes, need to resize buffer? */
      pr = reinterpret_cast<dispatch_private_info_template<T>*>(
          &th->th.th_dispatch->th_disp_buffer[my_buffer_index % __kmp_dispatch_num_buffers]);
      sh = reinterpret_cast<dispatch_shared_info_template<T> volatile*>(
          &team->t.t_disp_buffer[my_buffer_index % __kmp_dispatch_num_buffers]);
   }

   __kmp_dispatch_init_algorithm(loc, gtid, pr, schedule, lb, ub, st, chunk, (T)th->th.th_team_nproc,
                                 (T)th->th.th_info.ds.ds_tid);
   if(active)
   {
      if(pr->flags.ordered == 0)
      {
         th->th.th_dispatch->th_deo_fcn = __kmp_dispatch_deo_error;
         th->th.th_dispatch->th_dxo_fcn = __kmp_dispatch_dxo_error;
      }
      else
      {
         th->th.th_dispatch->th_deo_fcn = __kmp_dispatch_deo<UT>;
         th->th.th_dispatch->th_dxo_fcn = __kmp_dispatch_dxo<UT>;
      }
   }

   if(active)
   {
      /* The name of this buffer should be my_buffer_index when it's free to use
       * it */

      __kmp_wait<kmp_uint32>(&sh->buffer_index, my_buffer_index, __kmp_eq<kmp_uint32>);
      // Note: KMP_WAIT() cannot be used there: buffer index and
      // my_buffer_index are *always* 32-bit integers.

      th->th.th_dispatch->th_dispatch_pr_current = (dispatch_private_info_t*)pr;
      th->th.th_dispatch->th_dispatch_sh_current = CCAST(dispatch_shared_info_t*, (volatile dispatch_shared_info_t*)sh);
   }
}

template <typename T>
int __kmp_dispatch_next_algorithm(int gtid, dispatch_private_info_template<T>* pr,
                                  dispatch_shared_info_template<T> volatile* sh, kmp_int32* p_last, T* p_lb, T* p_ub,
                                  typename traits_t<T>::signed_t* p_st, T nproc, T tid)
{
   typedef typename traits_t<T>::unsigned_t UT;
   typedef typename traits_t<T>::signed_t ST;
   typedef typename traits_t<T>::floating_t DBL;
   int status = 0;
   kmp_int32 last = 0;
   T start;
   ST incr;
   UT limit, trip, init;

   // zero trip count
   if(pr->u.p.tc == 0)
   {
      return 0;
   }

   switch(pr->schedule)
   {
      case kmp_sch_static_balanced:
      {
         /* check if thread has any iteration to do */
         if((status = !pr->u.p.count) != 0)
         {
            pr->u.p.count = 1;
            *p_lb = pr->u.p.lb;
            *p_ub = pr->u.p.ub;
            last = pr->u.p.parm1;
            if(p_st != NULL)
               *p_st = pr->u.p.st;
         }
         else
         { /* no iterations to do */
            pr->u.p.lb = pr->u.p.ub + pr->u.p.st;
         }
      } // case
      break;
      case kmp_sch_static_greedy: /* original code for kmp_sch_static_greedy was
                                 merged here */
      case kmp_sch_static_chunked:
      {
         T parm1;

         parm1 = pr->u.p.parm1;

         trip = pr->u.p.tc - 1;
         init = parm1 * (pr->u.p.count + tid);

         if((status = (init <= trip)) != 0)
         {
            start = pr->u.p.lb;
            incr = pr->u.p.st;
            limit = parm1 + init - 1;

            if((last = (limit >= trip)) != 0)
               limit = trip;

            if(p_st != NULL)
               *p_st = incr;

            pr->u.p.count += nproc;

            if(incr == 1)
            {
               *p_lb = start + init;
               *p_ub = start + limit;
            }
            else
            {
               *p_lb = start + init * incr;
               *p_ub = start + limit * incr;
            }

            if(pr->flags.ordered)
            {
               pr->u.p.ordered_lower = init;
               pr->u.p.ordered_upper = limit;
            } // if
         }    // if
      }       // case
      break;

      case kmp_sch_dynamic_chunked:
      {
         T chunk = pr->u.p.parm1;

         init = chunk * test_then_inc_acq<ST>((volatile ST*)&sh->u.s.iteration);
         trip = pr->u.p.tc - 1;

         if((status = (init <= trip)) == 0)
         {
            *p_lb = 0;
            *p_ub = 0;
            if(p_st != NULL)
               *p_st = 0;
         }
         else
         {
            start = pr->u.p.lb;
            limit = chunk + init - 1;
            incr = pr->u.p.st;

            if((last = (limit >= trip)) != 0)
               limit = trip;

            if(p_st != NULL)
               *p_st = incr;

            if(incr == 1)
            {
               *p_lb = start + init;
               *p_ub = start + limit;
            }
            else
            {
               *p_lb = start + init * incr;
               *p_ub = start + limit * incr;
            }

            if(pr->flags.ordered)
            {
               pr->u.p.ordered_lower = init;
               pr->u.p.ordered_upper = limit;
            } // if
         }    // if
      }       // case
      break;

      case kmp_sch_guided_iterative_chunked:
      {
         T chunkspec = pr->u.p.parm1;
         trip = pr->u.p.tc;
         // Start atomic part of calculations
         while(1)
         {
            ST remaining;             // signed, because can be < 0
            init = sh->u.s.iteration; // shared value
            remaining = trip - init;
            if(remaining <= 0)
            { // AC: need to compare with 0 first
               // nothing to do, don't try atomic op
               status = 0;
               break;
            }
            if((T)remaining < pr->u.p.parm2)
            { // compare with K*nproc*(chunk+1), K=2 by default
               // use dynamic-style schedule
               // atomically increment iterations, get old value
               init = test_then_add<ST>(RCAST(volatile ST*, &sh->u.s.iteration), (ST)chunkspec);
               remaining = trip - init;
               if(remaining <= 0)
               {
                  status = 0; // all iterations got by other threads
               }
               else
               {
                  // got some iterations to work on
                  status = 1;
                  if((T)remaining > chunkspec)
                  {
                     limit = init + chunkspec - 1;
                  }
                  else
                  {
                     last = 1; // the last chunk
                     limit = init + remaining - 1;
                  } // if
               }    // if
               break;
            }                                                          // if
            limit = init + (UT)(remaining * *(double*)&pr->u.p.parm3); // divide by K*nproc
            if(compare_and_swap<ST>(RCAST(volatile ST*, &sh->u.s.iteration), (ST)init, (ST)limit))
            {
               // CAS was successful, chunk obtained
               status = 1;
               --limit;
               break;
            } // if
         }    // while
         if(status != 0)
         {
            start = pr->u.p.lb;
            incr = pr->u.p.st;
            if(p_st != NULL)
               *p_st = incr;
            *p_lb = start + init * incr;
            *p_ub = start + limit * incr;
            if(pr->flags.ordered)
            {
               pr->u.p.ordered_lower = init;
               pr->u.p.ordered_upper = limit;
            } // if
         }
         else
         {
            *p_lb = 0;
            *p_ub = 0;
            if(p_st != NULL)
               *p_st = 0;
         } // if
      }    // case
      break;

      case kmp_sch_guided_simd:
      {
         // same as iterative but curr-chunk adjusted to be multiple of given
         // chunk
         T chunk = pr->u.p.parm1;
         trip = pr->u.p.tc;
         // Start atomic part of calculations
         while(1)
         {
            ST remaining;             // signed, because can be < 0
            init = sh->u.s.iteration; // shared value
            remaining = trip - init;
            if(remaining <= 0)
            {              // AC: need to compare with 0 first
               status = 0; // nothing to do, don't try atomic op
               break;
            }
            // compare with K*nproc*(chunk+1), K=2 by default
            if((T)remaining < pr->u.p.parm2)
            {
               // use dynamic-style schedule
               // atomically increment iterations, get old value
               init = test_then_add<ST>(RCAST(volatile ST*, &sh->u.s.iteration), (ST)chunk);
               remaining = trip - init;
               if(remaining <= 0)
               {
                  status = 0; // all iterations got by other threads
               }
               else
               {
                  // got some iterations to work on
                  status = 1;
                  if((T)remaining > chunk)
                  {
                     limit = init + chunk - 1;
                  }
                  else
                  {
                     last = 1; // the last chunk
                     limit = init + remaining - 1;
                  } // if
               }    // if
               break;
            } // if
            // divide by K*nproc
            UT span = remaining * (*(double*)&pr->u.p.parm3);
            UT rem = span % chunk;
            if(rem) // adjust so that span%chunk == 0
               span += chunk - rem;
            limit = init + span;
            if(compare_and_swap<ST>(RCAST(volatile ST*, &sh->u.s.iteration), (ST)init, (ST)limit))
            {
               // CAS was successful, chunk obtained
               status = 1;
               --limit;
               break;
            } // if
         }    // while
         if(status != 0)
         {
            start = pr->u.p.lb;
            incr = pr->u.p.st;
            if(p_st != NULL)
               *p_st = incr;
            *p_lb = start + init * incr;
            *p_ub = start + limit * incr;
            if(pr->flags.ordered)
            {
               pr->u.p.ordered_lower = init;
               pr->u.p.ordered_upper = limit;
            } // if
         }
         else
         {
            *p_lb = 0;
            *p_ub = 0;
            if(p_st != NULL)
               *p_st = 0;
         } // if
      }    // case
      break;

      case kmp_sch_guided_analytical_chunked:
      {
         T chunkspec = pr->u.p.parm1;
         UT chunkIdx;

         trip = pr->u.p.tc;

         while(1)
         { /* this while loop is a safeguard against unexpected zero
         chunk sizes */
            chunkIdx = test_then_inc_acq<ST>((volatile ST*)&sh->u.s.iteration);
            if(chunkIdx >= (UT)pr->u.p.parm2)
            {
               --trip;
               /* use dynamic-style scheduling */
               init = chunkIdx * chunkspec + pr->u.p.count;
               /* need to verify init > 0 in case of overflow in the above
                * calculation */
               if((status = (init > 0 && init <= trip)) != 0)
               {
                  limit = init + chunkspec - 1;

                  if((last = (limit >= trip)) != 0)
                     limit = trip;
               }
               break;
            }
            else
            {
               /* use exponential-style scheduling */
               /* The following check is to workaround the lack of long double precision on
                  Windows* OS.
                  This check works around the possible effect that init != 0 for chunkIdx == 0.
                */
               if(chunkIdx)
               {
                  init = __kmp_dispatch_guided_remaining<T>(trip, *(DBL*)&pr->u.p.parm3, chunkIdx);
                  init = trip - init;
               }
               else
                  init = 0;
               limit = trip - __kmp_dispatch_guided_remaining<T>(trip, *(DBL*)&pr->u.p.parm3, chunkIdx + 1);
               if(init < limit)
               {
                  --limit;
                  status = 1;
                  break;
               } // if
            }    // if
         }       // while (1)
         if(status != 0)
         {
            start = pr->u.p.lb;
            incr = pr->u.p.st;
            if(p_st != NULL)
               *p_st = incr;
            *p_lb = start + init * incr;
            *p_ub = start + limit * incr;
            if(pr->flags.ordered)
            {
               pr->u.p.ordered_lower = init;
               pr->u.p.ordered_upper = limit;
            }
         }
         else
         {
            *p_lb = 0;
            *p_ub = 0;
            if(p_st != NULL)
               *p_st = 0;
         }
      } // case
      break;

      case kmp_sch_trapezoidal:
      {
         UT index;
         T parm2 = pr->u.p.parm2;
         T parm3 = pr->u.p.parm3;
         T parm4 = pr->u.p.parm4;

         index = test_then_inc<ST>((volatile ST*)&sh->u.s.iteration);

         init = (index * ((2 * parm2) - (index - 1) * parm4)) / 2;
         trip = pr->u.p.tc - 1;

         if((status = ((T)index < parm3 && init <= trip)) == 0)
         {
            *p_lb = 0;
            *p_ub = 0;
            if(p_st != NULL)
               *p_st = 0;
         }
         else
         {
            start = pr->u.p.lb;
            limit = ((index + 1) * (2 * parm2 - index * parm4)) / 2 - 1;
            incr = pr->u.p.st;

            if((last = (limit >= trip)) != 0)
               limit = trip;

            if(p_st != NULL)
               *p_st = incr;

            if(incr == 1)
            {
               *p_lb = start + init;
               *p_ub = start + limit;
            }
            else
            {
               *p_lb = start + init * incr;
               *p_ub = start + limit * incr;
            }

            if(pr->flags.ordered)
            {
               pr->u.p.ordered_lower = init;
               pr->u.p.ordered_upper = limit;
            } // if
         }    // if
      }       // case
      break;
      default:
      {
         status = 0; // to avoid complaints on uninitialized variable use
      }
      break;
   } // switch
   if(p_last)
      *p_last = last;
   return status;
}

template <typename T>
static int __kmp_dispatch_next(ident_t* loc, int gtid, kmp_int32* p_last, T* p_lb, T* p_ub,
                               typename traits_t<T>::signed_t* p_st)
{
   typedef typename traits_t<T>::unsigned_t UT;
   typedef typename traits_t<T>::signed_t ST;
   // This is potentially slightly misleading, schedule(runtime) will appear here
   // even if the actual runtime schedule is static. (Which points out a
   // disadvantage of schedule(runtime): even when static scheduling is used it
   // costs more than a compile time choice to use static scheduling would.)

   int status;
   dispatch_private_info_template<T>* pr;
   kmp_info_t* th = __kmp_threads[gtid];
   kmp_team_t* team = th->th.th_team;

   if(team->t.t_serialized)
   {
      /* NOTE: serialize this dispatch because we are not at the active level */
      pr = reinterpret_cast<dispatch_private_info_template<T>*>(
          th->th.th_dispatch->th_disp_buffer); /* top of the stack */

      if((status = (pr->u.p.tc != 0)) == 0)
      {
         *p_lb = 0;
         *p_ub = 0;
         //            if ( p_last != NULL )
         //                *p_last = 0;
         if(p_st != NULL)
            *p_st = 0;
      }
      else if(pr->flags.nomerge)
      {
         kmp_int32 last;
         T start;
         UT limit, trip, init;
         ST incr;
         T chunk = pr->u.p.parm1;

         init = chunk * pr->u.p.count++;
         trip = pr->u.p.tc - 1;

         if((status = (init <= trip)) == 0)
         {
            *p_lb = 0;
            *p_ub = 0;
            //                if ( p_last != NULL )
            //                    *p_last = 0;
            if(p_st != NULL)
               *p_st = 0;
         }
         else
         {
            start = pr->u.p.lb;
            limit = chunk + init - 1;
            incr = pr->u.p.st;

            if((last = (limit >= trip)) != 0)
            {
               limit = trip;
            }
            if(p_last != NULL)
               *p_last = last;
            if(p_st != NULL)
               *p_st = incr;
            if(incr == 1)
            {
               *p_lb = start + init;
               *p_ub = start + limit;
            }
            else
            {
               *p_lb = start + init * incr;
               *p_ub = start + limit * incr;
            }

            if(pr->flags.ordered)
            {
               pr->u.p.ordered_lower = init;
               pr->u.p.ordered_upper = limit;
            } // if
         }    // if
      }
      else
      {
         pr->u.p.tc = 0;
         *p_lb = pr->u.p.lb;
         *p_ub = pr->u.p.ub;
         if(p_last != NULL)
            *p_last = TRUE;
         if(p_st != NULL)
            *p_st = pr->u.p.st;
      } // if
      return status;
   }
   else
   {
      kmp_int32 last = 0;
      dispatch_shared_info_template<T> volatile* sh;

      pr = reinterpret_cast<dispatch_private_info_template<T>*>(th->th.th_dispatch->th_dispatch_pr_current);
      sh = reinterpret_cast<dispatch_shared_info_template<T> volatile*>(th->th.th_dispatch->th_dispatch_sh_current);

      status = __kmp_dispatch_next_algorithm<T>(gtid, pr, sh, &last, p_lb, p_ub, p_st, th->th.th_team_nproc,
                                                th->th.th_info.ds.ds_tid);
      // status == 0: no more iterations to execute
      if(status == 0)
      {
         UT num_done;

         num_done = test_then_inc<ST>((volatile ST*)&sh->u.s.num_done);

         if((ST)num_done == th->th.th_team_nproc - 1)
         {
            /* NOTE: release this buffer to be reused */

            sh->u.s.num_done = 0;
            sh->u.s.iteration = 0;

            /* TODO replace with general release procedure? */
            if(pr->flags.ordered)
            {
               sh->u.s.ordered_iteration = 0;
            }

            sh->buffer_index += __kmp_dispatch_num_buffers;

         } // if

         th->th.th_dispatch->th_deo_fcn = NULL;
         th->th.th_dispatch->th_dxo_fcn = NULL;
         th->th.th_dispatch->th_dispatch_sh_current = NULL;
         th->th.th_dispatch->th_dispatch_pr_current = NULL;
      } // if (status == 0)
      if(p_last != NULL && status != 0)
         *p_last = last;
   } // if

   return status;
}

//-----------------------------------------------------------------------------
// Dispatch routines
//    Transfer call to template< type T >
//    __kmp_dispatch_init( ident_t *loc, int gtid, enum sched_type schedule,
//                         T lb, T ub, ST st, ST chunk )
extern "C"
{
   /*!
   @ingroup WORK_SHARING
   @{
   @param loc Source location
   @param gtid Global thread id
   @param schedule Schedule type
   @param lb  Lower bound
   @param ub  Upper bound
   @param st  Step (or increment if you prefer)
   @param chunk The chunk size to block with

   This function prepares the runtime to start a dynamically scheduled for loop,
   saving the loop arguments.
   These functions are all identical apart from the types of the arguments.
   */

   void __kmpc_dispatch_init_4(ident_t* loc, kmp_int32 gtid, enum sched_type schedule, kmp_int32 lb, kmp_int32 ub,
                               kmp_int32 st, kmp_int32 chunk)
   {
      __kmp_dispatch_init<kmp_int32>(loc, gtid, schedule, lb, ub, st, chunk, true);
   }
   /*!
   See @ref __kmpc_dispatch_init_4
   */
   void __kmpc_dispatch_init_4u(ident_t* loc, kmp_int32 gtid, enum sched_type schedule, kmp_uint32 lb, kmp_uint32 ub,
                                kmp_int32 st, kmp_int32 chunk)
   {
      __kmp_dispatch_init<kmp_uint32>(loc, gtid, schedule, lb, ub, st, chunk, true);
   }

   /*!
   See @ref __kmpc_dispatch_init_4
   */
   void __kmpc_dispatch_init_8(ident_t* loc, kmp_int32 gtid, enum sched_type schedule, kmp_int64 lb, kmp_int64 ub,
                               kmp_int64 st, kmp_int64 chunk)
   {
      __kmp_dispatch_init<kmp_int64>(loc, gtid, schedule, lb, ub, st, chunk, true);
   }

   /*!
   See @ref __kmpc_dispatch_init_4
   */
   void __kmpc_dispatch_init_8u(ident_t* loc, kmp_int32 gtid, enum sched_type schedule, kmp_uint64 lb, kmp_uint64 ub,
                                kmp_int64 st, kmp_int64 chunk)
   {
      __kmp_dispatch_init<kmp_uint64>(loc, gtid, schedule, lb, ub, st, chunk, true);
   }
   /*!
   @param loc Source code location
   @param gtid Global thread id
   @param p_last Pointer to a flag set to one if this is the last chunk or zero
   otherwise
   @param p_lb   Pointer to the lower bound for the next chunk of work
   @param p_ub   Pointer to the upper bound for the next chunk of work
   @param p_st   Pointer to the stride for the next chunk of work
   @return one if there is work to be done, zero otherwise

   Get the next dynamically allocated chunk of work for this thread.
   If there is no more work, then the lb,ub and stride need not be modified.
   */
   int __kmpc_dispatch_next_4(ident_t* loc, kmp_int32 gtid, kmp_int32* p_last, kmp_int32* p_lb, kmp_int32* p_ub,
                              kmp_int32* p_st)
   {
      return __kmp_dispatch_next<kmp_int32>(loc, gtid, p_last, p_lb, p_ub, p_st);
   }

   /*!
   See @ref __kmpc_dispatch_next_4
   */
   int __kmpc_dispatch_next_4u(ident_t* loc, kmp_int32 gtid, kmp_int32* p_last, kmp_uint32* p_lb, kmp_uint32* p_ub,
                               kmp_int32* p_st)
   {
      return __kmp_dispatch_next<kmp_uint32>(loc, gtid, p_last, p_lb, p_ub, p_st);
   }

   /*!
   See @ref __kmpc_dispatch_next_4
   */
   int __kmpc_dispatch_next_8(ident_t* loc, kmp_int32 gtid, kmp_int32* p_last, kmp_int64* p_lb, kmp_int64* p_ub,
                              kmp_int64* p_st)
   {
      return __kmp_dispatch_next<kmp_int64>(loc, gtid, p_last, p_lb, p_ub, p_st);
   }

   /*!
   See @ref __kmpc_dispatch_next_4
   */
   int __kmpc_dispatch_next_8u(ident_t* loc, kmp_int32 gtid, kmp_int32* p_last, kmp_uint64* p_lb, kmp_uint64* p_ub,
                               kmp_int64* p_st)
   {
      return __kmp_dispatch_next<kmp_uint64>(loc, gtid, p_last, p_lb, p_ub, p_st);
   }

} // extern "C"
