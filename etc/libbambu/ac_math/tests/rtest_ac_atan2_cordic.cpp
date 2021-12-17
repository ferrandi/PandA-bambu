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
// ac_atan2_cordic() function using a variety of bit-widths.

// To compile standalone and run:
//   $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_atan2_cordic.cpp -o design
//   ./design

// Include the AC Math function that is exercised with this testbench
#include <ac_math/ac_atan2_cordic.h>
using namespace ac_math;

//==============================================================================
// Test Design
//   This simple function allows executing the ac_atan2_cordic() function.
//   Template parameters are used to configure the bit-widths of the types.

template <int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi, bool outSfi>
void test_ac_atan2_cordic(const ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>& y,
                          const ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>& x,
                          ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>& out_atan2)
{
   ac_atan2_cordic(y, x, out_atan2);
}

//==============================================================================

#include <fstream>
#include <iostream>
#include <math.h>
#include <string>
using namespace std;

//==============================================================================
// Function: test_driver()
// Description: A templatized function that can be configured for certain bit-
//   widths of the fixed point AC datatype. It uses the type information to
//   iterate through a range of valid values on that type in order to compare
//   the precision of the cordic based atan2 model with atan2 using
//   the standard C math library. The maximum error for each type is accumulated
//   in variables defined in the calling function.

template <int Wfi, int Ifi, int Sfi, int outWfi, int outIfi, bool outSfi>
int test_driver(double& cummulative_max_error_atan2, const double allowed_error, bool details = false)
{
   bool passed = true;
   double max_error_atan2 = 0.0; // reset for this run

   ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP> y;
   ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP> x;
   ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> out_atan2;

   typedef ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> T_out;

   double lower_limit, upper_limit, step;

   // set ranges and step size for the testbench
   lower_limit = y.template set_val<AC_VAL_MIN>().to_double();
   upper_limit = y.template set_val<AC_VAL_MAX>().to_double();
   step = y.template set_val<AC_VAL_QUANTUM>().to_double();

   cout << "TEST: ac_atan2_cordic() INPUT: ";
   cout.width(38);
   cout << left << y.type_name();
   cout << "        OUTPUTS: ";
   cout.width(38);
   cout << left << out_atan2.type_name();
   cout << "RESULT: ";

   // Dump the test details
   if(details)
   {
      cout << endl;                                           // LCOV_EXCL_LINE
      cout << "  Ranges for input types:" << endl;            // LCOV_EXCL_LINE
      cout << "    lower_limit    = " << lower_limit << endl; // LCOV_EXCL_LINE
      cout << "    upper_limit    = " << upper_limit << endl; // LCOV_EXCL_LINE
      cout << "    step           = " << step << endl;        // LCOV_EXCL_LINE
   }

   for(double i = lower_limit; i < upper_limit; i += step)
      for(double j = lower_limit; j < upper_limit; j += step)
      {
         // Set values for input.
         x = i;
         y = j;
         test_ac_atan2_cordic(y, x, out_atan2);

         double expected_atan2_value = ((T_out)(atan2(y.to_double(), x.to_double()))).to_double();
         double actual_atan2_value = out_atan2.to_double();
         double this_error_atan2;

         this_error_atan2 = fabs(expected_atan2_value - actual_atan2_value) * 100.0;

         if(this_error_atan2 > max_error_atan2)
         {
            max_error_atan2 = this_error_atan2;
         }
      }

   if(passed)
   {
      printf("PASSED , max err (%f atan2) \n", max_error_atan2);
   }
   else
   {
      printf("FAILED , max err (%f atan2) \n", max_error_atan2);
   } // LCOV_EXCL_LINE

   if(max_error_atan2 > cummulative_max_error_atan2)
   {
      cummulative_max_error_atan2 = max_error_atan2;
   }

   return 0;
}

int main(int argc, char* argv[])
{
   double max_error_atan2 = 0;
   double allowed_error = 0.1;
   cout << "=============================================================================" << endl;
   cout << "Testing function: ac_atan2_cordic() - Allowed error " << allowed_error << endl;

   // template <int Wfi, int Ifi, int Sfi>
   test_driver<12, 1, true, 24, 5, true>(max_error_atan2, allowed_error);
   test_driver<2, 0, true, 27, 2, true>(max_error_atan2, allowed_error);
   test_driver<11, 6, true, 24, 3, true>(max_error_atan2, allowed_error);
   test_driver<4, 1, true, 25, 3, true>(max_error_atan2, allowed_error);
   test_driver<10, -2, true, 30, 4, true>(max_error_atan2, allowed_error);
   test_driver<9, -2, true, 28, 2, true>(max_error_atan2, allowed_error);
   test_driver<2, -2, true, 32, 2, true>(max_error_atan2, allowed_error);
   test_driver<11, 1, true, 34, 2, true>(max_error_atan2, allowed_error);
   test_driver<8, -3, true, 25, 2, true>(max_error_atan2, allowed_error);
   test_driver<7, -3, true, 26, 2, true>(max_error_atan2, allowed_error);
   test_driver<5, 2, true, 24, 2, true>(max_error_atan2, allowed_error);
   test_driver<6, 2, true, 24, 2, true>(max_error_atan2, allowed_error);

   cout << "=============================================================================" << endl;
   cout << "  Testbench finished. Maximum errors observed across all bit-width variations:" << endl;
   cout << "    max_error_atan2       = " << max_error_atan2 << endl;

   // If error limits on any tested datatype have been crossed, the test has failed
   bool test_fail = max_error_atan2 > allowed_error;

   // Notify the user that the test was a failure.
   if(test_fail)
   {
      cout << "  ac_atan2_cordic - FAILED - Error tolerance(s) exceeded" << endl;                      // LCOV_EXCL_LINE
      cout << "=============================================================================" << endl; // LCOV_EXCL_LINE
      return -1;                                                                                       // LCOV_EXCL_LINE
   }
   else
   {
      cout << "  ac_atan2_cordic - PASSED" << endl;
      cout << "=============================================================================" << endl;
   }
   return 0;
}
