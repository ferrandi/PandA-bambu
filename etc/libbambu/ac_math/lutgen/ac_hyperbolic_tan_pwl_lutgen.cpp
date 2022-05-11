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
// Usage:
//   g++ -I$MGC_HOME/shared/include ac_hyperbolic_tan_pwl_lutgen.cpp -o ac_hyperbolic_tan_pwl_lutgen
//   ./ac_hyperbolic_tan_pwl_lutgen
// results in a text file ac_hyperbolic_tan_pwl_lut_values.txt which can be pasted into
// a locally modified version of ac_hyperbolic_tan_pwl.h.

#include <ac_fixed.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include <math.h>
#include <sstream>
#include <stdio.h>
#include <string>
using namespace std;

// Define the number of points in your LUT.
const unsigned npoints = 13;
const unsigned nsegments = npoints - 1;
const double x_min = 0;
const double x_max = 3;
const double prop_constant = nsegments / (x_max - x_min);
double m[nsegments];
double c[nsegments];

#include "helper_functions.h"

int main()
{
   FILE* fp;

   // The output ROM values will be printed to a file in c++ syntax.
   // Define the filename below. The new file will be in the same folder
   // as this .cpp file
   const char filename[] = "ac_hyperbolic_tan_pwl_lut_values.txt";

   // Define your domain for calculation. In case you want to use the
   // value of pi, use M_PI, a math.h macro that has the value of pi.
   //(e.g. "x_max = M_PI/4")
   double x[npoints];
   double x_sc[npoints];
   double y[npoints];
   double x_val_inc = x_min;
   ostringstream mstrstream;
   ostringstream cstrstream;
   string mstr = "";
   string cstr = "";

   // Find slope and intercept for each segment.
   for(int i = 0; i < npoints; i++)
   {
      // Replace this line with whichever function you want, e.g.
      //"y[i] = sin(x_val_inc)"
      y[i] = tanh(x_val_inc);
      x[i] = x_val_inc;
      x_sc[i] = (x_val_inc - x_min) * prop_constant;
      x_val_inc += (1 / prop_constant);
   }

   // Shift segments upward
   for(int i = 0; i < nsegments; i++)
   {
      m[i] = y[i + 1] - y[i];
      c[i] = y[i];
      double x_mid = 0.5 * (x[i + 1] + x[i]);
      // Insert your function as the last term here, e.g.
      // double max_diff = (m[i] * x_mid + c[i] - sin(x_mid));
      double max_diff = pwl_new(x_mid) - tanh(x_mid);
      c[i] = c[i] - 0.5 * (max_diff);
   }

   double y1_new, y2_new;

   // Correct slopes and intercepts in order for monotonicity to
   // be maintained.
   for(int i = 1; i < npoints - 1; i++)
   {
      y1_new = m[i - 1] + c[i - 1];
      y2_new = m[i] + c[i];
      m[i] = y2_new - y1_new;
      c[i] = y1_new;
   }

   // double increment = (x_max - x_min) / 65536;

   double increment = 3.0 / 65536.0;
   double abs_error, abs_error_max = 0, rel_error_max = 0, input_error_max;

   for(double input_tb = x_min; input_tb < x_max; input_tb += increment)
   {
      // Insert your function here,
      // e.g. double actual = sin(input_tb);
      double expected = tanh(input_tb);
      double actual = pwl_new(input_tb);
      double rel_error = abs((expected - actual) / expected) * 100;
      if(rel_error > rel_error_max)
      {
         rel_error_max = rel_error;
      }
      abs_error = abs(expected - actual);
      if(abs_error > abs_error_max)
      {
         abs_error_max = abs_error;
         input_error_max = input_tb;
      }
   }

   int nfrac_bits = abs(floor(log2(abs_error_max))) + 1;

   // Add elements to Objects that contain declaration of LUT arrays
   // in C++ syntax.
   for(int i = 0; i < nsegments; i++)
   {
      if(i == 0)
      {
         mstrstream << "{" << o_ac_f(m[i], nfrac_bits) << ", ";
         cstrstream << "{" << o_ac_f(c[i], nfrac_bits) << ", ";
      }
      else if(i == nsegments - 1)
      {
         mstrstream << o_ac_f(m[i], nfrac_bits) << "}";
         cstrstream << o_ac_f(c[i], nfrac_bits) << "}";
      }
      else
      {
         mstrstream << o_ac_f(m[i], nfrac_bits) << ", ";
         cstrstream << o_ac_f(c[i], nfrac_bits) << ", ";
      }
   }
   mstr = mstrstream.str();
   cstr = cstrstream.str();

   // Find max value in array, and see if it has any negative values. This helps figure out
   // the number of integer bits to use to store array values.
   double m_max_val, c_max_val;
   bool is_neg_m, is_neg_c;

   is_neg_m = is_neg_max_array(m, m_max_val);
   is_neg_c = is_neg_max_array(c, c_max_val);

   string is_neg_m_s = is_neg_m ? "true" : "false";
   string is_neg_c_s = is_neg_c ? "true" : "false";

   int m_int_bits = int_bits_calc(m_max_val, is_neg_m);
   int c_int_bits = int_bits_calc(c_max_val, is_neg_c);
   int x_min_int_bits = int_bits_calc(x_min, x_min < 0);
   int x_max_int_bits = int_bits_calc(x_max, x_max < 0);
   int p_c_int_bits = int_bits_calc(prop_constant, false);

   string is_neg_x_min_s = (x_min < 0) ? "true" : "false";
   string is_neg_x_max_s = (x_max < 0) ? "true" : "false";

   std::ofstream outfile(filename);
   outfile << "const unsigned n_segments_lut = " << nsegments << ";" << endl;
   outfile << "const int n_frac_bits = " << nfrac_bits << ";" << endl;
   outfile << "static const ac_fixed<" << m_int_bits << " + n_frac_bits, " << m_int_bits << ", " << is_neg_m_s
           << "> m_lut[n_segments_lut] = " << mstr << ";" << endl;
   outfile << "static const ac_fixed<" << c_int_bits << " + n_frac_bits, " << c_int_bits << ", " << is_neg_c_s
           << "> c_lut[n_segments_lut] = " << cstr << ";" << endl;
   outfile << "static const ac_fixed<" << x_min_int_bits << " + n_frac_bits, " << x_min_int_bits << ", "
           << is_neg_x_min_s << "> x_min_lut = " << o_ac_f(x_min, nfrac_bits) << ";" << endl;
   outfile << "static const ac_fixed<" << x_max_int_bits << " + n_frac_bits, " << x_max_int_bits << ", "
           << is_neg_x_max_s << "> x_max_lut = " << o_ac_f(x_max, nfrac_bits) << ";" << endl;
   outfile << "static const ac_fixed<" << p_c_int_bits << " + n_frac_bits, " << p_c_int_bits
           << ", false> sc_constant_lut = " << o_ac_f(prop_constant, nfrac_bits) << ";" << endl;
   outfile.close();

   cout << endl;
   cout << __FILE__ << ", " << __LINE__ << ": Values are written, check \"" << filename << "\" for "
        << "the required ROM values" << endl;
   cout << "abs_error_max = " << abs_error_max << endl;
   cout << "input_error_max = " << input_error_max << endl;
   cout << "nfrac_bits = " << nfrac_bits << endl;
   cout << "rel_error_max = " << rel_error_max << endl;

   return 0;
}
