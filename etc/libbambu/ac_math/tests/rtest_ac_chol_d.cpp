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
// =========================TESTBENCH=======================================
// This testbench file contains a stand-alone testbench that exercises the
// ac_chol_d() function using a variety of data types and bit-
// widths.

// To compile standalone and run:
//   $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_chol_d.cpp -o design
//   ./design

// Include the AC Math function that is exercised with this testbench
#include <ac_math/ac_chol_d.h>
using namespace ac_math;

// ==============================================================================
// Test Designs
//   These simple functions allow executing the ac_chol_d() function
//   using multiple data types at the same time. Template parameters are
//   used to configure the bit-widths of the types.

// Test Design for real and complex fixed point values, using PWL functions
// for cholesky decomposition.
template <unsigned M, int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi, bool outSfi>
void test_ac_chol_d_pwl(const ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP> A1[M][M],
                        ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> L1[M][M],
                        const ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>> A2[M][M],
                        ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>> L2[M][M],
                        const ac_matrix<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>, M, M>& A3,
                        ac_matrix<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>, M, M>& L3,
                        const ac_matrix<ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>>, M, M>& A4,
                        ac_matrix<ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>>, M, M>& L4)
{
   ac_chol_d<true>(A1, L1);
   ac_chol_d<true>(A2, L2);
   ac_chol_d<true>(A3, L3);
   ac_chol_d<true>(A4, L4);
}

// Test Design for real and complex fixed point values, using accurate div and sqrt
// functions for cholesky decomposition.
template <unsigned M, int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi, bool outSfi>
void test_ac_chol_d_accurate(const ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP> A1[M][M],
                             ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> L1[M][M],
                             const ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>> A2[M][M],
                             ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>> L2[M][M],
                             const ac_matrix<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>, M, M>& A3,
                             ac_matrix<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>, M, M>& L3,
                             const ac_matrix<ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>>, M, M>& A4,
                             ac_matrix<ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>>, M, M>& L4)
{
   ac_chol_d<false>(A1, L1);
   ac_chol_d<false>(A2, L2);
   ac_chol_d<false>(A3, L3);
   ac_chol_d<false>(A4, L4);
}

// ==============================================================================

#include <ac_math/ac_random.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <string>
using namespace std;

// ------------------------------------------------------------------------------
// Helper functions

// Print matrix, for debugging purposes.

#ifdef DEBUG
template <unsigned M, class T>
void print_matrix(T mat[M][M])
{
   cout << "FILE : " << __FILE__ << ", LINE : " << __LINE__ << endl;
   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)M; j++)
      {
         cout << "mat[" << i << "][" << j << "] = " << mat[i][j] << endl;
      }
   }
}

template <unsigned M, unsigned N, class T>
void print_matrix(T mat[M][N])
{
   cout << "FILE : " << __FILE__ << ", LINE : " << __LINE__ << endl;
   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)N; j++)
      {
         cout << "mat[" << i << "][" << j << "] = " << mat[i][j] << endl;
      }
   }
}
#endif

// Generate positive definite matrix of ac_fixed values
template <unsigned N, unsigned M, int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
void gen_matrix(ac_fixed<W, I, S, Q, O> A[M][M])
{
   // Declare an MxN and NxM matrix. The NxM matrix, i.e. tbmatT, stores the transpose
   // of the MxN matrix, i.e. tbmat
   ac_fixed<W, I, S, Q, O> tbmat[M][N];
   ac_fixed<W, I, S, Q, O> tbmatT[N][M];

   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)N; j++)
      {
         if(j == i)
         {
            // The diagonal elements of tbmat are all initialized to the same value, i.e. M
            // Any common, non-zero value can be used for this purpose, not just M
            tbmat[i][j] = M;
            tbmatT[j][i] = tbmat[i][j];
         }
         else
         {
            // The non-diagonal, lower triangular elements of tbmat are initialized to random, unique values
            ac_fixed<16, 1, true> rand_val;
            ac_random(rand_val);
            tbmat[i][j] = rand_val * M;
            tbmatT[j][i] = tbmat[i][j];
         }
      }
   }

#ifdef DEBUG
   cout << "tbmat is : " << endl;
   print_matrix(tbmat);
   cout << "tbmatT is : " << endl;
   print_matrix(tbmatT);
#endif

   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)M; j++)
      {
         A[i][j] = 0;
      }
   }

   // Multiply tbmat by its transpose to get the positive definite input matrix
   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)M; j++)
      {
         for(int k = 0; k < (int)N; k++)
         {
            A[i][j] += tbmat[i][k] * tbmatT[k][j];
         }
      }
   }

