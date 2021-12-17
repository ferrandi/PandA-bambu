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
// File: ac_qrd.h
//
//  Created on: Jul 11, 2017
//
//  Author: Sachchidanand Deo
//
//  Description: This function computes Q and R matrices which are decompositions of an input matrix (given by A), such
//  that: A is an input square matrix with dimension MxM Q matrix is an orthogonal matrix (such that, Q = inverse (Q)
//  and R is an upper triangular matrix such that all elemets below diagonal are zero.
//  This function uses Given's rotation algorithm, which takes input matrix of size MxM and in every iteration computes
//  a 2x2 matrix given by, [ c -s] [ s  c]
// where, c = x/sqrt(x^2+y^2) and s = -y/sqrt(x^2+y^2)
// c is cosine element where as s is sine element (See diagonal processing element function for further explaination).
// In above equation, y is element in matrix to be made zero and x being element in same column and row above x.
// After the above matrix is computed, we form a Givens rotation matrix. given by,
//  [c -s 0 0 0]
//  [s  c 0 0 0]
//  [0  0 1 0 0]
//  [0  0 0 1 0]
//  [0  0 0 0 1]

// Then Givens rotation matrix is multiplied to the original matrix and process is repeated until the result is obtained
// which is R matrix. In this way each iteration performs one rotation, so as to zero one element of matrix. In this
// file we start with zeroing, first column, last row element and then proceed towards up direction columnwise, upon
// reaching element below diagonal in every column, we proceed to next column on right.
//
// Systolic Array approach : This algorithm uses Systolic array approach to replace the matrix multiplication. This
// saves a lot of hardware and makes algorithm fully parallelizable. Systolic array approach takes advantage of the fact
// that only two rows change in one iteration and rather than doing matrix muliplication optimizes off-diagonal
// processing element to perform operation on two rows at a time, in a stateful manner (See off-diagonal processing
// element for further explaination).
//
// Computation of Q : To avoid matrix multiplication involved in computation of Q alongside storage of previous Q in
// every iteration, Q is computed in systolic manner, similar to R. First input matrix is concatenated with a identity
// matrix, to get a higher order matrix, which is then modified using givens rotation in a systolic manner. The larger
// matrix is then subdivided into R and output of Givens rotation on identity matrix which is Q (as Final_Q = Q1. Q2.
// Q3....is nothing but performing Givens rotation successively in identity matrix).
//----------------------------------------------------------------------------------------------------------------

#ifndef _INCLUDED_AC_QRD_H_
#define _INCLUDED_AC_QRD_H_

// include this part during debugging
//#define AC_QRD_DEBUG

// Include headers for data types supported by these implementations
#include <ac_complex.h>
#include <ac_fixed.h>
#include <ac_matrix.h>

// Include headers for required functions
#include <ac_math/ac_inverse_sqrt_pwl.h>
#include <ac_math/ac_sqrt_pwl.h>

#if !defined(__BAMBU__)
#include <iostream>
using namespace std;
#endif

//=========================================================================
// Inline functions to perform required matrix operations:
//=========================================================================
// initializing the larger matrix as well as the smaller matrices. Formation of A1 from A and identity matrix.

namespace ac_math
{
   template <unsigned M, typename T1, typename T2>
   void initialize_matrix(ac_matrix<T1, M, M>& A, ac_matrix<T2, M, 2 * M>& A1)
   {
   IDENT_ROW:
      for(unsigned i = 0; i < M; i++)
      {
      IDENT_COLUMN:
         for(unsigned j = 0; j < M; j++)
         {
            A1(i, j) = A(i, j);                      // formation of A1 using A
            A1(i, j + M) = (j + M - i == M) ? 1 : 0; // setting remaining part of A to identity matrix
         }
      }
   }

   // This function is used to get back final Q and R from large matrix A1 on which Givens rotations are performed.
   template <unsigned M, typename T1, typename T2>
   void qr_separate(ac_matrix<T1, M, 2 * M>& A1, ac_matrix<T2, M, M>& Q, ac_matrix<T2, M, M>& R)
   {
   SEPARATE_ROW:
      for(unsigned i = 0; i < M; i++)
      {
      SEPARATE_COLUMN:
         for(unsigned j = 0; j < M; j++)
         {
            Q(j, i) = A1(i, j + M); // Q is part where initial matrix was present
            R(i, j) = A1(i, j);     // R is a part where identity matrix was present
         }
      }
   }

