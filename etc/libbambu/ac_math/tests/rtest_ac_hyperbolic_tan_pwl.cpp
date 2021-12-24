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
// ac_hyperbolic_tan_pwl() function using a variety of data types and bit-
// widths.

// To compile standalone and run:
//   $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_hyperbolic_tan_pwl.cpp -o design
//   ./design

// Include the AC Math function that is exercised with this testbench
#include <ac_math/ac_hyperbolic_tan_pwl.h>
using namespace ac_math;

#define DEBUG

// ==============================================================================
// Test Design
//   This simple function allows executing the ac_hyperbolic_tan_pwl() function.
//   Template parameters are used to configure the bit-widths of the
//   ac_fixed inputs.

template <int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi, bool outSfi>
void test_ac_hyperbolic_tan_pwl(const ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>& in,
                                ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>& out)
{
   out = ac_hyperbolic_tan_pwl<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>>(in);
}

// ------------------------------------------------------------------------------
// Helper function for absolute value calculation. This can avoid any naming conflicts
// with other absolute value functions.

double abs_double(double x)
{
   return x >= 0 ? x : -x;
}

// ==============================================================================

#include <fstream>
#include <iostream>
#include <math.h>
#include <string>
using namespace std;

// ===============================================================================
// Function: test_driver()
// Description: A templatized function that can be configured for certain bit-
//   widths of ac_fixed inputs. It uses the type information to iterate through a
//   range of valid values on that type in order to compare the precision of the
//   piecewise linear hyperbolic tangent function with the computed hyperbolic
//   tangent function using a standard C double type. The maximum error for each
//   type is accumulated in variables defined in the calling function.

template <int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi, bool outSfi>
int test_driver(double& cumulative_max_error_hyperbolic_tan, const double allowed_error, bool details = false)
{
   double i;
   ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP> input;
   typedef ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> T_out;
   T_out output;

   // set ranges and step size for testbench
   double lower_limit = input.template set_val<AC_VAL_MIN>().to_double();
   double upper_limit = input.template set_val<AC_VAL_MAX>().to_double();
   double step = input.template set_val<AC_VAL_QUANTUM>().to_double();

   cout << "TEST: ac_hyperbolic_tan_pwl() INPUT: ";
   cout.width(38);
   cout << left << input.type_name();
   cout << "OUTPUT: ";
   cout.width(38);
   cout << left << output.type_name();
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

   bool passed = true;
   double max_hyperbolic_tan_error = 0.0;

   bool check_monotonic = true;
   bool compare_hyperbolic_tan = false;
   double old_output_hyperbolic_tan;

   for(i = lower_limit; i < upper_limit; i += step)
   {
      // Set values for input.
      input = i;

      // calculate hyperbolic tangent function convert back to double
      // an additional step of typecasting is required in order to perform
      // quantization on the expected output.
      double expected_hyperbolic_tan = ((T_out)(tanh(input.to_double()))).to_double();

      // call DUT with fixed-pt value
      test_ac_hyperbolic_tan_pwl(input, output);

      double actual_hyperbolic_tan = output.to_double();
      double this_error_hyperbolic_tan;

      // Calculate absolute error.
      this_error_hyperbolic_tan = abs_double(expected_hyperbolic_tan - actual_hyperbolic_tan) * 100.0;

      if(check_monotonic)
      {
         // MONOTONIC: Make sure that function is monotonic. Compare old value (value of previous iteration) with
         // current value. Since the hyperbolic_tan function that is being tested is an increasing function, and our
         // testbench value keeps incrementing or remains the same (in case of saturation), we expect the old value to
         // be lesser than or equal to the current one.

         // This comparison is only carried out once there is an old value to compare with
         if(compare_hyperbolic_tan)
         {
            // if by any chance the function output has dropped in value, print out at what point the problem has
            // occured and throw a runtime assertion.
            if(old_output_hyperbolic_tan > actual_hyperbolic_tan)
            {
               cout << endl;                                                                  // LCOV_EXCL_LINE
               cout << "  hyperbolic_tan output not monotonic at :" << endl;                  // LCOV_EXCL_LINE
               cout << "  x = " << input << endl;                                             // LCOV_EXCL_LINE
               cout << "  y = " << output << endl;                                            // LCOV_EXCL_LINE
               cout << "  old_output_hyperbolic_tan = " << old_output_hyperbolic_tan << endl; // LCOV_EXCL_LINE
               assert(false);                                                                 // LCOV_EXCL_LINE
            }
         }
         // Update the old value
         old_output_hyperbolic_tan = actual_hyperbolic_tan;
         // Once an old value has been stored, i.e. towards the end of the first iteration, this value is set to true.
         compare_hyperbolic_tan = true;
      }

#ifdef DEBUG
      if(this_error_hyperbolic_tan > allowed_error)
      {
         cout << endl;
         cout << "  Error exceeds tolerance" << endl;
         cout << "  input              = " << input << endl;
         cout << "  expected_hyperbolic_tan   = " << expected_hyperbolic_tan << endl;
         cout << "  actual_hyperbolic_tan     = " << actual_hyperbolic_tan << endl;
         cout << "  this_error_hyperbolic_tan = " << this_error_hyperbolic_tan << endl;
         assert(false);
      }
#endif

      if(this_error_hyperbolic_tan > max_hyperbolic_tan_error)
      {
         max_hyperbolic_tan_error = this_error_hyperbolic_tan;
      }
   }

   if(max_hyperbolic_tan_error > cumulative_max_error_hyperbolic_tan)
   {
      cumulative_max_error_hyperbolic_tan = max_hyperbolic_tan_error;
   }

   passed = (max_hyperbolic_tan_error < allowed_error);

   if(passed)
   {
      printf("PASSED , max err (%f)\n", max_hyperbolic_tan_error);
   }
   else
   {
      printf("FAILED , max err (%f)\n", max_hyperbolic_tan_error);
   } // LCOV_EXCL_LINE

   return 0;
}