#ifdef DEBUG
   cout << "A in gen_matrix function is : " << endl;
   print_matrix(A);
#endif
}

// Generate positive definite matrix of ac_complex<ac_fixed> values
template <unsigned N, unsigned M, int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
void gen_matrix(ac_complex<ac_fixed<W, I, S, Q, O>> A[M][M])
{
   // Declare an MxN and NxM matrix. The NxM matrix, i.e. tbmatT, stores the conjugate transpose
   // of the MxN matrix, i.e. tbmat
   ac_complex<ac_fixed<W, I, S, Q, O>> tbmat[M][N];
   ac_complex<ac_fixed<W, I, S, Q, O>> tbmatT[N][M];
   ac_complex<ac_fixed<W, I, S, Q, O>> zero_complex(0, 0);

   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)N; j++)
      {
         if(i == j)
         {
            // The diagonal elements of tbmat are all real, and initialized to the same value, i.e. M
            // Any common, real, non-zero value can be used for this purpose, not just M
            tbmat[i][j].r() = M;
            tbmat[i][j].i() = 0;
            tbmatT[j][i] = tbmat[i][j];
         }
         else
         {
            // The non-diagonal elements of tbmat are initialized to random, unique, complex values
            ac_fixed<16, 1, true> rand_val;
            ac_fixed<12, 1, true> rand_val_2;
            ac_random(rand_val);
            ac_random(rand_val_2);
            tbmat[i][j].r() = rand_val * M;
            tbmat[i][j].i() = rand_val_2 * M;
            tbmatT[j][i] = tbmat[i][j].conj();
         }
      }
   }

#ifdef DEBUG
   cout << "tbmat is : " << endl;
   print_matrix(tbmat);
   cout << "tbmatT is : " << endl;
   print_matrix(tbmatT);
#endif

   // Multiply tbmat by its conjugate transpose to get the positive definite input matrix
   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)M; j++)
      {
         A[i][j] = zero_complex;
      }
   }

   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)M; j++)
      {
         for(int k = 0; k < (int)N; k++)
         {
            A[i][j] += tbmat[i][k] * tbmatT[k][j];
         }
      }
   }
}

// Testbench for cholesky decomposition for ac_fixed matrix
// The testbench uses the cholesky-crout algorithm
template <unsigned M, int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
void chol_d_tb(const ac_fixed<W, I, S, Q, O> A[M][M], double L_tb[M][M])
{
   double sum_Ajj_Ljk_sq, sum_Aij_Lik_Ljk;

   // All elements of output initialized to zero by default
   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)M; j++)
      {
         L_tb[i][j] = 0;
      }
   }

   // L_tb = 0;

   for(int j = 0; j < (int)M; j++)
   {
      sum_Ajj_Ljk_sq = A[j][j].to_double();

      for(int k = 0; k < j; k++)
      {
         sum_Ajj_Ljk_sq -= L_tb[j][k] * L_tb[j][k];
      }

      // Check to make sure that the matrix is positive definite. If "sum_Ajj_Ljk_sq" is negative/zero, then the
      // diagonal element, i.e. L_tb(j, j) will be complex/zero, which is not valid. This condition will not be
      // encountered if the input matrix is positive definite
      assert(sum_Ajj_Ljk_sq > 0);
      // Assign value to diagonal elements.
      L_tb[j][j] = sqrt(sum_Ajj_Ljk_sq);

      for(int i = (j + 1); i < (int)M; i++)
      {
         sum_Aij_Lik_Ljk = A[i][j].to_double();
         for(int k = 0; k < j; k++)
         {
            sum_Aij_Lik_Ljk -= L_tb[i][k] * L_tb[j][k];
         }
         // Assign value to non-diagonal elements below the diagonal.
         L_tb[i][j] = sum_Aij_Lik_Ljk / L_tb[j][j];
      }
   }
}

