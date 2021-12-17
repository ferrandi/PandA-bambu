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
// ac_sqrt() function using a variety of data types and bit-
// widths.

// To compile standalone and run:
//   $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_sqrt.cpp -o design
//   ./design

// Include the AC Math function that is exercised with this testbench

#include <ac_math/ac_sqrt.h>
using namespace ac_math;

// ==============================================================================
// Test Designs
//   These simple functions allow executing the ac_sqrt() function
//   using multiple data types at the same time. Template parameters are used to
//   configure the bit-widths of the types.

// Test for ac_int inputs and outputs.
template <int Wint, int outWint>
void test_ac_sqrt_int(const ac_int<Wint, false>& in1, ac_int<outWint, false>& out1)
{
   ac_sqrt(in1, out1);
}

// Test for ac_fixed inputs and outputs.
template <int Wfi, int Ifi, int outWfi, int outIfi>
void test_ac_sqrt_fixed(const ac_fixed<Wfi, Ifi, false, AC_TRN, AC_WRAP>& in2,
                        ac_fixed<outWfi, outIfi, false, AC_TRN, AC_WRAP>& out2)
{
   ac_sqrt(in2, out2);
}

// ==============================================================================

#include <fstream>
#include <iostream>
#include <math.h>
#include <string>
using namespace std;

// -------------------------------------------------------
// Helper functions for output-matching/ error calculation

// See if output is correct for real ac_int inputs.

template <int Wint, int outWint>
bool output_check_int(const ac_int<Wint, false> input, ac_int<outWint, false> output)
{
   bool correct = output == (ac_int<outWint, false>)(sqrt(input.to_double()));

#ifdef DEBUG
   if(!correct)
   {
      cout << "Output not correct" << endl;
      cout << "input = " << input << endl;
      cout << "output = " << output << endl;
      assert(false);
   }
#endif

   return correct;
}

// Calculating error for real, ac_fixed datatype.

template <int Wfi, int Ifi, int outWfi, int outIfi>
double err_calc(const ac_fixed<Wfi, Ifi, false, AC_TRN, AC_WRAP> input,
                const ac_fixed<outWfi, outIfi, false, AC_TRN, AC_WRAP> output, const double allowed_error,
                const double threshold)
{
   // The typecasting is done in order to provide quantization on the expected output.
   double expected_value = ((ac_fixed<outWfi, outIfi, false, AC_TRN, AC_WRAP>)(sqrt(input.to_double()))).to_double();
   double actual_value = output.to_double();
   double this_error;

   // If expected value is greater than a particular threshold, calculate relative error, else, calculate absolute
   // error.
   if(abs(expected_value) > threshold)
   {
      this_error = abs((expected_value - actual_value) / expected_value) * 100.0;
   }
   else
   {
      this_error = abs(expected_value - actual_value) * 100.0;
   }

   return this_error;
}

// ==============================================================================
// Function: test_driver_int()
// Description: A templatized function that can be configured for certain bit-
//   widths of ac_int values. It uses the type information to iterate through a
//   range of valid values on that type in order to make sure that the
//   output of the ac_sqrt function is as expected.

