/**********************************************************************************************/
/*  This program is part of the Barcelona OpenMP Tasks Suite */
/*  Copyright (C) 2009 Barcelona Supercomputing Center - Centro Nacional de
 * Supercomputacion  */
/*  Copyright (C) 2009 Universitat Politecnica de Catalunya */
/*                                                                                            */
/*  This program is free software; you can redistribute it and/or modify */
/*  it under the terms of the GNU General Public License as published by */
/*  the Free Software Foundation; either version 2 of the License, or */
/*  (at your option) any later version. */
/*                                                                                            */
/*  This program is distributed in the hope that it will be useful, */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/*  GNU General Public License for more details. */
/*                                                                                            */
/*  You should have received a copy of the GNU General Public License */
/*  along with this program; if not, write to the Free Software */
/*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA            */
/**********************************************************************************************/

#include "kernel.hpp"
#include <algorithm>
#include <cmath>

void print_mat(Real* M, const char* name)
{
   if(name)
   {
      fprintf(stderr, "Mat %s\n", name);
   }
   else
   {
      fprintf(stderr, "Mat unkown\n");
   }
   for(unsigned i = 0; i < block_size; i++)
   {
      for(unsigned j = 0; j < block_size; j++)
      {
         fprintf(stderr, "%.3f ", M[i * block_size + j]);
      }
      fprintf(stderr, "\n");
   }
}

int checkmat(Real* M, Real* N)
{
   int i, j;
   Real r_err;
   int bad = 0;
   for(i = 0; i < block_size; i++)
   {
      for(j = 0; j < block_size; j++)
      {
         r_err = M[i * block_size + j] - N[i * block_size + j];
         if(r_err < 0.0)
            r_err = -r_err;
         r_err = r_err / M[i * block_size + j];
         if(r_err > EPSILON)
         {
            fprintf(stderr, "Checking failure: A[%d][%d]=%f  B[%d][%d]=%f; Relative Error=%f\n", i, j,
                    M[i * block_size + j], i, j, N[i * block_size + j], r_err);
            bad = 1;
         }
      }
   }
   return bad ? FALSE : TRUE;
}

int check(Real* T, Real* M)
{
   bool error = false;
   for(unsigned i = 0; i < (mat_size * mat_size * block_offset); i++)
   {
      const Real scale = std::max(std::abs(T[i]), static_cast<Real>(1));
      const Real r_err = std::abs(T[i] - M[i]) / scale;
      if(!std::isfinite(T[i]) || !std::isfinite(M[i]) || r_err > EPSILON)
      {
         fprintf(stderr, "Checking failure: T[%d]=%f  M[%d]=%f; Relative Error=%f\n", i, T[i], i, M[i], r_err);
         error = true;
      }
   }

   return error;
}

void genmat(Real* M[])
{
   int null_entry, init_val, i, j, ii, jj;
   Real* p;

   init_val = 1325;

   /* generating the structure */
   for(ii = 0; ii < mat_size; ii++)
   {
      for(jj = 0; jj < mat_size; jj++)
      {
         /* computing null entries */
         null_entry = FALSE;
         if((ii < jj) && (ii % 3 != 0))
            null_entry = TRUE;
         if((ii > jj) && (jj % 3 != 0))
            null_entry = TRUE;
         if(ii % 2 == 1)
            null_entry = TRUE;
         if(jj % 2 == 1)
            null_entry = TRUE;
         if(ii == jj)
            null_entry = FALSE;
         if(ii == jj - 1)
            null_entry = FALSE;
         if(ii - 1 == jj)
            null_entry = FALSE;
         /* allocating matrix */
         if(null_entry == FALSE)
         {
            M[ii * mat_size + jj] = (Real*)malloc(block_size * block_size * sizeof(Real));
            if(M[ii * mat_size + jj] == NULL)
            {
               fprintf(stderr, "Error: Out of memory\n");
               exit(101);
            }
            /* initializing matrix */
            p = M[ii * mat_size + jj];
            for(i = 0; i < block_size; i++)
            {
               for(j = 0; j < block_size; j++)
               {
                  init_val = (3125 * init_val) % 65536;
                  (*p) = (Real)((init_val - 32768.0) / 16384.0);
                  p++;
               }
            }
         }
         else
         {
            M[ii * mat_size + jj] = NULL;
         }
      }
   }
}