// Testbench for cholesky decomposition for ac_complex<ac_fixed> matrices
// The testbench uses the cholesky-crout algorithm
template <unsigned M, int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
void chol_d_tb(ac_complex<ac_fixed<W, I, S, Q, O>> A[M][M], ac_complex<double> L_tb[M][M])
{
   typedef ac_complex<double> output_type;
   output_type zero_complex(0, 0), sum_Ajj_Ljk_sq, sum_Aij_Lik_Ljk;

   // All elements of output initialized to zero by default
   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)M; j++)
      {
         L_tb[i][j] = zero_complex;
      }
   }

   // L_tb = zero_complex;

   for(int j = 0; j < (int)M; j++)
   {
      sum_Ajj_Ljk_sq.r() = A[j][j].r().to_double();
      sum_Ajj_Ljk_sq.i() = A[j][j].i().to_double();

      for(int k = 0; k < j; k++)
      {
         sum_Ajj_Ljk_sq -= L_tb[j][k] * L_tb[j][k].conj();
      }

      // Check to make sure that the matrix is positive definite. If "sum_Ajj_Ljk_sq" is negative/zero, then the
      // diagonal element, i.e. L_tb(j, j) will be complex/zero, which is not valid. This condition will not be
      // encountered if the input matrix is positive definite
      assert(sum_Ajj_Ljk_sq.r() > 0);
      // Assign value to diagonal elements. Since the diagonal elements are real, only initialize the real part.
      L_tb[j][j].r() = sqrt(sum_Ajj_Ljk_sq.r());

      for(int i = (j + 1); i < (int)M; i++)
      {
         sum_Aij_Lik_Ljk.r() = A[i][j].r().to_double();
         sum_Aij_Lik_Ljk.i() = A[i][j].i().to_double();
         for(int k = 0; k < j; k++)
         {
            sum_Aij_Lik_Ljk -= L_tb[i][k] * L_tb[j][k].conj();
         }
         // Assign value to non-diagonal elements below the diagonal.
         L_tb[i][j].r() = (1 / L_tb[j][j].r()) * sum_Aij_Lik_Ljk.r();
         L_tb[i][j].i() = (1 / L_tb[j][j].r()) * sum_Aij_Lik_Ljk.i();
      }
   }
}

// Return the absolute value of the matrix element that has the
// maximum absolute value.
template <unsigned M>
double abs_mat_max(const double L_tb[M][M])
{
   double max_val = 0;

   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)M; j++)
      {
         if(abs(L_tb[i][j]) > max_val)
         {
            max_val = abs(L_tb[i][j]);
         }
      }
   }

   return max_val;
}

// Return the absolute value of the matrix element that has the
// maximum absolute value.
template <unsigned M>
double abs_mat_max(const ac_complex<double> L_tb[M][M])
{
   double max_val = 0;

   for(int i = 0; i < M; i++)
   {
      for(int j = 0; j < M; j++)
      {
         if(L_tb[i][j].mag_sqr() > max_val)
         {
            max_val = L_tb[i][j].mag_sqr();
         }
      }
   }

   return max_val;
}

// Keep real, double element as it is (just kept in order to ensure that the error checking
// function is compatible even for real, double values)
double conv_val(double x)
{
   return x;
}

// Convert real, ac_fixed element to double.
template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
double conv_val(ac_fixed<W, I, S, Q, O> x)
{
   return x.to_double();
}

// Convert complex double element to mag_sqr.
double conv_val(ac_complex<double> x)
{
   return x.mag_sqr();
}

// Convert complex ac_fixed element to the double of it's
// mag_sqr
template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
double conv_val(ac_complex<ac_fixed<W, I, S, Q, O>> x)
{
   return x.mag_sqr().to_double();
}

// Compare DUT output matrix to testbench computed matrix and find error.
template <unsigned M, class T, class T_tb>
double compare_matrices(const T L[M][M], const T_tb L_tb[M][M], const double allowed_error)
{
   double this_error, max_error = 0, max_val;
   // Find the max. abs. value in the matrix. In case of complex matrices, this is the max.
   // mag_sqr value. For real matrices, it's the max. absolute value stored in the matrix.
   max_val = abs_mat_max(L_tb);

#ifdef DEBUG
   cout << "max_val = " << max_val << endl;
#endif

   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)M; j++)
      {
         // The error is calculated as the difference between the value of the expected
         // vs. the actual value, normalized w.r.t. the max_value in the matrix. For complex
         // numbers, the expected vs actual values are first converted to the their
         // mag_sqr() representations. For real numbers, the values are passed as they are
         // for the error calculation.
         this_error = 100 * abs(conv_val(L[i][j]) - conv_val(L_tb[i][j])) / (max_val);
         if(this_error > max_error)
         {
            max_error = this_error;
         }
#ifdef DEBUG
         cout << "FILE : " << __FILE__ << ", LINE : " << __LINE__ << endl;
         cout << "L[" << i << "][" << j << "]    = " << L[i][j] << endl;
         cout << "L_tb[" << i << "][" << j << "] = " << L_tb[i][j] << endl;
         cout << "this_error = " << this_error << endl;
         assert(this_error < allowed_error);
#endif
      }
   }

   return max_error;
}

