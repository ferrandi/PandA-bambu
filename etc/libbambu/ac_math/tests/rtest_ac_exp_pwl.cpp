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
// ac_exp_pwl() function using a variety of data types and bit-
// widths.

// To compile standalone and run:
//   $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_exp_pwl.cpp -o design
//   ./design

// Include the AC Math function that is exercised with this testbench
#include <ac_math/ac_pow_pwl.h>
using namespace ac_math;

// ==============================================================================
// Test Design
//   This simple function allows executing the ac_exp_pwl() function. Template
//   parameters are used to configure the bit-widths of the ac_fixed inputs.

template <int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi>
void test_ac_exp_pwl(const ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>& in,
                     ac_fixed<outWfi, outIfi, false, AC_TRN, AC_WRAP>& out_exp)
{
   out_exp = ac_exp_pwl<ac_fixed<outWfi, outIfi, false, AC_TRN, AC_WRAP>>(in);
}

// ==============================================================================

#include <fstream>
#include <iostream>
#include <math.h>
#include <string>
using namespace std;

// ==============================================================================
// Function: test_driver()
// Description: A templatized function that can be configured for certain bit-
//   widths of ac_fixed inputs. It uses the type information to iterate through a
//   range of valid values on that type in order to compare the precision of the
//   piece-wise linear power model with the computed power using a
//   standard C double type. The maximum error for each type is accumulated
//   in variables defined in the calling function.

template <int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi>
int test_driver(double& cumulative_max_error_exp, const double allowed_error, const double threshold,
                bool details = false)
{
   bool passed = true;
   bool check_monotonic = true;
   double max_error_exp = 0.0; // reset for this run

   ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP> input;

   typedef ac_fixed<outWfi, outIfi, false, AC_TRN, AC_WRAP> T_out;
   T_out output_exp;

   double lower_limit, upper_limit, step;

   // set ranges and step size for fixed point testbench
   lower_limit = input.template set_val<AC_VAL_MIN>().to_double();
   upper_limit = input.template set_val<AC_VAL_MAX>().to_double();
   step = input.template set_val<AC_VAL_QUANTUM>().to_double();

   cout << "TEST: ac_exp_pwl() INPUT: ";
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

   double old_output_exp;
   bool compare_exp = false;

   for(double i = lower_limit; i <= upper_limit; i += step)
   {
      // Set values for input.
      input = i;
      test_ac_exp_pwl(input, output_exp);

      double expected_value_exp = ((T_out)exp(input.to_double())).to_double();
      double actual_value_exp = output_exp.to_double();

      double this_error_exp;

      // If expected value of output falls below the threshold, calculate absolute error instead of relative

      if(expected_value_exp > threshold)
      {
         this_error_exp = abs((expected_value_exp - actual_value_exp) / expected_value_exp) * 100.0;
      }
      else
      {
         this_error_exp = abs(expected_value_exp - actual_value_exp) * 100.0;
      }

      if(check_monotonic)
      {
         // MONOTONIC: Make sure that function is monotonic. Compare old value (value of previous iteration) with
         // current value. Since the exponential function we are testing is an increasing function, and our testbench
         // value keeps incrementing or remains the same (in case of saturation), we expect the old value to be lesser
         // than or equal to the current one.

         // Update the old value
         old_output_exp = actual_value_exp;
         // Once an old value has been stored, i.e. towards the end of the first iteration, this value is set to true.
         compare_exp = true;

         // same thing as above, but for the natural exponential.
         if(compare_exp)
         {
            if(old_output_exp > actual_value_exp)
            {
               cout << "FILE : " << __FILE__ << ", LINE : " << __LINE__ << endl; // LCOV_EXCL_LINE
               cout << "exp output not monotonic at :" << endl;                  // LCOV_EXCL_LINE
               cout << "x = " << input << endl;                                  // LCOV_EXCL_LINE
               cout << "old_output_exp = " << old_output_exp << endl;            // LCOV_EXCL_LINE
               assert(false);                                                    // LCOV_EXCL_LINE
            }
         }
         old_output_exp = actual_value_exp;
         compare_exp = true;
      }

#ifdef DEBUG
      if(this_error_exp > allowed_error)
      {
         cout << endl;
         cout << "  Error exceeds tolerance" << endl;
         cout << "  input              = " << input << endl;
         cout << "  expected_value_exp = " << expected_value_exp << endl;
         cout << "  actual_value_exp   = " << actual_value_exp << endl;
         cout << "  this_error_exp     = " << this_error_exp << endl;
         cout << "  threshold          = " << threshold << endl;
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
      printf("PASSED , max err (%f exp)\n", max_error_exp);
   }
   else
   {
      printf("FAILED , max err (%f exp)\n", max_error_exp);
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
   double threshold = 0.05;
   cout << "=============================================================================" << endl;
   cout << "Testing function: ac_exp_pwl() - Allowed error " << allowed_error << endl;

   // template <int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi>
   test_driver<12, 3, true, 64, 32>(max_error_exp, allowed_error, threshold);
   test_driver<4, 2, true, 64, 32>(max_error_exp, allowed_error, threshold);
   test_driver<3, 5, true, 64, 32>(max_error_exp, allowed_error, threshold);
   test_driver<4, -2, true, 64, 32>(max_error_exp, allowed_error, threshold);
   test_driver<3, 5, true, 64, 32>(max_error_exp, allowed_error, threshold);
   test_driver<2, 5, true, 64, 32>(max_error_exp, allowed_error, threshold);
   test_driver<16, 5, true, 64, 32>(max_error_exp, allowed_error, threshold);
   test_driver<12, 4, false, 64, 32>(max_error_exp, allowed_error, threshold);
   test_driver<4, 2, false, 64, 32>(max_error_exp, allowed_error, threshold);
   test_driver<4, -2, false, 60, 30>(max_error_exp, allowed_error, threshold);
   test_driver<3, 4, false, 64, 32>(max_error_exp, allowed_error, threshold);
   test_driver<1, 5, false, 61, 33>(max_error_exp, allowed_error, threshold);
   test_driver<16, 4, false, 64, 32>(max_error_exp, allowed_error, threshold);
   test_driver<16, 0, false, 64, 32>(max_error_exp, allowed_error, threshold);

   cout << "=============================================================================" << endl;
   cout << "  Testbench finished. Maximum errors observed across all bit-width variations:" << endl;
   cout << "    max_error_exp  = " << max_error_exp << endl;

   // If error limits on any tested datatype have been crossed, the test has failed
   bool test_fail = (max_error_exp > allowed_error);

   // Notify the user that the test was a failure.
   if(test_fail)
   {
      cout << "  ac_exp_pwl - FAILED - Error tolerance(s) exceeded" << endl;                           // LCOV_EXCL_LINE
      cout << "=============================================================================" << endl; // LCOV_EXCL_LINE
      return -1;                                                                                       // LCOV_EXCL_LINE
   }
   else
   {
      cout << "  ac_exp_pwl - PASSED" << endl;
      cout << "=============================================================================" << endl;
   }
   return 0;
}
