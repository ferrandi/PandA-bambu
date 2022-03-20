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
// ac_shift() function using a variety of data types and bit-
// widths.

// To compile standalone and run:
//   $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_shift.cpp -o design
//   ./design

// Include the AC Math function that is exercised with this testbench

#include <ac_math/ac_shift.h>
using namespace ac_math;

// ==============================================================================
// Test Designs
//   These simple functions allow executing the ac_shift_left() function
//   using multiple data types at the same time. Template parameters are used to
//   configure the bit-widths of the types.

// Test for left shifts on unsigned ac_fixed and ac_complex<ac_fixed> inputs and outputs.
template <int W, int I, int outW, int outI, ac_q_mode outQ, ac_o_mode outO>
void test_ac_shift_left_us(const ac_fixed<W, I, false, AC_TRN, AC_WRAP>& us_fixed_input,
                           ac_fixed<outW, outI, false, outQ, outO>& us_fixed_output_1,
                           ac_fixed<outW, outI, false, outQ, outO>& us_fixed_output_2,
                           const ac_complex<ac_fixed<W, I, false, AC_TRN, AC_WRAP>>& us_complex_input,
                           ac_complex<ac_fixed<outW, outI, false, outQ, outO>>& us_complex_output,
                           const unsigned int& us_shift, const int& s_shift)
{
   ac_shift_left(us_fixed_input, us_shift, us_fixed_output_1);
   ac_shift_left(us_fixed_input, s_shift, us_fixed_output_2);
   ac_shift_left(us_complex_input, us_shift, us_complex_output);
}

// Test for left shifts on signed ac_fixed and ac_complex<ac_fixed> inputs and outputs.
template <int W, int I, int outW, int outI, ac_q_mode outQ, ac_o_mode outO>
void test_ac_shift_left_s(const ac_fixed<W, I, true, AC_TRN, AC_WRAP>& s_fixed_input,
                          ac_fixed<outW, outI, true, outQ, outO>& s_fixed_output_1,
                          ac_fixed<outW, outI, true, outQ, outO>& s_fixed_output_2,
                          const ac_complex<ac_fixed<W, I, true, AC_TRN, AC_WRAP>>& s_complex_input,
                          ac_complex<ac_fixed<outW, outI, true, outQ, outO>>& s_complex_output,
                          const unsigned int& us_shift, const int& s_shift)
{
   ac_shift_left(s_fixed_input, us_shift, s_fixed_output_1);
   ac_shift_left(s_fixed_input, s_shift, s_fixed_output_2);
   ac_shift_left(s_complex_input, us_shift, s_complex_output);
}

// Test for right shifts on unsigned ac_fixed and ac_complex<ac_fixed> inputs and outputs.
template <int W, int I, int outW, int outI, ac_q_mode outQ, ac_o_mode outO>
void test_ac_shift_right_us(const ac_fixed<W, I, false, AC_TRN, AC_WRAP>& us_fixed_input,
                            ac_fixed<outW, outI, false, outQ, outO>& us_fixed_output_1,
                            ac_fixed<outW, outI, false, outQ, outO>& us_fixed_output_2,
                            const ac_complex<ac_fixed<W, I, false, AC_TRN, AC_WRAP>>& us_complex_input,
                            ac_complex<ac_fixed<outW, outI, false, outQ, outO>>& us_complex_output,
                            const unsigned int& us_shift, const int& s_shift)
{
   ac_shift_right(us_fixed_input, us_shift, us_fixed_output_1);
   ac_shift_right(us_fixed_input, s_shift, us_fixed_output_2);
   ac_shift_right(us_complex_input, us_shift, us_complex_output);
}