// Check if real matrix is zero matrix.
template <unsigned M, class T>
bool check_if_zero_matrix(const T L[M][M])
{
   bool is_zero_matrix = true;

   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)M; j++)
      {
         if(L[i][j] != 0)
         {
            is_zero_matrix = false;
#ifdef DEBUG
            cout << "Matrix was not zero for a positive definite input. Non-zero element found:" << endl;
            cout << "L[" << i << "][" << j << "] = " << L[i][j] << endl;
            assert(false);
#endif
         }
      }
   }

   return is_zero_matrix;
}

// Check if complex matrix is zero matrix.
template <unsigned M, class T>
bool check_if_zero_matrix(const ac_complex<T> L[M][M])
{
   bool is_zero_matrix = true;

   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)M; j++)
      {
         if(L[i][j].r() != 0 || L[i][j].i() != 0)
         {
            is_zero_matrix = false;
#ifdef DEBUG
            cout << "Matrix was not zero for a positive definite input. Non-zero element found:" << endl;
            cout << "L[" << i << "][" << j << "] = " << L[i][j] << endl;
            assert(false);
#endif
         }
      }
   }

   return is_zero_matrix;
}

// Copy a C-style array's contents over to an ac_matrix.
template <unsigned M, class T_matrix, class T_ac_matrix>
void copy_to_ac_matrix(const T_matrix array_2D[M][M], T_ac_matrix& output)
{
   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)M; j++)
      {
         output(i, j) = array_2D[i][j];
      }
   }
}

// Copy an ac_matrix's contents over to a C-style array.
template <unsigned M, class T_matrix, class T_ac_matrix>
void copy_to_array_2D(const T_ac_matrix& input, T_matrix array_2D[M][M])
{
   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)M; j++)
      {
         array_2D[i][j] = input(i, j);
      }
   }
}

// ==============================================================================
// Functions: test_driver functions
// Description: Templatized functions that can be configured for certain bit-
//   widths of AC datatypes. They use the type information to iterate through a
//   range of valid values on that type in order to compare the precision of the
//   DUT cholesky decomposition with the computed cholesky decomposition using a
//   standard C double type. The maximum error for each type is accumulated
//   in variables defined in the calling function.

// ==============================================================================
// Function: test_driver_pwl()
// Description: test_driver function for ac_fixed and ac_complex<ac_fixed> inputs
//   and outputs, using PWL functions for cholesky decomposition.