   //=========================================================================
   // Processing Elements to perform orthogonal rotation in each iteration
   //=========================================================================
   // Diagonal Processing Elements:
   // Description: Diagonal processing elements are used to compute sine (s) and cosine (c) parameters. These parameters
   // are then passed down to the parent function which then broadcasts them to off-diagonal processing elements. The
   // computation of c and s is carried out using: c = x/sqrt(x^2+y^2) and s = -y/sqrt(x^2+y^2) where x and y are taken
   // from parent function. To reduce the area, piece wise linear implementation of reciprocal can be used to replace
   // dividers and piecewise linear implementation of square root can be used to replace accurate square root
   // computation.
   // ----------------------------------------------------------------------------------------------------------------
   // Helper struct (types_params_qrd) for defining internal variable:
   // Description: types_params_qrd is a helper structure that is used for defining
   // internal varaible given by temp_type which accounts for bit growth due to the
   // multiplication.
   // ----------------------------------------------------------------------------------------------------------------
   template <typename T>
   struct types_params_qrd
   {
   };

   // ----------------------------------------------------------------------------------------------------------------
   // Helper struct (types_params_qrd) for ac_fixed:
   // Description: This structue defines temp_type variable required
   // in ac_fixed implementation of qrd
   // ----------------------------------------------------------------------------------------------------------------
   template <int W, int I, bool S, ac_q_mode q_mode, ac_o_mode o_mode>
   struct types_params_qrd<ac_fixed<W, I, S, q_mode, o_mode>>
   {
      typedef ac_fixed<2 * W + 1, 2 * I + 1, false, q_mode, o_mode> sqr_type; // for additional of squares
      typedef ac_fixed<4 * W + 2, 2 * I + 1, false, q_mode, o_mode> root_type;
   };

   // ----------------------------------------------------------------------------------------------------------------
   // Helper struct (types_params_qrd) for ac_complex:
   // Description: This structue defines temp_type variable required
   // in ac_complex implementation of qrd
   // ----------------------------------------------------------------------------------------------------------------
   template <int W, int I, bool S, ac_q_mode q_mode, ac_o_mode o_mode>
   struct types_params_qrd<ac_complex<ac_fixed<W, I, S, q_mode, o_mode>>>
   {
      typedef ac_complex<ac_fixed<2 * W + 1, 2 * I + 1, true, q_mode, o_mode>> sqr_type;
      typedef ac_complex<ac_fixed<4 * W + 2, I + 1, true, q_mode, o_mode>> root_type;
   };

   //#pragma map_to_operator [CCORE]
   template <typename T1, typename T2, typename T3, bool ispwl>
   void diagonal_PE(T2 x, T2 y, T3& c, T3& s)
   {
      typename types_params_qrd<T1>::sqr_type sqr;
      typename types_params_qrd<T1>::root_type root;
      typename types_params_qrd<T1>::root_type inverse_root;
      sqr = (x * x) + (y * y);
      if(ispwl)
      {
         ac_math::ac_inverse_sqrt_pwl(sqr, inverse_root);
         c = y * inverse_root;
         s = -x * inverse_root;
      }
      else
      {
         ac_math::ac_sqrt_pwl(sqr, root);
         c = y / root;
         s = -x / root;
      }
   }

   //=========================================================================
   // Off-Diagonal Processing Elements:
   // Description: Off-diagonal processing elements take the broadcasted cosine and sine parameters from the diagonal
   // processing elements, larger matrix on which Givens rotation is to be performed and then systolically update two
   // rows of the matrices involved in one rotation. The formula for the rows modification is given by: row'(0,0) =
   // c*row(0,0) - s*row(1,0) row'(1,0) = c*row(1,0) + s*row(0,0) where initial value of row(0,0) and row(1,0) are
   // obtained from the two rows of input matrix, that are part of systolic implementation of that perticular
   // iteration(this requires initial storage, which is used to break dependancy and make algorithm fully parallelized).
   // After the processing is done, new values of row for that perticular iteration are assigned back to the input
   // matrix.
   //----------------------------------------------------------------------------------------------------------------

