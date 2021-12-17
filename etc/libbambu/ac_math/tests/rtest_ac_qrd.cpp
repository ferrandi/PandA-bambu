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
// ac_qrd() function using a variety of data types and bit-
// widths.

// To compile standalone and run:
//   $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_qrd.cpp -o design
//   ./design

// Include the AC Math function that is exercised with this testbench
#include <ac_math/ac_qrd.h>
using namespace ac_math;

// ==============================================================================
// Test Designs
//   These simple functions allow executing the ac_qrd() function
//   using multiple data types at the same time. Template parameters are
//   used to configure the bit-widths of the types.

// Test Design for real and complex fixed point values, using PWL functions
// for QR decomposition.
template <unsigned M, int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi, bool outSfi>
void test_ac_qrd_pwl(ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP> in_arr[M][M],
                     ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> Q_arr[M][M],
                     ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> R_arr[M][M],
                     ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>> in_cmplx_arr[M][M],
                     ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>> Q_cmplx_arr[M][M],
                     ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>> R_cmplx_arr[M][M],
                     ac_matrix<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>, M, M>& in_matrix,
                     ac_matrix<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>, M, M>& Q_matrix,
                     ac_matrix<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>, M, M>& R_matrix,
                     ac_matrix<ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>>, M, M>& in_cmplx_matrix,
                     ac_matrix<ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>>, M, M>& Q_cmplx_matrix,
                     ac_matrix<ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>>, M, M>& R_cmplx_matrix)
{
   ac_qrd<true>(in_arr, Q_arr, R_arr);
   ac_qrd<true>(in_cmplx_arr, Q_cmplx_arr, R_cmplx_arr);
   ac_qrd<true>(in_matrix, Q_matrix, R_matrix);
   ac_qrd<true>(in_cmplx_matrix, Q_cmplx_matrix, R_cmplx_matrix);
}

// Test Design for real and complex fixed point values, using the accurate
// functions for QR decomposition.
template <unsigned M, int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi, bool outSfi>
void test_ac_qrd_accurate(
    ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP> in_arr[M][M],
    ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> Q_arr[M][M],
    ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> R_arr[M][M],
    ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>> in_cmplx_arr[M][M],
    ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>> Q_cmplx_arr[M][M],
    ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>> R_cmplx_arr[M][M],
    ac_matrix<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>, M, M>& in_matrix,
    ac_matrix<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>, M, M>& Q_matrix,
    ac_matrix<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>, M, M>& R_matrix,
    ac_matrix<ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>>, M, M>& in_cmplx_matrix,
    ac_matrix<ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>>, M, M>& Q_cmplx_matrix,
    ac_matrix<ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>>, M, M>& R_cmplx_matrix)
{
   ac_qrd<false>(in_arr, Q_arr, R_arr);
   ac_qrd<false>(in_cmplx_arr, Q_cmplx_arr, R_cmplx_arr);
   ac_qrd<false>(in_matrix, Q_matrix, R_matrix);
   ac_qrd<false>(in_cmplx_matrix, Q_cmplx_matrix, R_cmplx_matrix);
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

// Overloaded function to convert type specific test input data to double
template <int input_width, int input_int, bool input_S, ac_q_mode input_Q, ac_o_mode input_O>
double type_to_double(ac_fixed<input_width, input_int, input_S, input_Q, input_O>& type_value)
{
   return type_value.to_double();
}

// Overloaded function to convert type specific test input data to ac_complex<double>
template <int input_width, int input_int, bool input_S, ac_q_mode input_Q, ac_o_mode input_O>
ac_complex<double> type_to_double(ac_complex<ac_fixed<input_width, input_int, input_S, input_Q, input_O>>& type_value)
{
   ac_complex<double> double_value;
   double_value.r() = type_to_double(type_value.r());
   double_value.i() = type_to_double(type_value.i());
   return double_value;
}

// Copy a C-style array's contents over to an ac_matrix.
template <unsigned M, class T_matrix, class T_ac_matrix>
void copy_to_input_ac_matrix(T_matrix array_2D[M][M], T_ac_matrix& output)
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
void copy_to_C_array_2D(T_ac_matrix& input, T_matrix array_2D[M][M])
{
   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)M; j++)
      {
         array_2D[i][j] = input(i, j);
      }
   }
}