template <unsigned M, unsigned N, int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi, bool outSfi>
int test_driver_pwl(double& cumulative_max_error, double& cumulative_max_error_cmplx, const double allowed_error)
{
   bool passed = true;

   ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP> A_C_array[M][M];
   ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> L_C_array[M][M];
   ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>> cmplx_A_C_array[M][M];
   ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>> cmplx_L_C_array[M][M];
   ac_matrix<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>, M, M> A_ac_matrix;
   ac_matrix<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>, M, M> L_ac_matrix;
   ac_matrix<ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>>, M, M> cmplx_A_ac_matrix;
   ac_matrix<ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>>, M, M> cmplx_L_ac_matrix;

   cout << "TEST: ac_chol_d(), with pwl functions.      M = ";
   cout << M << ",";
   cout << " N = ";
   cout << N << ",";
   cout << "INPUT: ";
   cout.width(38);
   cout << left << A_C_array[0][0].type_name();
   cout << "OUTPUT: ";
   cout.width(38);
   cout << left << L_C_array[0][0].type_name();
   cout << "RESULT: ";

   double L_tb[M][M];
   ac_complex<double> cmplx_L_tb[M][M];

   // The gen_matrix function takes an MxN matrix, and multiplies it by its
   // conjugate transpose to obtain a positive definite input matrix
   gen_matrix<N>(A_C_array);
   gen_matrix<N>(cmplx_A_C_array);

   copy_to_ac_matrix(A_C_array, A_ac_matrix);
   copy_to_ac_matrix(cmplx_A_C_array, cmplx_A_ac_matrix);

   test_ac_chol_d_pwl(A_C_array, L_C_array, cmplx_A_C_array, cmplx_L_C_array, A_ac_matrix, L_ac_matrix,
                      cmplx_A_ac_matrix, cmplx_L_ac_matrix);

   ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> L_ac_matrix_converted[M][M];
   ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>> L_cmplx_ac_matrix_converted[M][M];

   copy_to_array_2D(L_ac_matrix, L_ac_matrix_converted);
   copy_to_array_2D(cmplx_L_ac_matrix, L_cmplx_ac_matrix_converted);

   // Get output of testbench function for cholesky decomposition.
   chol_d_tb(A_C_array, L_tb);
   chol_d_tb(cmplx_A_C_array, cmplx_L_tb);

#ifdef DEBUG
   cout << "A_C_array = " << endl;
   print_matrix(A_C_array);
   cout << "L_C_array = " << endl;
   print_matrix(L_C_array);
   cout << "L_tb = " << endl;
   print_matrix(L_tb);
   cout << "cmplx_A_C_array = " << endl;
   print_matrix(cmplx_A_C_array);
   cout << "cmplx_L_C_array = " << endl;
   print_matrix(cmplx_L_C_array);
   cout << "cmplx_L_tb = " << endl;
   print_matrix(cmplx_L_tb);
   cout << "A_ac_matrix = " << endl;
   cout << A_ac_matrix << endl;
   cout << "L_ac_matrix = " << endl;
   cout << L_ac_matrix << endl;
   cout << "cmplx_A_ac_matrix = " << endl;
   cout << cmplx_A_ac_matrix << endl;
   cout << "cmplx_L_ac_matrix = " << endl;
   cout << cmplx_L_ac_matrix << endl;
#endif

   // Compare matrices and get the max error
   double max_error = compare_matrices(L_C_array, L_tb, allowed_error);
   double max_error_cmplx = compare_matrices(cmplx_L_C_array, cmplx_L_tb, allowed_error);
   double max_error_ac_matrix = compare_matrices(L_ac_matrix_converted, L_tb, allowed_error);
   double max_error_cmplx_ac_matrix = compare_matrices(L_cmplx_ac_matrix_converted, cmplx_L_tb, allowed_error);

   // Put max overall error in a separate variable.
   double max_error_overall = max_error > max_error_ac_matrix ? max_error : max_error_ac_matrix;
   double max_error_cmplx_overall =
       max_error_cmplx > max_error_cmplx_ac_matrix ? max_error_cmplx : max_error_cmplx_ac_matrix;

   passed = (max_error_overall < allowed_error) && (max_error_cmplx_overall < allowed_error);

   // Also, we must make sure that the output on passing a non-positive definite matrix is a zero matrix. To do this, we
   // pass a matrix with all the values set to the quantum values of the ac_fixed type as the input.
   ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP> ac_fixed_quantum_value;
   ac_fixed_quantum_value.template set_val<AC_VAL_QUANTUM>();
   ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>> ac_complex_quantum_value(ac_fixed_quantum_value,
                                                                                 ac_fixed_quantum_value);
   A_ac_matrix = ac_fixed_quantum_value;
   cmplx_A_ac_matrix = ac_complex_quantum_value;

   // Copy over a non-positive definite matrix to the standard C array inputs.
   copy_to_array_2D(A_ac_matrix, A_C_array);
   copy_to_array_2D(cmplx_A_ac_matrix, cmplx_A_C_array);

   test_ac_chol_d_pwl(A_C_array, L_C_array, cmplx_A_C_array, cmplx_L_C_array, A_ac_matrix, L_ac_matrix,
                      cmplx_A_ac_matrix, cmplx_L_ac_matrix);

   copy_to_array_2D(L_ac_matrix, L_ac_matrix_converted);
   copy_to_array_2D(cmplx_L_ac_matrix, L_cmplx_ac_matrix_converted);

   // Make sure that a zero matrix is returned at the output.
   passed = passed && check_if_zero_matrix(L_C_array) && check_if_zero_matrix(cmplx_L_C_array) &&
            check_if_zero_matrix(L_ac_matrix_converted) && check_if_zero_matrix(L_cmplx_ac_matrix_converted);

   if(passed)
   {
      printf("PASSED , max err (%f) (%f complex)\n", max_error_overall, max_error_cmplx_overall);
   }
   else
   {
      printf("FAILED , max err (%f) (%f complex)\n", max_error_overall, max_error_cmplx_overall);
   } // LCOV_EXCL_LINE

   if(max_error_overall > cumulative_max_error)
   {
      cumulative_max_error = max_error_overall;
   }
   if(max_error_cmplx_overall > cumulative_max_error_cmplx)
   {
      cumulative_max_error_cmplx = max_error_cmplx_overall;
   }

   return 0;
}

// ==============================================================================
// Function: test_driver_accurate()
// Description: test_driver function for ac_fixed and ac_complex<ac_fixed> inputs
//   and outputs, using accurate functions for cholesky decomposition.

