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
// ac_tan_pwl() function using a variety of data types and bit-
// widths.

// To compile standalone and run:
//   $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_tan_pwl.cpp -o design
//   ./design

// Include the AC Math function that is exercised with this testbench
#include <ac_math/ac_tan_pwl.h>
using namespace ac_math;

// ==============================================================================
// Test Design
//   This simple function allows executing the ac_tan_pwl() function.
//   Template parameters are used to configure the bit-widths of the
//   ac_fixed inputs.

template <int Wfi, int Ifi, int outWfi, int outIfi>
void test_ac_tan_pwl(const ac_fixed<Wfi, Ifi, false, AC_TRN, AC_WRAP>& in,
                     ac_fixed<outWfi, outIfi, false, AC_TRN, AC_WRAP>& tan_out)
{
   tan_out = ac_tan_pwl<ac_fixed<outWfi, outIfi, false, AC_TRN, AC_WRAP>>(in);
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
//   piecewise linear tan model with the computed tangent using a
//   standard C double type. The maximum error for each type is accumulated
//   in variables defined in the calling function.

template <int Wfi, int Ifi, int outWfi, int outIfi>
int test_driver(double& cumulative_max_error_tan, const double allowed_error, const double threshold,
                const double upper_limit_degrees, bool details = false)
{
   ac_fixed<Wfi + 2, Ifi + 1, false, AC_TRN, AC_WRAP> i; // make loop variable slightly larger
   ac_fixed<Wfi, Ifi, false, AC_TRN, AC_WRAP> input;
   ac_fixed<Wfi, Ifi, false, AC_TRN, AC_WRAP> last;
   typedef ac_fixed<outWfi, outIfi, false, AC_TRN, AC_WRAP> T_out;
   T_out tan_out;

   // set ranges and step size for testbench
   ac_fixed<Wfi, Ifi> lower_limit = input.template set_val<AC_VAL_MIN>();
   double upper_limit_type = input.template set_val<AC_VAL_MAX>().to_double();
   double upper_limit_rad = upper_limit_degrees * M_PI / 180;
   ac_fixed<Wfi, Ifi> upper_limit = upper_limit_type < upper_limit_rad ? upper_limit_type : upper_limit_rad;
   ac_fixed<Wfi, Ifi> step = input.template set_val<AC_VAL_QUANTUM>();

   cout << "TEST: ac_tan_pwl() INPUT: ";
   cout.width(38);
   cout << left << input.type_name();
   cout << "upper_limit_degrees = " << upper_limit_degrees << " ";
   cout << "OUTPUT: ";
   cout.width(38);
   cout << left << tan_out.type_name();
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
   double max_tan_error = 0.0;

   bool check_monotonic = true;
   bool compare_tan = false;
   double old_output_tan;

   for(i = lower_limit; i <= upper_limit; i += step)
   {
      // Set values for input.
      input = i;

      // For now, skip zero values.
      if(input == 0)
      {
         continue;
      }
      // call reference tan() with fixed-pt value converted back to double
      // an additional step of typecasting is required in order to perform
      // quantization on the expected output.
      double expected_tan = ((T_out)tan(input.to_double())).to_double();

      // If input is zero, saturate the expected value according to the min. value representible by
      // the fixed point output.
      if(input == 0)
      {
         T_out output_min;
         output_min.template set_val<AC_VAL_MIN>();
         expected_tan = output_min.to_double();
      }

      // call DUT with fixed-pt value
      test_ac_tan_pwl(input, tan_out);

      double actual_tan = tan_out.to_double();
      double this_error_tan;

      // If expected value of either output falls below the threshold, calculate absolute error instead of relative
      if(expected_tan > threshold)
      {
         this_error_tan = abs_double((expected_tan - actual_tan) / expected_tan) * 100.0;
      }
      else
      {
         this_error_tan = abs_double(expected_tan - actual_tan) * 100.0;
      }

      if(check_monotonic)
      {
         // MONOTONIC: Make sure that function is monotonic. Compare old value (value of previous iteration) with
         // current value. Since the tangent function we are testing is an increasing function, and our testbench value
         // keeps incrementing or remains the same (in case of saturation), we expect the old value to be lesser than or
         // equal to the current one.

         // This comparison is only carried out once there is an old value to compare with
         if(compare_tan)
         {
            // Figuring out what the normalized value was for the input is a good way to figure out where the
            // discontinuity occured w.r.t. the PWL segments.
            ac_fixed<Wfi, 0, false, AC_TRN, AC_WRAP> norm_input;
            ac_normalize(input, norm_input);
            // if by any chance the function output has dropped in value, print out at what point the problem has
            // occured and throw a runtime assertion.
            if(old_output_tan > actual_tan)
            {
               cout << endl;                                            // LCOV_EXCL_LINE
               cout << "  tan output not monotonic at :" << endl;       // LCOV_EXCL_LINE
               cout << "  x = " << input << endl;                       // LCOV_EXCL_LINE
               cout << "  y = " << tan_out << endl;                     // LCOV_EXCL_LINE
               cout << "  old_output_tan = " << old_output_tan << endl; // LCOV_EXCL_LINE
               assert(false);                                           // LCOV_EXCL_LINE
            }
         }
         // Update the old value
         old_output_tan = actual_tan;
         // Once an old value has been stored, i.e. towards the end of the first iteration, this value is set to true.
         compare_tan = true;
      }

#ifdef DEBUG
      double input_degrees = input.to_double() * 180 / M_PI;
      if(this_error_tan > allowed_error)
      {
         cout << endl;
         cout << "  Error exceeds tolerance" << endl;
         cout << "  input          = " << input << endl;
         cout << "  input_degrees  = " << input_degrees << endl;
         cout << "  expected_tan   = " << expected_tan << endl;
         cout << "  actual_tan     = " << actual_tan << endl;
         cout << "  this_error_tan = " << this_error_tan << endl;
         cout << "  threshold      = " << threshold << endl;
         assert(false);
      }
#endif

      if(this_error_tan > max_tan_error)
      {
         max_tan_error = this_error_tan;
      }
   }

   if(max_tan_error > cumulative_max_error_tan)
   {
      cumulative_max_error_tan = max_tan_error;
   }

   passed = (max_tan_error < allowed_error);

   if(passed)
   {
      printf("PASSED , max err (%f)\n", max_tan_error);
   }
   else
   {
      printf("FAILED , max err (%f)\n", max_tan_error);
   } // LCOV_EXCL_LINE

   return 0;
}

int main(int argc, char* argv[])
{
   double max_error_tan = 0.0;
   double allowed_error = 4.0;
   double threshold = 0.1;

   cout << "=============================================================================" << endl;
   cout << "Testing function: ac_tan_pwl() - Allowed error " << allowed_error << endl;

   // template <int Wfi, int Ifi, int outWfi, int outIfi>
   test_driver<22, 4, 64, 20>(max_error_tan, allowed_error, threshold, 45);
   test_driver<22, 4, 64, 20>(max_error_tan, allowed_error, threshold, 50);
   test_driver<22, 4, 64, 20>(max_error_tan, allowed_error, threshold, 55);
   test_driver<22, 4, 64, 20>(max_error_tan, allowed_error, threshold, 60);
   test_driver<22, 4, 64, 20>(max_error_tan, allowed_error, threshold, 65);
   test_driver<22, 4, 64, 20>(max_error_tan, allowed_error, threshold, 70);
   test_driver<22, 4, 64, 20>(max_error_tan, allowed_error, threshold, 75);
   test_driver<22, 4, 64, 20>(max_error_tan, allowed_error, threshold, 80);
   test_driver<22, 4, 64, 20>(max_error_tan, allowed_error, threshold, 81);
   test_driver<22, 4, 64, 20>(max_error_tan, allowed_error, threshold, 82);
   test_driver<22, 4, 64, 20>(max_error_tan, allowed_error, threshold, 83);
   test_driver<22, 4, 64, 20>(max_error_tan, allowed_error, threshold, 84);
   test_driver<22, 4, 64, 20>(max_error_tan, allowed_error, threshold, 85);
   test_driver<22, 4, 64, 20>(max_error_tan, allowed_error, threshold, 86);
   test_driver<22, 4, 64, 20>(max_error_tan, allowed_error, threshold, 87);
   test_driver<22, 4, 64, 20>(max_error_tan, allowed_error, threshold, 88);
   test_driver<22, 4, 64, 20>(max_error_tan, allowed_error, threshold, 89);
   test_driver<22, 4, 64, 20>(max_error_tan, allowed_error, threshold, 89.18);
   test_driver<22, -2, 64, 20>(max_error_tan, allowed_error, threshold, 45);
   test_driver<22, -3, 64, 20>(max_error_tan, allowed_error, threshold, 45);

   cout << "=============================================================================" << endl;
   cout << "  Testbench finished. Maximum errors observed across all bit-width variations:" << endl;
   cout << "    max_error_tan = " << max_error_tan << endl;

   // If error limits on any test value have been crossed, the test has failed
   // Notify the user that the test was a failure if that is the case.
   if(max_error_tan > allowed_error)
   {
      cout << "  ac_tan_pwl - FAILED - Error tolerance(s) exceeded" << endl;                           // LCOV_EXCL_LINE
      cout << "=============================================================================" << endl; // LCOV_EXCL_LINE
      return (-1);                                                                                     // LCOV_EXCL_LINE
   }

   cout << "  ac_tan_pwl - PASSED" << endl;
   cout << "=============================================================================" << endl;
   return (0);
}
