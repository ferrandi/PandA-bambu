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
// ac_normalize() function using a variety of data types and bit-
// widths.

// To compile standalone and run:
//   $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_normalize.cpp -o design
//   ./design

// Include the AC Math function that is exercised with this testbench

#include <ac_math/ac_normalize.h>
using namespace ac_math;

// ==============================================================================
// Test Design
//   This simple function allows executing the ac_normalize() function
//   using multiple data types at the same time (in this case, ac_fixed and
//   ac_complex<ac_fixed>). Template parameters are used to configure the
//   bit-widths of the types.

template <int Wfi, int Ifi, bool Sfi>
void test_ac_normalize(const ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>& in1,
                       ac_fixed<Wfi, int(Sfi), Sfi, AC_TRN, AC_WRAP>& out1,
                       const ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>>& in2,
                       ac_complex<ac_fixed<Wfi, int(Sfi), Sfi, AC_TRN, AC_WRAP>>& out2, int& expret_fixed,
                       int& expret_complex)
{
   expret_fixed = ac_normalize(in1, out1);
   expret_complex = ac_normalize(in2, out2);
}

// ==============================================================================
// Function: test_driver_fixed()
// Description: A templatized function that can be configured for certain bit-
//   widths of AC datatypes. It uses the type information to iterate through a
//   range of valid values on that type in order to make sure that the
//   normalized output representation corresponds to the input representation.

template <int Wfi, int Ifi, bool Sfi>
int test_driver_fixed(bool& all_tests_pass, bool details = false)
{
   // Declare types for testing real and complex inputs/outputs
   ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP> input_fixed;
   ac_fixed<Wfi, int(Sfi), Sfi, AC_TRN, AC_WRAP> output_fixed;
   ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>> cmplx_input_fixed;
   ac_complex<ac_fixed<Wfi, int(Sfi), Sfi, AC_TRN, AC_WRAP>> cmplx_output_fixed;

   double lower_limit, upper_limit, step;

   // set ranges and step size for fixed point testbench
   lower_limit = input_fixed.template set_val<AC_VAL_MIN>().to_double();
   upper_limit = input_fixed.template set_val<AC_VAL_MAX>().to_double();
   step = input_fixed.template set_val<AC_VAL_QUANTUM>().to_double();

   cout << "TEST: ac_normalize() AC_FIXED INPUT: ";
   cout.width(38);
   cout << left << input_fixed.type_name();
   cout << "AC_FIXED OUTPUT: ";
   cout.width(38);
   cout << left << output_fixed.type_name();
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

   bool incorrect = false;
   int nocalls, expret_fixed, expret_complex;

   // test fixed-point real and complex.

   // Go through every value that can be represented by the real and complex inputs.
   for(double i = lower_limit; i <= upper_limit; i += step)
   {
      for(double j = lower_limit; j <= upper_limit; j += step)
      {
         input_fixed = i;
         cmplx_input_fixed.r() = i;
         cmplx_input_fixed.i() = j;
         test_ac_normalize(input_fixed, output_fixed, cmplx_input_fixed, cmplx_output_fixed, expret_fixed,
                           expret_complex);
         nocalls++;

         // This flag is set to false if the real output is incorrect
         bool incorrect_fixed = (output_fixed.to_double() * pow(2, (double)expret_fixed) != input_fixed);
         // Make sure that the range of the normalized output is as expected.
         incorrect_fixed = incorrect_fixed || ((output_fixed > -0.5) && (output_fixed < 0.5)) ||
                           ((output_fixed >= 1) && (output_fixed < -1));
         // Inputs and outputs being zero is a special case, and the range-checking above will produce a false negative
         // in such a case. This is taken care of below.
         if(output_fixed == 0 && input_fixed == 0)
         {
            incorrect_fixed = false;
         }

         // This flag is set to false if the complex output is incorrect
         bool incorrect_complex = ((cmplx_output_fixed.r().to_double() * pow(2, (double)expret_complex) !=
                                    cmplx_input_fixed.r().to_double()) ||
                                   (cmplx_output_fixed.i().to_double() * pow(2, (double)expret_complex) !=
                                    cmplx_input_fixed.i().to_double()));
         // Make sure that the range of the normalized output is as expected.
         incorrect_complex = incorrect_complex || ((cmplx_output_fixed.r() > -0.5) && (cmplx_output_fixed.r() < 0.5) &&
                                                   (cmplx_output_fixed.i() > -0.5) && (cmplx_output_fixed.i() < 0.5));
         incorrect_complex = incorrect_complex || (cmplx_output_fixed.r() >= 1) || (cmplx_output_fixed.r() < -1) ||
                             ((cmplx_output_fixed.i() >= 1) && (cmplx_output_fixed.i() < -1));
         // Inputs and outputs being zero is a special case, and the range-checking above will produce a false negative
         // in such a case. This is taken care of below.
         if(cmplx_output_fixed.r() == 0 && cmplx_output_fixed.i() == 0 && cmplx_input_fixed.r() == 0 &&
            cmplx_input_fixed.i() == 0)
         {
            incorrect_complex = false;
         }

         // If even a single input produces an incorrect output, the incorrect flag is set to true
         if(incorrect_fixed || incorrect_complex)
         {
            incorrect = true;
         }
      }
   }

   if(incorrect)
   {
      printf("FAILED\n");
   }
   else
   {
      printf("PASSED\n");
   } // LCOV_EXCL_LINE

   all_tests_pass = all_tests_pass && !incorrect;
   return 0;
}