void print_structure(char const* name, Real* M[])
{
   int ii, jj;
   fprintf(stderr, "Structure for matrix %s @ 0x%p\n", name, M);
   for(ii = 0; ii < mat_size; ii++)
   {
      for(jj = 0; jj < mat_size; jj++)
      {
         if(M[ii * mat_size + jj] != NULL)
         {
            fprintf(stderr, "x");
         }
         else
         {
            fprintf(stderr, " ");
         }
      }
      fprintf(stderr, "\n");
   }
   fprintf(stderr, "\n");
}

void print_structure(char const* name, Real* M)
{
   int ii, jj;
   fprintf(stderr, "Structure of matrix %s\n", name);
   const unsigned bsize = block_size * block_size;
   for(ii = 0; ii < mat_size; ii++)
   {
      for(jj = 0; jj < mat_size; jj++)
      {
         if(M[(ii * mat_size + jj) * bsize] != null)
         {
            fprintf(stderr, "x");
         }
         else
         {
            fprintf(stderr, " ");
         }
      }
      fprintf(stderr, "\n");
   }
   fprintf(stderr, "\n");
}

void genmat(Real* M)
{
   int null_entry, init_val, i, j, ii, jj;
   Real* p;

   init_val = 1325;

   /* generating the structure */
   for(ii = 0; ii < mat_size; ii++)
   {
      for(jj = 0; jj < mat_size; jj++)
      {
         /* computing null entries */
         null_entry = FALSE;
         if((ii < jj) && (ii % 3 != 0))
            null_entry = TRUE;
         if((ii > jj) && (jj % 3 != 0))
            null_entry = TRUE;
         if(ii % 2 == 1)
            null_entry = TRUE;
         if(jj % 2 == 1)
            null_entry = TRUE;
         if(ii == jj)
            null_entry = FALSE;
         if(ii == jj - 1)
            null_entry = FALSE;
         if(ii - 1 == jj)
            null_entry = FALSE;
         /* allocating matrix */
         if(null_entry == FALSE)
         {
            /* initializing matrix */
            p = &(M[(ii * mat_size + jj) * (block_size * block_size)]);
            for(i = 0; i < block_size; i++)
            {
               for(j = 0; j < block_size; j++)
               {
                  init_val = (3125 * init_val) % 65536;
                  (*p) = static_cast<Real>((init_val - static_cast<Real>(32768)) / static_cast<Real>(16384));
                  p++;
               }
            }
         }
         else
         {
            p = &(M[(ii * mat_size + jj) * (block_size * block_size)]);
            for(i = 0; i < block_size; i++)
            {
               for(j = 0; j < block_size; j++)
               {
                  (*p) = null;
                  p++;
               }
            }
         }
      }
   }
}

Real* allocate_clean_block()
{
   int i, j;
   Real *p, *q;

   p = (Real*)malloc(block_size * block_size * sizeof(Real));
   q = p;
   if(p != nullptr)
   {
      for(i = 0; i < block_size; i++)
         for(j = 0; j < block_size; j++)
         {
            (*p) = 0.0;
            p++;
         }
   }
   else
   {
      exit(101);
   }
   return (q);
}

void lu0(Real* diag)
{
   int i, j, k;

   for(k = 0; k < block_size; k++)
      for(i = k + 1; i < block_size; i++)
      {
         diag[i * block_size + k] = diag[i * block_size + k] / diag[k * block_size + k];
         for(j = k + 1; j < block_size; j++)
         {
            diag[i * block_size + j] = diag[i * block_size + j] - diag[i * block_size + k] * diag[k * block_size + j];
         }
      }
}

