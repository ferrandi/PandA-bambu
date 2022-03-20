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
// ac_div() function using a variety of data types and bit-
// widths.

// To compile standalone and run:
//   $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_div.cpp -o design
//   ./design

// Include the AC Math function that is exercised with this testbench
#include <ac_math/ac_div.h>
using namespace ac_math;

// ==============================================================================
// Test Designs
//   These simple functions allow executing the ac_div() function
//   using multiple data types at the same time. Template parameters are
//   used to configure the bit-widths of the types.

template <int numW, int denW, int quoW, int remW>
void test_ac_div_int(const ac_int<numW, false>& us_real_num, const ac_int<denW, false>& us_real_den,
                     ac_int<quoW, false>& us_real_quo, ac_int<remW, false>& us_real_rem,
                     const ac_int<numW + 1, true>& s_real_num, const ac_int<denW + 1, true>& s_real_den,
                     ac_int<quoW + 1, true>& s_real_quo, ac_int<remW + 1, true>& s_real_rem)
{
   ac_div(us_real_num, us_real_den, us_real_quo, us_real_rem);
   ac_div(s_real_num, s_real_den, s_real_quo, s_real_rem);
}

template <int numW, int numI, int denW, int denI, int quoW, int quoI>
void test_ac_div_fixed(const ac_fixed<numW, numI, false, AC_TRN, AC_WRAP>& us_real_num,
                       const ac_fixed<denW, denI, false, AC_TRN, AC_WRAP>& us_real_den,
                       ac_fixed<quoW, quoI, false, AC_TRN, AC_WRAP>& us_real_quo,
                       const ac_fixed<numW + 1, numI + 1, true, AC_TRN, AC_WRAP>& s_real_num,
                       const ac_fixed<denW + 1, denI + 1, true, AC_TRN, AC_WRAP>& s_real_den,
                       ac_fixed<quoW + 1, quoI + 1, true, AC_TRN, AC_WRAP>& s_real_quo,
                       const ac_complex<ac_fixed<numW + 1, numI + 1, true, AC_TRN, AC_WRAP>>& complex_num,
                       const ac_complex<ac_fixed<denW + 1, denI + 1, true, AC_TRN, AC_WRAP>>& complex_den,
                       ac_complex<ac_fixed<quoW + 1, quoI + 1, true, AC_TRN, AC_WRAP>>& complex_quo)
{
   ac_div(us_real_num, us_real_den, us_real_quo);
   ac_div(s_real_num, s_real_den, s_real_quo);
   ac_div(complex_num, complex_den, complex_quo);
}

template <int numW, int numI, int numE, int denW, int denI, int denE, int quoW, int quoI, int quoE>
void test_ac_div_float(const ac_float<numW, numI, numE, AC_TRN>& real_num,
                       const ac_float<denW, denI, denE, AC_TRN>& real_den, ac_float<quoW, quoI, quoE, AC_TRN>& real_quo)
{
   ac_div(real_num, real_den, real_quo);
}

// ==============================================================================

#include <fstream>
#include <iostream>
#include <math.h>
#include <string>
using namespace std;

// ------------------------------------------------------------------------------
// Helper functions for output-checking/error calculation

// See if output is correct for real ac_int inputs, where both quotient and
// remainder are returned. No error calculation required for this datatype.

template <class T_num, class T_den, class T_quo, class T_rem>
bool correct_output(const T_num num, const T_den den, const T_quo quo, const T_rem rem)
{
   // numerator = denominator * quotient + remainder.
   return num.to_double() == den.to_double() * quo.to_double() + rem.to_double();
}

// Calculating error for real ac_fixed and ac_float inputs.

template <class T_num, class T_den, class T_quo>
double err_calc(const T_num num, const T_den den, const T_quo quo, const double allowed_error, const double threshold)
{
   // The typecasting is done in order to provide quantization on the expected output.
   double expected_value = ((T_quo)(num.to_double() / den.to_double())).to_double();
   double actual_value = quo.to_double();
   double this_error;

   // If expected value is greater than a particular threshold, calculate relative error, else, calculate absolute
   // error.
   if(abs(expected_value) > threshold)
   {
      this_error = abs((expected_value - actual_value) / expected_value) * 100.0;
   }
   else
   {
      this_error = abs(expected_value - actual_value) * 100.0;
   }

   return this_error;
}

// Calculating error for complex ac_fixed and ac_float inputs.

template <class T_num, class T_den, class T_quo>
double cmplx_err_calc(const ac_complex<T_num> num, const ac_complex<T_den> den, const ac_complex<T_quo> quo,
                      const double allowed_error, const double threshold)
{
   ac_complex<double> expected_value, actual_value, num_double, den_double, diff_op;
   double error;

   num_double.r() = num.r().to_double();
   num_double.i() = num.i().to_double();

   den_double.r() = den.r().to_double();
   den_double.i() = den.i().to_double();

   expected_value = num_double / den_double;

   expected_value.r() = ((T_quo)expected_value.r()).to_double();
   expected_value.i() = ((T_quo)expected_value.i()).to_double();

   actual_value.r() = quo.r().to_double();
   actual_value.i() = quo.i().to_double();

   // Store the difference between the expected vs actual output.
   diff_op = expected_value - actual_value;

   if(sqrt(expected_value.mag_sqr()) > threshold)
   {
      error = sqrt((diff_op / expected_value).mag_sqr()) * 100;
   }
   else
   {
      error = sqrt(diff_op.mag_sqr()) * 100;
   }

   return error;
}

// ==============================================================================
// Function: test_driver_int()
// Description: A templatized function that can be configured for certain bit-
//   widths of ac_int variables. It uses the type information to iterate through a
//   range of valid values on that type in order to make sure that the inputs and
//   outputs of the ac_div function match each other.

