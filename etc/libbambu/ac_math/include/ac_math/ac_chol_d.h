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
// *******************************************************************************************
// File: ac_chol_d.h
//
// Description: Provides an implementation for the Cholesky Decomposition of ac_fixed
//    and ac_complex<ac_fixed> matrices.
//
// Usage:
//    (see the type-specific examples below)
//
// Notes:
//    This file uses C++ function overloading to target implementations specific to each type
//    of data. Attempting to call the function with a type that is not implemented will result
//    in a compile error.
//
//    The user may choose to use the PWL functions for the internal calculations or they may
//    choose to use the accurate div and sqrt functions provided in the ac_math library,
//    using an optional template parameter. Please refer to the documentation for more details.
//
//    This library uses the ac_inverse_sqrt_pwl(), ac_sqrt_pwl(), ac_div() and ac_sqrt()
//    functions from the ac_math header files.
//
// Revision History:
//    Niramay Sanghvi : Nov 24 2017 : Used friend function to handle ac_matrix inputs/outputs.
//    Niramay Sanghvi : Nov 12 2017 : Added overloaded functions to handle standard C arrays.
//    Niramay Sanghvi : Oct 02 2017 : Incorporated the use of the inverse_sqrt PWL function.
//    Niramay Sanghvi : Aug 10 2017 : Added template parameters for precision configuration.
//    Niramay Sanghvi : Aug 09 2017 : Made output go to zero for non-positive def. matrix.
//    Niramay Sanghvi : Jul 28 2017 : Added choice between PWL and accurate functions.
//
// *******************************************************************************************

#if __cplusplus < 201103L
#error Please use C++11 or a later standard for compilation.
#endif

#ifndef _INCLUDED_AC_CHOL_D_H_
#define _INCLUDED_AC_CHOL_D_H_

// Include headers for data types supported by these implementations
#include <ac_complex.h>
#include <ac_fixed.h>
#include <ac_matrix.h>

// Include headers for required functions
#include <ac_math/ac_div.h>
#include <ac_math/ac_inverse_sqrt_pwl.h>
#include <ac_math/ac_sqrt.h>
#include <ac_math/ac_sqrt_pwl.h>

#if !defined(__BAMBU__) && defined(AC_CHOL_D_H_DEBUG)
#include <iostream>
using namespace std;
#endif

// =========================================================================
// Function: ac_chol_d (for ac_fixed using native C-style arrays)
//
// Description:
//    Calculation of the Cholesky Decomposition of real-valued matrices of
//    ac_fixed variables.
//
//    The Cholesky-Crout algorithm is used for Cholesky Decomposition.
//    By default, all temporary variables use the same precision as the
//    output. The user can change that by specifying number of bits to be
//    added or taken away from this default value. The user can also
//    add extra template parameters change the rounding and saturation
//    modes of the intermediate variables (by default, the rounding mode is
//    AC_RND and saturation mode is AC_SAT).
//
//    The user can also choose between using PWL vs. using accurate
//    division/sqrt functions, as mentioned earlier.
//
// Usage:
//    A sample implementation and its testbench looks like this:
//
//    #include <ac_fixed.h>
//    #include <ac_math/ac_chol_d.h>
//    using namespace ac_math;
//
//    // Define data types for input and output matrices
//    typedef ac_fixed<41, 21, true, AC_RND, AC_SAT> input_type;
//    typedef ac_fixed<64, 32, true, AC_RND, AC_SAT> output_type;
//    const unsigned M = 7;
//
//    #pragma hls_design top
//    void project(
//      const input_type input[M][M],
//      output_type output[M][M]
//    )
//    {
//      ac_chol_d(input, output);
//    }
//
//    #ifndef __BAMBU__
//    #include <mc_scverify.h>
//    CCS_MAIN(int arg, char **argc)
//    {
//      input_type input[7][7];
//      output_type output[7][7];
//      // Hypothetical function that generates a pos. def. matrix:
//      gen_pos_def_matrix(input);
//      CCS_DESIGN(project)(input, output);
//      CCS_RETURN (0);
//    }
//    #endif
//
// Notes:
//    A runtime error is thrown by AC_ASSERT if the input matrix is not
//    positive definite. Additionally the function will return a matrix
//    of all zeros if the input matrix is not positive definite, in case
//    the AC_ASSERT cannot provide an indication of that.
//
// -------------------------------------------------------------------------