// Test for right shifts on signed ac_fixed and ac_complex<ac_fixed> inputs and outputs.
template <int W, int I, int outW, int outI, ac_q_mode outQ, ac_o_mode outO>
void test_ac_shift_right_s(const ac_fixed<W, I, true, AC_TRN, AC_WRAP>& s_fixed_input,
                           ac_fixed<outW, outI, true, outQ, outO>& s_fixed_output_1,
                           ac_fixed<outW, outI, true, outQ, outO>& s_fixed_output_2,
                           const ac_complex<ac_fixed<W, I, true, AC_TRN, AC_WRAP>>& s_complex_input,
                           ac_complex<ac_fixed<outW, outI, true, outQ, outO>>& s_complex_output,
                           const unsigned int& us_shift, const int& s_shift)
{
   ac_shift_right(s_fixed_input, us_shift, s_fixed_output_1);
   ac_shift_right(s_fixed_input, s_shift, s_fixed_output_2);
   ac_shift_right(s_complex_input, us_shift, s_complex_output);
}

// ==============================================================================

#include <fstream>
#include <iostream>
#include <math.h>
#include <string>
using namespace std;

// ------------------------------------------------------------
// Helper functions for error detection and output checking.

#ifdef DEBUG

// Helper functions to pin-point error.

template <int W, int I>
void print_origin_error(const ac_fixed<W, I, true, AC_TRN, AC_WRAP> input, const int n)
{
   cout << "error coming from signed real input, signed shift function" << endl;
}

template <int W, int I>
void print_origin_error(const ac_fixed<W, I, true, AC_TRN, AC_WRAP> input, const unsigned int n)
{
   cout << "error coming from signed real input, unsigned shift function" << endl;
}

template <int W, int I>
void print_origin_error(const ac_fixed<W, I, false, AC_TRN, AC_WRAP> input, const int n)
{
   cout << "error coming from unsigned real input, signed shift function" << endl;
}

template <int W, int I>
void print_origin_error(const ac_fixed<W, I, false, AC_TRN, AC_WRAP> input, const unsigned int n)
{
   cout << "error coming from unsigned real input, unsigned shift function" << endl;
}

template <int W, int I>
void print_origin_error(const ac_complex<ac_fixed<W, I, false, AC_TRN, AC_WRAP>> input, const unsigned int n)
{
   cout << "error coming from unsigned complex input, unsigned shift function" << endl;
}

template <int W, int I>
void print_origin_error(const ac_complex<ac_fixed<W, I, true, AC_TRN, AC_WRAP>> input, const unsigned int n)
{
   cout << "error coming from signed complex input, unsigned shift function" << endl;
}

#endif

// Check output and make sure it is correct for real values and left shifts

template <class T_in, class T_in_shift, class T_out>
bool output_check_shift_left(const T_in input, const T_in_shift n, const T_out output)
{
   // Since the output of ac_shift operations have an arithmetic
   // basis and take into account rounding and saturation, test
   // the output against the input multiplied by 2^(shift_count)

   T_out expected_output;
   expected_output = input.to_double() * pow(2.0, (double)n);
   bool correct = (output == expected_output);

#ifdef DEBUG
   if(!correct)
   {
      cout << endl;
      cout << "shift_left" << endl;
      print_origin_error(input, n);
      cout << "The output is not as expected." << endl;
      cout << "input           = " << input << endl;
      cout << "expected_output = " << expected_output << endl;
      cout << "output          = " << output << endl;
      cout << "n               = " << n << endl;
      assert(false);
   }
#endif

   return correct;
}

// Check output and make sure it is correct for complex values and left shifts

template <class T_in, class T_out>
bool output_check_shift_left(const ac_complex<T_in> input, unsigned int n, const ac_complex<T_out> output)
{
   // Since the output of ac_shift operations have an arithmetic
   // basis and take into account rounding and saturation, test
   // the real and imaginary parts of the output against the
   // real and imaginary parts of the input multiplied by 2^(shift_count)

   ac_complex<T_out> expected_output;
   expected_output.r() = input.r().to_double() * pow(2.0, (double)n);
   expected_output.i() = input.i().to_double() * pow(2.0, (double)n);

   bool correct = (output == expected_output);

#ifdef DEBUG
   if(!correct)
   {
      cout << endl;
      cout << "  shift_left" << endl;
      print_origin_error(input, n);
      cout << "  The output is not as expected." << endl;
      cout << "  input           = " << input << endl;
      cout << "  expected_output = " << expected_output << endl;
      cout << "  output          = " << output << endl;
      cout << "  n               = " << n << endl;
      assert(false);
   }
#endif

   return correct;
}

