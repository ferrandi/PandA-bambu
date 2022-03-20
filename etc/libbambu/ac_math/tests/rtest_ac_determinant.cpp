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
// ac_determinant() function using a variety of data types and bit-
// widths.

// To compile standalone and run:
//   $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_determinant.cpp -o design
//   ./design

// Include the AC Math function that is exercised with this testbench
#include <ac_math/ac_determinant.h>
using namespace ac_math;

// ==============================================================================
// Test Design
//   This simple functions allow executing the ac_determinant() function
//   using multiple data types at the same time. Template parameters are
//   used to configure the bit-widths of the types.

template <unsigned M, int Wfi, int Ifi, bool Sfi>
void test_ac_determinant(
    const ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP> A1[M][M],
    ac_fixed<(M * Wfi + ac::log2_floor<Factorial<M>::value>::val), (M * Ifi + ac::log2_floor<Factorial<M>::value>::val),
             true, AC_TRN, AC_WRAP>& det1,
    const ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>> A2[M][M],
    ac_complex<ac_fixed<(M * Wfi + ac::log2_floor<Factorial<M>::value>::val),
                        (M * Ifi + ac::log2_floor<Factorial<M>::value>::val), true, AC_TRN, AC_WRAP>>& det2,
    const ac_matrix<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>, M, M>& A3,
    ac_fixed<(M * Wfi + ac::log2_floor<Factorial<M>::value>::val), (M * Ifi + ac::log2_floor<Factorial<M>::value>::val),
             true, AC_TRN, AC_WRAP>& det3,
    const ac_matrix<ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>>, M, M>& A4,
    ac_complex<ac_fixed<(M * Wfi + ac::log2_floor<Factorial<M>::value>::val),
                        (M * Ifi + ac::log2_floor<Factorial<M>::value>::val), true, AC_TRN, AC_WRAP>>& det4)
{
   det1 = ac_determinant<ac_fixed<(M * Wfi + ac::log2_floor<Factorial<M>::value>::val),
                                  (M * Ifi + ac::log2_floor<Factorial<M>::value>::val), true, AC_TRN, AC_WRAP>>(A1);
   det2 = ac_determinant<
       ac_complex<ac_fixed<(M * Wfi + ac::log2_floor<Factorial<M>::value>::val),
                           (M * Ifi + ac::log2_floor<Factorial<M>::value>::val), true, AC_TRN, AC_WRAP>>>(A2);
   det3 = ac_determinant<ac_fixed<(M * Wfi + ac::log2_floor<Factorial<M>::value>::val),
                                  (M * Ifi + ac::log2_floor<Factorial<M>::value>::val), true, AC_TRN, AC_WRAP>>(A3);
   det4 = ac_determinant<
       ac_complex<ac_fixed<(M * Wfi + ac::log2_floor<Factorial<M>::value>::val),
                           (M * Ifi + ac::log2_floor<Factorial<M>::value>::val), true, AC_TRN, AC_WRAP>>>(A4);
}

// ==============================================================================

#include <ac_math/ac_random.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <string>
using namespace std;

// ------------------------------------------------------------------------------
// Helper structs and functions for testbench determinant computation

// Template recursion is performed here to compute determinant of a function for
// datatype supplied to the implementation (given by template parameter T).
// Note that, helper struct does template recursion where as datatype is provided
// to the function in the form of typename T.
template <unsigned M>
struct determinant_double
{
   template <typename T>
   static T determinant_comp(T a[M][M])
   {
      // defining the arrays to store the determinant answers of smaller matrices and storing the answer of the
      // accumulation output.
      T c[M], temp1, b[M - 1][M - 1];
      unsigned j, p, q;
      int pr = -1;
      T d = 0;
      // outer for loop is used for going through pivot
      for(j = 0; j < M; j++)
      {
         // used for indicator variables for smaller matrix to store the submatrix of the original matrix.
         // Outer for loop is used for accessing elements of the matrix row-wise.
         for(p = 1; p < M; p++)
         {
            // Outer for loop is used for accessing elements of the matrix column-wise.
            for(q = 1; q < M; q++)
            {
               b[p - 1][q - 1] = a[p][q - (q <= j)];
            }
         }
         // for sign alteration in the determinant computation.
         pr = (-1) * pr;
         // The output of the function for M-1 dimension is stored in the accumulator array after doing sign adjustment
         // via pr.
         c[j] = pr * determinant_double<M - 1>::template determinant_comp<T>(b);
      }
      // accumulator for loop to get the final output as accumulation of all internal submatrix determinant values.
      for(j = 0; j < M; j++)
      {
         temp1 = a[0][j] * c[j];
         d = d + temp1;
      }
      // return the final answer
      return d;
   }
};