namespace ac_math
{
   template <bool use_pwl = false, int delta_w = 0, int delta_i = 0, ac_q_mode int_Q = AC_RND, ac_o_mode int_O = AC_SAT, int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int outW, int outI, bool outS, ac_q_mode outQ, ac_o_mode outO, unsigned M>
   void ac_chol_d(const ac_fixed<W, I, S, Q, O> A[M][M], ac_fixed<outW, outI, outS, outQ, outO> L[M][M])
   {
      // Make this go false if the input matrix is not positive definite (checking is done later)
      bool pos_def = true;

      typedef ac_fixed<outW, outI, outS, outQ, outO> T_out;
      T_out L_int[M][M];
      // Define type for the intermediate variables
      // Add an extra bit to W and I for intermediate variable type if the output is unsigned, and make sure that i_s_t is signed.
      typedef class ac_fixed<outW + delta_w, outI + delta_i, true, int_Q, int_O> i_s_t;
      // Unsigned versions of i_s_t and T_out
      typedef ac_fixed<outW, outI, false, outQ, outO> T_out_u;
      typedef ac_fixed<i_s_t::width, i_s_t::i_width, false, i_s_t::q_mode, i_s_t::o_mode> i_s_t_u;

   ARRAY_AC_FIXED_L_COL:
      for(unsigned j = 0; j < M; j++)
      {
         i_s_t sum_Ajj_Ljk_sq;
         sum_Ajj_Ljk_sq = A[j][j];
      ARRAY_AC_FIXED_LJJ_K:
         for(unsigned k = 0; k < M; k++)
         {
            sum_Ajj_Ljk_sq -= (k < j) ? (i_s_t)(L_int[j][k] * L_int[j][k]) : (i_s_t)0;
         }

         // Use a macro to activate the AC_ASSERT
#ifdef ASSERT_ON_INVALID_INPUT
         // Check to make sure that the matrix is positive definite. If "sum_Ajj_Ljk_sq" is negative/zero, then the diagonal
         // element will be complex/infinite, which is not valid. This condition will not be encountered if the
         // input matrix is positive definite
         AC_ASSERT(sum_Ajj_Ljk_sq > 0, "Input matrix is not positive definite");
#endif
         if(sum_Ajj_Ljk_sq <= 0)
         {
            pos_def = false;
         }
         i_s_t_u recip_Ljj;
         T_out_u int_Ljj;
         // Compute values for and initialize diagonal elements using PWL/accurate math functions, as may be the case.
         if(use_pwl)
         {
            // Use the PWL functions
            ac_math::ac_sqrt_pwl((i_s_t_u)sum_Ajj_Ljk_sq, int_Ljj);
            L_int[j][j] = int_Ljj;
            // Store inverse of diagonal element in separate variable (i.e. "recip_Ljj") for later calculations.
            ac_math::ac_inverse_sqrt_pwl((i_s_t_u)sum_Ajj_Ljk_sq, recip_Ljj);
         }
         else
         {
            // Use accurate math functions.
            ac_math::ac_sqrt((i_s_t_u)sum_Ajj_Ljk_sq, int_Ljj);
            L_int[j][j] = int_Ljj;
            // Make sure that every variable to be passed to the div function has the same sign.
            static const ac_fixed<1, 1, false> unity = 1;
            // Store inverse of diagonal element in separate variable (i.e. "recip_Ljj") for later calculations.
            ac_math::ac_div(unity, int_Ljj, recip_Ljj);
         }
#if !defined(__BAMBU__) && defined(AC_CHOL_D_H_DEBUG)
         cout << "FILE : " << __FILE__ << ", LINE : " << __LINE__ << endl;
         cout << "sum_Ajj_Ljk_sq = " << sum_Ajj_Ljk_sq << endl;
         cout << "int_Ljj        = " << int_Ljj << endl;
         cout << "recip_Ljj      = " << recip_Ljj << endl;
#endif

      // Initializing non-diagonal elements.
      ARRAY_AC_FIXED_L_ROW:
         for(unsigned i = 0; i < M; i++)
         {
            i_s_t sum_Aij_Lik_Ljk;
            sum_Aij_Lik_Ljk = A[i][j];
         ARRAY_AC_FIXED_LIJ_K:
            for(unsigned k = 0; k < M; k++)
            {
               sum_Aij_Lik_Ljk -= (k < j) ? (i_s_t)(L_int[i][k] * L_int[j][k]) : (i_s_t)0;
            }
            // Keep diagonal elements as they are
            // Initialize non-diagonal elements above diagonal to 0
            // Initialize non-diagonal elements below diagonal to appropriate calculated value.
            if(i != j)
            {
               L_int[i][j] = (i > j) ? (T_out)(recip_Ljj * sum_Aij_Lik_Ljk) : (T_out)0;
            }
         }
      }

      if(!pos_def)
      {
      // This functionality is provided in case the input matrix is not positive definite, and the AC_ASSERT
      // did not kick in, for whatever reason. In such a case, the output matrix should be set to zero.
      ARRAY_AC_FIXED_SET_OUTPUT_ZERO_ROW:
         for(unsigned i = 0; i < M; i++)
         {
         ARRAY_AC_FIXED_SET_OUTPUT_ZERO_COL:
            for(unsigned j = 0; j < M; j++)
            {
               L[i][j] = 0;
            }
         }
      }
      else
      {
      // If the array is positive definite, copy over the contents of the intermediate array to the final
      // output array.
      ARRAY_AC_FIXED_COPY_INT_ROW:
         for(unsigned i = 0; i < M; i++)
         {
         ARRAY_AC_FIXED_COPY_INT_COL:
            for(unsigned j = 0; j < M; j++)
            {
               L[i][j] = L_int[i][j];
            }
         }
      }
   }