int main(int argc, char* argv[])
{
   cout << "=============================================================================" << endl;
   cout << "Testing function: ac_normalize()" << endl;

   bool all_tests_pass = true;

   // If any of the tests fail, the all_tests_pass variable will be set to false

   // template <int Wfi, int Ifi, int Sfi>
   test_driver_fixed<7, 0, false>(all_tests_pass);
   test_driver_fixed<7, -3, false>(all_tests_pass);
   test_driver_fixed<7, 4, false>(all_tests_pass);
   test_driver_fixed<7, 9, false>(all_tests_pass);
   test_driver_fixed<7, 7, false>(all_tests_pass);
   test_driver_fixed<7, 0, true>(all_tests_pass);
   test_driver_fixed<7, -3, true>(all_tests_pass);
   test_driver_fixed<7, 4, true>(all_tests_pass);
   test_driver_fixed<7, 9, true>(all_tests_pass);
   test_driver_fixed<7, 7, true>(all_tests_pass);
   test_driver_fixed<10, 0, false>(all_tests_pass);
   test_driver_fixed<10, -3, false>(all_tests_pass);
   test_driver_fixed<10, 4, false>(all_tests_pass);
   test_driver_fixed<10, 14, false>(all_tests_pass);
   test_driver_fixed<10, 10, false>(all_tests_pass);
   test_driver_fixed<10, 0, true>(all_tests_pass);
   test_driver_fixed<10, -3, true>(all_tests_pass);
   test_driver_fixed<10, 4, true>(all_tests_pass);
   test_driver_fixed<10, 14, true>(all_tests_pass);
   test_driver_fixed<10, 10, true>(all_tests_pass);

   cout << "=============================================================================" << endl;
   cout << "  Testbench finished." << endl;

   // Notify the user whether or not the test was a failure.
   if(!all_tests_pass)
   {
      cout << "  ac_normalize - FAILED - Normalized output does not match input for all test values"
           << endl;                                                                                    // LCOV_EXCL_LINE
      cout << "=============================================================================" << endl; // LCOV_EXCL_LINE
      return -1;                                                                                       // LCOV_EXCL_LINE
   }
   else
   {
      cout << "  ac_normalize - PASSED" << endl;
      cout << "=============================================================================" << endl;
   }
   return 0;
}