template <int numW, int denW, int quoW, int remW>
int test_driver_int(bool& all_tests_pass_int, bool details = false)
{
   ac_int<numW, false> us_real_num, us_real_num_arr[numW];
   ac_int<numW, false> us_real_num_start = 0;
   ac_int<denW, false> us_real_den, us_real_den_arr[denW];
   ac_int<numW, false> us_real_den_start = 0;
   ac_int<quoW, false> us_real_quo;
   ac_int<remW, false> us_real_rem;

   ac_int<numW + 1, true> s_real_num;
   ac_int<denW + 1, true> s_real_den;
   ac_int<quoW + 1, true> s_real_quo;
   ac_int<remW + 1, true> s_real_rem;

   // printf("TEST: ac_div() INPUTS: ac_int<%2d, false>, ac_int<%2d, true> (numerator) ac_int<%2d, false>, ac_int<%2d,
   // true> (denominator)\nOUTPUTS: ac_int<%2d, false>, ac_int<%2d, true> (quotient) ac_int<%2d, false>, ac_int<%2d,
   // true> (remainder)  RESULT:
   // ",
   //       numW, numW + 1, denW, denW + 1, quoW, quoW + 1, remW, remW + 1);

   cout << "TEST: ac_div() INPUTS: ";
   cout.width(38);
   cout << left << us_real_num.type_name();
   cout.width(38);
   cout << left << us_real_den.type_name();
   cout.width(38);
   cout << left << s_real_num.type_name();
   cout.width(38);
   cout << left << s_real_den.type_name();
   cout << "OUTPUTS: ";
   cout.width(38);
   cout << left << us_real_quo.type_name();
   cout.width(38);
   cout << left << s_real_quo.type_name();
   cout << "RESULT: ";

   int bit_value = 1;

   // Assign a value for that has alternating 1s and 0s, with the
   // same precision as the unsigned numerator.
   for(int i = numW - 1; i >= 0; i--)
   {
      us_real_num_start[i] = bit_value;
      bit_value = ~bit_value;
   }

   bit_value = 1;

   // Do the same as above, but for the denominator.
   for(int i = denW - 1; i >= 0; i--)
   {
      us_real_den_start[i] = bit_value;
      bit_value = ~bit_value;
   }

   // Assign an array of reference values, and every time,
   // shift the initial value ("_start" value) by 1 bit, for
   // the numerator and the denominator.

   for(int i = 0; i <= numW - 1; i++)
   {
      us_real_num_arr[i] = us_real_num_start >> i;
   }

   for(int i = 0; i <= denW - 1; i++)
   {
      us_real_den_arr[i] = us_real_den_start >> i;
   }

   bool correct = true;

   // Iterate through all possible combinations of values stored in the arrays above.
   // For signed division, the denominator will be the negative of the
   // corresponding unsigned value for that iteration. The numerator,
   // on the other hand, will the same as the corresponding unsigned value.
   for(int i = 0; i <= numW - 1; i++)
   {
      us_real_num = us_real_num_arr[i];
      s_real_num = us_real_num_arr[i];
      for(int j = 0; j <= denW - 1; j++)
      {
         us_real_den = us_real_den_arr[j];
         s_real_den = -us_real_den_arr[j];
         if(us_real_den == 0)
         {
            continue;
         }
         // Pass all inputs at one go
         test_ac_div_int(us_real_num, us_real_den, us_real_quo, us_real_rem, s_real_num, s_real_den, s_real_quo,
                         s_real_rem);
         bool correct_iteration = correct_output(us_real_num, us_real_den, us_real_quo, us_real_rem) &&
                                  correct_output(s_real_num, s_real_den, s_real_quo, s_real_rem);
         // If even a single output isn't correct, the value of "correct" will be set to false.
         correct = correct && correct_iteration;
      }
   }

   // Also, test for corner cases

   // First corner: real numerator = MAX, real denominator = MAX
   us_real_num.template set_val<AC_VAL_MAX>();
   us_real_den.template set_val<AC_VAL_MAX>();
   s_real_num.template set_val<AC_VAL_MAX>();
   s_real_den.template set_val<AC_VAL_MAX>();
   test_ac_div_int(us_real_num, us_real_den, us_real_quo, us_real_rem, s_real_num, s_real_den, s_real_quo, s_real_rem);
   bool correct_corner = correct_output(us_real_num, us_real_den, us_real_quo, us_real_rem) &&
                         correct_output(s_real_num, s_real_den, s_real_quo, s_real_rem);
   correct = correct && correct_corner;

   // Second corner: signed real numerator = MAX, signed real denominator = MIN
   s_real_num.template set_val<AC_VAL_MAX>();
   s_real_den.template set_val<AC_VAL_MIN>();
   test_ac_div_int(us_real_num, us_real_den, us_real_quo, us_real_rem, s_real_num, s_real_den, s_real_quo, s_real_rem);
   correct_corner = correct_output(s_real_num, s_real_den, s_real_quo, s_real_rem);
   correct = correct && correct_corner;

   // Third corner: signed real numerator = MIN, signed real denominator = MAX
   s_real_num.template set_val<AC_VAL_MIN>();
   s_real_den.template set_val<AC_VAL_MAX>();
   test_ac_div_int(us_real_num, us_real_den, us_real_quo, us_real_rem, s_real_num, s_real_den, s_real_quo, s_real_rem);
   correct_corner = correct_output(s_real_num, s_real_den, s_real_quo, s_real_rem);
   correct = correct && correct_corner;

   // Fourth corner: signed real numerator = MIN, signed real denominator = MIN
   s_real_den.template set_val<AC_VAL_MIN>();
   test_ac_div_int(us_real_num, us_real_den, us_real_quo, us_real_rem, s_real_num, s_real_den, s_real_quo, s_real_rem);
   correct_corner = correct_output(s_real_num, s_real_den, s_real_quo, s_real_rem);
   correct = correct && correct_corner;

   // Fifth corner: real numerator = MAX, real denominator = QUANTUM
   us_real_num.template set_val<AC_VAL_MAX>();
   us_real_den.template set_val<AC_VAL_QUANTUM>();
   s_real_num.template set_val<AC_VAL_MAX>();
   s_real_den.template set_val<AC_VAL_QUANTUM>();
   test_ac_div_int(us_real_num, us_real_den, us_real_quo, us_real_rem, s_real_num, s_real_den, s_real_quo, s_real_rem);
   correct_corner = correct_output(us_real_num, us_real_den, us_real_quo, us_real_rem) &&
                    correct_output(s_real_num, s_real_den, s_real_quo, s_real_rem);
   correct = correct && correct_corner;

   // Sixth corner: real numerator = QUANTUM, real denominator = MAX
   us_real_num.template set_val<AC_VAL_QUANTUM>();
   us_real_den.template set_val<AC_VAL_MAX>();
   s_real_num.template set_val<AC_VAL_QUANTUM>();
   s_real_den.template set_val<AC_VAL_MAX>();
   test_ac_div_int(us_real_num, us_real_den, us_real_quo, us_real_rem, s_real_num, s_real_den, s_real_quo, s_real_rem);
   correct_corner = correct_output(us_real_num, us_real_den, us_real_quo, us_real_rem) &&
                    correct_output(s_real_num, s_real_den, s_real_quo, s_real_rem);
   correct = correct && correct_corner;

   // Seventh corner: signed real numerator = MIN, signed real denominator = QUANTUM
   s_real_num.template set_val<AC_VAL_MIN>();
   s_real_den.template set_val<AC_VAL_QUANTUM>();
   test_ac_div_int(us_real_num, us_real_den, us_real_quo, us_real_rem, s_real_num, s_real_den, s_real_quo, s_real_rem);
   correct_corner = correct_output(s_real_num, s_real_den, s_real_quo, s_real_rem);
   correct = correct && correct_corner;

   // Eighth corner: signed real numerator = QUANTUM, signed real denominator = MIN
   s_real_num.template set_val<AC_VAL_QUANTUM>();
   s_real_den.template set_val<AC_VAL_MIN>();
   test_ac_div_int(us_real_num, us_real_den, us_real_quo, us_real_rem, s_real_num, s_real_den, s_real_quo, s_real_rem);
   correct_corner = correct_output(s_real_num, s_real_den, s_real_quo, s_real_rem);
   correct = correct && correct_corner;

   // Ninth corner: real numerator = QUANTUM, real denominator = QUANTUM
   us_real_num.template set_val<AC_VAL_QUANTUM>();
   us_real_den.template set_val<AC_VAL_QUANTUM>();
   s_real_den.template set_val<AC_VAL_QUANTUM>();
   test_ac_div_int(us_real_num, us_real_den, us_real_quo, us_real_rem, s_real_num, s_real_den, s_real_quo, s_real_rem);
   correct_corner = correct_output(us_real_num, us_real_den, us_real_quo, us_real_rem) &&
                    correct_output(s_real_num, s_real_den, s_real_quo, s_real_rem);
   correct = correct && correct_corner;

   if(correct)
   {
      printf("PASSED\n");
   }
   else
   {
      printf("FAILED\n");
   }

   all_tests_pass_int = all_tests_pass_int && correct;

   return 0;
}

