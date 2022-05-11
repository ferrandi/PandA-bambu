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
// ac_log2_cordic() function using a variety of data types and bit-
// widths.

// To compile standalone and run:
//   $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_log2_cordic.cpp -o design
//   ./design

// Include the AC Math function that is exercised with this testbench
#include <ac_math/ac_hcordic.h>
using namespace ac_math;

// ==============================================================================
// Test Design
//   This simple function allows executing the ac_log2_cordic() function.
//   Template parameters are used to configure the bit-widths of the
//   ac_fixed inputs.

template <int Wfi, int Ifi, int outWfi, int outIfi, bool outSfi>
void test_ac_log2_cordic(const ac_fixed<Wfi, Ifi, false, AC_TRN, AC_WRAP>& in,
                         ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>& log2_out)
{
   ac_log2_cordic(in, log2_out);
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
//   cordic table log2 model with the computed base 2 logarithm using a
//   standard C double type. The maximum error for each type is accumulated
//   in variables defined in the calling function.

template <int Wfi, int Ifi, int outWfi, int outIfi, bool outSfi>
int test_driver(double& cumulative_max_error_log2, const double allowed_error, const double threshold,
                bool details = false)
{
   ac_fixed<Wfi + 2, Ifi + 1, false, AC_TRN, AC_WRAP> i; // make loop variable slightly larger
   ac_fixed<Wfi, Ifi, false, AC_TRN, AC_WRAP> input;
   ac_fixed<Wfi, Ifi, false, AC_TRN, AC_WRAP> last;
   typedef ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> T_out;
   T_out log2_out;

   // set ranges and step size for testbench
   ac_fixed<Wfi, Ifi, false> lower_limit = input.template set_val<AC_VAL_MIN>();
   ac_fixed<Wfi, Ifi, false> upper_limit = input.template set_val<AC_VAL_MAX>();
   ac_fixed<Wfi, Ifi, false> step = input.template set_val<AC_VAL_QUANTUM>();

   cout << "TEST: ac_log2_cordic() INPUT: ";
   cout.width(38);
   cout << left << input.type_name();
   cout << "OUTPUT: ";
   cout.width(38);
   cout << left << log2_out.type_name();
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
   double max_log2_error = 0.0;

   for(i = lower_limit; i <= upper_limit; i += step)
   {
      // Set values for input.
      input = i;
      if(input.to_double() == 0)
      {
         continue;
      }

      // call reference log2() with fixed-pt value converted back to double
      // an additional step of typecasting is required in order to perform
      // quantization on the expected output.
      double expected_log2 = ((T_out)log2(input.to_double())).to_double();

      // call DUT with fixed-pt value
      test_ac_log2_cordic(input, log2_out);

      double actual_log2 = log2_out.to_double();
      double this_error_log2;

      // If expected value of either output falls below the threshold, calculate absolute error instead of relative
      if(expected_log2 > threshold)
      {
         this_error_log2 = abs_double((expected_log2 - actual_log2) / expected_log2) * 100.0;
      }
      else
      {
         this_error_log2 = abs_double(expected_log2 - actual_log2) * 100.0;
      }

#ifdef DEBUG
      if(this_error_log2 > allowed_error)
      {
         cout << endl;
         cout << "  Error exceeds tolerance" << endl;
         cout << "  input           = " << input << endl;
         cout << "  expected_log2   = " << expected_log2 << endl;
         cout << "  actual_log2     = " << actual_log2 << endl;
         cout << "  this_error_log2 = " << this_error_log2 << endl;
         cout << "  threshold       = " << threshold << endl;
         assert(false);
      }
#endif

      if(this_error_log2 > max_log2_error)
      {
         max_log2_error = this_error_log2;
      }
   }

   if(max_log2_error > cumulative_max_error_log2)
   {
      cumulative_max_error_log2 = max_log2_error;
   }

   passed = (max_log2_error < allowed_error);

   if(passed)
   {
      printf("PASSED , max err (%f)\n", max_log2_error);
   }
   else
   {
      printf("FAILED , max err (%f)\n", max_log2_error);
   } // LCOV_EXCL_LINE

   return 0;
}

int main(int argc, char* argv[])
{
   double max_error_log2 = 0.0;
   double allowed_error = 0.5;
   double threshold = 0.005;

   cout << "=============================================================================" << endl;
   cout << "Testing function: ac_log2_cordic() - Allowed error " << allowed_error << endl;

   // template <int Wfi, int Ifi, int outWfi, int outIfi, bool outSfi>
   test_driver<16, -5, 40, 12, true>(max_error_log2, allowed_error, threshold);
   test_driver<16, -4, 40, 12, true>(max_error_log2, allowed_error, threshold);
   test_driver<16, -3, 40, 12, true>(max_error_log2, allowed_error, threshold);
   test_driver<16, -2, 40, 12, true>(max_error_log2, allowed_error, threshold);
   test_driver<16, -1, 40, 12, true>(max_error_log2, allowed_error, threshold);
   test_driver<16, 0, 40, 12, true>(max_error_log2, allowed_error, threshold);
   test_driver<16, 1, 40, 12, true>(max_error_log2, allowed_error, threshold);
   test_driver<16, 2, 40, 12, true>(max_error_log2, allowed_error, threshold);
   test_driver<16, 3, 40, 12, true>(max_error_log2, allowed_error, threshold);
   test_driver<16, 4, 40, 12, true>(max_error_log2, allowed_error, threshold);
   test_driver<16, 5, 40, 12, true>(max_error_log2, allowed_error, threshold);
   test_driver<16, 6, 40, 12, true>(max_error_log2, allowed_error, threshold);
   test_driver<16, 7, 40, 12, true>(max_error_log2, allowed_error, threshold);
   test_driver<16, 8, 40, 12, true>(max_error_log2, allowed_error, threshold);
   test_driver<16, 9, 40, 12, true>(max_error_log2, allowed_error, threshold);
   test_driver<8, 12, 40, 12, true>(max_error_log2, allowed_error, threshold);

   cout << "=============================================================================" << endl;
   cout << "  Testbench finished. Maximum errors observed across all bit-width variations:" << endl;
   cout << "    max_error_log2 = " << max_error_log2 << endl;

   // If error limits on any test value have been crossed, the test has failed
   // Notify the user that the test was a failure if that is the case.
   if(max_error_log2 > allowed_error)
   {
      cout << "  ac_log2_cordic - FAILED - Error tolerance(s) exceeded" << endl;                       // LCOV_EXCL_LINE
      cout << "=============================================================================" << endl; // LCOV_EXCL_LINE
      return (-1);                                                                                     // LCOV_EXCL_LINE
   }

   cout << "  ac_log2_cordic - PASSED" << endl;
   cout << "=============================================================================" << endl;
   return (0);
}
