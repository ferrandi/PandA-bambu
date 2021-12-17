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
//*******************************************************************************
// File: ac_matrixmul.h
//
// Description:
//    Matrix multiplication for ac_fixed and ac_complex types with full precision
//
// Usage:
//    A sample testbench and its implementation look like
//    this:
//    #include <ac_fixed.h>
//    #include <ac_complex.h>
//    #include <ac_math/ac_matrixmul.h>
//    #include <mc_scverify.h>
//
//    typedef ac_matrix<ac_fixed<20, 10, true, AC_RND, AC_SAT>, M, N> input_type1;
//    typedef ac_matrix<ac_fixed<20, 10, true, AC_RND, AC_SAT>, N, P> input_type2;
//    typedef ac_matrix<ac_fixed<61, 31, true, AC_RND, AC_SAT>, M, P> output_type;
//
//    #pragma hls_design top
//    void CCS_BLOCK(project)(
//    const input_type1 &A,
//    const input_type2 &B,
//    output_type &C
//    )
//    {
//        ac_matrixmul(A,B,C);
//    }
//    #ifndef __BAMBU__
//    CCS_MAIN(int argc, char *argv[])
//    {
//        input_type1 A;
//        input_type2 B;
//        output_type C;
//        gen_matrix(A, B);
//        CCS_DESIGN(project)(A, B, C);
//        CCS_RETURN (0);
//    }
//    #endif
//
//    For a more detailed example, please refer to the "ac_matrixmul_tb.cpp" and
//    "ac_matrixmul_tb.h" files
//
//***************************************************************************

#ifndef _INCLUDED_AC_MATRIXMUL_H_
#define _INCLUDED_AC_MATRIXMUL_H_

// Include headers for data types supported by these implementations
#include <ac_complex.h>
#include <ac_fixed.h>
#include <ac_matrix.h>

#if !defined(__BAMBU__)
#include <iostream>
using namespace std;
#endif

namespace ac_math
{
   template <unsigned M, unsigned N, unsigned P, class TA, class TB, class TC>
   void arraymultiply(const TA A[M][N], const TB B[N][P], TC C[M][P])
   {
   MLOOP:
      for(unsigned i = 0; i < M; i++)
      {
      PLOOP:
         for(unsigned j = 0; j < P; j++)
         {
            typedef typename TA::template rt_T<TB>::mult T;
            typename T::rt_unary::template set<N>::sum add = 0;
         NLOOP:
            for(unsigned k = 0; k < N; k++)
            {
               T multiply = A[i][k] * B[k][j];
               add = add + multiply;
            }
            C[i][j] = add;
         }
      }
   }

   //============================================================================
   // Function: matrix multiplication (for ac_fixed)
   //
   // Usage:
   //    See above example code for usage.
   //-------------------------------------------------------------------------
   template <unsigned M, unsigned N, unsigned P, int W1, int I1, bool S1, ac_q_mode Q1, ac_o_mode O1, int W2, int I2,
             bool S2, ac_q_mode Q2, ac_o_mode O2, int outW, int outI, bool outS, ac_q_mode outQ, ac_o_mode outO>
   void ac_matrixmul(const ac_fixed<W1, I1, S1, Q1, O1> A[M][N], const ac_fixed<W2, I2, S2, Q2, O2> B[N][P],
                     ac_fixed<outW, outI, outS, outQ, outO> C[M][P])
   {
      arraymultiply<M, N, P>(A, B, C);
   }

   //============================================================================
   // Function: matrix multiplication (for ac_complex<ac_fixed>)
   //
   // Usage:
   //    See above example code for usage.
   //-------------------------------------------------------------------------
   template <unsigned M, unsigned N, unsigned P, int W1, int I1, bool S1, ac_q_mode Q1, ac_o_mode O1, int W2, int I2,
             bool S2, ac_q_mode Q2, ac_o_mode O2, int outW, int outI, bool outS, ac_q_mode outQ, ac_o_mode outO>
   void ac_matrixmul(const ac_complex<ac_fixed<W1, I1, S1, Q1, O1>> A[M][N],
                     const ac_complex<ac_fixed<W2, I2, S2, Q2, O2>> B[N][P],
                     ac_complex<ac_fixed<outW, outI, outS, outQ, outO>> C[M][P])
   {
      arraymultiply<M, N, P>(A, B, C);
   }
} // namespace ac_math

//=========================================================================
// Function: ac_matrix_matrixmul
// Helper function for using ac_chol_d on ac_matrix objects
//-------------------------------------------------------------------------

template <class TA, class TB, class TC>
void ac_matrix_matrixmul(const TA& A, const TB& B, TC& C)
{
   // Extract 2D array member data, and pass it over to the 2D array implementation.
   ac_math::ac_matrixmul<TA::dim1, TA::dim2, TB::dim2>(A.m_data, B.m_data, C.m_data);
}

namespace ac_math
{
   //============================================================================
   // Function: matrix multiplication (for ac_matrix<ac_fixed>)
   //
   // Usage:
   //    See above example code for usage.
   //-------------------------------------------------------------------------
   template <unsigned M, unsigned N, unsigned P, int W1, int I1, bool S1, ac_q_mode Q1, ac_o_mode O1, int W2, int I2,
             bool S2, ac_q_mode Q2, ac_o_mode O2, int outW, int outI, bool outS, ac_q_mode outQ, ac_o_mode outO>
   void ac_matrixmul(const ac_matrix<ac_fixed<W1, I1, S1, Q1, O1>, M, N>& A,
                     const ac_matrix<ac_fixed<W2, I2, S2, Q2, O2>, N, P>& B,
                     ac_matrix<ac_fixed<outW, outI, outS, outQ, outO>, M, P>& C)

   {
      ac_matrix_matrixmul(A, B, C);
   }

   //============================================================================
   // Function: matrix multiplication (for ac_matrix<ac_complex<ac_fixed> >)
   //
   // Usage:
   //    See above example code for usage.
   //-------------------------------------------------------------------------
   template <unsigned M, unsigned N, unsigned P, int W1, int I1, bool S1, ac_q_mode Q1, ac_o_mode O1, int W2, int I2,
             bool S2, ac_q_mode Q2, ac_o_mode O2, int outW, int outI, bool outS, ac_q_mode outQ, ac_o_mode outO>
   void ac_matrixmul(const ac_matrix<ac_complex<ac_fixed<W1, I1, S1, Q1, O1>>, M, N>& A,
                     const ac_matrix<ac_complex<ac_fixed<W2, I2, S2, Q2, O2>>, N, P>& B,
                     ac_matrix<ac_complex<ac_fixed<outW, outI, outS, outQ, outO>>, M, P>& C)
   {
      ac_matrix_matrixmul(A, B, C);
   }
} // namespace ac_math
#endif