// Matrix generator for ac_fixed
template <unsigned M, class T_in>
void gen_matrix(T_in in_arr[M][M])
{
   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)M; j++)
      {
         ac_random(in_arr[i][j]);
      }
   }
}
// Matrix generator for ac_complex<ac_fixed>
template <unsigned M, class T_in>
void gen_matrix(ac_complex<T_in> in_cmplx_arr[M][M])
{
   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)M; j++)
      {
         ac_random(in_cmplx_arr[i][j].r());
         ac_random(in_cmplx_arr[i][j].i());
      }
   }
}

// Overloaded implementation for square root of complex numbers, defined using double datatypes
ac_complex<double> sqrt(ac_complex<double> input)
{
   ac_complex<double> output;
   double a = input.r();
   double b = input.i();
   double mod = a * a + b * b;
   double sqrt_mod = sqrt(mod);
   double sqr_x = (sqrt_mod + a) / 2.0;
   double sqr_y = (sqrt_mod - a) / 2.0;
   double x = sqrt(sqr_x);
   double y = sqrt(sqr_y);
   output.r() = x;
   output.i() = y;
   return output;
}

// Matrix multiplication
template <unsigned M, typename T>
void matrixmul_double(T Q[M][M], T R[M][M], T mult[M][M])
{
   T sum = 0;
   for(unsigned i = 0; i < M; i++)
   {
      for(unsigned j = 0; j < M; j++)
      {
         for(unsigned k = 0; k < M; k++)
         {
            sum = sum + Q[i][k] * R[k][j];
         }
         mult[i][j] = sum;
         sum = 0;
      }
   }
}

template <unsigned M, typename T>
void matrixmul_double(ac_matrix<T, M, M>& Q, ac_matrix<T, M, M>& R, ac_matrix<T, M, M>& mult)
{
   T sum = 0;
   for(unsigned i = 0; i < M; i++)
   {
      for(unsigned j = 0; j < M; j++)
      {
         for(unsigned k = 0; k < M; k++)
         {
            sum = sum + Q(i, k) * R(k, j);
         }
         mult(i, j) = sum;
         sum = 0;
      }
   }
}

// Convert complex double element to mag_sqr.
double conv_val(ac_complex<double> x)
{
   return x.mag_sqr();
}

// Comparing the actual output matrix to the testbench output matrix for ac_matrix<ac_fixed>
template <unsigned M>
double compare_matrices(double A[M][M], double B[M][M], const double allowed_error)
{
   double this_error, max_error = 0;
   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)M; j++)
      {
         this_error = (abs(A[i][j] - B[i][j]) / B[i][j]) * 100;
         if(this_error > max_error)
         {
            max_error = this_error;
         }
      }
   }
   return max_error;
}
// Comparing the actual output matrix to the testbench output matrix for ac_matrix<ac_complex<ac_fixed> >
template <unsigned M>
double compare_matrices(ac_complex<double> A[M][M], ac_complex<double> B[M][M], const double allowed_error)
{
   double this_error, max_error = 0;
   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)M; j++)
      {
         // The error is calculated as the difference between the value of the expected
         // vs. the actual value, normalized w.r.t. the max_value in the matrix. For complex
         // numbers, the expected vs actual values are first converted to the their
         // mag_sqr() representations. For real numbers, the values are passed as they are
         // for the error calculation.
         this_error = (abs(conv_val(A[i][j]) - conv_val(B[i][j])) / conv_val(B[i][j])) * 100;
         if(this_error > max_error)
         {
            max_error = this_error;
         }
      }
   }
   return max_error;
}

//==============================================================================
// Function: test_driver()
// Description: A templatized function that can be configured for certain bit-
//   widths of the fixed point AC datatype. It uses the type information to
//   iterate through a range of valid values on that type in order to compare
//   the precision of the DUT matrix multiplication with the computed matrix
//   multiplication using the standard C double types. The maximum error for each
//   type is accumulated in variables defined in the calling function.

