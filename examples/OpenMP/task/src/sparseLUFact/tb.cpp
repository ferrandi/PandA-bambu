/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
#include "kernel.hpp"

int main()
{
   Real* mat = (Real*)malloc(sizeof(Real) * (mat_size * mat_size) * (block_size * block_size));
   Real* mat_host = (Real*)malloc(sizeof(Real) * (mat_size * mat_size) * (block_size * block_size));
   Real** original;
   init(mat, "openmp allocation");
   init(mat_host, "host allocation");
   original_init(&original, "host dynamic allocation");

   fprintf(stderr, "Checking before kernel\n");
   assert(!check(mat_host, mat));
   // assert(check(original, mat));
   fprintf(stderr, "Check before kernel passed\n");

#ifdef __BAMBU_SIM__
   m_param_alloc(0, sizeof(Real) * (mat_size * mat_size) * (block_size * block_size));
#endif
   fn(mat);

   fn_vanilla(mat_host);
   original_fn(original);

   fprintf(stderr, "\nStructures after kernel\n");
   print_structure("openmp allocation", mat);
   print_structure("host allocation", mat_host);
   print_structure("original dynamic allocation", original);

   const bool res = check(mat_host, mat);
   assert(!res);
   fprintf(stderr, "res %d\n", res);
   // assert(check(original, mat));
   fprintf(stderr, "Checking after kernel result: %d\n", !res);

#ifdef PRINT_FLAT_MAT
   fprintf(stderr, "mat openmp:\n");
   print_mat_flat(mat);
   fprintf(stderr, "mat host:\n");
   print_mat_flat(mat_host);
#endif

   free(mat);
   free(mat_host);

   return res;
}