   // ==============================================================================================
   // Function: ac_chol_d (for ac_complex<ac_fixed> using native C-style arrays)
   //
   // Description:
   //    Calculation of the Cholesky Decomposition of complex-valued matrices
   //    with ac_complex<ac_fixed> variables.
   //
   //    This function can also be configured to change the type for the
   //    intermediate variables, and to use the pwl/accurate math functions for
   //    intermediate computations, just like the ac_fixed version.
   //
   // Usage:
   //    A sample implementation and its testbench looks like this:
   //
   //    #include <ac_fixed.h>
   //    #include <ac_math/ac_chol_d.h>
   //    using namespace ac_math;
   //
   //    // Define data types for input and output matrices
   //    typedef ac_complex<ac_fixed<41, 21, true, AC_RND, AC_SAT> > input_type;
   //    typedef ac_complex<ac_fixed<64, 32, true, AC_RND, AC_SAT> > output_type;
   //    const unsigned M = 7;
   //
   //    #pragma hls_design top
   //    void project(
   //      const input_type input[M][M],
   //      output_type output[M][M]
   //    )
   //    {
   //      ac_chol_d(input, output);
   //    }
   //
   //    #ifndef __BAMBU__
   //    #include <mc_scverify.h>
   //    CCS_MAIN(int arg, char **argc)
   //    {
   //      input_type input[M][M];
   //      output_type output[M][M];
   //      gen_pos_def_matrix(input); // Hypothetical function that generates a pos. def. matrix
   //      CCS_DESIGN(project)(input, output);
   //      CCS_RETURN (0);
   //    }
   //    #endif
   //
   // Notes:
   //    Similar to the ac_fixed version, the ac_complex version also has an AC_ASSERT and a backup
   //    functionality to return a matrix of zeros if the input matrix is not positive definite.
   //
   //    As the diagonal elements of the output matrix are always real, the calculations involved
   //    are optimized to ensure that only the real part of diagonal elements is ever calculated,
   //    and only the real part is used for future calculations. The imaginary part is always set
   //    to zero.
   //
   // ----------------------------------------------------------------------------------------------