int main(int argc, char* argv[])
{
   double max_error_hyperbolic_tan = 0.0;
   double allowed_error = 1.0;

   cout << "=============================================================================" << endl;
   cout << "Testing function: ac_hyperbolic_tan_pwl() - Allowed error " << allowed_error << endl;

   // template <int Wfi, int Ifi, int outWfi, int outIfi>
   test_driver<15, 6, true, 33, 2, true>(max_error_hyperbolic_tan, allowed_error);
   test_driver<12, 5, false, 37, 3, false>(max_error_hyperbolic_tan, allowed_error);
   test_driver<10, 5, true, 25, 3, true>(max_error_hyperbolic_tan, allowed_error);
   test_driver<20, 10, true, 38, 4, true>(max_error_hyperbolic_tan, allowed_error);
   test_driver<22, 7, true, 33, 2, true>(max_error_hyperbolic_tan, allowed_error);
   test_driver<17, 11, false, 40, 2, false>(max_error_hyperbolic_tan, allowed_error);
   test_driver<23, 7, true, 31, 2, true>(max_error_hyperbolic_tan, allowed_error);
   test_driver<19, 12, false, 34, 2, false>(max_error_hyperbolic_tan, allowed_error);
   test_driver<24, 13, false, 29, 1, false>(max_error_hyperbolic_tan, allowed_error);
   test_driver<16, 9, true, 35, 2, true>(max_error_hyperbolic_tan, allowed_error);

   cout << "=============================================================================" << endl;
   cout << "  Testbench finished. Maximum errors observed across all bit-width variations:" << endl;
   cout << "    max_error_hyperbolic_tan = " << max_error_hyperbolic_tan << endl;

   // If error limits on any test value have been crossed, the test has failed
   // Notify the user that the test was a failure if that is the case.
   if(max_error_hyperbolic_tan > allowed_error)
   {
      cout << "  ac_hyperbolic_tan_pwl - FAILED - Error tolerance(s) exceeded" << endl;                // LCOV_EXCL_LINE
      cout << "=============================================================================" << endl; // LCOV_EXCL_LINE
      return (-1);                                                                                     // LCOV_EXCL_LINE
   }

   cout << "  ac_hyperbolic_tan_pwl - PASSED" << endl;
   cout << "=============================================================================" << endl;
   return (0);
}
