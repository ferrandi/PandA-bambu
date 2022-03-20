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
// ac_reciprocal_pwl() function using a variety of data types and bit-
// widths.

// To compile standalone and run:
//   $MGC_HOME/bin/c++ -std=c++11 -I$MGC_HOME/shared/include rtest_ac_reciprocal_pwl.cpp -o design
//   ./design

// Include the AC Math function that is exercised with this testbench
#include <ac_math/ac_reciprocal_pwl.h>
using namespace ac_math;

// ==============================================================================
// Test Designs
//   These simple functions allow executing the ac_reciprocal_pwl() function
//   using multiple data types at the same time. Template parameters are
//   used to configure the bit-widths of the types.

// Test Design for real and complex fixed point values.
template <int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi, bool outSfi>
void test_ac_reciprocal_pwl_fixed(const ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>& in1,
                                  ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>& out1,
                                  const ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>>& in2,
                                  ac_complex<ac_fixed<outWfi, outIfi, true, AC_TRN, AC_WRAP>>& out2)
{
   out1 = ac_reciprocal_pwl<ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP>>(in1);
   out2 = ac_reciprocal_pwl<ac_complex<ac_fixed<outWfi, outIfi, true, AC_TRN, AC_WRAP>>>(in2);
}

// Test Design for real and complex floating point values.
template <int Wfl, int Ifl, int Efl, int outWfl, int outIfl, int outEfl>
void test_ac_reciprocal_pwl_float(const ac_float<Wfl, Ifl, Efl, AC_TRN>& in3,
                                  ac_float<outWfl, outIfl, outEfl, AC_TRN>& out3,
                                  const ac_complex<ac_float<Wfl, Ifl, Efl, AC_TRN>>& in4,
                                  ac_complex<ac_float<outWfl, outIfl, outEfl, AC_TRN>>& out4)
{
   out3 = ac_reciprocal_pwl<ac_float<outWfl, outIfl, outEfl, AC_TRN>>(in3);
   out4 = ac_reciprocal_pwl<ac_complex<ac_float<outWfl, outIfl, outEfl, AC_TRN>>>(in4);
}

// ==============================================================================

#include <fstream>
#include <iostream>
#include <math.h>
#include <string>
using namespace std;

// ------------------------------------------------------------------------------
// Helper functions for error calculation and monotonicity checks.

// Calculating error for real datatype.
template <class T_in, class T_out>
double err_calc(const T_in input, double& actual_value, const T_out output, const double allowed_error,
                const double threshold)
{
   double expected_value;

   if(input.to_double() != 0)
   {
      // The typecasting is done in order to provide quantization on the expected output.
      expected_value = ((T_out)(1.0 / input.to_double())).to_double();
   }
   else
   {
      // If input is zero, saturate the expected output according to the type of the real/imaginary part.
      T_out output_max;
      expected_value = output_max.template set_val<AC_VAL_MAX>().to_double();
   }

   actual_value = output.to_double();
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

#ifdef DEBUG
   if(this_error > allowed_error)
   {
      cout << endl;
      cout << "  Error tolerance exceeded for following values: " << endl;
      cout << "  input          = " << input << endl;
      cout << "  output         = " << output << endl;
      cout << "  expected_value = " << expected_value << endl;
      cout << "  this_error     = " << this_error << endl;
      assert(false);
   }
#endif

   return this_error;
}

// Calculating error for complex datatype.
template <class T_in, class T_out>
double cmplx_err_calc(const ac_complex<T_in> input, const ac_complex<T_out> output, const double allowed_error,
                      const double threshold)
{
   double in_r = input.r().to_double();
   double in_i = input.i().to_double();

   // Declare variables to store the expected output value, store the difference between
   // expected and actual output values and store the actual output value (converted to
   // double)
   ac_complex<double> exp_op, diff_op, act_op;

   // Convert actual output to double and store it in a separate complex variable.
   act_op.r() = output.r().to_double();
   act_op.i() = output.i().to_double();

   // Calculate expected value of real and imaginary parts.
   exp_op.r() = in_r / (in_r * in_r + in_i * in_i);
   exp_op.i() = -in_i / (in_r * in_r + in_i * in_i);

   if(input.r() != 0 || input.i() != 0)
   {
      // The typecasting is done in order to provide quantization on the expected output.
      exp_op.r() = ((T_out)exp_op.r()).to_double();
      exp_op.i() = ((T_out)exp_op.i()).to_double();
   }
   else
   {
      // If input is zero, saturate the expected output according to the type of the real/imaginary part.
      T_out output_max;
      exp_op.r() = output_max.template set_val<AC_VAL_MAX>().to_double();
      exp_op.i() = output_max.template set_val<AC_VAL_MAX>().to_double();
   }

   diff_op = exp_op - act_op;
   double this_error;

   // If magnitude of expected value is greater than a particular threshold, calculate relative error, else, calculate
   // absolute error.
   if(sqrt(exp_op.mag_sqr()) > threshold)
   {
      this_error = sqrt((diff_op / exp_op).mag_sqr()) * 100;
   }
   else
   {
      this_error = sqrt(diff_op.mag_sqr()) * 100;
   }

#ifdef DEBUG
   if(this_error > allowed_error)
   {
      cout << endl;
      cout << "  Error tolerance exceeded for following values: " << endl;
      cout << "  input          = " << input << endl;
      cout << "  output         = " << output << endl;
      cout << "  exp_op         = " << exp_op << endl;
      cout << "  this_error     = " << this_error << endl;
      assert(false);
   }
#endif

   return this_error;
}

