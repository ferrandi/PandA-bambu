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
// ac_atan_pwl() function using a variety of data types and bit-
// widths.

// To compile satandalone and run:
//   $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_atan_pwl.cpp -o design
//   ./design

// Include the AC Math function that is exercised with this testbench
#include <ac_math/ac_atan_pwl.h>
using namespace ac_math;

// ==============================================================================
// Test Design
//   This simple function allows executing the ac_atan_pwl() function.
//   Template parameters are used to configure the bit-widths of the
//   ac_fixed inputs.

template <int Wfi, int Ifi, int outWfi, int outIfi>
void test_ac_atan_pwl(const ac_fixed<Wfi, Ifi, false, AC_TRN, AC_WRAP>& in,
                      ac_fixed<outWfi, outIfi, false, AC_TRN, AC_WRAP>& atan_out)
{
   atan_out = ac_atan_pwl<ac_fixed<outWfi, outIfi, false, AC_TRN, AC_WRAP>>(in);
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
//   piecewise linear atan model with the computed arctangent using a
//   standard C double type. The maximum error for each type is accumulated
//   in variables defined in the calling function.

template <int Wfi, int Ifi, int outWfi, int outIfi>
int test_driver(double& cumulative_max_error_atan, const double allowed_error, bool details = false)
{
   double i; // make loop variable slightly larger
   ac_fixed<Wfi, Ifi, false, AC_TRN, AC_WRAP> input;
   typedef ac_fixed<outWfi, outIfi, false, AC_TRN, AC_WRAP> T_out;
   T_out atan_out;

   // set ranges and step size for testbench
   double lower_limit = input.template set_val<AC_VAL_MIN>().to_double();
   double upper_limit = input.template set_val<AC_VAL_MAX>().to_double();
   double step = input.template set_val<AC_VAL_QUANTUM>().to_double();

   cout << "TEST: ac_atan_pwl() INPUT: ";
   cout.width(38);
   cout << left << input.type_name();
   cout << "OUTPUT: ";
   cout.width(38);
   cout << left << atan_out.type_name();
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
   double max_atan_error = 0.0;

   bool check_monotonic = true;
   bool compare_atan = false;
   double old_output_atan;

   for(i = lower_limit; i <= upper_limit; i += step)
   {
      // Set values for input.
      input = i;

      // call reference atan() with fixed-pt value converted back to double
      // an additional step of typecasting is required in order to perform
      // quantization on the expected output.
      double expected_atan = ((T_out)atan(input.to_double())).to_double();

      // call DUT with fixed-pt value
      test_ac_atan_pwl(input, atan_out);

      double actual_atan = atan_out.to_double();
      double this_error_atan;

      // Calculate absolute error.
      this_error_atan = abs_double(expected_atan - actual_atan) * 100.0;

      if(check_monotonic)
      {
         // MONOTONIC: Make sure that function is monotonic. Compare old value (value of previous iteration) with
         // current value. Since the arctangent function we are testing is an increasing function, and our testbench
         // value keeps incrementing or remains the same (in case of saturation), we expect the old value to be lesser
         // than or equal to the current one.

         // This comparison is only carried out once there is an old value to compare with
         if(compare_atan)
         {
            // if by any chance the function output has dropped in value, print out at what point the problem has
            // occured and throw a runtime assertion.
            if(old_output_atan > actual_atan)
            {
               cout << endl;                                              // LCOV_EXCL_LINE
               cout << "  atan output not monotonic at :" << endl;        // LCOV_EXCL_LINE
               cout << "  x = " << input << endl;                         // LCOV_EXCL_LINE
               cout << "  y = " << atan_out << endl;                      // LCOV_EXCL_LINE
               cout << "  old_output_atan = " << old_output_atan << endl; // LCOV_EXCL_LINE
               assert(false);                                             // LCOV_EXCL_LINE
            }
         }
         // Update the old value
         old_output_atan = actual_atan;
         // Once an old value has been stored, i.e. towards the end of the first iteration, this value is set to true.
         compare_atan = true;
      }

#ifdef DEBUG
      double output_degrees = actual_atan * 180 / M_PI;
      if(this_error_atan > allowed_error)
      {
         cout << endl;
         cout << "  Error exceeds tolerance" << endl;
         cout << "  input           = " << input << endl;
         cout << "  expected_atan   = " << expected_atan << endl;
         cout << "  actual_atan     = " << actual_atan << endl;
         cout << "  output_degrees  = " << output_degrees << endl;
         cout << "  this_error_atan = " << this_error_atan << endl;
         assert(false);
      }
#endif

      if(this_error_atan > max_atan_error)
      {
         max_atan_error = this_error_atan;
      }
   }

   if(max_atan_error > cumulative_max_error_atan)
   {
      cumulative_max_error_atan = max_atan_error;
   }

   passed = (max_atan_error < allowed_error);

   if(passed)
   {
      printf("PASSED , max err (%f)\n", max_atan_error);
   }
   else
   {
      printf("FAILED , max err (%f)\n", max_atan_error);
   } // LCOV_EXCL_LINE

   return 0;
}

int main(int argc, char* argv[])
{
   double max_error_atan = 0.0;
   double allowed_error = 1.0;

   cout << "=============================================================================" << endl;
   cout << "Testing function: ac_atan_pwl() - Allowed error " << allowed_error << endl;

   // template <int Wfi, int Ifi, int outWfi, int outIfi>
   test_driver<20, 6, 33, 1>(max_error_atan, allowed_error);
   test_driver<20, -6, 33, 1>(max_error_atan, allowed_error);
   test_driver<22, 8, 33, 1>(max_error_atan, allowed_error);
   test_driver<20, 0, 33, 1>(max_error_atan, allowed_error);
   test_driver<8, 12, 33, 1>(max_error_atan, allowed_error);
   test_driver<20, 6, 42, 10>(max_error_atan, allowed_error);
   test_driver<20, -6, 42, 10>(max_error_atan, allowed_error);
   test_driver<22, 8, 42, 10>(max_error_atan, allowed_error);
   test_driver<20, 0, 42, 10>(max_error_atan, allowed_error);
   test_driver<8, 12, 42, 10>(max_error_atan, allowed_error);

   cout << "=============================================================================" << endl;
   cout << "  Testbench finished. Maximum errors observed across all bit-width variations:" << endl;
   cout << "    max_error_atan = " << max_error_atan << endl;

   // If error limits on any test value have been crossed, the test has failed
   // Notify the user that the test was a failure if that is the case.
   if(max_error_atan > allowed_error)
   {
      cout << "  ac_atan_pwl - FAILED - Error tolerance(s) exceeded" << endl;                          // LCOV_EXCL_LINE
      cout << "=============================================================================" << endl; // LCOV_EXCL_LINE
      return (-1);                                                                                     // LCOV_EXCL_LINE
   }

   cout << "  ac_atan_pwl - PASSED" << endl;
   cout << "=============================================================================" << endl;
   return (0);
}