void bdiv(Real* diag, Real* row)
{
   int i, j, k;
   for(i = 0; i < block_size; i++)
      for(k = 0; k < block_size; k++)
      {
         row[i * block_size + k] = row[i * block_size + k] / diag[k * block_size + k];
         for(j = k + 1; j < block_size; j++)
            row[i * block_size + j] = row[i * block_size + j] - row[i * block_size + k] * diag[k * block_size + j];
      }
}

void bmod(Real* row, Real* col, Real* inner)
{
   int i, j, k;
   for(i = 0; i < block_size; i++)
      for(j = 0; j < block_size; j++)
         for(k = 0; k < block_size; k++)
            inner[i * block_size + j] = inner[i * block_size + j] - row[i * block_size + k] * col[k * block_size + j];
}

void fwd(Real* diag, Real* col)
{
   int i, j, k;
   for(j = 0; j < block_size; j++)
      for(k = 0; k < block_size; k++)
         for(i = k + 1; i < block_size; i++)
            col[i * block_size + j] = col[i * block_size + j] - diag[i * block_size + k] * col[k * block_size + j];
}

void init(Real* mat, char const* name)
{
   genmat(mat);
   print_structure(name, mat);
}

__attribute__((noinline)) void fn_vanilla(Real* mat)
{
   for(unsigned kk = 0; kk < mat_size; kk++)
   {
      lu0(&(mat[(kk * mat_size + kk) * block_offset]));

      for(unsigned jj = kk + 1; jj < mat_size; jj++)
      {
         if(mat[(kk * mat_size + jj) * block_offset] != null)
         {
            fwd(&(mat[(kk * mat_size + kk) * block_offset]), &(mat[(kk * mat_size + jj) * block_offset]));
         }
      }

      for(unsigned ii = kk + 1; ii < mat_size; ii++)
      {
         if(mat[(ii * mat_size + kk) * block_offset] != null)
         {
            bdiv(&(mat[(kk * mat_size + kk) * block_offset]), &(mat[(ii * mat_size + kk) * block_offset]));
         }
      }

      for(unsigned ii = kk + 1; ii < mat_size; ii++)
      {
         for(unsigned jj = kk + 1; jj < mat_size; jj++)
         {
            if(mat[(kk * mat_size + jj) * block_offset] != null && mat[(ii * mat_size + kk) * block_offset] != null)
            {
               if(mat[(ii * mat_size + jj) * block_offset] == null)
               {
                  // Allocate clean block
                  Real* p = &mat[(ii * mat_size + jj) * block_offset];
                  for(unsigned pos = 0; pos < block_offset; pos++, p++)
                  {
                     (*p) = static_cast<Real>(0);
                  }
               }
               bmod(&(mat[(ii * mat_size + kk) * block_offset]), &(mat[(kk * mat_size + jj) * block_offset]),
                    &(mat[(ii * mat_size + jj) * block_offset]));
            }
         }
      }
   }
}

void print_mat_flat(Real* mat)
{
   fprintf(stderr, "[");
   for(unsigned i = 0; i < (mat_size * mat_size * block_size * block_size); i++)
   {
      fprintf(stderr, "%.2f ", mat[i]);
   }
   fprintf(stderr, "]\n");
}

