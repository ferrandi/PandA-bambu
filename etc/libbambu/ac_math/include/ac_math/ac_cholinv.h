/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Math Library                                       *
 *                                                                        *
 *  Software Version: 2.0                                                 *
 *                                                                        *
 *  Release Date    : Thu Aug  2 11:19:34 PDT 2018                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 2.0.10                                              *
 *                                                                        *
 *  Copyright , Mentor Graphics Corporation,                     *
 *                                                                        *
 *  All Rights Reserved.                                                  *
 *
 **************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
 *                                                                        *
 *      http://www.apache.org/licenses/LICENSE-2.0                        *
 *                                                                        *
 *  Unless required by applicable law or agreed to in writing, software   *
 *  distributed under the License is distributed on an "AS IS" BASIS,     *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or       *
 *  implied.                                                              *
 *  See the License for the specific language governing permissions and   *
 *  limitations under the License.                                        *
 **************************************************************************
 *                                                                        *
 *  The most recent version of this package is available at github.       *
 *                                                                        *
 *************************************************************************/
//*******************************************************************************************
// File: ac_cholinv.h
//
// Description: Provides an implementation for the matrix inversion using
//    cholesky decomposition of ac_fixed and ac_complex<ac_fixed> matrices
//    which are positive definite.
//    The Cholesky-Crout algorithm is used for cholesky decomposition and
//    forward decomposition is used to calculate the inverse of lower triangular
//    matrix returned by ac_chol_d.h library file which computes the Cholesky
//    decomposition of a matrix.
//    By default, the width, integer width and sign of temporary variables
//    are the same as the output. The user can change that by specifying number
//    of bits to be added or taken away from this default value. The user can
//    also add extra template parameters to turn off rounding and saturation
//    and rounding for temp. variables (these are turned on by default).
//
//    The user can also choose between using PWL vs. using accurate
//    division/sqrt functions.
//
// Usage:
//    (see the type-specific examples below)
//
// Notes:
//    This file uses C++ function overloading to target implementations
//    specific to each type of data. Attempting to call the function
//    with a type that is not implemented will result in a compile error.
//
//    The user may choose to use the PWL functions for the internal calculations
//    (square root and reciprocal) or they may choose to use the accurate div and sqrt
//    functions provided in the mgc_ac_math library, using an optional template parameter.
//    Please refer to the documentation for ac_chol_d.h which computes the Cholesky decomposition
//    and documentation for this file for more details.
//
//*******************************************************************************************

#if __cplusplus < 201103L
#error Please use C++11 or a later standard for compilation.
#endif

#ifndef _INCLUDED_AC_CHOLINV_H_
#define _INCLUDED_AC_CHOLINV_H_

// Include headers for data types supported by these implementations
#include <ac_complex.h>
#include <ac_fixed.h>
#include <ac_float.h>
#include <ac_int.h>
#include <ac_matrix.h>

// Include headers for required functions
#include <ac_math/ac_chol_d.h>
#include <ac_math/ac_reciprocal_pwl.h>

#ifndef __BAMBU__
#include <iostream>
using namespace std;
#endif

//==============================================================================
// Function: ac_cholinv (for ac_fixed using native C-style arrays)
//
// Description:
//    Matrix inversion using cholesky decomposition of real-valued matrices with
//    ac_fixed variables.
//
// Usage:
//    A sample testbench and its implementation look like this:
//
//    #include <mc_scverify.h>
//    #include <ac_fixed.h>
//    #include <ac_math/ac_cholinv.h>
//
//    // Define data types for input and output matrices
//    typedef ac_fixed<30, 15, true, AC_RND, AC_SAT> input_type;
//    typedef ac_fixed<64, 32, true, AC_RND, AC_SAT> output_type;
//
//    #pragma hls_design top
//    void CCS_BLOCK(project)(
//      const input_type &input,
//      output_type &output
//    )
//    {
//      ac_cholinv(input, output);
//    }
//
//    #ifndef __BAMBU__
//    CCS_MAIN(int arg, char **argc)
//    {
//      input_type input[7][7];
//      output_type output[7][7];
//      //Hypothetical function that generates a pos. def. matrix
//      gen_pos_def_matrix(input);
//      CCS_DESIGN(project)(input, output);
//      CCS_RETURN (0);
//    }
//    #endif
//
// Notes:
//    A runtime error is thrown by AC_ASSERT if the input matrix is not
//    positive definite. On top of that, there's also a functionality to
//    return a matrix of zeros if the input matrix is not positive
//    definite, in case the AC_ASSERT cannot provide an indication of that.
//    The above functionality is defined in the ac_chol_d.h file.
//
//------------------------------------------------------------------------------

