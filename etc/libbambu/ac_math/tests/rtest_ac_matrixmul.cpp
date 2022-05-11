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
// ac_matrixmul() function using a variety of data types and bit-
// widths.

// To compile standalone and run:
//   $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_matrixmul.cpp -o design
//   ./design

// Include the AC Math function that is exercised with this testbench
#include <ac_math/ac_matrixmul.h>
using namespace ac_math;

// ==============================================================================
// Test Designs
//   These simple functions allow executing the ac_matrixmul() function
//   using multiple data types at the same time. Template parameters are
//   used to configure the bit-widths of the types.
template <unsigned M, unsigned N, unsigned P, int Wfi1, int Ifi1, bool Sfi1, int Wfi2, int Ifi2, bool Sfi2, int outWfi,
          int outIfi, bool outSfi>
void test_ac_matrixmul(const ac_fixed<Wfi1, Ifi1, Sfi1, AC_TRN, AC_WRAP> A1[M][N],
                       const ac_fixed<Wfi2, Ifi2, Sfi2, AC_TRN, AC_WRAP> B1[N][P],
                       ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> C1[M][P],
                       const ac_complex<ac_fixed<Wfi1, Ifi1, Sfi1, AC_TRN, AC_WRAP>> A2[M][N],
                       const ac_complex<ac_fixed<Wfi2, Ifi2, Sfi2, AC_TRN, AC_WRAP>> B2[N][P],
                       ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>> C2[M][P],
                       const ac_matrix<ac_fixed<Wfi1, Ifi1, Sfi1, AC_TRN, AC_WRAP>, M, N>& A3,
                       const ac_matrix<ac_fixed<Wfi2, Ifi2, Sfi2, AC_TRN, AC_WRAP>, N, P>& B3,
                       ac_matrix<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>, M, P>& C3,
                       const ac_matrix<ac_complex<ac_fixed<Wfi1, Ifi1, Sfi1, AC_TRN, AC_WRAP>>, M, N>& A4,
                       const ac_matrix<ac_complex<ac_fixed<Wfi2, Ifi2, Sfi2, AC_TRN, AC_WRAP>>, N, P>& B4,
                       ac_matrix<ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>>, M, P>& C4)
{
   ac_matrixmul<M, N, P>(A1, B1, C1);
   ac_matrixmul<M, N, P>(A2, B2, C2);
   ac_matrixmul(A3, B3, C3);
   ac_matrixmul(A4, B4, C4);
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

// Copy a C-style array's contents over to an ac_matrix.
template <unsigned M, unsigned N, class T_matrix, class T_ac_matrix>
void copy_to_A_ac_matrix(const T_matrix array_2D[M][N], T_ac_matrix& output)
{
   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)N; j++)
      {
         output(i, j) = array_2D[i][j];
      }
   }
}

template <unsigned N, unsigned P, class T_matrix, class T_ac_matrix>
void copy_to_B_ac_matrix(const T_matrix array_2D[N][P], T_ac_matrix& output)
{
   for(int i = 0; i < (int)N; i++)
   {
      for(int j = 0; j < (int)P; j++)
      {
         output(i, j) = array_2D[i][j];
      }
   }
}

// Copy an ac_matrix's contents over to a C-style array.
template <unsigned M, unsigned P, class T_matrix, class T_ac_matrix>
void copy_to_C_array_2D(const T_ac_matrix& input, T_matrix array_2D[M][P])
{
   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)P; j++)
      {
         array_2D[i][j] = input(i, j);
      }
   }
}

// Matrix generator for ac_fixed
template <unsigned M, unsigned N, unsigned P, class T_in_A, class T_in_B>
void gen_matrix(T_in_A A[M][N], T_in_B B[N][P])
{
   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)N; j++)
      {
         ac_random(A[i][j]);
      }
   }
   for(int i = 0; i < (int)N; i++)
   {
      for(int j = 0; j < (int)P; j++)
      {
         ac_random(B[i][j]);
      }
   }
}
// Matrix generator for ac_complex<ac_fixed>
template <unsigned M, unsigned N, unsigned P, class T_in_A, class T_in_B>
void gen_matrix(ac_complex<T_in_A> A[M][N], ac_complex<T_in_B> B[N][P])
{
   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)N; j++)
      {
         ac_random(A[i][j].r());
         ac_random(A[i][j].i());
      }
   }
   for(int i = 0; i < (int)N; i++)
   {
      for(int j = 0; j < (int)P; j++)
      {
         ac_random(B[i][j].r());
         ac_random(B[i][j].i());
      }
   }
}