   template <bool use_pwl = false, int delta_w = 0, int delta_i = 0, ac_q_mode int_Q = AC_RND, ac_o_mode int_O = AC_SAT, int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int outW, int outI, bool outS, ac_q_mode outQ, ac_o_mode outO, unsigned M>
   void ac_chol_d(const ac_complex<ac_fixed<W, I, S, Q, O>> A[M][M], ac_complex<ac_fixed<outW, outI, outS, outQ, outO>> L[M][M])
   {
      // Make this go false if the input matrix is not positive definite (checking is done later)
      bool pos_def = true;

      ac_complex<ac_fixed<outW, outI, outS, outQ, outO>> zero_complex(0, 0);
      typedef ac_fixed<outW, outI, outS, outQ, outO> T_out;
      ac_complex<T_out> L_int[M][M];
      // Define type for the intermediate variables
      // Add an extra bit to W and I for intermediate variable type if the output is unsigned, and make sure that i_s_t is signed.
      typedef ac_fixed<outW + delta_w, outI + delta_i, true, int_Q, int_O> i_s_t;
      typedef ac_fixed<outW, outI, false, outQ, outO> T_out_fixed_u;
      typedef ac_fixed<i_s_t::width, i_s_t::i_width, false, i_s_t::q_mode, i_s_t::o_mode> i_s_t_u;

   ARRAY_AC_COMP_AC_FIXED_L_COL:
      for(unsigned j = 0; j < M; j++)
      {
         i_s_t sum_Ajj_Ljk_sq;
         sum_Ajj_Ljk_sq = A[j][j].r();
      ARRAY_AC_COMP_AC_FIXED_LJJ_K:
         for(unsigned k = 0; k < M; k++)
         {
            sum_Ajj_Ljk_sq -= (k < j) ? (i_s_t)(L_int[j][k].mag_sqr()) : (i_s_t)0;
         }

         // Use a macro to activate the AC_ASSERT
#ifdef ASSERT_ON_INVALID_INPUT
         // Check to make sure that the input matrix is positive definite. If "sum_Ajj_Ljk_sq" is negative/zero, then
         // the diagonal element will be complex/infinite, which is not valid. This condition will not be encountered if the
         // input matrix is positive definite
         AC_ASSERT(sum_Ajj_Ljk_sq > 0, "Input matrix is not positive definite");
#endif

         if(sum_Ajj_Ljk_sq <= 0)
         {
            pos_def = false;
         }

         i_s_t_u recip_Ljj;
         T_out_fixed_u int_Ljj;

         // Compute values for and initialize diagonal elements using PWL/accurate math functions, as may be the case.
         if(use_pwl)
         {
            // Use the PWL functions
            // Initialize diagonal elements. Since the diagonal elements are real, initialize the imaginary part to 0.
            // Only bother with the real part, as the diagonal elements of the decomposed matrix are always real.
            ac_math::ac_sqrt_pwl((i_s_t_u)sum_Ajj_Ljk_sq, int_Ljj);
            L_int[j][j].r() = int_Ljj;
            L_int[j][j].i() = 0;
            // Store inverse of real part of diagonal element in separate variable (i.e. "recip_Ljj") for later calculations.
            ac_math::ac_inverse_sqrt_pwl((i_s_t_u)sum_Ajj_Ljk_sq, recip_Ljj);
         }
         else
         {
            // Use accurate math functions.
            // Only bother with the real part, as the diagonal elements of the decomposed matrix are always real.
            ac_math::ac_sqrt((i_s_t_u)sum_Ajj_Ljk_sq, int_Ljj);
            L_int[j][j].r() = int_Ljj;
            L_int[j][j].i() = 0;
            // Make sure that every variable to be passed to the div function has the same sign.
            static const ac_fixed<1, 1, false> unity = 1;
            // Store inverse of diagonal element in separate variable (i.e. "recip_Ljj") for later calculations.
            ac_math::ac_div(unity, int_Ljj, recip_Ljj);
         }
#if !defined(__BAMBU__) && defined(AC_CHOL_D_H_DEBUG)
         cout << "FILE : " << __FILE__ << ", LINE : " << __LINE__ << endl;
         cout << "j = " << j << endl;
         cout << "sum_Ajj_Ljk_sq = " << sum_Ajj_Ljk_sq << endl;
         cout << "int_Ljj        = " << int_Ljj << endl;
         cout << "recip_Ljj      = " << recip_Ljj << endl;
#endif

      // Initializing non-diagonal elements.
      ARRAY_AC_COMP_AC_FIXED_L_ROW:
         for(unsigned i = 0; i < M; i++)
         {
            ac_complex<i_s_t> sum_Aij_Lik_Ljk;
            sum_Aij_Lik_Ljk = A[i][j];
         ARRAY_AC_COMP_AC_FIXED_LIJ_K:
            for(unsigned k = 0; k < M; k++)
            {
               sum_Aij_Lik_Ljk -= (k < j) ? (ac_complex<i_s_t> )(L_int[i][k] * L_int[j][k].conj()) : (ac_complex<i_s_t> )0;
            }
            // Keep diagonal elements as they are
            // Initialize non-diagonal elements above diagonal to 0
            // Initialize non-diagonal elements below diagonal to appropriate calculated value.
            if(i != j)
            {
               L_int[i][j].r() = (i > j) ? (T_out)(recip_Ljj * sum_Aij_Lik_Ljk.r()) : (T_out)0;
               L_int[i][j].i() = (i > j) ? (T_out)(recip_Ljj * sum_Aij_Lik_Ljk.i()) : (T_out)0;
            }
         }
      }

      if(!pos_def)
      {
      // This functionality is provided in case the input matrix is not positive definite, and the AC_ASSERT
      // did not kick in, for whatever reason. In such a case, the output matrix should be set to zero.
      ARRAY_AC_COMP_AC_FIXED_SET_OUTPUT_ZERO_ROW:
         for(unsigned i = 0; i < M; i++)
         {
         ARRAY_AC_COMP_AC_FIXED_SET_OUTPUT_ZERO_COL:
            for(unsigned j = 0; j < M; j++)
            {
               L[i][j].r() = 0;
               L[i][j].i() = 0;
            }
         }
      }
      else
      {
      // If the array is positive definite, copy over the contents of the intermediate array to the final
      // output array.
      ARRAY_AC_COMP_AC_FIXED_COPY_INT_ROW:
         for(unsigned i = 0; i < M; i++)
         {
         ARRAY_AC_COMP_AC_FIXED_COPY_INT_COL:
            for(unsigned j = 0; j < M; j++)
            {
               L[i][j] = L_int[i][j];
            }
         }
      }
   }
} // namespace ac_math