// Check output and make sure it is correct for real values and right shifts

template <class T_in, class T_in_shift, class T_out>
bool output_check_shift_right(const T_in input, const T_in_shift n, const T_out output)
{
   // Since the output of ac_shift operations have an arithmetic
   // basis and take into account rounding and saturation, test
   // the output against the input multiplied by 2^(-shift_count)

   T_out expected_output;
   expected_output = input.to_double() * pow(2.0, -((double)n));
   bool correct = (output == expected_output);

#ifdef DEBUG
   if(!correct)
   {
      cout << endl;
      cout << "  shift_right" << endl;
      print_origin_error(input, n);
      cout << "  The output is not as expected." << endl;
      cout << "  input           = " << input << endl;
      cout << "  expected_output = " << expected_output << endl;
      cout << "  output          = " << output << endl;
      cout << "  n               = " << n << endl;
      assert(false);
   }
#endif

   return correct;
}

// Check output and make sure it is correct for complex values and right shifts

template <class T_in, class T_out>
bool output_check_shift_right(const ac_complex<T_in> input, unsigned int n, const ac_complex<T_out> output)
{
   // Since the output of ac_shift operations have an arithmetic
   // basis and take into account rounding and saturation, test
   // the real and imaginary parts of the output against the
   // real and imaginary parts of the input multiplied by 2^(-shift_count)

   ac_complex<T_out> expected_output;
   expected_output.r() = input.r().to_double() * pow(2.0, -((double)n));
   expected_output.i() = input.i().to_double() * pow(2.0, -((double)n));

   bool correct = (output == expected_output);

#ifdef DEBUG
   if(!correct)
   {
      cout << endl;
      cout << "  shift_right" << endl;
      print_origin_error(input, n);
      cout << "  The output is not as expected." << endl;
      cout << "  input           = " << input << endl;
      cout << "  expected_output = " << expected_output << endl;
      cout << "  output          = " << output << endl;
      cout << "  n               = " << n << endl;
      assert(false);
   }
#endif

   return correct;
}

// ==============================================================================
// Functions: test_driver functions
// Description: Templatized functions that can be configured for certain bit-
//   widths of AC datatypes. They use the type information to iterate through a
//   range of valid values on that type and make sure that the output of the
//   ac_shift function is correct

// ==============================================================================
// Function: test_driver_us()
// Description: test_driver function for unsigned ac_fixed and ac_complex inputs
//   and outputs.