// Function for monotonicity checking in ac_fixed inputs.
template <int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi, bool outSfi>
void monotonicity_check(double& old_real_output, const double actual_value_fixed, bool& compare,
                        ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP> input_fixed,
                        ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> output_fixed)
{
   // MONOTONIC: Make sure that function is monotonic. Compare old value (value of previous iteration) with current
   // value. Since the reciprocal function we are testing is a decreasing function, and our testbench value keeps
   // incrementing or remains the same (in case of saturation), we expect the old value to be greater than or equal to
   // the current one. Also, since the reciprocal function has a large discontinuity at x = 0; we make sure we don't
   // compare values when we cross this point. We do this by checking the signage of the old output vs that of the new
   // output. Since we aren't checking for zero inputs, crossing x = 0 will mean that the old output is negative and the
   // new one is positive, in case of an increasing testbench.
   bool sign_same = (old_real_output > 0 && actual_value_fixed > 0) || (old_real_output < 0 && actual_value_fixed < 0);
   if(compare && sign_same)
   {
      // Figuring out what the normalized value was for the input is a good way to figure out where the discontinuity
      // occured w.r.t. the PWL segments.
      ac_fixed<Wfi, int(Sfi), Sfi, AC_TRN, AC_WRAP> norm_input_fixed;
      ac_normalize(input_fixed, norm_input_fixed);
      if(old_real_output < actual_value_fixed)
      {
         cout << endl;                                                    // LCOV_EXCL_LINE
         cout << "  Real, fixed point output not monotonic at :" << endl; // LCOV_EXCL_LINE
         cout << "  input_fixed = " << input_fixed << endl;               // LCOV_EXCL_LINE
         cout << "  output_fixed = " << output_fixed << endl;             // LCOV_EXCL_LINE
         cout << "  old_real_output = " << old_real_output << endl;       // LCOV_EXCL_LINE
         cout << "  normalized x    = " << norm_input_fixed << endl;      // LCOV_EXCL_LINE
         assert(false);                                                   // LCOV_EXCL_LINE
      }
   }
   // Update the variable for old_real_output.
   old_real_output = actual_value_fixed;
   // By setting compare to true, we make sure that once there is an old value stored, we can start comparing for
   // monotonicity.
   compare = true;
}

// ==============================================================================
// Functions: test_driver functions
// Description: Templatized functions that can be configured for certain bit-
//   widths of AC datatypes. They use the type information to iterate through a
//   range of valid values on that type in order to compare the precision of the
//   piece-wise linear reciprocal model with the computed reciprocal using a
//   standard C double type. The maximum error for each type is accumulated
//   in variables defined in the calling function.

// ==============================================================================
// Function: test_driver_fixed()
// Description: test_driver function for ac_fixed and ac_complex<ac_fixed> inputs
//   and outputs.

