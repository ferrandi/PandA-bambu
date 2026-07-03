/*! \file */
/*
 * omp.cpp -- OpenMP runtime extern routines
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

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

   /// OMP_1.0 section begin
   __attribute__((always_inline)) int omp_get_thread_num(void)
   {
      int gtid;
      gtid = __kmp_entry_gtid();
      return __kmp_tid_from_gtid(gtid);
   }

   /* returns the number of threads in current team */
   __attribute__((always_inline)) int omp_get_num_threads(void)
   {
      // __kmpc_bound_num_threads initializes the library if needed
      return __kmpc_bound_num_threads(NULL);
   }

/// OMP_1.0 end
#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