template <unsigned M, unsigned N, int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi, bool outSfi>
int test_driver_accurate(double& cumulative_max_error, double& cumulative_max_error_cmplx, const double allowed_error)
{
   bool passed = true;

   ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP> A_C_array[M][M];
   ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> L_C_array[M][M];
   ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>> cmplx_A_C_array[M][M];
   ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>> cmplx_L_C_array[M][M];
   ac_matrix<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>, M, M> A_ac_matrix;
   ac_matrix<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>, M, M> L_ac_matrix;
   ac_matrix<ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>>, M, M> cmplx_A_ac_matrix;
   ac_matrix<ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>>, M, M> cmplx_L_ac_matrix;

   cout << "TEST: ac_chol_d(), with accurate functions. M = ";
   cout << M << ",";
   cout << " N = ";
   cout << N << ",";
   cout << "INPUT: ";
   cout.width(38);
   cout << left << A_C_array[0][0].type_name();
   cout << "OUTPUT: ";
   cout.width(38);
   cout << left << L_C_array[0][0].type_name();
   cout << "RESULT: ";

   double L_tb[M][M];
   ac_complex<double> cmplx_L_tb[M][M];

   // The gen_matrix function takes an MxN matrix, and multiplies it by its
   // conjugate transpose to obtain a positive definite input matrix
   gen_matrix<N>(A_C_array);
   gen_matrix<N>(cmplx_A_C_array);

   copy_to_ac_matrix(A_C_array, A_ac_matrix);
   copy_to_ac_matrix(cmplx_A_C_array, cmplx_A_ac_matrix);

   test_ac_chol_d_accurate(A_C_array, L_C_array, cmplx_A_C_array, cmplx_L_C_array, A_ac_matrix, L_ac_matrix,
                           cmplx_A_ac_matrix, cmplx_L_ac_matrix);

   ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> L_ac_matrix_converted[M][M];
   ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>> L_cmplx_ac_matrix_converted[M][M];

   copy_to_array_2D(L_ac_matrix, L_ac_matrix_converted);
   copy_to_array_2D(cmplx_L_ac_matrix, L_cmplx_ac_matrix_converted);

   // Get output of testbench function for cholesky decomposition.
   chol_d_tb(A_C_array, L_tb);
   chol_d_tb(cmplx_A_C_array, cmplx_L_tb);

#ifdef DEBUG
   cout << "A_C_array = " << endl;
   print_matrix(A_C_array);
   cout << "L_C_array = " << endl;
   print_matrix(L_C_array);
   cout << "L_tb = " << endl;
   print_matrix(L_tb);
   cout << "cmplx_A_C_array = " << endl;
   print_matrix(cmplx_A_C_array);
   cout << "cmplx_L_C_array = " << endl;
   print_matrix(cmplx_L_C_array);
   cout << "cmplx_L_tb = " << endl;
   print_matrix(cmplx_L_tb);
   cout << "A_ac_matrix = " << endl;
   cout << A_ac_matrix << endl;
   cout << "L_ac_matrix = " << endl;
   cout << L_ac_matrix << endl;
   cout << "cmplx_A_ac_matrix = " << endl;
   cout << cmplx_A_ac_matrix << endl;
   cout << "cmplx_L_ac_matrix = " << endl;
   cout << cmplx_L_ac_matrix << endl;
#endif

   // Compare matrices and get the max error
   double max_error = compare_matrices(L_C_array, L_tb, allowed_error);
   double max_error_cmplx = compare_matrices(cmplx_L_C_array, cmplx_L_tb, allowed_error);
   double max_error_ac_matrix = compare_matrices(L_ac_matrix_converted, L_tb, allowed_error);
   double max_error_cmplx_ac_matrix = compare_matrices(L_cmplx_ac_matrix_converted, cmplx_L_tb, allowed_error);

   // Put max overall error in a separate variable.
   double max_error_overall = max_error > max_error_ac_matrix ? max_error : max_error_ac_matrix;
   double max_error_cmplx_overall =
       max_error_cmplx > max_error_cmplx_ac_matrix ? max_error_cmplx : max_error_cmplx_ac_matrix;

   passed = (max_error_overall < allowed_error) && (max_error_cmplx_overall < allowed_error);

   // Also, we must make sure that the output on passing a non-positive definite matrix is a zero matrix. To do this, we
   // pass a matrix with all the values set to the quantum values of the ac_fixed type as the input.
   ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP> ac_fixed_quantum_value;
   ac_fixed_quantum_value.template set_val<AC_VAL_QUANTUM>();
   ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>> ac_complex_quantum_value(ac_fixed_quantum_value,
                                                                                 ac_fixed_quantum_value);
   A_ac_matrix = ac_fixed_quantum_value;
   cmplx_A_ac_matrix = ac_complex_quantum_value;

   // Copy over a non-positive definite matrix to the standard C array inputs.
   copy_to_array_2D(A_ac_matrix, A_C_array);
   copy_to_array_2D(cmplx_A_ac_matrix, cmplx_A_C_array);

   test_ac_chol_d_accurate(A_C_array, L_C_array, cmplx_A_C_array, cmplx_L_C_array, A_ac_matrix, L_ac_matrix,
                           cmplx_A_ac_matrix, cmplx_L_ac_matrix);

   copy_to_array_2D(L_ac_matrix, L_ac_matrix_converted);
   copy_to_array_2D(cmplx_L_ac_matrix, L_cmplx_ac_matrix_converted);

   // Make sure that a zero matrix is returned at the output.
   passed = passed && check_if_zero_matrix(L_C_array) && check_if_zero_matrix(cmplx_L_C_array) &&
            check_if_zero_matrix(L_ac_matrix_converted) && check_if_zero_matrix(L_cmplx_ac_matrix_converted);

   if(passed)
   {
      printf("PASSED , max err (%f) (%f complex)\n", max_error_overall, max_error_cmplx_overall);
   }
   else
   {
      printf("FAILED , max err (%f) (%f complex)\n", max_error_overall, max_error_cmplx_overall);
   } // LCOV_EXCL_LINE

   if(max_error_overall > cumulative_max_error)
   {
      cumulative_max_error = max_error_overall;
   }
   if(max_error_cmplx_overall > cumulative_max_error_cmplx)
   {
      cumulative_max_error_cmplx = max_error_cmplx_overall;
   }

   return 0;
}