// Testbench for matrix multiplication for ac_matrix<ac_fixed>
template <unsigned M, unsigned N, unsigned P, class T_in_A, class T_in_B>
void matrixmul_tb(const T_in_A A[M][N], const T_in_B B[N][P], double C_tb[M][P])
{
   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)P; j++)
      {
         double sum = 0;
         for(int k = 0; k < (int)N; k++)
         {
            sum = sum + A[i][k].to_double() * B[k][j].to_double();
         }
         C_tb[i][j] = sum;
      }
   }
}
// Testbench for matrix multiplication for ac_matrix<ac_complex<ac_fixed> >
template <unsigned M, unsigned N, unsigned P, class T_in_A, class T_in_B>
void matrixmul_tb(const ac_complex<T_in_A> A[M][N], const ac_complex<T_in_B> B[N][P], ac_complex<double> C_tb[M][P])
{
   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)P; j++)
      {
         double sumreal = 0;
         double sumimag = 0;
         for(int k = 0; k < (int)N; k++)
         {
            sumreal = sumreal + A[i][k].r().to_double() * B[k][j].r().to_double() -
                      A[i][k].i().to_double() * B[k][j].i().to_double();
            sumimag = sumimag + A[i][k].r().to_double() * B[k][j].i().to_double() +
                      A[i][k].i().to_double() * B[k][j].r().to_double();
            ;
         }
         C_tb[i][j].r() = sumreal;
         C_tb[i][j].i() = sumimag;
      }
   }
}

// Convert complex double element to mag_sqr.
double conv_val(ac_complex<double> x)
{
   return x.mag_sqr();
}