namespace ac_math
{
   template <bool use_pwl1 = false, bool use_pwl2 = false, int add2w = 0, int add2i = 0, ac_q_mode temp_Q = AC_RND, ac_o_mode temp_O = AC_SAT, unsigned M, int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int outW, int outI, bool outS, ac_q_mode outQ,
             ac_o_mode outO>
   void ac_cholinv(const ac_fixed<W, I, S, Q, O> A[M][M], ac_fixed<outW, outI, outS, outQ, outO> Ainv[M][M])
   {
      typedef ac_fixed<outW, outI, outS, outQ, outO> T_out;
      // Define type for the intermediate variables
      typedef ac_fixed<T_out::width + add2w, T_out::i_width + add2i, T_out::sign, temp_Q, temp_O> Tout;
      // typedef ac_matrix<Tout, M, M> outputm_type;
      Tout recip_L;
      Tout sum;
      Tout Linv[M][M];
      Tout L[M][M];
      ac_math::ac_chol_d<use_pwl1>(A, L);

   // Using Forward decomposition to calculate the inverse of the lower triangular matrix returned by the ac_chol_d file
   L_Linv_COL:
      for(unsigned i = 0; i < M; i++)
      {
         if(use_pwl2)
         {
            ac_math::ac_reciprocal_pwl(L[i][i], recip_L);
         }
         else
         {
            static const ac_fixed<1 + T_out::sign, 1 + T_out::sign, T_out::sign> unity = 1;
            ac_math::ac_div(unity, L[i][i], recip_L);
         }
         // Diagonal elements of Linv are reciprocal of L matrix
         Linv[i][i] = recip_L;
      L_Linv_ROW:
         for(unsigned j = 0; j < M; j++)
         {
            if(j >= i + 1)
            {
               sum = 0;
            L_COL_Linv_ROW:
               for(unsigned k = 0; k < M; k++)
               {
                  sum += (k < j & k >= i) ? (Tout)(L[j][k] * Linv[k][i]) : (Tout)0;
                  if(k >= j)
                  {
                     break;
                  }
               }
               if(use_pwl2)
               {
                  ac_math::ac_reciprocal_pwl(L[j][j], recip_L);
               }
               else
               {
                  static const ac_fixed<1 + T_out::sign, 1 + T_out::sign, T_out::sign> unity = 1;
                  ac_math::ac_div(unity, L[j][j], recip_L);
               }
               Linv[j][i] = -(sum * recip_L);
            }
         }
      }

   // Multiplying the inverse lower triangular matrix by its conjugate transpose to get the inverse of the input matrix
   Linvtrans_ROW:
      for(unsigned i = 0; i < M; i++)
      Linv_ROW:
         for(unsigned j = 0; j < M; j++)
         {
            sum = 0;
         Linv_Linvtrans_COL:
            for(unsigned k = 0; k < M; k++)
            {
               sum += (k < j | k < i) ? (Tout)0 :  (Tout)(Linv[k][i] * Linv[k][j]);
            }
            Ainv[i][j] = sum;
         }
   }

   //=====================================================================================
   // Function: ac_cholinv (for ac_complex<ac_fixed> using native C-style arrays)
   //
   // Description:
   //    Matrix inversion using cholesky decomposition of complex-valued matrices with
   //    ac_complex<ac_fixed> variables.
   //
   // Usage:
   //    A sample testbench and its implementation look like this:
   //
   //    #include <mc_scverify.h>
   //    #include <ac_complex.h>
   //    #include <ac_fixed.h>
   //    #include <ac_math/ac_cholinv.h>
   //
   //    // Define data types for input and output matrices
   //    typedef ac_complex<ac_fixed<30, 15, true, AC_RND, AC_SAT> > input_type;
   //    typedef ac_complex<ac_fixed<64, 32, true, AC_RND, AC_SAT> > output_type;
   //
   //    #pragma hls_design top
   //    void CCS_BLOCK(project)(
   //      const input_type &input,
   //      output_type &output
   //    )
   //    {
   //      ac_cholinv(input, output);
   //    }
   //
   //    #ifndef __BAMBU__
   //    CCS_MAIN(int arg, char **argc)
   //    {
   //      input_type input[7][7];
   //      output_type output[7][7];
   //      //Hypothetical function that generates a pos. def. matrix
   //      gen_pos_def_matrix(input);
   //      CCS_DESIGN(project)(input, output);
   //      CCS_RETURN (0);
   //    }
   //    #endif
   //
   // Notes:
   //    A runtime error is thrown by AC_ASSERT if the input matrix is not
   //    positive definite. On top of that, there's also a functionality to
   //    return a matrix of zeros if the input matrix is not positive
   //    definite, in case the AC_ASSERT cannot provide an indication of that.
   //    The above functionality is defined in the ac_chol_d.h file.
   //
   //--------------------------------------------------------------------------------------