// =========================================================================
// Function: indirect_chol_d
// Helper function for using ac_chol_d on ac_matrix objects

template <bool use_pwl = false, int delta_w = 0, int delta_i = 0, ac_q_mode int_Q = AC_RND, ac_o_mode int_O = AC_SAT, class T1, unsigned M1, class T2>
void indirect_chol_d(const ac_matrix<T1, M1, M1>& input, ac_matrix<T2, M1, M1>& output)
{
   // Extract 2D array member data, and pass it over to the 2D array implementation.
   ac_math::ac_chol_d<use_pwl, delta_w, delta_i, int_Q, int_O>(input.m_data, output.m_data);
}

namespace ac_math
{
   // ===============================================================================
   // Function: ac_chol_d (for ac_fixed using ac_matrix 2-D storage class)
   //
   // Description:
   //    Calculation of the Cholesky Decomposition of real-valued matrices of
   //    ac_fixed variables.
   //
   //    The Cholesky-Crout algorithm is used for Cholesky Decomposition.
   //    By default, all temporary variables use the same precision as the
   //    output. The user can change that by specifying number of bits to be
   //    added or taken away from this default value. The user can also
   //    add extra template parameters to turn off rounding and saturation
   //    and rounding for temp. variables (these are turned on by default).
   //
   //    The user can also choose between using PWL vs. using accurate
   //    division/sqrt functions, as mentioned earlier.
   //
   // Usage:
   //    A sample implementation and its testbench looks like this:
   //
   //    #include <ac_fixed.h>
   //    #include <ac_math/ac_chol_d.h>
   //    #include <ac_matrix.h>
   //
   //    // Define data types for input and output matrices
   //    typedef ac_matrix<ac_fixed<41, 21, true, AC_RND, AC_SAT>, 7, 7> input_type;
   //    typedef ac_matrix<ac_fixed<64, 32, true, AC_RND, AC_SAT>, 7, 7> output_type;
   //
   //    #pragma hls_design top
   //    void project(
   //      const input_type &input,
   //      output_type &output
   //    )
   //    {
   //      ac_chol_d(input, output);
   //    }
   //
   //    #ifndef __BAMBU__
   //    #include <mc_scverify.h>
   //    CCS_MAIN(int arg, char **argc)
   //    {
   //      input_type input;
   //      output_type output;
   //      // Hypothetical function that generates a pos. def. matrix:
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
   // -------------------------------------------------------------------------------

