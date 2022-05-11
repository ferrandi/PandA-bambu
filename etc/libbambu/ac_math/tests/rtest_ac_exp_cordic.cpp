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
// ac_exp_cordic() function using a variety of data types and bit-
// widths.

// To compile standalone and run:
//   $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_exp_cordic.cpp -o design
//   ./design

// Include the AC Math function that is exercised with this testbench
#include <ac_math/ac_hcordic.h>
using namespace ac_math;

// ==============================================================================
// Test Design
//   This simple function allows executing the ac_exp_cordic() function.
//   Template parameters are used to configure the bit-widths of the
//   ac_fixed inputs.

template <int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi>
void test_ac_exp_cordic(const ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>& in,
                        ac_fixed<outWfi, outIfi, false, AC_TRN, AC_WRAP>& out_exp)
{
   ac_exp_cordic(in, out_exp);
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
//   cordic table exp model with the computed natural exponential using a
//   standard C double type. The maximum error for each type is accumulated
//   in variables defined in the calling function.

template <int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi>
int test_driver(double& cumulative_max_error_exp, const double allowed_error, const double threshold,
                bool details = false)
{
   bool passed = true;
   double max_error_exp = 0.0; // reset for this run

   ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP> input;
   typedef ac_fixed<outWfi, outIfi, false, AC_TRN, AC_WRAP> T_out;
   T_out output_exp;

   double lower_limit, upper_limit, step;

   // set ranges and step size for testbench
   lower_limit = input.template set_val<AC_VAL_MIN>().to_double();
   upper_limit = input.template set_val<AC_VAL_MAX>().to_double();
   step = input.template set_val<AC_VAL_QUANTUM>().to_double();

   cout << "TEST: ac_exp_cordic() INPUT: ";
   cout.width(38);
   cout << left << input.type_name();
   cout << "OUTPUT: ";
   cout.width(38);
   cout << left << output_exp.type_name();
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

   for(double i = lower_limit; i <= upper_limit; i += step)
   {
      // Set values for input.
      input = i;
      test_ac_exp_cordic(input, output_exp);

      // call reference exp() with fixed-pt value converted back to double
      // an additional step of typecasting is required in order to perform
      // quantization on the expected output.
      double expected_value_exp = ((T_out)exp(input.to_double())).to_double();
      double actual_value_exp = output_exp.to_double();

      double this_error_exp;
      // If expected value of output falls below the threshold, calculate absolute error instead of relative
      if(expected_value_exp > threshold)
      {
         this_error_exp = abs_double((expected_value_exp - actual_value_exp) / expected_value_exp) * 100.0;
      }
      else
      {
         this_error_exp = abs_double(expected_value_exp - actual_value_exp) * 100.0;
      }

#ifdef DEBUG
      if(this_error_exp > allowed_error)
      {
         cout << endl;
         cout << "  Error exceeds tolerance" << endl;
         cout << "  input               = " << input << endl;
         cout << "  expected_value_exp = " << expected_value_exp << endl;
         cout << "  actual_value_exp   = " << actual_value_exp << endl;
         cout << "  this_error_exp     = " << this_error_exp << endl;
         cout << "  threshold           = " << threshold << endl;
         assert(false);
      }
#endif

      if(this_error_exp > max_error_exp)
      {
         max_error_exp = this_error_exp;
      }
   }

   passed = (max_error_exp < allowed_error);

   if(passed)
   {
      printf("PASSED , max err %f\n", max_error_exp);
   }
   else
   {
      printf("FAILED , max err %f\n", max_error_exp);
   } // LCOV_EXCL_LINE

   if(max_error_exp > cumulative_max_error_exp)
   {
      cumulative_max_error_exp = max_error_exp;
   }

   return 0;
}

int main(int argc, char* argv[])
{
   double max_error_exp = 0;
   double allowed_error = 0.5;
   double threshold = 0.005;

   cout << "=============================================================================" << endl;
   cout << "Testing function: ac_exp_cordic() - Allowed error " << allowed_error << endl;

   // template <int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi>

   test_driver<16, 4, false, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<16, 3, false, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<16, 2, false, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<16, 1, false, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<16, 0, false, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<16, -1, false, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<16, -2, false, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<16, -3, false, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<16, -4, false, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<2, 4, false, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<3, 4, false, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<1, 3, false, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<5, 4, false, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<16, 5, true, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<16, 4, true, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<16, 3, true, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<16, 2, true, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<16, 1, true, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<16, 0, true, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<16, -1, true, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<16, -2, true, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<16, -3, true, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<16, -4, true, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<2, 4, true, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<3, 4, true, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<3, 3, true, 50, 30>(max_error_exp, allowed_error, threshold);
   test_driver<5, 4, true, 50, 30>(max_error_exp, allowed_error, threshold);

   cout << "=============================================================================" << endl;
   cout << "  Testbench finished. Maximum error observed across all bit-width variations:" << endl;
   cout << "    max_error_exp = " << max_error_exp << endl;

   bool test_fail = (max_error_exp > allowed_error);

   // If error limits on any test value have been crossed, the test has failed
   // Notify the user that the test was a failure if that is the case.
   if(test_fail)
   {
      cout << "  ac_exp_cordic - FAILED - Error tolerance(s) exceeded" << endl;                        // LCOV_EXCL_LINE
      cout << "=============================================================================" << endl; // LCOV_EXCL_LINE
      return -1;                                                                                       // LCOV_EXCL_LINE
   }
   else
   {
      cout << "  ac_exp_cordic - PASSED" << endl;
      cout << "=============================================================================" << endl;
   }

   return 0;
}