template <int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi, bool outSfi>
int test_driver_fixed(double& cumulative_max_error_fixed, double& cumulative_max_error_cmplx_fixed,
                      const double allowed_error_fixed, const double threshold, bool details = false)
{
   bool passed = true;
   bool check_monotonic = true;
   double max_error_fixed = 0.0;       // reset for this run
   double max_error_cmplx_fixed = 0.0; // reset for this run

   ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP> input_fixed;
   ac_fixed<outWfi, outIfi, outSfi, AC_TRN, AC_WRAP> output_fixed;
   ac_complex<ac_fixed<Wfi, Ifi, Sfi, AC_TRN, AC_WRAP>> cmplx_input_fixed;
   ac_complex<ac_fixed<outWfi, outIfi, true, AC_TRN, AC_WRAP>> cmplx_output_fixed;

   double lower_limit_fixed, upper_limit_fixed, step_fixed;

   // set ranges and step size for fixed point testbench
   lower_limit_fixed = input_fixed.template set_val<AC_VAL_MIN>().to_double();
   upper_limit_fixed = input_fixed.template set_val<AC_VAL_MAX>().to_double();
   step_fixed = input_fixed.template set_val<AC_VAL_QUANTUM>().to_double();

   cout << "TEST: ac_reciprocal_pwl() INPUT: ";
   cout.width(38);
   cout << left << input_fixed.type_name();
   cout << "        OUTPUTS: ";
   cout.width(38);
   cout << left << output_fixed.type_name();
   cout.width(50);
   cout << left << cmplx_output_fixed.type_name();
   cout << "RESULT: ";

   // Dump the test details
   if(details)
   {
      cout << "  Ranges for input types:" << endl;                        // LCOV_EXCL_LINE
      cout << "    lower_limit_fixed    = " << lower_limit_fixed << endl; // LCOV_EXCL_LINE
      cout << "    upper_limit_fixed    = " << upper_limit_fixed << endl; // LCOV_EXCL_LINE
      cout << "    step_fixed           = " << step_fixed << endl;        // LCOV_EXCL_LINE
   }

   double old_real_output;
   double actual_value_fixed;

   // test fixed-point real and complex.
   for(double i = lower_limit_fixed; i <= upper_limit_fixed; i += step_fixed)
   {
      bool compare = false;
      for(double j = lower_limit_fixed; j <= upper_limit_fixed; j += step_fixed)
      {
         cmplx_input_fixed.r() = i;
         cmplx_input_fixed.i() = j;
         input_fixed = j;
         test_ac_reciprocal_pwl_fixed(input_fixed, output_fixed, cmplx_input_fixed, cmplx_output_fixed);

         double this_error_fixed =
             err_calc(input_fixed, actual_value_fixed, output_fixed, allowed_error_fixed, threshold);
         double this_error_complex =
             cmplx_err_calc(cmplx_input_fixed, cmplx_output_fixed, allowed_error_fixed, threshold);

         if(check_monotonic)
         {
            monotonicity_check(old_real_output, actual_value_fixed, compare, input_fixed, output_fixed);
         }
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

   passed = (max_error_fixed < allowed_error_fixed) && (max_error_cmplx_fixed < allowed_error_fixed);

   if(passed)
   {
      printf("PASSED , max err (%f) (%f complex)\n", max_error_fixed, max_error_cmplx_fixed);
   }
   else
   {
      printf("FAILED , max err (%f) (%f complex)\n", max_error_fixed, max_error_cmplx_fixed);
   } // LCOV_EXCL_LINE

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

// ==============================================================================
// Function: test_driver_float()
// Description: test_driver function for ac_float and ac_complex<ac_float> inputs
//   and outputs.

template <int Wfl, int Ifl, int Efl, int outWfl, int outIfl, int outEfl>
int test_driver_float(double& cumulative_max_error_float, double& cumulative_max_error_cmplx_float,
                      const double allowed_error_float, const double threshold, bool details = false)
{
   bool passed = true;
   double max_error_float = 0.0;       // reset for this run
   double max_error_cmplx_float = 0.0; // reset for this run

   ac_float<Wfl, Ifl, Efl, AC_TRN> input_float;
   ac_float<outWfl, outIfl, outEfl, AC_TRN> output_float;
   ac_complex<ac_float<Wfl, Ifl, Efl, AC_TRN>> cmplx_input_float;
   ac_complex<ac_float<outWfl, outIfl, outEfl, AC_TRN>> cmplx_output_float;

   double lower_limit_mantissa, upper_limit_mantissa, step_mantissa;
   double actual_value_float;

   // Declare an ac_fixed variable of same type as mantissa
   ac_fixed<Wfl, Ifl, true> sample_mantissa;

   lower_limit_mantissa = sample_mantissa.template set_val<AC_VAL_MIN>().to_double();
   upper_limit_mantissa = sample_mantissa.template set_val<AC_VAL_MAX>().to_double();
   step_mantissa = sample_mantissa.template set_val<AC_VAL_QUANTUM>().to_double();

   string empty_str = "";

   cout << "TEST: ac_reciprocal_pwl() AC_FLOAT INPUT: ";
   cout.width(38);
   cout << left << input_float.type_name();
   cout << "AC_FLOAT OUTPUT: ";
   cout.width(38);
   cout << left << output_float.type_name();
   cout << "RESULT: ";

   // Dump the test details
   if(details)
   {
      cout << endl << "  Ranges for input types:" << endl;                   // LCOV_EXCL_LINE
      cout << "    lower_limit_mantissa = " << lower_limit_mantissa << endl; // LCOV_EXCL_LINE
      cout << "    upper_limit_mantissa = " << upper_limit_mantissa << endl; // LCOV_EXCL_LINE
      cout << "    step_mantissa        = " << step_mantissa << endl;        // LCOV_EXCL_LINE
   }

   // Declare arrays to store all values of exponent to be tested.
   const int exp_arr_size = 2 * (Efl - 1) + 3;
   ac_int<Efl, true> sample_exponent;
   ac_int<Efl, true> sample_exponent_array[exp_arr_size];

   // The first element of the array is the minimum exponent value, the middle element is a zero exponent, and
   // the last element is the maximum possible value.
   sample_exponent_array[0].template set_val<AC_VAL_MIN>();
   sample_exponent_array[Efl] = 0;
   sample_exponent_array[exp_arr_size - 1].template set_val<AC_VAL_MAX>();

   // All the other elements are set to values that correspond to a one-hot encoding scheme, in which only one
   // bit of the absolute value of the exponent is set to one. Both negative and positive values are encoded this way.
   for(int i = (Efl - 2); i >= 0; i--)
   {
      sample_exponent = 0;
      sample_exponent[i] = 1;
      sample_exponent_array[Efl + i + 1] = sample_exponent;
      sample_exponent_array[Efl - i - 1] = -sample_exponent;
   }

   for(int i = 0; i < exp_arr_size; i++)
   {
      // Extract a value to be tested for the exponent part.
      input_float.e = sample_exponent_array[i];
      cmplx_input_float.r().e = sample_exponent_array[i];
      cmplx_input_float.i().e = sample_exponent_array[i];

      for(double i = lower_limit_mantissa; i <= upper_limit_mantissa; i += step_mantissa)
      {
         for(double j = lower_limit_mantissa; j <= upper_limit_mantissa; j += step_mantissa)
         {
            cmplx_input_float.r().m = i;
            cmplx_input_float.i().m = j;
            input_float.m = i;
            test_ac_reciprocal_pwl_float(input_float, output_float, cmplx_input_float, cmplx_output_float);

            double this_error_float =
                err_calc(input_float, actual_value_float, output_float, allowed_error_float, threshold);
            double this_error_complex =
                cmplx_err_calc(cmplx_input_float, cmplx_output_float, allowed_error_float, threshold);

            if(this_error_float > max_error_float)
            {
               max_error_float = this_error_float;
            }
            if(this_error_complex > max_error_cmplx_float)
            {
               max_error_cmplx_float = this_error_complex;
            }
         }
      }
   }

   passed = (max_error_float < allowed_error_float) && (max_error_cmplx_float < allowed_error_float);

   if(passed)
   {
      printf("PASSED , max err (%f) (%f complex)\n", max_error_float, max_error_cmplx_float);
   }
   else
   {
      printf("FAILED , max err (%f) (%f complex)\n", max_error_float, max_error_cmplx_float);
   } // LCOV_EXCL_LINE

   if(max_error_float > cumulative_max_error_float)
   {
      cumulative_max_error_float = max_error_float;
   }
   if(max_error_cmplx_float > cumulative_max_error_cmplx_float)
   {
      cumulative_max_error_cmplx_float = max_error_cmplx_float;
   }

   return 0;
}

int main(int argc, char* argv[])
{
   double max_error_fixed = 0, cmplx_max_error_fixed = 0, max_error_float = 0, cmplx_max_error_float = 0;
   double allowed_error_fixed = 0.5;
   double allowed_error_float = 0.5;
   const double threshold = 0.005;
   cout << "=============================================================================" << endl;
   cout << "Testing function: ac_reciprocal_pwl() - Allowed error " << allowed_error_fixed << " (fixed pt), "
        << allowed_error_float << " (float pt)" << endl;

   // template <int Wfi, int Ifi, bool Sfi, int outWfi, int outIfi, bool outSfi>
   test_driver_fixed<10, 3, true, 64, 32, true>(max_error_fixed, cmplx_max_error_fixed, allowed_error_fixed, threshold);
   test_driver_fixed<11, 1, true, 64, 32, true>(max_error_fixed, cmplx_max_error_fixed, allowed_error_fixed, threshold);
   test_driver_fixed<10, 0, false, 64, 32, false>(max_error_fixed, cmplx_max_error_fixed, allowed_error_fixed,
                                                  threshold);
   test_driver_fixed<12, 2, false, 64, 32, true>(max_error_fixed, cmplx_max_error_fixed, allowed_error_fixed,
                                                 threshold);
   test_driver_fixed<4, 9, true, 64, 32, true>(max_error_fixed, cmplx_max_error_fixed, allowed_error_fixed, threshold);
   test_driver_fixed<4, -2, true, 64, 32, true>(max_error_fixed, cmplx_max_error_fixed, allowed_error_fixed, threshold);
   test_driver_fixed<5, 8, false, 64, 32, false>(max_error_fixed, cmplx_max_error_fixed, allowed_error_fixed,
                                                 threshold);
   test_driver_fixed<4, -2, false, 60, 30, false>(max_error_fixed, cmplx_max_error_fixed, allowed_error_fixed,
                                                  threshold);
   test_driver_fixed<10, 4, true, 64, 32, true>(max_error_fixed, cmplx_max_error_fixed, allowed_error_fixed, threshold);
   test_driver_fixed<10, 3, false, 64, 32, false>(max_error_fixed, cmplx_max_error_fixed, allowed_error_fixed,
                                                  threshold);
   test_driver_fixed<9, 4, true, 60, 30, true>(max_error_fixed, cmplx_max_error_fixed, allowed_error_fixed, threshold);
   test_driver_fixed<9, 2, false, 64, 32, false>(max_error_fixed, cmplx_max_error_fixed, allowed_error_fixed,
                                                 threshold);

   // template <int Wfl, int Ifl, int Efl, int outWfl, int outIfl, int outEfl>
   test_driver_float<5, 3, 3, 64, 32, 10>(max_error_float, cmplx_max_error_float, allowed_error_float, threshold);
   test_driver_float<5, 1, 8, 64, 32, 10>(max_error_float, cmplx_max_error_float, allowed_error_float, threshold);
   test_driver_float<5, 0, 3, 64, 32, 10>(max_error_float, cmplx_max_error_float, allowed_error_float, threshold);
   test_driver_float<5, -2, 3, 60, 30, 11>(max_error_float, cmplx_max_error_float, allowed_error_float, threshold);
   test_driver_float<5, 9, 6, 64, 32, 10>(max_error_float, cmplx_max_error_float, allowed_error_float, threshold);
   test_driver_float<5, 5, 3, 64, 32, 10>(max_error_float, cmplx_max_error_float, allowed_error_float, threshold);
   test_driver_float<5, 5, 1, 60, 30, 11>(max_error_float, cmplx_max_error_float, allowed_error_float, threshold);
   test_driver_float<10, 5, 4, 64, 32, 10>(max_error_float, cmplx_max_error_float, allowed_error_float, threshold);
   test_driver_float<5, 3, 5, 64, 32, 10>(max_error_float, cmplx_max_error_float, allowed_error_float, threshold);

   cout << "=============================================================================" << endl;
   cout << "  Testbench finished. Maximum errors observed across all data type / bit-width variations:" << endl;
   cout << "    max_error_fixed       = " << max_error_fixed << endl;
   cout << "    cmplx_max_error_fixed = " << cmplx_max_error_fixed << endl;
   cout << "    max_error_float       = " << max_error_float << endl;
   cout << "    cmplx_max_error_float = " << cmplx_max_error_float << endl;

   // If error limits on any tested datatype have been crossed, the test has failed
   bool test_fail = (max_error_fixed > allowed_error_fixed) || (cmplx_max_error_fixed > allowed_error_fixed) ||
                    (max_error_float > allowed_error_float) || (cmplx_max_error_float > allowed_error_float);

   // Notify the user whether or not the test was a failure.
   if(test_fail)
   {
      cout << "  ac_reciprocal_pwl - FAILED - Error tolerance(s) exceeded" << endl;                    // LCOV_EXCL_LINE
      cout << "=============================================================================" << endl; // LCOV_EXCL_LINE
      return -1;                                                                                       // LCOV_EXCL_LINE
   }
   else
   {
      cout << "  ac_reciprocal_pwl - PASSED" << endl;
      cout << "=============================================================================" << endl;
   }
   return 0;
}