template <int Wint, int outWint>
int test_driver_int(bool& all_tests_pass)
{
   ac_int<Wint, false> input_int;
   ac_int<outWint, false> output_int;

   cout << "TEST: ac_sqrt() INPUT: ";
   cout.width(38);
   cout << left << input_int.type_name();
   cout << "OUTPUT: ";
   cout.width(38);
   cout << left << output_int.type_name();
   cout << "RESULT: ";

   bool correct = true;

   // set ranges and step size for integer testbench
   double lower_limit_int = input_int.template set_val<AC_VAL_MIN>().to_double();
   double upper_limit_int = input_int.template set_val<AC_VAL_MAX>().to_double();
   double step_int = input_int.template set_val<AC_VAL_QUANTUM>().to_double();

   // Test integer inputs
   for(double i = lower_limit_int; i <= upper_limit_int; i += step_int)
   {
      // Set values for input.
      input_int = i;
      test_ac_sqrt_int(input_int, output_int);
      bool correct_iteration = output_check_int(input_int, output_int);
      correct = correct_iteration && correct;
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
// Function: test_driver_fixed()
// Description: A templatized function that can be configured for certain bit-
//   widths of unsigned ac_fixed values. It uses the type information to iterate through a
//   range of valid values on that type in order to compare the precision of the
//   output of the ac_sqrt() function with the computed square root output
//   using a standard C double type. The maximum error for each type is
//   accumulated in variables defined in the calling function.

template <int Wfi, int Ifi, int outWfi, int outIfi>
int test_driver_fixed(double& cumulative_max_error_fixed, const double allowed_error_fixed, const double threshold,
                      bool details = false)
{
   bool passed;
   double max_error_fixed = 0.0; // reset for this run

   ac_fixed<Wfi, Ifi, false, AC_TRN, AC_WRAP> input_fixed;
   ac_fixed<outWfi, outIfi, false, AC_TRN, AC_WRAP> output_fixed;

   double lower_limit_fixed, upper_limit_fixed, step_fixed;

   // set ranges and step size for fixed point testbench
   lower_limit_fixed = input_fixed.template set_val<AC_VAL_MIN>().to_double();
   upper_limit_fixed = input_fixed.template set_val<AC_VAL_MAX>().to_double();
   step_fixed = input_fixed.template set_val<AC_VAL_QUANTUM>().to_double();

   cout << "TEST: ac_sqrt() INPUT: ";
   cout.width(38);
   cout << left << input_fixed.type_name();
   cout << "OUTPUT: ";
   cout.width(38);
   cout << left << output_fixed.type_name();
   cout << "RESULT: ";

   // Dump the test details
   if(details)
   {
      cout << endl;                                                    // LCOV_EXCL_LINE
      cout << "  Ranges for input types:" << endl;                     // LCOV_EXCL_LINE
      cout << "    lower_limit_fixed = " << lower_limit_fixed << endl; // LCOV_EXCL_LINE
      cout << "    upper_limit_fixed = " << upper_limit_fixed << endl; // LCOV_EXCL_LINE
      cout << "    step_fixed        = " << step_fixed << endl;        // LCOV_EXCL_LINE
   }

   // Test unsigned fixed point inputs
   for(double i = lower_limit_fixed; i <= upper_limit_fixed; i += step_fixed)
   {
      // Set values for input.
      input_fixed = i;
      test_ac_sqrt_fixed(input_fixed, output_fixed);
      double this_error_fixed = err_calc(input_fixed, output_fixed, allowed_error_fixed, threshold);
      if(this_error_fixed > max_error_fixed)
      {
         max_error_fixed = this_error_fixed;
      }
   }

   passed = max_error_fixed < allowed_error_fixed;

   if(passed)
   {
      printf("PASSED, max err (%f)\n", max_error_fixed);
   }
   else
   {
      printf("FAILED, max err (%f)\n", max_error_fixed);
   } // LCOV_EXCL_LINE

   if(max_error_fixed > cumulative_max_error_fixed)
   {
      cumulative_max_error_fixed = max_error_fixed;
   }

   return 0;
}

int main(int argc, char* argv[])
{
   double max_error_fixed = 0;

   // Set tolerance
   double allowed_error_fixed = 0.5;

   // threshold below which we calculate absolute error instead of relative for fixed point
   double threshold_fixed = 0.005;

   cout << "=============================================================================" << endl;
   cout << "Testing function: ac_sqrt() - Allowed error " << allowed_error_fixed << " (fixed pt)" << endl;

   bool all_tests_pass = true;

   // If any of the tests fail for ac_int, the all_tests_pass variable will be set to false

   // template <int Wint, int outWint>
   test_driver_int<13, 24>(all_tests_pass);
   test_driver_int<10, 24>(all_tests_pass);
   test_driver_int<16, 32>(all_tests_pass);
   test_driver_int<8, 16>(all_tests_pass);
   test_driver_int<11, 16>(all_tests_pass);

   // template <int Wfi, int Ifi, int outWfi, int outIfi>
   test_driver_fixed<12, 0, 64, 32>(max_error_fixed, allowed_error_fixed, threshold_fixed);
   test_driver_fixed<8, -2, 64, 32>(max_error_fixed, allowed_error_fixed, threshold_fixed);
   test_driver_fixed<10, -4, 64, 32>(max_error_fixed, allowed_error_fixed, threshold_fixed);
   test_driver_fixed<12, 8, 64, 32>(max_error_fixed, allowed_error_fixed, threshold_fixed);
   test_driver_fixed<16, 8, 64, 32>(max_error_fixed, allowed_error_fixed, threshold_fixed);
   test_driver_fixed<9, 4, 60, 30>(max_error_fixed, allowed_error_fixed, threshold_fixed);
   test_driver_fixed<4, 9, 60, 30>(max_error_fixed, allowed_error_fixed, threshold_fixed);
   test_driver_fixed<12, 12, 64, 32>(max_error_fixed, allowed_error_fixed, threshold_fixed);
   test_driver_fixed<12, 5, 64, 32>(max_error_fixed, allowed_error_fixed, threshold_fixed);

   cout << "=============================================================================" << endl;
   cout << "  Testbench finished. Maximum error observed across all bit-width variations for unsigned ac_fixed:"
        << endl;
   cout << "    max_error_fixed       = " << max_error_fixed << endl;

   // If error limits on ac_fixed datatypes have been crossed, or the output for ac_int datatypes is not correct, the
   // test has failed.
   bool test_fail = (max_error_fixed > allowed_error_fixed) || (!all_tests_pass);

   // Notify the user that the test was a failure.
   if(test_fail)
   {
      cout << "  ac_sqrt - FAILED - Error tolerance(s) exceeded" << endl;                              // LCOV_EXCL_LINE
      cout << "=============================================================================" << endl; // LCOV_EXCL_LINE
      return -1;                                                                                       // LCOV_EXCL_LINE
   }
   else
   {
      cout << "  ac_sqrt - PASSED" << endl;
      cout << "=============================================================================" << endl;
   }

   return 0;
}