template <int numW, int numI, int denW, int denI, int quoW, int quoI>
int test_driver_fixed(double& cumulative_max_error_fixed, double& cumulative_max_error_cmplx_fixed,
                      const double allowed_error_fixed, const double threshold, bool details = false)
{
   bool passed;
   double max_error_fixed = 0.0;       // reset for this run
   double max_error_cmplx_fixed = 0.0; // reset for this run

   ac_fixed<numW, numI, false, AC_TRN, AC_WRAP> us_real_num, us_real_num_start = 0, us_real_num_arr[numW];
   ac_fixed<denW, denI, false, AC_TRN, AC_WRAP> us_real_den, us_real_den_start = 0, us_real_den_arr[denW];
   ac_fixed<quoW, quoI, false, AC_TRN, AC_WRAP> us_real_quo;
   ac_fixed<numW + 1, numI + 1, true, AC_TRN, AC_WRAP> s_real_num;
   ac_fixed<denW + 1, denI + 1, true, AC_TRN, AC_WRAP> s_real_den;
   ac_fixed<quoW + 1, quoI + 1, true, AC_TRN, AC_WRAP> s_real_quo;
   ac_complex<ac_fixed<numW + 1, numI + 1, true, AC_TRN, AC_WRAP>> complex_num;
   ac_complex<ac_fixed<denW + 1, denI + 1, true, AC_TRN, AC_WRAP>> complex_den;
   ac_complex<ac_fixed<quoW + 1, quoI + 1, true, AC_TRN, AC_WRAP>> complex_quo;

   cout << "TEST: ac_div() INPUTS: ";
   cout.width(38);
   cout << left << us_real_num.type_name();
   cout.width(38);
   cout << left << us_real_den.type_name();
   cout.width(38);
   cout << left << s_real_num.type_name();
   cout.width(38);
   cout << left << s_real_den.type_name();
   cout << "OUTPUTS: ";
   cout.width(38);
   cout << left << us_real_quo.type_name();
   cout.width(38);
   cout << left << s_real_quo.type_name();
   cout << "RESULT: ";

   int bit_value = 1;

   // Assign a value for that has alternating 1s and 0s, with the
   // same precision as the unsigned numerator.
   for(int i = numW - 1; i >= 0; i--)
   {
      us_real_num_start[i] = bit_value;
      bit_value = ~bit_value;
   }

   bit_value = 1;

   // Do the same as above, but for the denominator.
   for(int i = denW - 1; i >= 0; i--)
   {
      us_real_den_start[i] = bit_value;
      bit_value = ~bit_value;
   }

   // Assign an array of reference values, and every time,
   // shift the initial value ("_start" value) right by 1 bit, for
   // the numerator and the denominator.

   for(int i = 0; i <= numW - 1; i++)
   {
      us_real_num_arr[i] = us_real_num_start >> i;
   }

   for(int i = 0; i <= denW - 1; i++)
   {
      us_real_den_arr[i] = us_real_den_start >> i;
   }

   double this_error_s, this_error_us, this_error_fixed, this_error_complex;

   // Iterate through all possible combinations of values stored in the arrays above.
   // For signed division, the denominator will be the negative of the
   // corresponding unsigned value for that iteration. The numerator,
   // on the other hand, will the same value as the corresponding unsigned value.
   for(int i = 0; i <= numW - 1; i++)
   {
      us_real_num = us_real_num_arr[i];
      s_real_num = us_real_num_arr[i];
      complex_num.r() = s_real_num;
      for(int j = 0; j <= denW - 1; j++)
      {
         us_real_den = us_real_den_arr[j];
         s_real_den = -us_real_den_arr[j];
         complex_den.r() = s_real_den;
         for(int k = 0; k <= numW - 1; k++)
         {
            complex_num.i() = us_real_num_arr[k];
            for(int l = 0; l <= denW - 1; l++)
            {
               complex_den.i() = -us_real_den_arr[l];
               if(us_real_den == 0 || (complex_den.r() == 0 && complex_den.i() == 0))
               {
                  continue;
               }
               test_ac_div_fixed(us_real_num, us_real_den, us_real_quo, s_real_num, s_real_den, s_real_quo, complex_num,
                                 complex_den, complex_quo);
               // Calculate error for signed values.
               this_error_s = err_calc(s_real_num, s_real_den, s_real_quo, allowed_error_fixed, threshold);
               // Calculate error for unsigned values.
               this_error_us = err_calc(us_real_num, us_real_den, us_real_quo, allowed_error_fixed, threshold);
               // Whichever error is greater is assigned to this_error_fixed
               this_error_fixed = this_error_s > this_error_us ? this_error_s : this_error_us;
               // Calculate error for complex inputs and outputs
               this_error_complex =
                   cmplx_err_calc(complex_num, complex_den, complex_quo, allowed_error_fixed, threshold);
               if(this_error_fixed > max_error_fixed)
               {
                  max_error_fixed = this_error_fixed;
               }
               if(this_error_complex > max_error_cmplx_fixed)
               {
                  max_error_cmplx_fixed = this_error_complex;
               }
            }
         }
      }
   }

   // Also, test for corner cases

   // First corner: real numerator = MAX, real denominator = MAX, complex numerator = (MAX, MAX), complex denominator =
   // (MAX, MAX)
   us_real_num.template set_val<AC_VAL_MAX>();
   us_real_den.template set_val<AC_VAL_MAX>();
   s_real_num.template set_val<AC_VAL_MAX>();
   s_real_den.template set_val<AC_VAL_MAX>();
   complex_num.r().template set_val<AC_VAL_MAX>();
   complex_num.i().template set_val<AC_VAL_MAX>();
   complex_den.r().template set_val<AC_VAL_MAX>();
   complex_den.i().template set_val<AC_VAL_MAX>();
   test_ac_div_fixed(us_real_num, us_real_den, us_real_quo, s_real_num, s_real_den, s_real_quo, complex_num,
                     complex_den, complex_quo);
   // Calculate error for signed values.
   this_error_s = err_calc(s_real_num, s_real_den, s_real_quo, allowed_error_fixed, threshold);
   // Calculate error for unsigned values.
   this_error_us = err_calc(us_real_num, us_real_den, us_real_quo, allowed_error_fixed, threshold);
   // Whichever error is greater is assigned to this_error_fixed
   this_error_fixed = this_error_s > this_error_us ? this_error_s : this_error_us;
   // Calculate error for complex inputs and outputs
   this_error_complex = cmplx_err_calc(complex_num, complex_den, complex_quo, allowed_error_fixed, threshold);
   if(this_error_fixed > max_error_fixed)
   {
      max_error_fixed = this_error_fixed;
   }
   if(this_error_complex > max_error_cmplx_fixed)
   {
      max_error_cmplx_fixed = this_error_complex;
   }

   // Second corner: signed real numerator = MAX, signed real denominator = MIN, complex numerator = (MAX, MAX), complex
   // denominator = (MIN, MIN)
   s_real_den.template set_val<AC_VAL_MIN>();
   complex_den.r().template set_val<AC_VAL_MIN>();
   complex_den.i().template set_val<AC_VAL_MIN>();
   test_ac_div_fixed(us_real_num, us_real_den, us_real_quo, s_real_num, s_real_den, s_real_quo, complex_num,
                     complex_den, complex_quo);
   // Calculate error for signed values, assign it to this_error_fixed
   this_error_fixed = err_calc(s_real_num, s_real_den, s_real_quo, allowed_error_fixed, threshold);
   // Calculate error for complex inputs and outputs
   this_error_complex = cmplx_err_calc(complex_num, complex_den, complex_quo, allowed_error_fixed, threshold);
   if(this_error_fixed > max_error_fixed)
   {
      max_error_fixed = this_error_fixed;
   }
   if(this_error_complex > max_error_cmplx_fixed)
   {
      max_error_cmplx_fixed = this_error_complex;
   }

   // Third corner: signed real numerator = MIN, signed real denominator = MAX, complex numerator = (MIN, MIN), complex
   // denominator = (MAX, MAX)
   s_real_num.template set_val<AC_VAL_MIN>();
   s_real_den.template set_val<AC_VAL_MAX>();
   complex_num.r().template set_val<AC_VAL_MIN>();
   complex_num.i().template set_val<AC_VAL_MIN>();
   complex_den.r().template set_val<AC_VAL_MAX>();
   complex_den.i().template set_val<AC_VAL_MAX>();
   test_ac_div_fixed(us_real_num, us_real_den, us_real_quo, s_real_num, s_real_den, s_real_quo, complex_num,
                     complex_den, complex_quo);
   // Calculate error for signed values, assign it to this_error_fixed
   this_error_fixed = err_calc(s_real_num, s_real_den, s_real_quo, allowed_error_fixed, threshold);
   // Calculate error for complex inputs and outputs
   this_error_complex = cmplx_err_calc(complex_num, complex_den, complex_quo, allowed_error_fixed, threshold);
   if(this_error_fixed > max_error_fixed)
   {
      max_error_fixed = this_error_fixed;
   }
   if(this_error_complex > max_error_cmplx_fixed)
   {
      max_error_cmplx_fixed = this_error_complex;
   }

   // Fourth corner: signed real numerator = MIN, signed real denominator = MIN, complex numerator = (MIN, MIN), complex
   // denominator = (MIN, MIN)
   s_real_den.template set_val<AC_VAL_MIN>();
   complex_den.r().template set_val<AC_VAL_MIN>();
   complex_den.i().template set_val<AC_VAL_MIN>();
   test_ac_div_fixed(us_real_num, us_real_den, us_real_quo, s_real_num, s_real_den, s_real_quo, complex_num,
                     complex_den, complex_quo);
   // Calculate error for signed values, assign it to this_error_fixed
   this_error_fixed = err_calc(s_real_num, s_real_den, s_real_quo, allowed_error_fixed, threshold);
   // Calculate error for complex inputs and outputs
   this_error_complex = cmplx_err_calc(complex_num, complex_den, complex_quo, allowed_error_fixed, threshold);
   if(this_error_fixed > max_error_fixed)
   {
      max_error_fixed = this_error_fixed;
   }
   if(this_error_complex > max_error_cmplx_fixed)
   {
      max_error_cmplx_fixed = this_error_complex;
   }

   // Fifth corner: real numerator = MAX, real denominator = QUANTUM, complex numerator = (MAX, MAX), complex
   // denominator = (QUANTUM, QUANTUM)
   us_real_num.template set_val<AC_VAL_MAX>();
   us_real_den.template set_val<AC_VAL_QUANTUM>();
   s_real_num.template set_val<AC_VAL_MAX>();
   s_real_den.template set_val<AC_VAL_QUANTUM>();
   complex_num.r().template set_val<AC_VAL_MAX>();
   complex_num.i().template set_val<AC_VAL_MAX>();
   complex_den.r().template set_val<AC_VAL_QUANTUM>();
   complex_den.i().template set_val<AC_VAL_QUANTUM>();
   test_ac_div_fixed(us_real_num, us_real_den, us_real_quo, s_real_num, s_real_den, s_real_quo, complex_num,
                     complex_den, complex_quo);
   // Calculate error for signed values.
   this_error_s = err_calc(s_real_num, s_real_den, s_real_quo, allowed_error_fixed, threshold);
   // Calculate error for unsigned values.
   this_error_us = err_calc(us_real_num, us_real_den, us_real_quo, allowed_error_fixed, threshold);
   // Whichever error is greater is assigned to this_error_fixed
   this_error_fixed = this_error_s > this_error_us ? this_error_s : this_error_us;
   // Calculate error for complex inputs and outputs
   this_error_complex = cmplx_err_calc(complex_num, complex_den, complex_quo, allowed_error_fixed, threshold);
   if(this_error_fixed > max_error_fixed)
   {
      max_error_fixed = this_error_fixed;
   }
   if(this_error_complex > max_error_cmplx_fixed)
   {
      max_error_cmplx_fixed = this_error_complex;
   }

   // Sixth corner: real numerator = QUANTUM, real denominator = MAX, complex numerator = (QUANTUM, QUANTUM), complex
   // denominator = (MAX, MAX)
   us_real_num.template set_val<AC_VAL_QUANTUM>();
   us_real_den.template set_val<AC_VAL_MAX>();
   s_real_num.template set_val<AC_VAL_QUANTUM>();
   s_real_den.template set_val<AC_VAL_MAX>();
   complex_num.r().template set_val<AC_VAL_QUANTUM>();
   complex_num.i().template set_val<AC_VAL_QUANTUM>();
   complex_den.r().template set_val<AC_VAL_MAX>();
   complex_den.i().template set_val<AC_VAL_MAX>();
   test_ac_div_fixed(us_real_num, us_real_den, us_real_quo, s_real_num, s_real_den, s_real_quo, complex_num,
                     complex_den, complex_quo);
   // Calculate error for signed values.
   this_error_s = err_calc(s_real_num, s_real_den, s_real_quo, allowed_error_fixed, threshold);
   // Calculate error for unsigned values.
   this_error_us = err_calc(us_real_num, us_real_den, us_real_quo, allowed_error_fixed, threshold);
   // Whichever error is greater is assigned to this_error_fixed
   this_error_fixed = this_error_s > this_error_us ? this_error_s : this_error_us;
   // Calculate error for complex inputs and outputs
   this_error_complex = cmplx_err_calc(complex_num, complex_den, complex_quo, allowed_error_fixed, threshold);
   if(this_error_fixed > max_error_fixed)
   {
      max_error_fixed = this_error_fixed;
   }
   if(this_error_complex > max_error_cmplx_fixed)
   {
      max_error_cmplx_fixed = this_error_complex;
   }

   // Seventh corner: signed real numerator = MIN, signed real denominator = QUANTUM, complex numerator = (MIN, MIN),
   // complex denominator = (QUANTUM, QUANTUM)
   s_real_num.template set_val<AC_VAL_MIN>();
   s_real_den.template set_val<AC_VAL_QUANTUM>();
   complex_num.r().template set_val<AC_VAL_MIN>();
   complex_num.i().template set_val<AC_VAL_MIN>();
   complex_den.r().template set_val<AC_VAL_QUANTUM>();
   complex_den.i().template set_val<AC_VAL_QUANTUM>();
   test_ac_div_fixed(us_real_num, us_real_den, us_real_quo, s_real_num, s_real_den, s_real_quo, complex_num,
                     complex_den, complex_quo);
   // Calculate error for signed values, assign it to this_error_fixed
   this_error_fixed = err_calc(s_real_num, s_real_den, s_real_quo, allowed_error_fixed, threshold);
   // Calculate error for complex inputs and outputs
   this_error_complex = cmplx_err_calc(complex_num, complex_den, complex_quo, allowed_error_fixed, threshold);
   if(this_error_fixed > max_error_fixed)
   {
      max_error_fixed = this_error_fixed;
   }
   if(this_error_complex > max_error_cmplx_fixed)
   {
      max_error_cmplx_fixed = this_error_complex;
   }

   // Eighth corner: signed real numerator = QUANTUM, signed real denominator = MIN, complex numerator = (QUANTUM,
   // QUANTUM), complex denominator = (MIN, MIN)
   s_real_num.template set_val<AC_VAL_QUANTUM>();
   s_real_den.template set_val<AC_VAL_MIN>();
   complex_num.r().template set_val<AC_VAL_QUANTUM>();
   complex_num.i().template set_val<AC_VAL_QUANTUM>();
   complex_den.r().template set_val<AC_VAL_MIN>();
   complex_den.i().template set_val<AC_VAL_MIN>();
   test_ac_div_fixed(us_real_num, us_real_den, us_real_quo, s_real_num, s_real_den, s_real_quo, complex_num,
                     complex_den, complex_quo);
   // Calculate error for signed values, assign it to this_error_fixed
   this_error_fixed = err_calc(s_real_num, s_real_den, s_real_quo, allowed_error_fixed, threshold);
   // Calculate error for complex inputs and outputs
   this_error_complex = cmplx_err_calc(complex_num, complex_den, complex_quo, allowed_error_fixed, threshold);
   if(this_error_fixed > max_error_fixed)
   {
      max_error_fixed = this_error_fixed;
   }
   if(this_error_complex > max_error_cmplx_fixed)
   {
      max_error_cmplx_fixed = this_error_complex;
   }

   // Ninth corner: real numerator = QUANTUM, real denominator = QUANTUM, complex numerator = (QUANTUM, QUANTUM),
   // complex denominator = (QUANTUM, QUANTUM)
   us_real_num.template set_val<AC_VAL_QUANTUM>();
   us_real_den.template set_val<AC_VAL_QUANTUM>();
   s_real_den.template set_val<AC_VAL_QUANTUM>();
   complex_den.r().template set_val<AC_VAL_QUANTUM>();
   complex_den.i().template set_val<AC_VAL_QUANTUM>();
   test_ac_div_fixed(us_real_num, us_real_den, us_real_quo, s_real_num, s_real_den, s_real_quo, complex_num,
                     complex_den, complex_quo);
   // Calculate error for signed values.
   this_error_s = err_calc(s_real_num, s_real_den, s_real_quo, allowed_error_fixed, threshold);
   // Calculate error for unsigned values.
   this_error_us = err_calc(us_real_num, us_real_den, us_real_quo, allowed_error_fixed, threshold);
   // Whichever error is greater is assigned to this_error_fixed
   this_error_fixed = this_error_s > this_error_us ? this_error_s : this_error_us;
   // Calculate error for complex inputs and outputs
   this_error_complex = cmplx_err_calc(complex_num, complex_den, complex_quo, allowed_error_fixed, threshold);
   if(this_error_fixed > max_error_fixed)
   {
      max_error_fixed = this_error_fixed;
   }
   if(this_error_complex > max_error_cmplx_fixed)
   {
      max_error_cmplx_fixed = this_error_complex;
   }

   passed = (max_error_fixed < allowed_error_fixed) && (max_error_cmplx_fixed < allowed_error_fixed);

   if(passed)
   {
      printf("PASSED , max err (%f) (%f complex)\n", max_error_fixed, max_error_cmplx_fixed);
   }
   else
   {
      printf("FAILED , max err (%f) (%f complex)\n", max_error_fixed, max_error_cmplx_fixed);
   }

   if(max_error_fixed > cumulative_max_error_fixed)
   {
      cumulative_max_error_fixed = max_error_fixed;
   }
   if(max_error_cmplx_fixed > cumulative_max_error_cmplx_fixed)
   {
      cumulative_max_error_cmplx_fixed = max_error_cmplx_fixed;
   }

   return 0;
}