template <unsigned M, int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi, bool outSfi>
int test_driver_pwl(double& cumulative_max_error, double& cumulative_max_error_cmplx, const double allowed_error)
{
   bool passed = true;

   ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP> in_arr[M][M];
   ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> Q_arr[M][M];
   ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> R_arr[M][M];
   ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>> in_cmplx_arr[M][M];
   ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>> Q_cmplx_arr[M][M];
   ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>> R_cmplx_arr[M][M];
   ac_matrix<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>, M, M> in_matrix;
   ac_matrix<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>, M, M> Q_matrix;
   ac_matrix<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>, M, M> R_matrix;
   ac_matrix<ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>>, M, M> in_cmplx_matrix;
   ac_matrix<ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>>, M, M> Q_cmplx_matrix;
   ac_matrix<ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>>, M, M> R_cmplx_matrix;

   cout << "TEST: ac_qrd(), M = ";
   cout << M << ",";
   cout << "INPUT: ";
   cout.width(38);
   cout << left << in_arr[0][0].type_name();
   cout << "FIRST OUTPUT: ";
   cout.width(38);
   cout << left << Q_arr[0][0].type_name();
   cout << "SECOND OUTPUT: ";
   cout.width(38);
   cout << left << R_arr[0][0].type_name();
   cout << "RESULT: ";

   double input_double[M][M];
   double Q_actual_double[M][M];
   double R_actual_double[M][M];
   double Q_actual_matrix_double[M][M];
   double R_actual_matrix_double[M][M];
   double recovered_double[M][M];
   double recovered_matrix_double[M][M];
   ac_complex<double> input_cmplx_double[M][M];
   ac_complex<double> Q_actual_cmplx_double[M][M];
   ac_complex<double> R_actual_cmplx_double[M][M];
   ac_complex<double> Q_actual_matrix_cmplx_double[M][M];
   ac_complex<double> R_actual_matrix_cmplx_double[M][M];
   ac_complex<double> recovered_cmplx_double[M][M];
   ac_complex<double> recovered_cmplx_matrix_double[M][M];

   // initialize the matrix elements using ac_random
   gen_matrix(in_arr);
   gen_matrix(in_cmplx_arr);

   copy_to_input_ac_matrix(in_arr, in_matrix);
   copy_to_input_ac_matrix(in_cmplx_arr, in_cmplx_matrix);

   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)M; j++)
      {
         input_double[i][j] = type_to_double(in_arr[i][j]);
         input_cmplx_double[i][j] = type_to_double(in_cmplx_arr[i][j]);
      }
   }

   test_ac_qrd_pwl(in_arr, Q_arr, R_arr, in_cmplx_arr, Q_cmplx_arr, R_cmplx_arr, in_matrix, Q_matrix, R_matrix,
                   in_cmplx_matrix, Q_cmplx_matrix, R_cmplx_matrix);

   ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> Q_ac_matrix_converted[M][M];
   ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> R_ac_matrix_converted[M][M];
   ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>> Q_cmplx_ac_matrix_converted[M][M];
   ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>> R_cmplx_ac_matrix_converted[M][M];

   copy_to_C_array_2D(Q_matrix, Q_ac_matrix_converted);
   copy_to_C_array_2D(R_matrix, R_ac_matrix_converted);
   copy_to_C_array_2D(Q_cmplx_matrix, Q_cmplx_ac_matrix_converted);
   copy_to_C_array_2D(R_cmplx_matrix, R_cmplx_ac_matrix_converted);

   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)M; j++)
      {
         Q_actual_double[i][j] = type_to_double(Q_arr[i][j]);
         R_actual_double[i][j] = type_to_double(R_arr[i][j]);
         Q_actual_matrix_double[i][j] = type_to_double(Q_ac_matrix_converted[i][j]);
         R_actual_matrix_double[i][j] = type_to_double(R_ac_matrix_converted[i][j]);
         Q_actual_cmplx_double[i][j] = type_to_double(Q_cmplx_arr[i][j]);
         R_actual_cmplx_double[i][j] = type_to_double(R_cmplx_arr[i][j]);
         Q_actual_matrix_cmplx_double[i][j] = type_to_double(Q_cmplx_ac_matrix_converted[i][j]);
         R_actual_matrix_cmplx_double[i][j] = type_to_double(R_cmplx_ac_matrix_converted[i][j]);
      }
   }

   // Recover the original matrix value
   matrixmul_double(Q_actual_double, R_actual_double, recovered_double);
   matrixmul_double(Q_actual_cmplx_double, R_actual_cmplx_double, recovered_cmplx_double);
   matrixmul_double(Q_actual_matrix_double, R_actual_matrix_double, recovered_matrix_double);
   matrixmul_double(Q_actual_matrix_cmplx_double, R_actual_matrix_cmplx_double, recovered_cmplx_matrix_double);

   // Compare matrices and get the max error
   double max_error = compare_matrices(recovered_double, input_double, allowed_error);
   double max_error_cmplx = compare_matrices(recovered_cmplx_double, input_cmplx_double, allowed_error);
   double max_error_ac_matrix = compare_matrices(recovered_matrix_double, input_double, allowed_error);
   double max_error_cmplx_ac_matrix =
       compare_matrices(recovered_cmplx_matrix_double, input_cmplx_double, allowed_error);

   // Put max overall error in a separate variable.
   double max_error_overall = max_error > max_error_ac_matrix ? max_error : max_error_ac_matrix;
   double max_error_cmplx_overall =
       max_error_cmplx > max_error_cmplx_ac_matrix ? max_error_cmplx : max_error_cmplx_ac_matrix;

   passed = (max_error_overall < allowed_error) && (max_error_cmplx_overall < allowed_error);

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