   template <bool use_pwl1 = false, bool use_pwl2 = false, int add2w = 0, int add2i = 0, ac_q_mode temp_Q = AC_RND, ac_o_mode temp_O = AC_SAT, unsigned M, int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int outW, int outI, bool outS, ac_q_mode outQ,
             ac_o_mode outO>
   void ac_cholinv(const ac_complex<ac_fixed<W, I, S, Q, O>> A[M][M], ac_complex<ac_fixed<outW, outI, outS, outQ, outO>> Ainv[M][M])
   {
      typedef ac_fixed<outW, outI, outS, outQ, outO> T_out;
      typedef ac_complex<ac_fixed<T_out::width + add2w, T_out::i_width + add2i, T_out::sign, temp_Q, temp_O>> Tout;
      // typedef ac_matrix<Tout, M, M> outputm_type;
      Tout recip_L;
      Tout sum;
      Tout Linv[M][M];
      Tout L[M][M];
      ac_math::ac_chol_d<use_pwl1>(A, L);

   // Using Forward decomposition to calculate the inverse of the lower triangular matrix returned by the ac_chol_d file
   L_Linv_COL:
      for(unsigned i = 0; i < M; i++)
      {
         if(use_pwl2)
         {
            ac_math::ac_reciprocal_pwl(L[i][i], recip_L);
         }
         else
         {
            static const ac_fixed<1 + T_out::sign, 1 + T_out::sign, T_out::sign> unity = 1;
            ac_math::ac_div(unity, L[i][i].r(), recip_L.r());
            recip_L.i() = 0;
         }
         // Diagonal elements of Linv are reciprocal of L matrix
         Linv[i][i] = recip_L;
      L_Linv_ROW:
         for(unsigned j = 0; j < M; j++)
         {
            if(j >= i + 1)
            {
               sum = 0;
            L_COL_Linv_ROW:
               for(unsigned k = 0; k < M; k++)
               {
                  sum += (k < j & k >= i) ? L[j][k] * Linv[k][i] : 0;
                  if(k >= j)
                  {
                     break;
                  }
               }
               if(use_pwl2)
               {
                  ac_math::ac_reciprocal_pwl(L[j][j], recip_L);
               }
               else
               {
                  static const ac_fixed<1 + T_out::sign, 1 + T_out::sign, T_out::sign> unity = 1;
                  ac_math::ac_div(unity, L[j][j].r(), recip_L.r());
                  recip_L.i() = 0;
               }
               Linv[j][i] = -(sum * recip_L);
            }
         }
      }
   // Multiplying the inverse lower triangular matrix by its conjugate transpose to get the inverse of the input matrix
   Linvtrans_ROW:
      for(unsigned i = 0; i < M; i++)
      Linv_ROW:
         for(unsigned j = 0; j < M; j++)
         {
            sum = 0;
         Linv_Linvtrans_COL:
            for(unsigned k = 0; k < M; k++)
            {
               sum += (k < j | k < i) ? 0 : Linv[k][i].conj() * Linv[k][j];
            }
            Ainv[i][j] = sum;
         }
   }
} // namespace ac_math

//============================================================================
// Function: ac_matrix_cholinv
// Helper function for using ac_cholinv on ac_matrix objects

template <bool use_pwl1 = false, bool use_pwl2 = false, int add2w = 0, int add2i = 0, ac_q_mode temp_Q = AC_RND, ac_o_mode temp_O = AC_SAT, unsigned M1, class T1, class T2>
void ac_matrix_cholinv(const ac_matrix<T1, M1, M1>& input, ac_matrix<T2, M1, M1>& output)
{
   // Extract 2D array member data, and pass it over to the 2D array implementation.
   ac_math::ac_cholinv<use_pwl1, use_pwl2, add2w, add2i, temp_Q, temp_O>(input.m_data, output.m_data);
}