template <int numW, int numI, int numE, int denW, int denI, int denE, int quoW, int quoI, int quoE>
int test_driver_float(double& cumulative_max_error_float, const double allowed_error_float, const double threshold,
                      bool details = false)
{
   bool passed;
   double max_error_float = 0.0; // reset for this run

   ac_float<numW, numI, numE, AC_TRN> real_num;
   ac_float<denW, denI, denE, AC_TRN> real_den;
   ac_float<quoW, quoI, quoE, AC_TRN> real_quo;

   cout << "TEST: ac_div() INPUTS: ";
   cout.width(38);
   cout << left << real_num.type_name();
   cout.width(38);
   cout << left << real_den.type_name();
   cout << "OUTPUT: ";
   cout.width(38);
   cout << left << real_quo.type_name();
   cout << "RESULT: ";

   ac_fixed<numW - 1, numI - 1, false, AC_TRN, AC_WRAP> us_real_num_m_start = 0, us_real_num_m_arr[numW - 1];
   ac_fixed<denW - 1, denI - 1, false, AC_TRN, AC_WRAP> us_real_den_m_start = 0, us_real_den_m_arr[denW - 1];

   int bit_value = 1;

   // Assign a value for that has alternating 1s and 0s, with the
   // same precision as the unsigned counterpart of the numerator's mantissa.
   for(int i = numW - 2; i >= 0; i--)
   {
      us_real_num_m_start[i] = bit_value;
      bit_value = ~bit_value;
   }

   bit_value = 1;

   // Do the same as above, but for the denominator.
   for(int i = denW - 2; i >= 0; i--)
   {
      us_real_den_m_start[i] = bit_value;
      bit_value = ~bit_value;
   }

#ifdef DEBUG
   cout << endl;
#endif

   // Assign an array of reference values, and every time,
   // shift the initial value ("_start" value) right by 1 bit, for
   // unsigned counterpart of the numerator's and the denominator's mantissa.
   for(int i = 0; i <= numW - 2; i++)
   {
      us_real_num_m_arr[i] = us_real_num_m_start >> i;
#ifdef DEBUG
      cout << "us_real_num_m_arr[" << i << "] = " << us_real_num_m_arr[i] << endl;
#endif
   }

   for(int i = 0; i <= denW - 2; i++)
   {
      us_real_den_m_arr[i] = us_real_den_m_start >> i;
#ifdef DEBUG
      cout << "us_real_den_m_arr[" << i << "] = " << us_real_den_m_arr[i] << endl;
#endif
   }

   // Declare arrays to store all values of exponent to be tested for numerator.
   const int exp_arr_size_num = 2 * (numE - 1) + 3;
   ac_int<numE, true> sample_exponent_num;
   ac_int<numE, true> sample_exponent_array_num[exp_arr_size_num];

   // The first element of the array is the minimum exponent value, the middle element is a zero exponent, and
   // the last element is the maximum possible value.
   sample_exponent_array_num[0].template set_val<AC_VAL_MIN>();
   sample_exponent_array_num[numE] = 0;
   sample_exponent_array_num[exp_arr_size_num - 1].template set_val<AC_VAL_MAX>();

   // All the other elements are set to values that correspond to a one-hot encoding scheme, in which only one
   // bit of the absolute value of the exponent is set to one. Both negative and positive values are encoded this way.
   for(int i = (numE - 2); i >= 0; i--)
   {
      sample_exponent_num = 0;
      sample_exponent_num[i] = 1;
      sample_exponent_array_num[numE + i + 1] = sample_exponent_num;
      sample_exponent_array_num[numE - i - 1] = -sample_exponent_num;
   }

   // Do the same as above, but for denominator.
   const int exp_arr_size_den = 2 * (denE - 1) + 3;
   ac_int<denE, true> sample_exponent_den;
   ac_int<denE, true> sample_exponent_array_den[exp_arr_size_den];

   sample_exponent_array_den[0].template set_val<AC_VAL_MIN>();
   sample_exponent_array_den[denE] = 0;
   sample_exponent_array_den[exp_arr_size_den - 1].template set_val<AC_VAL_MAX>();

   for(int i = (denE - 2); i >= 0; i--)
   {
      sample_exponent_den = 0;
      sample_exponent_den[i] = 1;
      sample_exponent_array_den[denE + i + 1] = sample_exponent_den;
      sample_exponent_array_den[denE - i - 1] = -sample_exponent_den;
   }

   double this_error_float;

   for(int i = 0; i < exp_arr_size_num; i++)
   {
      real_num.e = sample_exponent_array_num[i];
      for(int j = 0; j < exp_arr_size_den; j++)
      {
         real_den.e = sample_exponent_array_den[j];
         // Iterate through all possible combinations of mantissa values stored in the arrays above.
         // The denominator will be the negative of the corresponding unsigned value for that iteration.
         // The numerator will have the same value as the corresponding unsigned value.
         for(int i = 0; i <= numW - 2; i++)
         {
            real_num.m = us_real_num_m_arr[i];
            for(int j = 0; j <= denW - 2; j++)
            {
               real_den.m = -us_real_den_m_arr[j];
               if(real_den.m == 0)
               {
                  continue;
               }
               test_ac_div_float(real_num, real_den, real_quo);
               // Calculate error for real values.
               this_error_float = err_calc(real_num, real_den, real_quo, allowed_error_float, threshold);
               if(this_error_float > max_error_float)
               {
                  max_error_float = this_error_float;
               }
            }
         }
      }
   }

   // Also, test for corner cases

   // First corner: real numerator = MAX, real denominator = MAX
   real_num.template set_val<AC_VAL_MAX>();
   real_den.template set_val<AC_VAL_MAX>();
   test_ac_div_float(real_num, real_den, real_quo);
   // Calculate error for real values.
   this_error_float = err_calc(real_num, real_den, real_quo, allowed_error_float, threshold);
   if(this_error_float > max_error_float)
   {
      max_error_float = this_error_float;
   }

   // Second corner: real numerator = MAX, real denominator = MIN
   real_den.template set_val<AC_VAL_MIN>();
   test_ac_div_float(real_num, real_den, real_quo);
   // Calculate error for real values.
   this_error_float = err_calc(real_num, real_den, real_quo, allowed_error_float, threshold);
   if(this_error_float > max_error_float)
   {
      max_error_float = this_error_float;
   }

   // Third corner: real numerator = MIN, real denominator = MAX
   real_num.template set_val<AC_VAL_MIN>();
   real_den.template set_val<AC_VAL_MAX>();
   test_ac_div_float(real_num, real_den, real_quo);
   // Calculate error for real values.
   this_error_float = err_calc(real_num, real_den, real_quo, allowed_error_float, threshold);
   if(this_error_float > max_error_float)
   {
      max_error_float = this_error_float;
   }

   // Fourth corner: real numerator = MIN, real denominator = MIN
   real_den.template set_val<AC_VAL_MIN>();
   test_ac_div_float(real_num, real_den, real_quo);
   // Calculate error for real values.
   this_error_float = err_calc(real_num, real_den, real_quo, allowed_error_float, threshold);
   if(this_error_float > max_error_float)
   {
      max_error_float = this_error_float;
   }

   // Fifth corner: real numerator = MAX, real denominator = QUANTUM
   real_num.template set_val<AC_VAL_MAX>();
   real_den.template set_val<AC_VAL_QUANTUM>();
   test_ac_div_float(real_num, real_den, real_quo);
   // Calculate error for real values.
   this_error_float = err_calc(real_num, real_den, real_quo, allowed_error_float, threshold);
   if(this_error_float > max_error_float)
   {
      max_error_float = this_error_float;
   }

   // Sixth corner: real numerator = QUANTUM, real denominator = MAX
   real_num.template set_val<AC_VAL_QUANTUM>();
   real_den.template set_val<AC_VAL_MAX>();
   test_ac_div_float(real_num, real_den, real_quo);
   // Calculate error for real values.
   this_error_float = err_calc(real_num, real_den, real_quo, allowed_error_float, threshold);
   if(this_error_float > max_error_float)
   {
      max_error_float = this_error_float;
   }

   // Seventh corner: real numerator = MIN, real denominator = QUANTUM
   real_num.template set_val<AC_VAL_MIN>();
   real_den.template set_val<AC_VAL_QUANTUM>();
   test_ac_div_float(real_num, real_den, real_quo);
   // Calculate error for real values.
   this_error_float = err_calc(real_num, real_den, real_quo, allowed_error_float, threshold);
   if(this_error_float > max_error_float)
   {
      max_error_float = this_error_float;
   }

   // Eighth corner: real numerator = QUANTUM, real denominator = MIN
   real_num.template set_val<AC_VAL_QUANTUM>();
   real_den.template set_val<AC_VAL_MIN>();
   test_ac_div_float(real_num, real_den, real_quo);
   // Calculate error for real values.
   this_error_float = err_calc(real_num, real_den, real_quo, allowed_error_float, threshold);
   if(this_error_float > max_error_float)
   {
      max_error_float = this_error_float;
   }

   // Ninth corner: real numerator = QUANTUM, real denominator = QUANTUM
   real_den.template set_val<AC_VAL_QUANTUM>();
   test_ac_div_float(real_num, real_den, real_quo);
   // Calculate error for real values.
   this_error_float = err_calc(real_num, real_den, real_quo, allowed_error_float, threshold);
   if(this_error_float > max_error_float)
   {
      max_error_float = this_error_float;
   }

   passed = (max_error_float < allowed_error_float);

   if(passed)
   {
      printf("PASSED , max err (%f)\n", max_error_float);
   }
   else
   {
      printf("FAILED , max err (%f)\n", max_error_float);
   }

   if(max_error_float > cumulative_max_error_float)
   {
      cumulative_max_error_float = max_error_float;
   }

   return 0;
}