#if BANK_NUMBER == 2
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 2 chunk_size = 4
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 2 chunk_size = 16
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 2 chunk_size = 32
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 2 chunk_size = 64
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 2 chunk_size = 128
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 2 chunk_size = 256
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 2 chunk_size = 512
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 2 chunk_size = 1024
#endif
#elif BANK_NUMBER == 32
#ifdef USE_AXI
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 32 chunk_size = 4 mode = m_axi
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 32 chunk_size = 16 mode = m_axi
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 32 chunk_size = 32 mode = m_axi
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 32 chunk_size = 64 mode = m_axi
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 32 chunk_size = 128 mode = m_axi
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 32 chunk_size = 256 mode = m_axi
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 32 chunk_size = 512 mode = m_axi
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 32 chunk_size = 1024 mode = m_axi
#endif
#else
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 32 chunk_size = 4
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 32 chunk_size = 16
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 32 chunk_size = 32
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 32 chunk_size = 64
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 32 chunk_size = 128
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 32 chunk_size = 256
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 32 chunk_size = 512
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 32 chunk_size = 1024
#endif
#endif
#elif BANK_NUMBER == 8
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 8 chunk_size = 4
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 8 chunk_size = 16
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 8 chunk_size = 32
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 8 chunk_size = 64
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 8 chunk_size = 128
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 8 chunk_size = 256
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 8 chunk_size = 512
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 8 chunk_size = 1024
#endif
#elif BANK_NUMBER == 16
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 16 chunk_size = 4
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 16 chunk_size = 16
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 16 chunk_size = 32
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 16 chunk_size = 64
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 16 chunk_size = 128
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 16 chunk_size = 256
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 16 chunk_size = 512
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 16 chunk_size = 1024
#endif
#elif BANK_NUMBER == 4
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 4 chunk_size = 4
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 4 chunk_size = 16
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 4 chunk_size = 32
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 4 chunk_size = 64
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 4 chunk_size = 128
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 4 chunk_size = 256
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 4 chunk_size = 512
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 4 chunk_size = 1024
#endif
#elif BANK_NUMBER == 64
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 64 chunk_size = 4
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 64 chunk_size = 16
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 64 chunk_size = 32
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 64 chunk_size = 64
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 64 chunk_size = 128
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 64 chunk_size = 256
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 64 chunk_size = 512
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 64 chunk_size = 1024
#endif
#elif BANK_NUMBER == 128
#if BANK_CHUNK_SIZE == 4
#pragma HLS bus bank_number = 128 chunk_size = 4
#elif BANK_CHUNK_SIZE == 16
#pragma HLS bus bank_number = 128 chunk_size = 16
#elif BANK_CHUNK_SIZE == 32
#pragma HLS bus bank_number = 128 chunk_size = 32
#elif BANK_CHUNK_SIZE == 64
#pragma HLS bus bank_number = 128 chunk_size = 64
#elif BANK_CHUNK_SIZE == 128
#pragma HLS bus bank_number = 128 chunk_size = 128
#elif BANK_CHUNK_SIZE == 256
#pragma HLS bus bank_number = 128 chunk_size = 256
#elif BANK_CHUNK_SIZE == 512
#pragma HLS bus bank_number = 128 chunk_size = 512
#elif BANK_CHUNK_SIZE == 1024
#pragma HLS bus bank_number = 128 chunk_size = 1024
#endif
#endif
#ifdef USE_AXI
#ifndef BANK_NUMBER
#pragma HLS bus mode = m_axi
#endif
#pragma HLS interface port = mat mode = bus offset = direct
#else
#pragma HLS interface port = mat mode = bus
#endif
__attribute__((noinline)) void fn(Real* mat)
{
#pragma omp parallel shared(mat) num_threads(1)
   {
#pragma omp single nowait
      {
         for(unsigned kk = 0; kk < mat_size; kk++)
         {
#pragma omp task firstprivate(kk) depend(inout : mat[(kk * mat_size + kk) * block_offset])
            {
               lu0(&(mat[(kk * mat_size + kk) * block_offset]));
            }

            for(unsigned jj = kk + 1; jj < mat_size; jj++)
            {
#pragma omp task firstprivate(kk, jj) depend(in                                          \
                                             : mat[(kk * mat_size + kk) * block_offset]) \
    depend(inout                                                                         \
           : mat[(kk * mat_size + jj) * block_offset])
               {
                  if(mat[(kk * mat_size + jj) * block_offset] != null)
                  {
                     fwd(&(mat[(kk * mat_size + kk) * block_offset]), &(mat[(kk * mat_size + jj) * block_offset]));
                  }
               }
            }

            for(unsigned ii = kk + 1; ii < mat_size; ii++)
            {
#pragma omp task firstprivate(kk, ii) shared(mat) depend(in                                          \
                                                         : mat[(kk * mat_size + kk) * block_offset]) \
    depend(inout                                                                                     \
           : mat[(ii * mat_size + kk) * block_offset])
               {
                  if(mat[(ii * mat_size + kk) * block_offset] != null)
                  {
                     bdiv(&(mat[(kk * mat_size + kk) * block_offset]), &(mat[(ii * mat_size + kk) * block_offset]));
                  }
               }
            }

            for(unsigned ii = kk + 1; ii < mat_size; ii++)
            {
               for(unsigned jj = kk + 1; jj < mat_size; jj++)
               {
#pragma omp task firstprivate(kk, ii, jj)                                                        \
    depend(in                                                                                    \
           : mat[(kk * mat_size + jj) * block_offset], mat[(ii * mat_size + kk) * block_offset]) \
        depend(inout                                                                             \
               : mat[(ii * mat_size + jj) * block_offset])
                  {
                     if(mat[(kk * mat_size + jj) * block_offset] != null &&
                        mat[(ii * mat_size + kk) * block_offset] != null)
                     {
                        if(mat[(ii * mat_size + jj) * block_offset] == null)
                        {
                           // Allocate clean block
                           Real* p = &mat[(ii * mat_size + jj) * block_offset];
                           for(unsigned pos = 0; pos < block_offset; pos++, p++)
                           {
                              (*p) = static_cast<Real>(0);
                           }
                        }
                        bmod(&(mat[(ii * mat_size + kk) * block_offset]), &(mat[(kk * mat_size + jj) * block_offset]),
                             &(mat[(ii * mat_size + jj) * block_offset]));
                     }
                  }
               }
            }
         }
      } // end single
   }    // end parallel
}