// Template specialized case for 2x2 matrix
template <>
struct determinant_double<2>
{
   template <typename T>
   static T determinant_comp(T a[2][2])
   {
      // Determinant of 2x2 matrix formula.
      return ((a[0][0] * a[1][1]) - (a[0][1] * a[1][0]));
   }
};

// Template specialized case for 1x1 matrix
template <>
struct determinant_double<1>
{
   template <typename T>
   static T determinant_comp(T a[1][1])
   {
      // Determinant of 1x1 matrix formula.
      return a[0][0];
   }
};

// Function to get output for datatype T, accepted as a template parameter
template <unsigned M, typename T>
T determinant_compute(T a[M][M])
{
   return determinant_double<M>::template determinant_comp<T>(a);
}

// ==============================================================================
// Functions: test_driver function
// Description: A templatized function that can be configured for certain bit-
//   widths of AC datatypes. It uses the type information to iterate through a
//   range of valid values on that type in order to compare the precision of the
//   DUT determinant with the computed determinant using a standard C double type.
//   The maximum error for each type is accumulated in variables defined in the
//   calling function.

template <unsigned M, int Wfi, int Ifi, bool Sfi>
int test_driver(double& cumulative_max_error, double& cumulative_max_error_cmplx, const double allowed_error,
                bool details = false)
{
   bool passed = true;

   typedef ac_fixed<M * Wfi + ac::log2_floor<Factorial<M>::value>::val,
                    M * Ifi + ac::log2_floor<Factorial<M>::value>::val, true, AC_TRN, AC_WRAP>
       T_out;

   ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP> input_arr[M][M];
   ac_fixed<M * Wfi + ac::log2_floor<Factorial<M>::value>::val, M * Ifi + ac::log2_floor<Factorial<M>::value>::val,
            true, AC_TRN, AC_WRAP>
       output_arr;
   ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>> input_cmplx_arr[M][M];
   ac_complex<ac_fixed<M * Wfi + ac::log2_floor<Factorial<M>::value>::val,
                       M * Ifi + ac::log2_floor<Factorial<M>::value>::val, true, AC_TRN, AC_WRAP>>
       output_cmplx_arr;

   ac_matrix<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>, M, M> input;
   ac_fixed<M * Wfi + ac::log2_floor<Factorial<M>::value>::val, M * Ifi + ac::log2_floor<Factorial<M>::value>::val,
            true, AC_TRN, AC_WRAP>
       output;
   ac_matrix<ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>>, M, M> input_cmplx;
   ac_complex<ac_fixed<M * Wfi + ac::log2_floor<Factorial<M>::value>::val,
                       M * Ifi + ac::log2_floor<Factorial<M>::value>::val, true, AC_TRN, AC_WRAP>>
       output_cmplx;

   double input_double[M][M];
   double output_double;
   ac_complex<double> input_cmplx_double[M][M];
   ac_complex<double> output_cmplx_double;

   cout << "TEST: ac_determinant(), M = ";
   cout << M << ",";
   cout << "INPUT: ";
   cout.width(38);
   cout << left << input_arr[0][0].type_name();
   cout << "OUTPUT: ";
   cout.width(38);
   cout << left << output_arr.type_name();
   cout << "RESULT: ";

   // initialize the matrix elements using ac_random
   for(unsigned i = 0; i < M; i++)
   {
      for(unsigned j = 0; j < M; j++)
      {
         ac_random(input(i, j));
         ac_random(input_cmplx(i, j).i());
         ac_random(input_cmplx(i, j).r());
         input_arr[i][j] = input(i, j);
         input_cmplx_arr[i][j].i() = input_cmplx(i, j).i();
         input_cmplx_arr[i][j].r() = input_cmplx(i, j).r();
         input_double[i][j] = input(i, j).to_double();
         input_cmplx_double[i][j].i() = input_cmplx(i, j).i().to_double();
         input_cmplx_double[i][j].r() = input_cmplx(i, j).r().to_double();
      }
   }

   test_ac_determinant(input_arr, output_arr, input_cmplx_arr, output_cmplx_arr, input, output, input_cmplx,
                       output_cmplx);

   // call the c datatype equivalent of determinant function
   output_double = determinant_compute<M, double>(input_double);
   output_cmplx_double = determinant_compute<M, ac_complex<double>>(input_cmplx_double);

   // compute the final error
   double final_error = 0;
   double double_error = 0;
   double cmplx_error = 0;
   double double_arr_error = 0;
   double cmplx_arr_error = 0;

   // Fixed point implementation for ac_matrix
   final_error = abs((output_double - output.to_double()) / output_double) * 100;
   double_error = final_error;

   // Complex of Fixed point implementation for ac_matrix
   ac_complex<double> diff_op, act_op;
   act_op.r() = output_cmplx.r().to_double();
   act_op.i() = output_cmplx.i().to_double();
   // Use typecasting to perform quantization on testbench double output
   output_cmplx_double.r() = ((T_out)output_cmplx_double.r()).to_double();
   output_cmplx_double.i() = ((T_out)output_cmplx_double.i()).to_double();
   diff_op = output_cmplx_double - act_op;
   final_error = sqrt((diff_op / output_cmplx_double).mag_sqr()) * 100;
   cmplx_error = final_error;

   // Fixed point implementation for c arrays
   final_error = abs((output_double - output_arr.to_double()) / output_double) * 100;
   double_arr_error = final_error;

   // Complex of Fixed point implementation for c arrays
   act_op.r() = output_cmplx_arr.r().to_double();
   act_op.i() = output_cmplx_arr.i().to_double();
   diff_op = output_cmplx_double - act_op;
   final_error = sqrt((diff_op / output_cmplx_double).mag_sqr()) * 100;
   cmplx_arr_error = final_error;

   // Put max overall error in a separate variable.
   double max_error_overall = double_error > double_arr_error ? double_error : double_arr_error;
   double max_error_cmplx_overall = cmplx_error > cmplx_arr_error ? cmplx_error : cmplx_arr_error;

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
   cout << "Testing function: ac_determinant(), for scalar and complex datatypes - allowed error = " << allowed_error
        << endl;

   // template <unsigned M, int Wfi, int Ifi, int Sfi>
   test_driver<4, 16, 8, true>(max_error, cmplx_max_error, allowed_error);
   test_driver<5, 16, 8, true>(max_error, cmplx_max_error, allowed_error);
   test_driver<6, 16, 8, true>(max_error, cmplx_max_error, allowed_error);
   test_driver<7, 16, 8, true>(max_error, cmplx_max_error, allowed_error);
   test_driver<8, 16, 8, true>(max_error, cmplx_max_error, allowed_error);
   test_driver<9, 16, 8, true>(max_error, cmplx_max_error, allowed_error);

   cout << "=============================================================================" << endl;
   cout << "  Testbench finished. Maximum errors observed across all data type / bit-width variations:" << endl;
   cout << "    max_error            = " << max_error << endl;
   cout << "    cmplx_max_error      = " << cmplx_max_error << endl;

   // If error limits on any tested datatype have been crossed, the test has failed
   bool test_fail = (max_error > allowed_error) || (cmplx_max_error > allowed_error);

   // Notify the user whether or not the test was a failure.
   if(test_fail)
   {
      cout << "  ac_determinant - FAILED - Error tolerance(s) exceeded" << endl;                       // LCOV_EXCL_LINE
      cout << "=============================================================================" << endl; // LCOV_EXCL_LINE
      return -1;                                                                                       // LCOV_EXCL_LINE
   }
   else
   {
      cout << "  ac_determinant - PASSED" << endl;
      cout << "=============================================================================" << endl;
   }
   return 0;
}