template <unsigned M, int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi, bool outSfi>
int test_driver_accurate(double& cumulative_max_error, double& cumulative_max_error_cmplx, const double allowed_error)
{
   bool passed = true;

   ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP> in_arr[M][M];
   ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> Q_arr[M][M];
   ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> R_arr[M][M];
   ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>> in_cmplx_arr[M][M];
   ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>> Q_cmplx_arr[M][M];
   ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>> R_cmplx_arr[M][M];
   ac_matrix<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>, M, M> in_matrix;
   ac_matrix<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>, M, M> Q_matrix;
   ac_matrix<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>, M, M> R_matrix;
   ac_matrix<ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>>, M, M> in_cmplx_matrix;
   ac_matrix<ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>>, M, M> Q_cmplx_matrix;
   ac_matrix<ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>>, M, M> R_cmplx_matrix;

   cout << "TEST: ac_qrd(), M = ";
   cout << M << ",";
   cout << "INPUT: ";
   cout.width(38);
   cout << left << in_arr[0][0].type_name();
   cout << "FIRST OUTPUT: ";
   cout.width(38);
   cout << left << Q_arr[0][0].type_name();
   cout << "SECOND OUTPUT: ";
   cout.width(38);
   cout << left << R_arr[0][0].type_name();
   cout << "RESULT: ";

   double input_double[M][M];
   double Q_actual_double[M][M];
   double R_actual_double[M][M];
   double Q_actual_matrix_double[M][M];
   double R_actual_matrix_double[M][M];
   double recovered_double[M][M];
   double recovered_matrix_double[M][M];
   ac_complex<double> input_cmplx_double[M][M];
   ac_complex<double> Q_actual_cmplx_double[M][M];
   ac_complex<double> R_actual_cmplx_double[M][M];
   ac_complex<double> Q_actual_matrix_cmplx_double[M][M];
   ac_complex<double> R_actual_matrix_cmplx_double[M][M];
   ac_complex<double> recovered_cmplx_double[M][M];
   ac_complex<double> recovered_cmplx_matrix_double[M][M];

   // initialize the matrix elements using ac_random
   gen_matrix(in_arr);
   gen_matrix(in_cmplx_arr);

   copy_to_input_ac_matrix(in_arr, in_matrix);
   copy_to_input_ac_matrix(in_cmplx_arr, in_cmplx_matrix);

   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)M; j++)
      {
         input_double[i][j] = type_to_double(in_arr[i][j]);
         input_cmplx_double[i][j] = type_to_double(in_cmplx_arr[i][j]);
      }
   }

   test_ac_qrd_accurate(in_arr, Q_arr, R_arr, in_cmplx_arr, Q_cmplx_arr, R_cmplx_arr, in_matrix, Q_matrix, R_matrix,
                        in_cmplx_matrix, Q_cmplx_matrix, R_cmplx_matrix);

   ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> Q_ac_matrix_converted[M][M];
   ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> R_ac_matrix_converted[M][M];
   ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>> Q_cmplx_ac_matrix_converted[M][M];
   ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>> R_cmplx_ac_matrix_converted[M][M];

   copy_to_C_array_2D(Q_matrix, Q_ac_matrix_converted);
   copy_to_C_array_2D(R_matrix, R_ac_matrix_converted);
   copy_to_C_array_2D(Q_cmplx_matrix, Q_cmplx_ac_matrix_converted);
   copy_to_C_array_2D(R_cmplx_matrix, R_cmplx_ac_matrix_converted);

   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)M; j++)
      {
         Q_actual_double[i][j] = type_to_double(Q_arr[i][j]);
         R_actual_double[i][j] = type_to_double(R_arr[i][j]);
         Q_actual_matrix_double[i][j] = type_to_double(Q_ac_matrix_converted[i][j]);
         R_actual_matrix_double[i][j] = type_to_double(R_ac_matrix_converted[i][j]);
         Q_actual_cmplx_double[i][j] = type_to_double(Q_cmplx_arr[i][j]);
         R_actual_cmplx_double[i][j] = type_to_double(R_cmplx_arr[i][j]);
         Q_actual_matrix_cmplx_double[i][j] = type_to_double(Q_cmplx_ac_matrix_converted[i][j]);
         R_actual_matrix_cmplx_double[i][j] = type_to_double(R_cmplx_ac_matrix_converted[i][j]);
      }
   }

   // Recover the original matrix value
   matrixmul_double(Q_actual_double, R_actual_double, recovered_double);
   matrixmul_double(Q_actual_cmplx_double, R_actual_cmplx_double, recovered_cmplx_double);
   matrixmul_double(Q_actual_matrix_double, R_actual_matrix_double, recovered_matrix_double);
   matrixmul_double(Q_actual_matrix_cmplx_double, R_actual_matrix_cmplx_double, recovered_cmplx_matrix_double);

   // Compare matrices and get the max error
   double max_error = compare_matrices(recovered_double, input_double, allowed_error);
   double max_error_cmplx = compare_matrices(recovered_cmplx_double, input_cmplx_double, allowed_error);
   double max_error_ac_matrix = compare_matrices(recovered_matrix_double, input_double, allowed_error);
   double max_error_cmplx_ac_matrix =
       compare_matrices(recovered_cmplx_matrix_double, input_cmplx_double, allowed_error);

   // Put max overall error in a separate variable.
   double max_error_overall = max_error > max_error_ac_matrix ? max_error : max_error_ac_matrix;
   double max_error_cmplx_overall =
       max_error_cmplx > max_error_cmplx_ac_matrix ? max_error_cmplx : max_error_cmplx_ac_matrix;

   passed = (max_error_overall < allowed_error) && (max_error_cmplx_overall < allowed_error);

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
   double max_error = 0, cmplx_max_error = 0;
   double allowed_error = 4;

   cout << "=============================================================================" << endl;
   cout << "Testing function: ac_qrd(), for scalar and complex datatypes - allowed error = " << allowed_error << endl;

   // template <unsigned M, int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi, bool outSfi>
   test_driver_pwl<3, 16, 8, true, 64, 32, true>(max_error, cmplx_max_error, allowed_error);
   test_driver_pwl<4, 12, 5, true, 64, 32, true>(max_error, cmplx_max_error, allowed_error);
   test_driver_pwl<3, 7, 4, true, 64, 32, true>(max_error, cmplx_max_error, allowed_error);
   test_driver_pwl<2, 15, 7, true, 64, 32, true>(max_error, cmplx_max_error, allowed_error);
   test_driver_pwl<5, 11, 4, true, 64, 32, true>(max_error, cmplx_max_error, allowed_error);

   // template <unsigned M, int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi, bool outSfi>
   test_driver_accurate<3, 16, 7, true, 64, 32, true>(max_error, cmplx_max_error, allowed_error);
   test_driver_accurate<4, 12, 5, true, 64, 32, true>(max_error, cmplx_max_error, allowed_error);
   test_driver_accurate<3, 7, 4, true, 64, 32, true>(max_error, cmplx_max_error, allowed_error);
   test_driver_accurate<2, 15, 7, true, 64, 32, true>(max_error, cmplx_max_error, allowed_error);
   test_driver_accurate<5, 11, 4, true, 64, 32, true>(max_error, cmplx_max_error, allowed_error);

   cout << "=============================================================================" << endl;
   cout << "  Testbench finished. Maximum errors observed across all data type / bit-width variations:" << endl;
   cout << "    max_error            = " << max_error << endl;
   cout << "    cmplx_max_error      = " << cmplx_max_error << endl;

   // If error limits on any tested datatype have been crossed, the test has failed
   bool test_fail = (max_error > allowed_error) || (cmplx_max_error > allowed_error);

   // Notify the user whether or not the test was a failure.
   if(test_fail)
   {
      cout << "  ac_qrd - FAILED - Error tolerance(s) exceeded" << endl;                               // LCOV_EXCL_LINE
      cout << "=============================================================================" << endl; // LCOV_EXCL_LINE
      return -1;                                                                                       // LCOV_EXCL_LINE
   }
   else
   {
      cout << "  ac_qrd - PASSED" << endl;
      cout << "=============================================================================" << endl;
   }
   return 0;
}