void original_init(Real*** pBENCH, char const* pass)
{
   *pBENCH = (Real**)malloc(mat_size * mat_size * sizeof(Real*));
   genmat(*pBENCH);
   print_structure(pass, *pBENCH);
}

void original_fn(Real** N)
{
   int ii, jj, kk;
   for(kk = 0; kk < mat_size; kk++)
   {
      lu0(N[kk * mat_size + kk]);

      for(jj = kk + 1; jj < mat_size; jj++)
      {
         if(N[kk * mat_size + jj] != NULL)
         {
            fwd(N[kk * mat_size + kk], N[kk * mat_size + jj]);
         }
      }
      for(ii = kk + 1; ii < mat_size; ii++)
         if(N[ii * mat_size + kk] != NULL)
         {
            bdiv(N[kk * mat_size + kk], N[ii * mat_size + kk]);
         }
      for(ii = kk + 1; ii < mat_size; ii++)
         if(N[ii * mat_size + kk] != NULL)
            for(jj = kk + 1; jj < mat_size; jj++)
               if(N[kk * mat_size + jj] != NULL)
               {
                  if(N[ii * mat_size + jj] == NULL)
                     N[ii * mat_size + jj] = allocate_clean_block();
                  bmod(N[ii * mat_size + kk], N[kk * mat_size + jj], N[ii * mat_size + jj]);
               }
   }
}

void originall_print_structure(char const* name, Real* M[])
{
   int ii, jj;
   fprintf(stderr, "Structure for matrix %s @ 0x%p\n", name, M);
   for(ii = 0; ii < mat_size; ii++)
   {
      for(jj = 0; jj < mat_size; jj++)
      {
         if(M[ii * mat_size + jj] != NULL)
         {
            fprintf(stderr, "x");
         }
         else
            fprintf(stderr, " ");
      }
      fprintf(stderr, "\n");
   }
   fprintf(stderr, "\n");
}