template <int W, int I, int outW, int outI, ac_q_mode outQ, ac_o_mode outO>
int test_driver_us(bool& all_tests_pass, bool details = false)
{
   ac_fixed<W, I, false, AC_TRN, AC_WRAP> us_fixed_input;
   ac_fixed<outW, outI, false, outQ, outO> us_fixed_output_1, us_fixed_output_2;
   ac_complex<ac_fixed<W, I, false, AC_TRN, AC_WRAP>> us_complex_input;
   ac_complex<ac_fixed<outW, outI, false, outQ, outO>> us_complex_output;

   double lower_limit, upper_limit, step;

   // set ranges and step size for fixed point testbench
   lower_limit = us_fixed_input.template set_val<AC_VAL_MIN>().to_double();
   upper_limit = us_fixed_input.template set_val<AC_VAL_MAX>().to_double();
   step = us_fixed_input.template set_val<AC_VAL_QUANTUM>().to_double();

   cout << "TEST: ac_shift() INPUT: ";
   cout.width(38);
   cout << left << us_fixed_input.type_name();
   cout << "OUTPUT: ";
   cout.width(50);
   cout << left << us_fixed_output_1.type_name();
   cout << "RESULT: ";

   // Dump the test details
   if(details)
   {
      cout << endl;                                        // LCOV_EXCL_LINE
      cout << "  Ranges for input types:" << endl;         // LCOV_EXCL_LINE
      cout << "    lower_limit = " << lower_limit << endl; // LCOV_EXCL_LINE
      cout << "    upper_limit = " << upper_limit << endl; // LCOV_EXCL_LINE
      cout << "    step        = " << step << endl;        // LCOV_EXCL_LINE
   }

   bool correct = true;

   for(int n = -W; n <= W; n++)
   {
      unsigned int un = n >= 0 ? n : -n;
      for(double i = lower_limit; i <= upper_limit; i += step)
      {
         for(double j = lower_limit; j <= upper_limit; j += step)
         {
            us_fixed_input = i;
            us_complex_input.r() = i;
            us_complex_input.i() = j;
            test_ac_shift_left_us(us_fixed_input, us_fixed_output_1, us_fixed_output_2, us_complex_input,
                                  us_complex_output, un, n);
            bool correct_iteration_shift_left = output_check_shift_left(us_fixed_input, un, us_fixed_output_1) &&
                                                output_check_shift_left(us_fixed_input, n, us_fixed_output_2) &&
                                                output_check_shift_left(us_complex_input, un, us_complex_output);

            test_ac_shift_right_us(us_fixed_input, us_fixed_output_1, us_fixed_output_2, us_complex_input,
                                   us_complex_output, un, n);
            bool correct_iteration_shift_right = output_check_shift_right(us_fixed_input, un, us_fixed_output_1) &&
                                                 output_check_shift_right(us_fixed_input, n, us_fixed_output_2) &&
                                                 output_check_shift_right(us_complex_input, un, us_complex_output);

            correct = correct && correct_iteration_shift_left && correct_iteration_shift_right;
         }
      }
   }

   if(correct)
   {
      printf("PASSED\n");
   }
   else
   {
      printf("FAILED\n");
   } // LCOV_EXCL_LINE

   all_tests_pass = all_tests_pass && correct;

   return 0;
}

// ==============================================================================
// Function: test_driver_s()
// Description: test_driver function for signed ac_fixed and ac_complex
//   inputs and outputs.

template <int W, int I, int outW, int outI, ac_q_mode outQ, ac_o_mode outO>
int test_driver_s(bool& all_tests_pass, bool details = false)
{
   ac_fixed<W, I, true, AC_TRN, AC_WRAP> s_fixed_input;
   ac_fixed<outW, outI, true, outQ, outO> s_fixed_output_1, s_fixed_output_2;
   ac_complex<ac_fixed<W, I, true, AC_TRN, AC_WRAP>> s_complex_input;
   ac_complex<ac_fixed<outW, outI, true, outQ, outO>> s_complex_output;

   double lower_limit, upper_limit, step;

   // set ranges and step size for fixed point testbench
   lower_limit = s_fixed_input.template set_val<AC_VAL_MIN>().to_double();
   upper_limit = s_fixed_input.template set_val<AC_VAL_MAX>().to_double();
   step = s_fixed_input.template set_val<AC_VAL_QUANTUM>().to_double();

   cout << "TEST: ac_shift() INPUT: ";
   cout.width(38);
   cout << left << s_fixed_input.type_name();
   cout << "OUTPUT: ";
   cout.width(50);
   cout << left << s_fixed_output_1.type_name();
   cout << "RESULT: ";

   // Dump the test details
   if(details)
   {
      cout << endl;                                        // LCOV_EXCL_LINE
      cout << "  Ranges for input types:" << endl;         // LCOV_EXCL_LINE
      cout << "    lower_limit = " << lower_limit << endl; // LCOV_EXCL_LINE
      cout << "    upper_limit = " << upper_limit << endl; // LCOV_EXCL_LINE
      cout << "    step        = " << step << endl;        // LCOV_EXCL_LINE
   }

   bool correct = true;

   for(int n = -W; n <= W; n++)
   {
      unsigned int un = n >= 0 ? n : -n;
      for(double i = lower_limit; i <= upper_limit; i += step)
      {
         for(double j = lower_limit; j <= upper_limit; j += step)
         {
            s_fixed_input = i;
            s_complex_input.r() = i;
            s_complex_input.i() = j;
            test_ac_shift_left_s(s_fixed_input, s_fixed_output_1, s_fixed_output_2, s_complex_input, s_complex_output,
                                 un, n);
            bool correct_iteration_shift_left = output_check_shift_left(s_fixed_input, un, s_fixed_output_1) &&
                                                output_check_shift_left(s_fixed_input, n, s_fixed_output_2) &&
                                                output_check_shift_left(s_complex_input, un, s_complex_output);

            test_ac_shift_right_s(s_fixed_input, s_fixed_output_1, s_fixed_output_2, s_complex_input, s_complex_output,
                                  un, n);
            bool correct_iteration_shift_right = output_check_shift_right(s_fixed_input, un, s_fixed_output_1) &&
                                                 output_check_shift_right(s_fixed_input, n, s_fixed_output_2) &&
                                                 output_check_shift_right(s_complex_input, un, s_complex_output);
            correct = correct && correct_iteration_shift_left && correct_iteration_shift_right;
         }
      }
   }

   if(correct)
   {
      printf("PASSED\n");
   }
   else
   {
      printf("FAILED\n");
   } // LCOV_EXCL_LINE

   all_tests_pass = all_tests_pass && correct;

   return 0;
}