int main(int argc, char* argv[])
{
   double max_error_pwl = 0, cmplx_max_error_pwl = 0, max_error_accurate = 0, cmplx_max_error_accurate = 0;
   double allowed_error_pwl = 1;
   double allowed_error_accurate = 0.0005;

   cout << "=============================================================================" << endl;
   cout << "Testing function: ac_chol_d(), for scalar and complex datatypes - allowed_error_pwl = " << allowed_error_pwl
        << ", allowed_error_accurate = " << allowed_error_accurate << endl;

   // template <unsigned M, unsigned N, int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi, bool outSfi>
   test_driver_pwl<10, 10, 41, 21, true, 64, 32, true>(max_error_pwl, cmplx_max_error_pwl, allowed_error_pwl);
   // test_driver_pwl<10, 11, 41, 21, true, 64, 32, true> (max_error_pwl, cmplx_max_error_pwl, allowed_error_pwl); //
   // (corner case, will fail)
   test_driver_pwl<10, 12, 41, 21, true, 64, 32, true>(max_error_pwl, cmplx_max_error_pwl, allowed_error_pwl);
   test_driver_pwl<10, 13, 41, 21, true, 64, 32, true>(max_error_pwl, cmplx_max_error_pwl, allowed_error_pwl);
   test_driver_pwl<11, 11, 41, 21, true, 64, 32, true>(max_error_pwl, cmplx_max_error_pwl, allowed_error_pwl);
   test_driver_pwl<11, 12, 41, 21, true, 64, 32, true>(max_error_pwl, cmplx_max_error_pwl, allowed_error_pwl);
   test_driver_pwl<11, 13, 41, 21, true, 64, 32, true>(max_error_pwl, cmplx_max_error_pwl, allowed_error_pwl);
   test_driver_pwl<11, 14, 41, 21, true, 64, 32, true>(max_error_pwl, cmplx_max_error_pwl, allowed_error_pwl);
   test_driver_pwl<12, 12, 41, 21, true, 64, 32, true>(max_error_pwl, cmplx_max_error_pwl, allowed_error_pwl);
   test_driver_pwl<12, 13, 41, 21, true, 64, 32, true>(max_error_pwl, cmplx_max_error_pwl, allowed_error_pwl);
   test_driver_pwl<12, 14, 41, 21, true, 64, 32, true>(max_error_pwl, cmplx_max_error_pwl, allowed_error_pwl);
   test_driver_pwl<12, 15, 41, 21, true, 64, 32, true>(max_error_pwl, cmplx_max_error_pwl, allowed_error_pwl);
   test_driver_pwl<13, 13, 41, 21, true, 64, 32, true>(max_error_pwl, cmplx_max_error_pwl, allowed_error_pwl);
   test_driver_pwl<13, 14, 41, 21, true, 64, 32, true>(max_error_pwl, cmplx_max_error_pwl, allowed_error_pwl);
   test_driver_pwl<13, 15, 41, 21, true, 64, 32, true>(max_error_pwl, cmplx_max_error_pwl, allowed_error_pwl);
   test_driver_pwl<13, 16, 41, 21, true, 64, 32, true>(max_error_pwl, cmplx_max_error_pwl, allowed_error_pwl);

   test_driver_accurate<10, 10, 41, 21, true, 64, 32, true>(max_error_accurate, cmplx_max_error_accurate,
                                                            allowed_error_accurate);
   test_driver_accurate<10, 11, 41, 21, true, 64, 32, true>(max_error_accurate, cmplx_max_error_accurate,
                                                            allowed_error_accurate);
   test_driver_accurate<10, 12, 41, 21, true, 64, 32, true>(max_error_accurate, cmplx_max_error_accurate,
                                                            allowed_error_accurate);
   test_driver_accurate<10, 13, 41, 21, true, 64, 32, true>(max_error_accurate, cmplx_max_error_accurate,
                                                            allowed_error_accurate);
   test_driver_accurate<11, 11, 41, 21, true, 64, 32, true>(max_error_accurate, cmplx_max_error_accurate,
                                                            allowed_error_accurate);
   test_driver_accurate<11, 12, 41, 21, true, 64, 32, true>(max_error_accurate, cmplx_max_error_accurate,
                                                            allowed_error_accurate);
   test_driver_accurate<11, 13, 41, 21, true, 64, 32, true>(max_error_accurate, cmplx_max_error_accurate,
                                                            allowed_error_accurate);
   test_driver_accurate<11, 14, 41, 21, true, 64, 32, true>(max_error_accurate, cmplx_max_error_accurate,
                                                            allowed_error_accurate);
   test_driver_accurate<12, 12, 41, 21, true, 64, 32, true>(max_error_accurate, cmplx_max_error_accurate,
                                                            allowed_error_accurate);
   test_driver_accurate<12, 13, 41, 21, true, 64, 32, true>(max_error_accurate, cmplx_max_error_accurate,
                                                            allowed_error_accurate);
   test_driver_accurate<12, 14, 41, 21, true, 64, 32, true>(max_error_accurate, cmplx_max_error_accurate,
                                                            allowed_error_accurate);
   test_driver_accurate<12, 15, 41, 21, true, 64, 32, true>(max_error_accurate, cmplx_max_error_accurate,
                                                            allowed_error_accurate);
   test_driver_accurate<13, 13, 41, 21, true, 64, 32, true>(max_error_accurate, cmplx_max_error_accurate,
                                                            allowed_error_accurate);
   test_driver_accurate<13, 14, 41, 21, true, 64, 32, true>(max_error_accurate, cmplx_max_error_accurate,
                                                            allowed_error_accurate);
   test_driver_accurate<13, 15, 41, 21, true, 64, 32, true>(max_error_accurate, cmplx_max_error_accurate,
                                                            allowed_error_accurate);
   test_driver_accurate<13, 16, 41, 21, true, 64, 32, true>(max_error_accurate, cmplx_max_error_accurate,
                                                            allowed_error_accurate);

   cout << "=============================================================================" << endl;
   cout << "  Testbench finished. Maximum errors observed across all data type / bit-width variations:" << endl;
   cout << "    max_error_pwl            = " << max_error_pwl << endl;
   cout << "    cmplx_max_error_pwl      = " << cmplx_max_error_pwl << endl;
   cout << "    max_error_accurate       = " << max_error_accurate << endl;
   cout << "    cmplx_max_error_accurate = " << cmplx_max_error_accurate << endl;

   // If error limits on any tested datatype have been crossed, the test has failed
   bool test_fail = (max_error_pwl > allowed_error_pwl) || (cmplx_max_error_pwl > allowed_error_pwl) ||
                    (max_error_accurate > allowed_error_accurate) ||
                    (cmplx_max_error_accurate > allowed_error_accurate);

   // Notify the user whether or not the test was a failure.
   if(test_fail)
   {
      cout << "  ac_chol_d - FAILED - Error tolerance(s) exceeded" << endl;                            // LCOV_EXCL_LINE
      cout << "=============================================================================" << endl; // LCOV_EXCL_LINE
      return -1;                                                                                       // LCOV_EXCL_LINE
   }
   else
   {
      cout << "  ac_chol_d - PASSED" << endl;
      cout << "=============================================================================" << endl;
   }
   return 0;
}