namespace ac_math
{
   //=============================================================================
   // Function: ac_cholinv (for ac_fixed using ac_matrix 2-D storage class)
   //
   // Description:
   //    Matrix inversion using cholesky decomposition of real-valued matrices with
   //    ac_fixed variables.
   //
   // Usage:
   //    A sample testbench and its implementation look like this:
   //
   //    #include <mc_scverify.h>
   //    #include <ac_matrix.h>
   //    #include <ac_fixed.h>
   //    #include <ac_math/ac_cholinv.h>
   //
   //    typedef ac_matrix<ac_fixed<30, 15, true, AC_RND, AC_SAT>, 7, 7> input_type;
   //    typedef ac_matrix<ac_fixed<64, 32, true, AC_RND, AC_SAT>, 7, 7> output_type;
   //
   //    #pragma hls_design top
   //    void CCS_BLOCK(project)(
   //      const input_type &input,
   //      output_type &output
   //    )
   //    {
   //      ac_cholinv(input, output);
   //    }
   //
   //    #ifndef __BAMBU__
   //    CCS_MAIN(int arg, char **argc)
   //    {
   //      input_type input;
   //      output_type output;
   //      //Hypothetical function that generates a pos. def. matrix
   //      gen_pos_def_matrix(input);
   //      CCS_DESIGN(project)(input, output);
   //      CCS_RETURN (0);
   //    }
   //    #endif
   //
   // Notes:
   //    This version uses the C++ 2D array implementation for it's functioning.
   //    It does this by using indirect_chol_d friend function from the
   //    ac_matrix class to pass the 2D array member data to the relevant
   //    implementation.
   //
   //-----------------------------------------------------------------------------

   template <bool use_pwl1 = false, bool use_pwl2 = false, int add2w = 0, int add2i = 0, ac_q_mode temp_Q = AC_RND, ac_o_mode temp_O = AC_SAT, unsigned M, int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int outW, int outI, bool outS, ac_q_mode outQ,
             ac_o_mode outO>
   void ac_cholinv(const ac_matrix<ac_fixed<W, I, S, Q, O>, M, M>& A, ac_matrix<ac_fixed<outW, outI, outS, outQ, outO>, M, M>& Ainv)
   {
      ac_matrix_cholinv<use_pwl1, use_pwl2, add2w, add2i, temp_Q, temp_O>(A, Ainv);
   }

   //===============================================================================================
   // Function: ac_cholinv (for ac_complex<ac_fixed> using ac_matrix 2-D storage class)
   //
   // Description:
   //    Matrix inversion using cholesky decomposition of complex-valued matrices with
   //    ac_complex<ac_fixed> variables.
   //
   // Usage:
   //    A sample testbench and its implementation look like this:
   //
   //    #include <mc_scverify.h>
   //    #include <ac_matrix.h>
   //    #include <ac_fixed.h>
   //    #include <ac_math/ac_cholinv.h>
   //
   //    typedef ac_matrix<ac_complex<ac_fixed<30, 15, true, AC_RND, AC_SAT> >, 7, 7> input_type;
   //    typedef ac_matrix<ac_complex<ac_fixed<64, 32, true, AC_RND, AC_SAT> >, 7, 7> output_type;
   //
   //    #pragma hls_design top
   //    void CCS_BLOCK(project)(
   //      const input_type &input,
   //      output_type &output
   //    )
   //    {
   //      ac_cholinv(input, output);
   //    }
   //
   //    #ifndef __BAMBU__
   //    CCS_MAIN(int arg, char **argc)
   //    {
   //      input_type input;
   //      output_type output;
   //      //Hypothetical function that generates a pos. def. matrix
   //      gen_pos_def_matrix(input);
   //      CCS_DESIGN(project)(input, output);
   //      CCS_RETURN (0);
   //    }
   //    #endif
   //
   // Notes:
   //    This version uses the C++ 2D array implementation for it's functioning.
   //    It does this by using indirect_chol_d friend function from the
   //    ac_matrix class to pass the 2D array member data to the relevant
   //    implementation.
   //-------------------------------------------------------------------------

   template <bool use_pwl1 = false, bool use_pwl2 = false, int add2w = 0, int add2i = 0, ac_q_mode temp_Q = AC_RND, ac_o_mode temp_O = AC_SAT, unsigned M, int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int outW, int outI, bool outS, ac_q_mode outQ,
             ac_o_mode outO>
   void ac_cholinv(const ac_matrix<ac_complex<ac_fixed<W, I, S, Q, O>>, M, M>& A, ac_matrix<ac_complex<ac_fixed<outW, outI, outS, outQ, outO>>, M, M>& Ainv)
   {
      ac_matrix_cholinv<use_pwl1, use_pwl2, add2w, add2i, temp_Q, temp_O>(A, Ainv);
   }

} // namespace ac_math

#endif