int main(int argc, char* argv[])
{
   double max_error_fixed = 0, max_error_cmplx_fixed = 0;
   double max_error_float = 0;

   // Set tolerance
   double allowed_error_fixed = 0.5;

   // threshold below which we calculate absolute error instead of relative for fixed point
   double threshold_fixed = 0;

   // Set tolerance for float
   double allowed_error_float = 0.5;

   // threshold below which we calculate absolute error instead of relative for floating point
   double threshold_float = 0;

   cout << "=============================================================================" << endl;
   cout << "Testing function: ac_div(), Scalar and complex values. - Allowed error " << allowed_error_fixed
        << " (fixed pt)" << endl;

   bool all_tests_pass = true;

   // If any of the tests fail for ac_int, the all_tests_pass variable will be set to false

   // template <int numW, int denW, int quoW, int remW>
   test_driver_int<16, 16, 16, 64>(all_tests_pass);

   // template <int numW, int numI, int denW, int denI, int quoW, int quoI>
   test_driver_fixed<16, 8, 16, 8, 64, 32>(max_error_fixed, max_error_cmplx_fixed, allowed_error_fixed,
                                           threshold_fixed);
   test_driver_fixed<16, -8, 16, -8, 64, 32>(max_error_fixed, max_error_cmplx_fixed, allowed_error_fixed,
                                             threshold_fixed);
   test_driver_fixed<8, 16, 8, 16, 64, 32>(max_error_fixed, max_error_cmplx_fixed, allowed_error_fixed,
                                           threshold_fixed);

   // template <int numW, int numI, int numE, int denW, int denI, int denE, int quoW, int quoI, int quoE>
   test_driver_float<10, 5, 4, 10, 5, 4, 64, 32, 10>(max_error_float, allowed_error_float, threshold_float);
   test_driver_float<10, 4, 4, 8, 5, 5, 64, 32, 10>(max_error_float, allowed_error_float, threshold_float);
   test_driver_float<10, -4, 4, 8, -5, 5, 64, 32, 10>(max_error_float, allowed_error_float, threshold_float);
   test_driver_float<4, 10, 4, 5, 8, 5, 64, 32, 10>(max_error_float, allowed_error_float, threshold_float);

   cout << "=============================================================================" << endl;
   cout << "  Testbench finished. Maximum error observed across all bit-width variations:" << endl;
   cout << "    max_error_fixed       = " << max_error_fixed << endl;
   cout << "    max_error_cmplx_fixed = " << max_error_cmplx_fixed << endl;
   cout << "    max_error_float       = " << max_error_float << endl;

   // If error limits on ac_fixed/ac_float datatypes have been crossed, or the output for ac_int datatypes is not
   // correct, the test has failed.
   bool test_fail = (max_error_fixed > allowed_error_fixed) || (max_error_cmplx_fixed > allowed_error_fixed) ||
                    (max_error_float > allowed_error_float) || (!all_tests_pass);

   // Notify the user that the test was a failure.
   if(test_fail)
   {
      cout << "  ac_div - FAILED - Error tolerance(s) exceeded" << endl;
      cout << "=============================================================================" << endl;
      return -1;
   }
   else
   {
      cout << "  ac_div - PASSED" << endl;
      cout << "=============================================================================" << endl;
   }

   return 0;
}