   //#pragma map_to_operator [CCORE]
   template <unsigned M, typename T1, typename T2>
   void offdiagonal_PE(ac_matrix<T1, M, 2 * M>(&A1), unsigned pivot, T2 c, T2 s, unsigned j)
   {
      ac_matrix<T1, 2, 2 * M> row1;
      ac_matrix<T1, 2, 2 * M> row2;
   ROW_COPY:
      for(unsigned i = 0; i < 2 * M; i++)
      {
         row1(0, i) = A1(pivot - 1, i);
         row1(1, i) = A1(pivot, i);
         row2(0, i) = c * row1(0, i) - s * row1(1, i);
         row2(1, i) = c * row1(1, i) + s * row1(0, i);
         A1(pivot - 1, i) = row2(0, i);
         A1(pivot, i) = row2(1, i);
      }
      A1(pivot, j) = 0;

#if !defined(__BAMBU__) && defined(AC_QRD_DEBUG)
      cout << "pivot = " << pivot << endl;
      cout << "row1 = " << row1 << endl;
      cout << "row2 = " << row2 << endl;
#endif
   }

   //=========================================================================
   // Function : ac_qrd (ac_fixed and ac_complex<ac_fixed> implementation)
   // Description :
   // The implementation is necessary similar to the ac_fixed implementation.
   // The function takes three matrices as input, which are passed by reference.
   // First one (A) is input matrix, where as Q and R are results of decomposition,
   // which are written at memory location for them.
   // First A1 is initialized using A and a unitary matrix.
   // Then for each and every element below diagonal of input_matrix, c and s are
   // computed using diagonal processing elements.
   // These values are then propagated to off-diagonal processing elements which then
   // systolically performs Givens rotation in every iteration of algorithm and
   // modifies the input matrix.
   // Then finally, function qr_separate is called which divides the modified matrix into
   // two matrices which are final output matrices Q and R.
   //----------------------------------------------------------------------------------------------------------------

   template <bool ispwl = true, unsigned M, int W1, int I1, ac_q_mode q1, ac_o_mode o1, int W2, int I2, ac_q_mode q2,
             ac_o_mode o2>
   void ac_qrd(ac_matrix<ac_fixed<W1, I1, true, q1, o1>, M, M>& A, ac_matrix<ac_fixed<W2, I2, true, q2, o2>, M, M>& Q,
               ac_matrix<ac_fixed<W2, I2, true, q2, o2>, M, M>& R)
   {
      // Defining all typedefs
      typedef ac_fixed<4 * W1, 4 * I1, true, AC_RND, AC_SAT> intermediate_type;
      typedef ac_fixed<W1, I1, true, q1, o1> input_type;
      typedef ac_fixed<5 * W1 + 2, 2 * I1 + 2, true, AC_RND, AC_SAT> sin_cosine_type;
      typedef ac_fixed<W2, I2, true, q2, o2> output_type;
      // Defining intermediate variables
      sin_cosine_type c;
      sin_cosine_type s;
      ac_matrix<intermediate_type, M, 2 * M> A1;

      initialize_matrix<M, input_type, intermediate_type>(A, A1);
   COLUMN:
      for(unsigned column = 0; column < M; column++)
      {
      ROW:
         for(unsigned row = M - 1; row > 0; row--)
         {
            if(row == column)
            {
               break;
            }
            diagonal_PE<input_type, intermediate_type, sin_cosine_type, ispwl>(A1(row, column), A1(row - 1, column), c,
                                                                               s);
            offdiagonal_PE<M, intermediate_type, sin_cosine_type>(A1, row, c, s, column);
         }
      }
      qr_separate<M, intermediate_type, output_type>(A1, Q, R);
   }

