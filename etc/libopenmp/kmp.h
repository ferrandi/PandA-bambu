/*! \file */
/*
 * kmp.h -- KPTS runtime header file.
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

#ifndef KMP_H
#define KMP_H

#include "kmp_os.h"

/*!
@ingroup PARALLEL
The type for a microtask which gets passed to @ref __kmpc_fork_call().
The arguments to the outlined function are
@param global_tid the global thread identity of the thread executing the
function.
@param bound_tid  the local identity of the thread executing the function
@param ... pointers to shared variables accessed by the function.
*/
typedef void (*kmpc_micro)(kmp_int32 global_tid, kmp_int32 bound_tid, ...);

/*!
 * The ident structure that describes a source location.
 */
typedef struct ident
{
   // not used by bambu
} ident_t;

#ifdef __cplusplus
extern "C"
{
#endif

   enum dynamic_mode
   {
      dynamic_default,
#ifdef USE_LOAD_BALANCE
      dynamic_load_balance,
#endif /* USE_LOAD_BALANCE */
      dynamic_random,
      dynamic_thread_limit,
      dynamic_max
   };

/* external schedule constants, duplicate enum omp_sched in omp.h in order to
 * not include it here */
#ifndef KMP_SCHED_TYPE_DEFINED
#define KMP_SCHED_TYPE_DEFINED
   typedef enum kmp_sched
   {
      kmp_sched_lower = 0, // lower and upper bounds are for routine parameter check
      // Note: need to adjust __kmp_sch_map global array in case enum is changed
      kmp_sched_static = 1,        // mapped to kmp_sch_static_chunked           (33)
      kmp_sched_dynamic = 2,       // mapped to kmp_sch_dynamic_chunked          (35)
      kmp_sched_guided = 3,        // mapped to kmp_sch_guided_chunked           (36)
      kmp_sched_auto = 4,          // mapped to kmp_sch_auto                     (38)
      kmp_sched_upper_std = 5,     // upper bound for standard schedules
      kmp_sched_lower_ext = 100,   // lower bound of Intel extension schedules
      kmp_sched_trapezoidal = 101, // mapped to kmp_sch_trapezoidal (39)
#if KMP_STATIC_STEAL_ENABLED
      kmp_sched_static_steal = 102, // mapped to kmp_sch_static_steal (44)
#endif
      kmp_sched_upper,
      kmp_sched_default = kmp_sched_static, // default scheduling
      kmp_sched_monotonic = 0x80000000
   } kmp_sched_t;
#endif

   /*!
    @ingroup WORK_SHARING
    * Describes the loop schedule to be used for a parallel for loop.
    */
   enum sched_type : kmp_int32
   {
      kmp_sch_lower = 32, /**< lower bound for unordered values */
      kmp_sch_static_chunked = 33,
      kmp_sch_static = 34, /**< static unspecialized */
      kmp_sch_dynamic_chunked = 35,
      kmp_sch_guided_chunked = 36, /**< guided unspecialized */
      kmp_sch_runtime = 37,
      kmp_sch_auto = 38, /**< auto */
      kmp_sch_trapezoidal = 39,

      /* accessible only through KMP_SCHEDULE environment variable */
      kmp_sch_static_greedy = 40,
      kmp_sch_static_balanced = 41,
      /* accessible only through KMP_SCHEDULE environment variable */
      kmp_sch_guided_iterative_chunked = 42,
      kmp_sch_guided_analytical_chunked = 43,
      /* accessible only through KMP_SCHEDULE environment variable */
      kmp_sch_static_steal = 44,

      /* static with chunk adjustment (e.g., simd) */
      kmp_sch_static_balanced_chunked = 45,
      kmp_sch_guided_simd = 46,  /**< guided with chunk adjustment */
      kmp_sch_runtime_simd = 47, /**< runtime with chunk adjustment */

      /* accessible only through KMP_SCHEDULE environment variable */
      kmp_sch_upper, /**< upper bound for unordered values */

      kmp_ord_lower = 64, /**< lower bound for ordered values, must be power of 2 */
      kmp_ord_static_chunked = 65,
      kmp_ord_static = 66, /**< ordered static unspecialized */
      kmp_ord_dynamic_chunked = 67,
      kmp_ord_guided_chunked = 68,
      kmp_ord_runtime = 69,
      kmp_ord_auto = 70, /**< ordered auto */
      kmp_ord_trapezoidal = 71,
      kmp_ord_upper, /**< upper bound for ordered values */

      /* Schedules for Distribute construct */
      kmp_distribute_static_chunked = 91, /**< distribute static chunked */
      kmp_distribute_static = 92,         /**< distribute static unspecialized */

      /* For the "nomerge" versions, kmp_dispatch_next*() will always return a
         single iteration/chunk, even if the loop is serialized. For the schedule
         types listed above, the entire iteration vector is returned if the loop is
         serialized. This doesn't work for gcc/gcomp sections. */
      kmp_nm_lower = 160, /**< lower bound for nomerge values */

      kmp_nm_static_chunked = (kmp_sch_static_chunked - kmp_sch_lower + kmp_nm_lower),
      kmp_nm_static = 162, /**< static unspecialized */
      kmp_nm_dynamic_chunked = 163,
      kmp_nm_guided_chunked = 164, /**< guided unspecialized */
      kmp_nm_runtime = 165,
      kmp_nm_auto = 166, /**< auto */
      kmp_nm_trapezoidal = 167,

      /* accessible only through KMP_SCHEDULE environment variable */
      kmp_nm_static_greedy = 168,
      kmp_nm_static_balanced = 169,
      /* accessible only through KMP_SCHEDULE environment variable */
      kmp_nm_guided_iterative_chunked = 170,
      kmp_nm_guided_analytical_chunked = 171,
      kmp_nm_static_steal = 172, /* accessible only through OMP_SCHEDULE environment variable */

      kmp_nm_ord_static_chunked = 193,
      kmp_nm_ord_static = 194, /**< ordered static unspecialized */
      kmp_nm_ord_dynamic_chunked = 195,
      kmp_nm_ord_guided_chunked = 196,
      kmp_nm_ord_runtime = 197,
      kmp_nm_ord_auto = 198, /**< auto */
      kmp_nm_ord_trapezoidal = 199,
      kmp_nm_upper, /**< upper bound for nomerge values */

      /* Support for OpenMP 4.5 monotonic and nonmonotonic schedule modifiers. Since
         we need to distinguish the three possible cases (no modifier, monotonic
         modifier, nonmonotonic modifier), we need separate bits for each modifier.
         The absence of monotonic does not imply nonmonotonic, especially since 4.5
         says that the behaviour of the "no modifier" case is implementation defined
         in 4.5, but will become "nonmonotonic" in 5.0.

         Since we're passing a full 32 bit value, we can use a couple of high bits
         for these flags; out of paranoia we avoid the sign bit.

         These modifiers can be or-ed into non-static schedules by the compiler to
         pass the additional information. They will be stripped early in the
         processing in __kmp_dispatch_init when setting up schedules, so most of the
         code won't ever see schedules with these bits set.  */
      kmp_sch_modifier_monotonic = (1 << 29),    /**< Set if the monotonic schedule modifier was present */
      kmp_sch_modifier_nonmonotonic = (1 << 30), /**< Set if the nonmonotonic schedule modifier was present */

#define SCHEDULE_WITHOUT_MODIFIERS(s) \
   (enum sched_type)((s) & ~(kmp_sch_modifier_nonmonotonic | kmp_sch_modifier_monotonic))
#define SCHEDULE_HAS_MONOTONIC(s) (((s)&kmp_sch_modifier_monotonic) != 0)
#define SCHEDULE_HAS_NONMONOTONIC(s) (((s)&kmp_sch_modifier_nonmonotonic) != 0)
#define SCHEDULE_HAS_NO_MODIFIERS(s) (((s) & (kmp_sch_modifier_nonmonotonic | kmp_sch_modifier_monotonic)) == 0)
#define SCHEDULE_GET_MODIFIERS(s) \
   ((enum sched_type)((s) & (kmp_sch_modifier_nonmonotonic | kmp_sch_modifier_monotonic)))
#define SCHEDULE_SET_MODIFIERS(s, m) (s = (enum sched_type)((kmp_int32)s | (kmp_int32)m))
#define SCHEDULE_NONMONOTONIC 0
#define SCHEDULE_MONOTONIC 1

      kmp_sch_default = kmp_sch_static /**< default scheduling algorithm */
   };

   /* -- fast reduction stuff ------------------------------------------------ */

#undef KMP_FAST_REDUCTION_BARRIER
#define KMP_FAST_REDUCTION_BARRIER 0

#undef KMP_FAST_REDUCTION_CORE_DUO
#if KMP_ARCH_X86 || KMP_ARCH_X86_64
#define KMP_FAST_REDUCTION_CORE_DUO 1
#endif

   enum _reduction_method
   {
      reduction_method_not_defined = 0,
      critical_reduce_block = (1 << 8),
      atomic_reduce_block = (2 << 8),
      tree_reduce_block = (3 << 8),
      empty_reduce_block = (4 << 8)
   };

   // Description of the packed_reduction_method variable:
   // The packed_reduction_method variable consists of two enum types variables
   // that are packed together into 0-th byte and 1-st byte:
   // 0: (packed_reduction_method & 0x000000FF) is a 'enum barrier_type' value of
   // barrier that will be used in fast reduction: bs_plain_barrier or
   // bs_reduction_barrier
   // 1: (packed_reduction_method & 0x0000FF00) is a reduction method that will
   // be used in fast reduction;
   // Reduction method is of 'enum _reduction_method' type and it's defined the way
   // so that the bits of 0-th byte are empty, so no need to execute a shift
   // instruction while packing/unpacking

#if KMP_FAST_REDUCTION_BARRIER
#define PACK_REDUCTION_METHOD_AND_BARRIER(reduction_method, barrier_type) ((reduction_method) | (barrier_type))

#define UNPACK_REDUCTION_METHOD(packed_reduction_method) \
   ((enum _reduction_method)((packed_reduction_method) & (0x0000FF00)))

#define UNPACK_REDUCTION_BARRIER(packed_reduction_method) \
   ((enum barrier_type)((packed_reduction_method) & (0x000000FF)))
#else
#define PACK_REDUCTION_METHOD_AND_BARRIER(reduction_method, barrier_type) (reduction_method)

#define UNPACK_REDUCTION_METHOD(packed_reduction_method) (packed_reduction_method)

#define UNPACK_REDUCTION_BARRIER(packed_reduction_method) (bs_plain_barrier)
#endif

#define TEST_REDUCTION_METHOD(packed_reduction_method, which_reduction_block) \
   ((UNPACK_REDUCTION_METHOD(packed_reduction_method)) == (which_reduction_block))

#if KMP_FAST_REDUCTION_BARRIER
#define TREE_REDUCE_BLOCK_WITH_REDUCTION_BARRIER \
   (PACK_REDUCTION_METHOD_AND_BARRIER(tree_reduce_block, bs_reduction_barrier))

#define TREE_REDUCE_BLOCK_WITH_PLAIN_BARRIER (PACK_REDUCTION_METHOD_AND_BARRIER(tree_reduce_block, bs_plain_barrier))
#endif

   typedef int PACKED_REDUCTION_METHOD_T;

   /* -- end of fast reduction stuff ----------------------------------------- */

#define KMP_MASTER_TID(tid) ((tid) == 0)

#define KMP_DEFAULT_CHUNK 1

#ifndef TRUE
#define FALSE 0
#define TRUE (!FALSE)
#endif

   typedef kmp_int32 kmp_critical_name[8];

   typedef struct kmp_sched_flags
   {
      unsigned ordered : 1;
      unsigned nomerge : 1;
      unsigned contains_last : 1;
      unsigned unused : 29;
   } kmp_sched_flags_t;

   enum barrier_type
   {
      bs_plain_barrier = 0, /* 0, All non-fork/join barriers (except reduction
                               barriers if enabled) */
      bs_forkjoin_barrier,  /* 1, All fork/join (parallel region) barriers */
#if KMP_FAST_REDUCTION_BARRIER
      bs_reduction_barrier, /* 2, All barriers that are used in reduction */
#endif                      // KMP_FAST_REDUCTION_BARRIER
      bs_last_barrier       /* Just a placeholder to mark the end */
   };

// to work with reduction barriers just like with plain barriers
#if !KMP_FAST_REDUCTION_BARRIER
#define bs_reduction_barrier bs_plain_barrier
#endif // KMP_FAST_REDUCTION_BARRIER

   typedef enum kmp_bar_pat
   {                          /* Barrier communication patterns */
     bp_linear_bar = 0,       /* Single level (degenerate) tree */
     bp_tree_bar = 1,         /* Balanced tree with branching factor 2^n */
     bp_hyper_bar = 2,        /* Hypercube-embedded tree with min branching
                                 factor 2^n */
     bp_hierarchical_bar = 3, /* Machine hierarchy tree */
     bp_last_bar              /* Placeholder to mark the end */
   } kmp_bar_pat_e;

   const kmp_bar_pat_e __kmp_barrier_gather_pattern[bs_last_barrier] = {bp_linear_bar, bp_tree_bar};
   const kmp_uint32 __kmp_barrier_gather_branch_bits[bs_last_barrier] = {0, 1};

   const enum sched_type __kmp_static = kmp_sch_static_greedy;
   const enum sched_type __kmp_guided = kmp_sch_guided_iterative_chunked; /* default guided scheduling method */
   const enum sched_type __kmp_auto = kmp_sch_guided_analytical_chunked;  /* default auto scheduling method */
   /* ------------------------------------------------------------------------- */

#include "kmp_bambu.h"

#define __kmp_get_global_thread_id() __kmp_get_global_thread_id_reg()
#define __kmp_get_gtid() __kmp_get_global_thread_id()
#define __kmp_entry_gtid() __kmp_get_global_thread_id_reg()
#define __kmp_get_tid() KMP_CS_GET_TID()

#define __kmp_get_team_num_threads(gtid) KMP_T_NPROC()

   static inline int __kmp_tid_from_gtid(int)
   {
      return KMP_CS_GET_TID();
   }

   // int __kmp_gtid_get_specific(void);
   extern int __kmp_get_global_thread_id_reg(void);

   extern void __kmp_push_num_threads(ident_t* loc, int gtid, int num_threads);

   extern int __kmp_barrier(enum barrier_type bt, int gtid, int is_split, size_t reduce_size, void* reduce_data,
                            void (*reduce)(void*, void*));

   KMP_EXPORT kmp_int32 __kmpc_global_thread_num(ident_t*);
   KMP_EXPORT kmp_int32 __kmpc_bound_num_threads(ident_t*);

   KMP_EXPORT void __kmpc_barrier(ident_t*, kmp_int32 global_tid);
   KMP_EXPORT void __kmpc_critical(ident_t*, kmp_int32 global_tid, kmp_critical_name*);
   KMP_EXPORT void __kmpc_end_critical(ident_t*, kmp_int32 global_tid, kmp_critical_name*);
   KMP_EXPORT void __kmpc_critical_with_hint(ident_t*, kmp_int32 global_tid, kmp_critical_name*, unsigned int hint);
   KMP_EXPORT void __kmpc_for_static_fini(ident_t* loc, kmp_int32 global_tid);

   /* Interface to fast scalable reduce methods routines */

   KMP_EXPORT kmp_int32 __kmpc_reduce_nowait(ident_t* loc, kmp_int32 global_tid, kmp_int32 num_vars, size_t reduce_size,
                                             void* reduce_data, void (*reduce_func)(void* lhs_data, void* rhs_data),
                                             kmp_critical_name* lck);
   KMP_EXPORT void __kmpc_end_reduce_nowait(ident_t* loc, kmp_int32 global_tid, kmp_critical_name* lck);
   KMP_EXPORT kmp_int32 __kmpc_reduce(ident_t* loc, kmp_int32 global_tid, kmp_int32 num_vars, size_t reduce_size,
                                      void* reduce_data, void (*reduce_func)(void* lhs_data, void* rhs_data),
                                      kmp_critical_name* lck);
   KMP_EXPORT void __kmpc_end_reduce(ident_t* loc, kmp_int32 global_tid, kmp_critical_name* lck);

   /* Internal fast reduction routines */

   extern PACKED_REDUCTION_METHOD_T
   __kmp_determine_reduction_method(ident_t* loc, kmp_int32 global_tid, kmp_int32 num_vars, size_t reduce_size,
                                    void* reduce_data, void (*reduce_func)(void* lhs_data, void* rhs_data),
                                    kmp_critical_name* lck);

   KMP_EXPORT void __kmpc_push_num_threads(ident_t* loc, kmp_int32 global_tid, kmp_int32 num_threads);

   KMP_EXPORT void __kmpc_for_static_init_4(ident_t* loc, kmp_int32 gtid, kmp_int32 schedtype, kmp_int32* plastiter,
                                            kmp_int32* plower, kmp_int32* pupper, kmp_int32* pstride, kmp_int32 incr,
                                            kmp_int32 chunk);
   KMP_EXPORT void __kmpc_for_static_init_4u(ident_t* loc, kmp_int32 gtid, kmp_int32 schedtype, kmp_int32* plastiter,
                                             kmp_uint32* plower, kmp_uint32* pupper, kmp_int32* pstride, kmp_int32 incr,
                                             kmp_int32 chunk);
   KMP_EXPORT void __kmpc_for_static_init_8(ident_t* loc, kmp_int32 gtid, kmp_int32 schedtype, kmp_int32* plastiter,
                                            kmp_int64* plower, kmp_int64* pupper, kmp_int64* pstride, kmp_int64 incr,
                                            kmp_int64 chunk);
   KMP_EXPORT void __kmpc_for_static_init_8u(ident_t* loc, kmp_int32 gtid, kmp_int32 schedtype, kmp_int32* plastiter,
                                             kmp_uint64* plower, kmp_uint64* pupper, kmp_int64* pstride, kmp_int64 incr,
                                             kmp_int64 chunk);

#ifdef __cplusplus
}
#endif
#endif /* KMP_H */
