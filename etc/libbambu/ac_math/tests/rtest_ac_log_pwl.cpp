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
// ac_log_pwl() function using a variety of data types and bit-
// widths.

// To compile standalone and run:
//   $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_log_pwl.cpp -o design
//   ./design

// Include the AC Math function that is exercised with this testbench
#include <ac_math/ac_log_pwl.h>
using namespace ac_math;

// ------------------------------------------------------------------------------
// Helper function for absolute value calculation. This can avoid any naming conflicts
// with other absolute value functions.

double abs_double(double x)
{
   return x >= 0 ? x : -x;
}

// ==============================================================================
// Test Design
//   This simple function allows executing the ac_log_pwl() function.
//   Template parameters are used to configure the bit-widths of the
//   ac_fixed inputs.

template <int Wfi, int Ifi, int outWfi, int outIfi, bool outSfi>
void test_ac_log_pwl(const ac_fixed<Wfi, Ifi, false, AC_TRN, AC_WRAP>& in,
                     ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>& log_out)
{
   log_out = ac_log_pwl<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>>(in);
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
//   piecewise linear log model with the computed base 2 logarithm using a
//   standard C double type. The maximum error for each type is accumulated
//   in variables defined in the calling function.

template <int Wfi, int Ifi, int outWfi, int outIfi, bool outSfi>
int test_driver(double& cumulative_max_error_log, const double allowed_error, bool details = false)
{
   ac_fixed<Wfi + 2, Ifi + 1, false, AC_TRN, AC_WRAP> i; // make loop variable slightly larger
   ac_fixed<Wfi, Ifi, false, AC_TRN, AC_WRAP> input;
   ac_fixed<Wfi, Ifi, false, AC_TRN, AC_WRAP> last;
   typedef ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> T_out;
   T_out log_out;

   // set ranges and step size for testbench
   ac_fixed<Wfi, Ifi, false> lower_limit;
   // If output is unsigned, make sure that the input is always greater than or equal to 1
   // by setting the lower_limit of the testbench to 1. This is because input values lesser
   // than 1 correspond to a negative output, which can't be stored in unsigned variables.
   // Also, keep in mind that the output can only ever be negative if the
   // input word-width is greater than the integer width.
   if(outSfi || (Wfi <= Ifi))
   {
      lower_limit = input.template set_val<AC_VAL_MIN>();
   }
   else
   {
      lower_limit = 1.0;
   }
   ac_fixed<Wfi, Ifi, false> upper_limit = input.template set_val<AC_VAL_MAX>();
   ac_fixed<Wfi, Ifi, false> step = input.template set_val<AC_VAL_QUANTUM>();

   cout << "TEST: ac_log_pwl() INPUT: ";
   cout.width(38);
   cout << left << input.type_name();
   cout << "OUTPUT: ";
   cout.width(38);
   cout << left << log_out.type_name();
   cout << "RESULT: ";

   // Dump the test details
   if(details)
   {
      cout << endl;                                                 // LCOV_EXCL_LINE
      cout << "  Ranges for input types:" << endl;                  // LCOV_EXCL_LINE
      cout << "    lower_limit          = " << lower_limit << endl; // LCOV_EXCL_LINE
      cout << "    upper_limit          = " << upper_limit << endl; // LCOV_EXCL_LINE
      cout << "    step                 = " << step << endl;        // LCOV_EXCL_LINE
   }

   bool passed = true;
   double max_log_error = 0.0;

   bool check_monotonic = true;
   bool compare_log = false;
   double old_output_log;

   for(i = lower_limit; i <= upper_limit; i += step)
   {
      // Set values for input.
      input = i;

      // call reference log() with fixed-pt value converted back to double
      // an additional step of typecasting is required in order to perform
      // quantization on the expected output.
      double expected_log = ((T_out)log(input.to_double())).to_double();

      // If input is zero, saturate the expected value according to the max. value representible by
      // the fixed point output.
      if(input == 0)
      {
         T_out output_min;
         output_min.template set_val<AC_VAL_MIN>();
         expected_log = output_min.to_double();
      }

      // call DUT with fixed-pt value
      test_ac_log_pwl(input, log_out);

      double actual_log = log_out.to_double();
      double this_error_log;

      // Calculate absolute value of error for log2 and log output. Since the scaling of outputs that lie outside the
      // range of normalization of log2 is done using an addition instead of a shift, the absolute value should work out
      // as an error metric for all outputs, large and small, provided that the output has enough precision.
      this_error_log = abs_double(expected_log - actual_log) * 100.0;

      // This comparison is only carried out once there is an old value to compare with
      if(compare_log)
      {
         // Figuring out what the normalized value was for the input is a good way to figure out where the discontinuity
         // occured w.r.t. the PWL segments.
         ac_fixed<Wfi, 0, false, AC_TRN, AC_WRAP> norm_input;
         ac_normalize(input, norm_input);
         // if by any chance the function output has dropped in value, print out at what point the problem has occured
         // and throw a runtime assertion.
         if(old_output_log > actual_log)
         {
            cout << endl;                                            // LCOV_EXCL_LINE
            cout << "  log output not monotonic at :" << endl;       // LCOV_EXCL_LINE
            cout << "  x = " << input << endl;                       // LCOV_EXCL_LINE
            cout << "  y = " << log_out << endl;                     // LCOV_EXCL_LINE
            cout << "  old_output_log = " << old_output_log << endl; // LCOV_EXCL_LINE
            cout << "  normalized x    = " << norm_input << endl;    // LCOV_EXCL_LINE
            assert(false);                                           // LCOV_EXCL_LINE
         }
      }

#ifdef DEBUG
      if(this_error_log > allowed_error)
      {
         cout << endl;
         cout << "  Error exceeds tolerance" << endl;
         cout << "  input           = " << input << endl;
         cout << "  expected_log   = " << expected_log << endl;
         cout << "  actual_log     = " << actual_log << endl;
         cout << "  this_error_log = " << this_error_log << endl;
         assert(false);
      }
#endif

      if(this_error_log > max_log_error)
      {
         max_log_error = this_error_log;
      }
   }

   if(max_log_error > cumulative_max_error_log)
   {
      cumulative_max_error_log = max_log_error;
   }

   passed = (max_log_error < allowed_error);

   if(passed)
   {
      printf("PASSED , max err (%f)\n", max_log_error);
   }
   else
   {
      printf("FAILED , max err (%f)\n", max_log_error);
   } // LCOV_EXCL_LINE

   return 0;
}

int main(int argc, char* argv[])
{
   double max_error_log = 0.0;
   double allowed_error = 1;

   cout << "=============================================================================" << endl;
   cout << "Testing function: ac_log_pwl() - Allowed error " << allowed_error << endl;

   // template <int Wfi, int Ifi, int outWfi, int outIfi, bool outSfi>
   test_driver<20, 12, 64, 32, true>(max_error_log, allowed_error);
   test_driver<20, 8, 64, 32, true>(max_error_log, allowed_error);
   test_driver<20, 30, 64, 32, true>(max_error_log, allowed_error);
   test_driver<20, 20, 64, 32, true>(max_error_log, allowed_error);
   test_driver<20, -2, 64, 32, true>(max_error_log, allowed_error);
   test_driver<20, -3, 64, 32, true>(max_error_log, allowed_error);

   cout << "=============================================================================" << endl;
   cout << "  Testbench finished. Maximum errors observed across all bit-width variations:" << endl;
   cout << "    max_error_log = " << max_error_log << endl;

   // If error limits on any test value have been crossed, the test has failed
   // Notify the user that the test was a failure if that is the case.
   if(max_error_log > allowed_error)
   {
      cout << "  ac_log_pwl - FAILED - Error tolerance(s) exceeded" << endl;                           // LCOV_EXCL_LINE
      cout << "=============================================================================" << endl; // LCOV_EXCL_LINE
      return (-1);                                                                                     // LCOV_EXCL_LINE
   }

   cout << "  ac_log_pwl - PASSED" << endl;
   cout << "=============================================================================" << endl;
   return (0);
}