   template <bool ispwl = true, unsigned M, int W1, int I1, ac_q_mode q1, ac_o_mode o1, int W2, int I2, ac_q_mode q2,
             ac_o_mode o2>
   void ac_qrd(ac_matrix<ac_complex<ac_fixed<W1, I1, true, q1, o1>>, M, M>& A,
               ac_matrix<ac_complex<ac_fixed<W2, I2, true, q2, o2>>, M, M>& Q,
               ac_matrix<ac_complex<ac_fixed<W2, I2, true, q2, o2>>, M, M>& R)
   {
      // Defining all typedefs
      typedef ac_complex<ac_fixed<4 * W1, 4 * I1, true, AC_RND, AC_SAT>> intermediate_type;
      typedef ac_complex<ac_fixed<W1, I1, true, q1, o1>> input_type;
      typedef ac_complex<ac_fixed<5 * W1 + 2, 2 * I1 + 2, true, AC_RND, AC_SAT>> sin_cosine_type;
      typedef ac_complex<ac_fixed<W2, I2, true, q2, o2>> output_type;
      // Defining intermediate variables
      sin_cosine_type c;
      sin_cosine_type s;
      ac_matrix<intermediate_type, M, 2 * M> A1;

      initialize_matrix<M, input_type, intermediate_type>(A, A1);
   COLUMN:
      for(unsigned column = 0; column < M; column++)
      {
      ROW:
         for(unsigned row = M - 1; row > column; row--)
         {
            diagonal_PE<input_type, intermediate_type, sin_cosine_type, ispwl>(A1(row, column), A1(row - 1, column), c,
                                                                               s);
            offdiagonal_PE<M, intermediate_type, sin_cosine_type>(A1, row, c, s, column);
         }
      }
      qr_separate<M, intermediate_type, output_type>(A1, Q, R);
   }

   template <bool ispwl = true, unsigned M, int W1, int I1, ac_q_mode q1, ac_o_mode o1, int W2, int I2, ac_q_mode q2,
             ac_o_mode o2>
   void ac_qrd(ac_fixed<W1, I1, true, q1, o1> A[M][M], ac_fixed<W2, I2, true, q2, o2> Q[M][M],
               ac_fixed<W2, I2, true, q2, o2> R[M][M])
   {
      ac_matrix<ac_fixed<W1, I1, true, q1, o1>, M, M> input_mat;
      ac_matrix<ac_fixed<W2, I2, true, q2, o2>, M, M> Q_mat;
      ac_matrix<ac_fixed<W2, I2, true, q2, o2>, M, M> R_mat;

      for(unsigned i = 0; i < M; i++)
      {
         for(unsigned j = 0; j < M; j++)
         {
            input_mat(i, j) = A[i][j];
         }
      }

      ac_qrd<ispwl>(input_mat, Q_mat, R_mat);

      for(unsigned i = 0; i < M; i++)
      {
         for(unsigned j = 0; j < M; j++)
         {
            Q[i][j] = Q_mat(i, j);
            R[i][j] = R_mat(i, j);
         }
      }
   }

   template <bool ispwl = true, unsigned M, int W1, int I1, ac_q_mode q1, ac_o_mode o1, int W2, int I2, ac_q_mode q2,
             ac_o_mode o2>
   void ac_qrd(ac_complex<ac_fixed<W1, I1, true, q1, o1>> A[M][M], ac_complex<ac_fixed<W2, I2, true, q2, o2>> Q[M][M],
               ac_complex<ac_fixed<W2, I2, true, q2, o2>> R[M][M])
   {
      ac_matrix<ac_complex<ac_fixed<W1, I1, true, q1, o1>>, M, M> input_mat;
      ac_matrix<ac_complex<ac_fixed<W2, I2, true, q2, o2>>, M, M> Q_mat;
      ac_matrix<ac_complex<ac_fixed<W2, I2, true, q2, o2>>, M, M> R_mat;

      for(unsigned i = 0; i < M; i++)
      {
         for(unsigned j = 0; j < M; j++)
         {
            input_mat(i, j) = A[i][j];
         }
      }

      ac_qrd<ispwl>(input_mat, Q_mat, R_mat);

      for(unsigned i = 0; i < M; i++)
      {
         for(unsigned j = 0; j < M; j++)
         {
            Q[i][j] = Q_mat(i, j);
            R[i][j] = R_mat(i, j);
         }
      }
   }
} // namespace ac_math

#endif