   template <bool use_pwl = false, int delta_w = 0, int delta_i = 0, ac_q_mode int_Q = AC_RND, ac_o_mode int_O = AC_SAT, int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int outW, int outI, bool outS, ac_q_mode outQ, ac_o_mode outO, unsigned M>
   void ac_chol_d(const ac_matrix<ac_fixed<W, I, S, Q, O>, M, M>& A, ac_matrix<ac_fixed<outW, outI, outS, outQ, outO>, M, M>& L)
   {
      indirect_chol_d<use_pwl, delta_w, delta_i, int_Q, int_O>(A, L);
   }

   // ==============================================================================================
   // Function: ac_chol_d (for ac_complex<ac_fixed> using ac_matrix 2-D storage class)
   //
   // Description:
   //    Calculation of the Cholesky Decomposition of complex-valued matrices
   //    with ac_complex<ac_fixed> variables.
   //
   //    This function can also be configured to change the type for the
   //    temporary variables, and to use the pwl/accurate math functions for
   //    intermediate computations, just like the ac_fixed version.
   //
   // Usage:
   //    A sample implementation and its testbench looks like this:
   //
   //    #include <ac_fixed.h>
   //    #include <ac_math/ac_chol_d.h>
   //    using namespace ac_math;
   //    #include <ac_matrix.h>
   //
   //    // Define data types for input and output matrices
   //    typedef ac_matrix<ac_complex<ac_fixed<41, 21, true, AC_RND, AC_SAT> >, 7, 7> input_type;
   //    typedef ac_matrix<ac_complex<ac_fixed<64, 32, true, AC_RND, AC_SAT> >, 7, 7> output_type;
   //
   //    #pragma hls_design top
   //    void project(
   //      const input_type &input,
   //      output_type &output
   //    )
   //    {
   //      ac_chol_d(input, output);
   //    }
   //
   //    #ifndef __BAMBU__
   //    #include <mc_scverify.h>
   //    CCS_MAIN(int arg, char **argc)
   //    {
   //      input_type input;
   //      output_type output;
   //      // Hypothetical function that generates a pos. def. matrix:
   //      gen_pos_def_matrix(input);
   //      CCS_DESIGN(project)(input, output);
   //      CCS_RETURN (0);
   //    }
   //    #endif
   //
   // Notes:
   //    This version uses the C++ 2D array implementation for its functioning.
   //    It does this by using indirect_chol_d friend function from the
   //    ac_matrix class to pass the 2D array member data to the relevant
   //    implementation.
   //
   // ----------------------------------------------------------------------------------------------

   template <bool use_pwl = false, int delta_w = 0, int delta_i = 0, ac_q_mode int_Q = AC_RND, ac_o_mode int_O = AC_SAT, int W, int I, bool S, ac_q_mode Q, ac_o_mode O, int outW, int outI, bool outS, ac_q_mode outQ, ac_o_mode outO, unsigned M>
   void ac_chol_d(const ac_matrix<ac_complex<ac_fixed<W, I, S, Q, O>>, M, M>& A, ac_matrix<ac_complex<ac_fixed<outW, outI, outS, outQ, outO>>, M, M>& L)
   {
      indirect_chol_d<use_pwl, delta_w, delta_i, int_Q, int_O>(A, L);
   }

} // namespace ac_math

#endif