// Comparing the actual output matrix to the testbench output matrix for ac_matrix<ac_fixed>
template <unsigned M, unsigned P, class T_op>
double compare_matrices(const T_op C[M][P], const double C_tb[M][P], const double allowed_error)
{
   double this_error, max_error = 0;
   double exp_op;
   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)P; j++)
      {
         // Use typecasting to perform quantization on testbench double output
         exp_op = ((T_op)C_tb[i][j]).to_double();
         this_error = (abs(C[i][j].to_double() - exp_op) / exp_op) * 100;
         if(this_error > max_error)
         {
            max_error = this_error;
         }
      }
   }
   return max_error;
}
// Comparing the actual output matrix to the testbench output matrix for ac_matrix<ac_complex<ac_fixed> >
template <unsigned M, unsigned P, class T_op>
double compare_matrices(const ac_complex<T_op> C[M][P], const ac_complex<double> C_tb[M][P], const double allowed_error)
{
   double this_error, max_error = 0;
   ac_complex<double> act_op, exp_op;
   for(int i = 0; i < (int)M; i++)
   {
      for(int j = 0; j < (int)P; j++)
      {
         act_op.r() = C[i][j].r().to_double();
         act_op.i() = C[i][j].i().to_double();
         // Use typecasting to perform quantization on testbench double output
         exp_op.r() = ((T_op)C_tb[i][j].r()).to_double();
         exp_op.i() = ((T_op)C_tb[i][j].i()).to_double();
         this_error = (abs(conv_val(act_op) - conv_val(exp_op)) / conv_val(exp_op)) * 100;
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

template <unsigned M, unsigned N, unsigned P, int Wfi1, int Ifi1, bool Sfi1, int Wfi2, int Ifi2, bool Sfi2, int outWfi,
          int outIfi, bool outSfi>
int test_driver(double& cumulative_max_error, double& cumulative_max_error_cmplx, const double allowed_error)
{
   bool passed = true;

   ac_fixed<Wfi1, Ifi1, Sfi1, AC_TRN, AC_WRAP> A_c_array[M][N];
   ac_fixed<Wfi2, Ifi2, Sfi2, AC_TRN, AC_WRAP> B_c_array[N][P];
   ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> C_c_array[M][P];
   ac_complex<ac_fixed<Wfi1, Ifi1, Sfi1, AC_TRN, AC_WRAP>> cmplx_A_c_array[M][N];
   ac_complex<ac_fixed<Wfi2, Ifi2, Sfi2, AC_TRN, AC_WRAP>> cmplx_B_c_array[N][P];
   ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>> cmplx_C_c_array[M][P];
   ac_matrix<ac_fixed<Wfi1, Ifi1, Sfi1, AC_TRN, AC_WRAP>, M, N> A_ac_matrix;
   ac_matrix<ac_fixed<Wfi2, Ifi2, Sfi2, AC_TRN, AC_WRAP>, N, P> B_ac_matrix;
   ac_matrix<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>, M, P> C_ac_matrix;
   ac_matrix<ac_complex<ac_fixed<Wfi1, Ifi1, Sfi1, AC_TRN, AC_WRAP>>, M, N> cmplx_A_ac_matrix;
   ac_matrix<ac_complex<ac_fixed<Wfi2, Ifi2, Sfi2, AC_TRN, AC_WRAP>>, N, P> cmplx_B_ac_matrix;
   ac_matrix<ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>>, M, P> cmplx_C_ac_matrix;

   cout << "TEST: ac_matrixmul(), M = ";
   cout << M << ",";
   cout << " N = ";
   cout << N << ",";
   cout << " P = ";
   cout << P << ",";
   cout << "FIRST INPUT: ";
   cout.width(38);
   cout << left << A_c_array[0][0].type_name();
   cout << "SECOND INPUT: ";
   cout.width(38);
   cout << left << B_c_array[0][0].type_name();
   cout << "OUTPUT: ";
   cout.width(38);
   cout << left << C_c_array[0][0].type_name();
   cout << "RESULT: ";

   double C_tb[M][P];
   ac_complex<double> cmplx_C_tb[M][P];

   // initialize the matrix elements using ac_random
   gen_matrix<M, N, P>(A_c_array, B_c_array);
   gen_matrix<M, N, P>(cmplx_A_c_array, cmplx_B_c_array);

   copy_to_A_ac_matrix<M, N>(A_c_array, A_ac_matrix);
   copy_to_B_ac_matrix<N, P>(B_c_array, B_ac_matrix);
   copy_to_A_ac_matrix<M, N>(cmplx_A_c_array, cmplx_A_ac_matrix);
   copy_to_B_ac_matrix<N, P>(cmplx_B_c_array, cmplx_B_ac_matrix);

   test_ac_matrixmul(A_c_array, B_c_array, C_c_array, cmplx_A_c_array, cmplx_B_c_array, cmplx_C_c_array, A_ac_matrix,
                     B_ac_matrix, C_ac_matrix, cmplx_A_ac_matrix, cmplx_B_ac_matrix, cmplx_C_ac_matrix);

   ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> C_ac_matrix_converted[M][P];
   ac_complex<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>> C_cmplx_ac_matrix_converted[M][P];

   copy_to_C_array_2D<M, P>(C_ac_matrix, C_ac_matrix_converted);
   copy_to_C_array_2D<M, P>(cmplx_C_ac_matrix, C_cmplx_ac_matrix_converted);

   // Get output of testbench function for matrix multiplication.
   matrixmul_tb<M, N, P>(A_c_array, B_c_array, C_tb);
   matrixmul_tb<M, N, P>(cmplx_A_c_array, cmplx_B_c_array, cmplx_C_tb);

   // Compare matrices and get the max error
   double max_error = compare_matrices<M, P>(C_c_array, C_tb, allowed_error);
   double max_error_cmplx = compare_matrices<M, P>(cmplx_C_c_array, cmplx_C_tb, allowed_error);
   double max_error_ac_matrix = compare_matrices<M, P>(C_ac_matrix_converted, C_tb, allowed_error);
   double max_error_cmplx_ac_matrix = compare_matrices<M, P>(C_cmplx_ac_matrix_converted, cmplx_C_tb, allowed_error);

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
   double allowed_error = 0.5;

   cout << "=============================================================================" << endl;
   cout << "Testing function: ac_matrixmul(), for scalar and complex datatypes - allowed error = " << allowed_error
        << endl;

   // template <unsigned M, unsigned N, unsigned P, int Wfi1, int Ifi1, bool Sfi1, int Wfi2, int Ifi2, bool Sfi2, int
   // outWfi, int outIfi, bool outSfi>
   test_driver<6, 12, 8, 20, 11, false, 24, 12, false, 50, 26, false>(max_error, cmplx_max_error, allowed_error);
   test_driver<9, 8, 4, 18, 12, true, 23, 13, true, 52, 24, true>(max_error, cmplx_max_error, allowed_error);
   test_driver<8, 7, 9, 17, 13, true, 23, 13, true, 49, 28, true>(max_error, cmplx_max_error, allowed_error);
   test_driver<10, 9, 12, 15, 9, true, 19, 11, true, 53, 22, true>(max_error, cmplx_max_error, allowed_error);
   test_driver<13, 11, 11, 19, 8, true, 17, 8, true, 55, 20, true>(max_error, cmplx_max_error, allowed_error);

   cout << "=============================================================================" << endl;
   cout << "  Testbench finished. Maximum errors observed across all data type / bit-width variations:" << endl;
   cout << "    max_error            = " << max_error << endl;
   cout << "    cmplx_max_error      = " << cmplx_max_error << endl;

   // If error limits on any tested datatype have been crossed, the test has failed
   bool test_fail = (max_error > allowed_error) || (cmplx_max_error > allowed_error);

   // Notify the user whether or not the test was a failure.
   if(test_fail)
   {
      cout << "  ac_matrixmul - FAILED - Error tolerance(s) exceeded" << endl;                         // LCOV_EXCL_LINE
      cout << "=============================================================================" << endl; // LCOV_EXCL_LINE
      return -1;                                                                                       // LCOV_EXCL_LINE
   }
   else
   {
      cout << "  ac_matrixmul - PASSED" << endl;
      cout << "=============================================================================" << endl;
   }
   return 0;
}
