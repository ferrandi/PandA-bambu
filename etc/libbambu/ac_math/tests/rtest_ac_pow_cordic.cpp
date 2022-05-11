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
// ac_pow_cordic() function using a variety of data types and bit-
// widths.

// To compile standalone and run:
//   $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_pow_cordic.cpp -o design
//   ./design

// Include the AC Math function that is exercised with this testbench
#include <ac_math/ac_hcordic.h>
using namespace ac_math;

// ==============================================================================
// Test Design
//   This simple function allows executing the ac_pow_cordic() function.
//   Template parameters are used to configure the bit-widths of the
//   ac_fixed inputs.

template <int baseW, int baseI, int expW, int expI, bool expS, int outW, int outI>
void test_ac_pow_cordic(const ac_fixed<baseW, baseI, false, AC_TRN, AC_WRAP>& base,
                        const ac_fixed<expW, expI, expS, AC_TRN, AC_WRAP>& exponent,
                        ac_fixed<outW, outI, false, AC_TRN, AC_WRAP>& out_pow)
{
   ac_pow_cordic(base, exponent, out_pow);
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

// ==============================================================================
// Function: test_driver()
// Description: A templatized function that can be configured for certain bit-
//   widths of AC datatypes. It uses the type information to iterate through a
//   range of valid values on that type in order to compare the precision of the
//   cordic table pow model with the computed power output using a
//   standard C double type. The maximum error for each type is accumulated
//   in variables defined in the calling function.

template <int baseW, int baseI, int expW, int expI, bool expS, int outW, int outI>
int test_driver(double& cumulative_max_error, const double allowed_error, const double threshold, bool details = false)
{
   bool passed = true;
   double max_error_pow = 0.0; // reset for this run

   ac_fixed<baseW, baseI, false, AC_TRN, AC_WRAP> base;
   ac_fixed<expW, expI, expS, AC_TRN, AC_WRAP> exponent;

   typedef ac_fixed<outW, outI, false, AC_TRN, AC_WRAP> T_out;
   T_out out_pow;

   double lower_limit_base, upper_limit_base, step_base, lower_limit_exponent, upper_limit_exponent, step_exponent;

   // set ranges and step size for testbench
   lower_limit_base = base.template set_val<AC_VAL_MIN>().to_double();
   upper_limit_base = base.template set_val<AC_VAL_MAX>().to_double();
   step_base = base.template set_val<AC_VAL_QUANTUM>().to_double();

   lower_limit_exponent = exponent.template set_val<AC_VAL_MIN>().to_double();
   upper_limit_exponent = exponent.template set_val<AC_VAL_MAX>().to_double();
   step_exponent = exponent.template set_val<AC_VAL_QUANTUM>().to_double();

   cout << "TEST: ac_pow_cordic() INPUTS: ";
   cout.width(38);
   cout << left << base.type_name();
   cout.width(38);
   cout << left << exponent.type_name();
   cout << "OUTPUT: ";
   cout.width(38);
   cout << left << out_pow.type_name();
   cout << "RESULT: ";

   // Dump the test details
   if(details)
   {
      cout << endl;                                                              // LCOV_EXCL_LINE
      cout << "  Ranges for input types:" << endl;                               // LCOV_EXCL_LINE
      cout << "    lower_limit_base     = " << lower_limit_base << endl;         // LCOV_EXCL_LINE
      cout << "    upper_limit_base     = " << upper_limit_base << endl;         // LCOV_EXCL_LINE
      cout << "    step_base            = " << step_base << endl;                // LCOV_EXCL_LINE
      cout << "    lower_limit_exponent     = " << lower_limit_exponent << endl; // LCOV_EXCL_LINE
      cout << "    upper_limit_exponent     = " << upper_limit_exponent << endl; // LCOV_EXCL_LINE
      cout << "    step_exponent            = " << step_exponent << endl;        // LCOV_EXCL_LINE
   }

   for(double i = lower_limit_base; i <= upper_limit_base; i += step_base)
   {
      for(double j = lower_limit_exponent; j < upper_limit_exponent; j += step_exponent)
      {
         // Set values for inputs
         base = i;
         exponent = j;
         if(base == 0 || exponent == 0)
         {
            continue;
         }
         test_ac_pow_cordic(base, exponent, out_pow);
         double expected_value_pow = ((T_out)pow(base.to_double(), exponent.to_double())).to_double();
         double actual_value_pow = out_pow.to_double();
         double this_error_pow;

         // If expected value of output falls below the threshold, calculate absolute error instead of relative
         if(expected_value_pow > threshold)
         {
            this_error_pow = abs_double((expected_value_pow - actual_value_pow) / expected_value_pow) * 100.0;
         }
         else
         {
            this_error_pow = abs_double(expected_value_pow - actual_value_pow) * 100.0;
         }

#ifdef DEBUG
         if(this_error_pow > allowed_error)
         {
            cout << endl;
            cout << "  Error exceeds tolerance" << endl;
            cout << "  base               = " << base << endl;
            cout << "  exponent           = " << exponent << endl;
            cout << "  expected_value_pow = " << expected_value_pow << endl;
            cout << "  actual_value_pow   = " << actual_value_pow << endl;
            cout << "  this_error_pow     = " << this_error_pow << endl;
            cout << "  threshold          = " << threshold << endl;
            assert(false);
         }
#endif

         if(this_error_pow > max_error_pow)
         {
            max_error_pow = this_error_pow;
         }
      }
   }

   passed = (max_error_pow < allowed_error);

   if(passed)
   {
      printf("PASSED , max err %f\n", max_error_pow);
   }
   else
   {
      printf("FAILED , max err %f\n", max_error_pow);
   } // LCOV_EXCL_LINE

   if(max_error_pow > cumulative_max_error)
   {
      cumulative_max_error = max_error_pow;
   }

   return 0;
}

int main(int argc, char* argv[])
{
   double max_error_pow = 0;
   double allowed_error = 0.5;
   double threshold = 0.005;

   cout << "=============================================================================" << endl;
   cout << "Testing function: ac_pow_cordic() - Allowed error " << allowed_error << endl;

   // template <int baseW, int baseI, int expW, int expI, bool expS, int outW, int outI>
   test_driver<8, 3, 10, 2, true, 40, 20>(max_error_pow, allowed_error, threshold);
   test_driver<8, 2, 10, 2, true, 40, 20>(max_error_pow, allowed_error, threshold);
   test_driver<8, 2, 10, 2, false, 40, 20>(max_error_pow, allowed_error, threshold);
   test_driver<8, 3, 10, 2, false, 40, 20>(max_error_pow, allowed_error, threshold);
   test_driver<10, 4, 10, 2, false, 40, 20>(max_error_pow, allowed_error, threshold);

   cout << "=============================================================================" << endl;
   cout << "  Testbench finished. Maximum errors observed across all bit-width variations:" << endl;
   cout << "    max_error_pow = " << max_error_pow << endl;

   bool test_fail = (max_error_pow > allowed_error);

   // If error limits on any test value have been crossed, the test has failed
   // Notify the user that the test was a failure if that is the case.
   if(test_fail)
   {
      cout << "  ac_pow_cordic - FAILED - Error tolerance(s) exceeded" << endl;                        // LCOV_EXCL_LINE
      cout << "=============================================================================" << endl; // LCOV_EXCL_LINE
      return -1;                                                                                       // LCOV_EXCL_LINE
   }
   else
   {
      cout << "  ac_pow_cordic - PASSED" << endl;
      cout << "=============================================================================" << endl;
   }

   return 0;
}