int main(int argc, char* argv[])
{
   cout << "=============================================================================" << endl;
   cout << "Testing function: ac_shift(), scalar and complex types." << endl;

   bool all_tests_pass = true;

   // If any of the tests fail, the all_tests_pass variable will be set to false

   // template <int W, int I, int outW, int outI, ac_q_mode outQ, ac_o_mode outO>
   test_driver_us<7, 5, 16, 8, AC_TRN, AC_WRAP>(all_tests_pass);
   test_driver_us<7, 5, 16, 8, AC_TRN_ZERO, AC_SAT>(all_tests_pass);
   test_driver_us<7, 5, 16, 8, AC_RND, AC_SAT_ZERO>(all_tests_pass);
   test_driver_us<7, 5, 16, 8, AC_RND_ZERO, AC_SAT_SYM>(all_tests_pass);
   test_driver_us<7, 5, 16, 8, AC_RND_INF, AC_SAT>(all_tests_pass);
   test_driver_us<7, 5, 16, 8, AC_RND_MIN_INF, AC_SAT>(all_tests_pass);
   test_driver_us<7, 5, 16, 8, AC_RND_CONV, AC_SAT>(all_tests_pass);
   test_driver_us<7, 5, 16, 8, AC_RND_CONV_ODD, AC_SAT>(all_tests_pass);

   test_driver_us<7, -5, 16, -3, AC_TRN, AC_WRAP>(all_tests_pass);
   test_driver_us<7, -5, 16, -3, AC_TRN_ZERO, AC_SAT>(all_tests_pass);
   test_driver_us<7, -5, 16, -3, AC_RND, AC_SAT_ZERO>(all_tests_pass);
   test_driver_us<7, -5, 16, -3, AC_RND_ZERO, AC_SAT_SYM>(all_tests_pass);
   test_driver_us<7, -5, 16, -3, AC_RND_INF, AC_SAT>(all_tests_pass);
   test_driver_us<7, -5, 16, -3, AC_RND_MIN_INF, AC_SAT>(all_tests_pass);
   test_driver_us<7, -5, 16, -3, AC_RND_CONV, AC_SAT>(all_tests_pass);
   test_driver_us<7, -5, 16, -3, AC_RND_CONV_ODD, AC_SAT>(all_tests_pass);

   test_driver_us<7, 15, 16, 20, AC_TRN, AC_WRAP>(all_tests_pass);
   test_driver_us<7, 15, 16, 20, AC_TRN_ZERO, AC_SAT>(all_tests_pass);
   test_driver_us<7, 15, 16, 20, AC_RND, AC_SAT_ZERO>(all_tests_pass);
   test_driver_us<7, 15, 16, 20, AC_RND_ZERO, AC_SAT_SYM>(all_tests_pass);
   test_driver_us<7, 15, 16, 20, AC_RND_INF, AC_SAT>(all_tests_pass);
   test_driver_us<7, 15, 16, 20, AC_RND_MIN_INF, AC_SAT>(all_tests_pass);
   test_driver_us<7, 15, 16, 20, AC_RND_CONV, AC_SAT>(all_tests_pass);
   test_driver_us<7, 15, 16, 20, AC_RND_CONV_ODD, AC_SAT>(all_tests_pass);
   test_driver_us<7, 15, 16, 20, AC_RND_CONV_ODD, AC_SAT_SYM>(all_tests_pass);

   test_driver_s<7, 5, 16, 8, AC_TRN, AC_WRAP>(all_tests_pass);
   test_driver_s<7, 5, 16, 8, AC_TRN_ZERO, AC_SAT>(all_tests_pass);
   test_driver_s<7, 5, 16, 8, AC_RND, AC_SAT_ZERO>(all_tests_pass);
   test_driver_s<7, 5, 16, 8, AC_RND_ZERO, AC_SAT_SYM>(all_tests_pass);
   test_driver_s<7, 5, 16, 8, AC_RND_INF, AC_SAT>(all_tests_pass);
   test_driver_s<7, 5, 16, 8, AC_RND_MIN_INF, AC_SAT>(all_tests_pass);
   test_driver_s<7, 5, 16, 8, AC_RND_CONV, AC_SAT>(all_tests_pass);
   test_driver_s<7, 5, 16, 8, AC_RND_CONV_ODD, AC_SAT>(all_tests_pass);

   test_driver_s<7, -5, 16, -3, AC_TRN, AC_WRAP>(all_tests_pass);
   test_driver_s<7, -5, 16, -3, AC_TRN_ZERO, AC_SAT>(all_tests_pass);
   test_driver_s<7, -5, 16, -3, AC_RND, AC_SAT_ZERO>(all_tests_pass);
   test_driver_s<7, -5, 16, -3, AC_RND_ZERO, AC_SAT_SYM>(all_tests_pass);
   test_driver_s<7, -5, 16, -3, AC_RND_INF, AC_SAT>(all_tests_pass);
   test_driver_s<7, -5, 16, -3, AC_RND_MIN_INF, AC_SAT>(all_tests_pass);
   test_driver_s<7, -5, 16, -3, AC_RND_CONV, AC_SAT>(all_tests_pass);
   test_driver_s<7, -5, 16, -3, AC_RND_CONV_ODD, AC_SAT>(all_tests_pass);

   test_driver_s<7, 15, 16, 20, AC_TRN, AC_WRAP>(all_tests_pass);
   test_driver_s<7, 15, 16, 20, AC_TRN_ZERO, AC_SAT>(all_tests_pass);
   test_driver_s<7, 15, 16, 20, AC_RND, AC_SAT_ZERO>(all_tests_pass);
   test_driver_s<7, 15, 16, 20, AC_RND_ZERO, AC_SAT_SYM>(all_tests_pass);
   test_driver_s<7, 15, 16, 20, AC_RND_INF, AC_SAT>(all_tests_pass);
   test_driver_s<7, 15, 16, 20, AC_RND_MIN_INF, AC_SAT>(all_tests_pass);
   test_driver_s<7, 15, 16, 20, AC_RND_CONV, AC_SAT>(all_tests_pass);
   test_driver_s<7, 15, 16, 20, AC_RND_CONV_ODD, AC_SAT>(all_tests_pass);

   cout << "=============================================================================" << endl;
   cout << "  Testbench finished." << endl;

   // Notify the user if the test was a failure.
   if(!all_tests_pass)
   {
      cout << "  ac_shift - FAILED - output not correct for all test values" << endl;                  // LCOV_EXCL_LINE
      cout << "=============================================================================" << endl; // LCOV_EXCL_LINE
      return -1;                                                                                       // LCOV_EXCL_LINE
   }
   else
   {
      cout << "  ac_shift - PASSED" << endl;
      cout << "=============================================================================" << endl;
   }

   return 0;
}
