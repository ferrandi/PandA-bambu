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
//===================================================================================
//
// File : ac_determinant.h
//
//
// Author: Sachchidanand Deo
//
// Description: This file provides the recursive template implementation
// for computing the determinant of matrix for ac_datatypes ac_fixed and
// ac_complex<ac_fixed>.
//
// Usage: (using ac_matrix class)
//    A sample testbench and its implementation looks like
//    this (for ac_fixed datatype):
//
//    #include <ac_fixed.h>
//    #include <ac_matrix.h>
//    #include <ac_math/ac_determinant.h>
//    using namespace ac_math;
//
//    const unsigned M = 1;
//    typedef ac_matrix <ac_fixed<16, 8, true, AC_RND, AC_SAT> , M ,M> input_type;
//
//    typedef ac_fixed<17, 9, true, AC_RND, AC_SAT> output_type;
//
//
//    #pragma hls_design top
//    void project(
//      const input_type &input,
//      output_type &output
//    )
//    {
//      ac_determinant(input, output);
//    }
//
//    #ifndef __BAMBU__
//    #include <mc_scverify.h>
//    CCS_MAIN(int arg, char **argc)
//    {
//      input_type input;
//      input = 3; // can also be written as input(0,0) = 3;
//      output_type output;
//      CCS_DESIGN(project)(input, output);
//      CCS_RETURN (0);
//    }
//    #endif
//
//-----------------------------------------------------------------------------------
//
//    Usage: (using standard C array)
//    A sample testbench and its implementation looks like
//    this (for ac_fixed datatype):
//
//    #include <ac_fixed.h>
//    #include <ac_math/ac_determinant.h>
//    using namespace ac_math;
//
//    const unsigned M = 1;
//    typedef ac_fixed<16, 8, true, AC_RND, AC_SAT> input_type;
//
//    typedef ac_fixed<17, 9, true, AC_RND, AC_SAT> output_type;
//
//
//    #pragma hls_design top
//    void project(
//      const input_type input[M][M],
//      output_type &output
//    )
//    {
//      ac_determinant<M>(input, output);
//    }
//
//    #ifndef __BAMBU__
//    #include <mc_scverify.h>
//    CCS_MAIN(int arg, char **argc)
//    {
//      input_type input[1][1];
//      input[0][0] = 1;
//      output_type output;
//      CCS_DESIGN(project)(input, output);
//      CCS_RETURN (0);
//    }
//    #endif
//
// Note:
//    This file uses C++ function overloading to target implementations
//    specific to each type of data. Attempting to call the function
//    with a type that is not implemented will result in a compile-time error.
//
//===================================================================================

#ifndef _INCLUDED_AC_DETERMINANT_H_
#define _INCLUDED_AC_DETERMINANT_H_

// Include headers for data types supported by these implementations
#include <ac_complex.h>
#include <ac_fixed.h>
#include <ac_matrix.h>

#if !defined(__BAMBU__) && defined(AC_DETERMINANT_H_DEBUG)
#include <iostream>
using namespace std;
#endif

//=================================================================================================================
// Templatized recursive struct: Factorial
// Description:
//    Recursive template implementation for computing factorial of an unsigned value.
//
//    The helper struct makes use of recursive nature of factorial computation. We start off with M and then reduce
//    the value of M by 1 each time and then multiply the original value with the reduced value.
//
//    Base case of recursion is implemented for M = 0 at which struct returns value = 1, as 0! = 1.
//    This whole factorial computation is done at compile time.
// ----------------------------------------------------------------------------------------------------------------

namespace ac_math
{
   template <unsigned M>
   struct Factorial
   {
      enum
      {
         value = M * Factorial<M - 1>::value
      };
   };

   template <>
   struct Factorial<0>
   {
      enum
      {
         value = 1
      };
   };

   //=================================================================================================================
   // Templatized helper struct: determinant_matrix
   // Containing function: determinant_compute
   //
   // Description:
   //    Recursive template implementation, for computing determinant of a matrix.
   //    This type of implementation is a balanced implementation and allows efficient
   //    hardware structure in terms of area and performance.
   //
   //    The helper struct makes use of recursive nature of determinant computation, by
   //    reducing the dimension by one until it reaches the matrix of size 2x2. This 2x2 matrix is then
   //    treated as a specialized case and is used to terminate the recursion. Value of determinant
   //    of this matrix is then computed using specialization case, which calculates the determinant as:
   //    det = A(1, 1)*A(2, 2) - A(1, 2)*A(2, 1)
   //
   //    Function takes typedefs as template parameters, which are used to define internal, as well input
   //    and output typedefs.
   //
   // Note:
   //    This implementation provides an accurate computation of determinant. However, errors are introduced as
   //    higher dimension matrices are used at input, which leads to the internally calculated bitwidths being
   //    insufficient. This problem is handled by giving user complete control over internal bitwidths by supplying
   //    them as template parameters.
   //
   // ----------------------------------------------------------------------------------------------------------------

   template <unsigned M>
   struct determinant_matrix
   {
      // Function inside helper struct that carries out the procedure of computing determinant
      template <typename input_type, typename output_type, typename c_type, typename temp_type, typename d_type,
                typename base_type>
      static output_type determinant_compute(const ac_matrix<input_type, M, M>& A)
      {
         int pr = -1;  // pr is used to keep track of sign in determinant computation
         c_type c[M];  // c is used to store the internal results of intermediate matrices, before multiplication and
                       // accumulation happens
         d_type d = 0; // d is used to store and return the final result
         ac_matrix<input_type, M - 1, M - 1>
             b; // b is used to store minor matrix whose determinant is then to be computed recursively

         temp_type temp;
         // PIVOT loop to keep track of pivot (element about which minor is to be computed)
#if !defined(__clang__)
#pragma unroll yes
#endif
      PIVOT:
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(unsigned j = 0; j < M; j++)
         {
            // ROW and COLUMN loop to create matrices
#if !defined(__clang__)
#pragma unroll yes
#endif
         ROW:
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
            for(unsigned p = 1; p < M; p++)
            {
#if !defined(__clang__)
#pragma unroll yes
#endif
            COLUMN:
               for(unsigned q = 1; q < M; q++)
               {
                  b(p - 1, q - 1) = A(p, q - (q <= j));
               }
            }
            // Change sign every alternate time
            pr = (-1) * pr;
            // Recursive call using template recursion
            temp = determinant_matrix<M - 1>::template determinant_compute<input_type, output_type, c_type, temp_type,
                                                                           d_type, base_type>(b);
            c[j] = pr * temp;
         }
         // Accumulate loop for accumulation after multiplication with pivot
#if !defined(__clang__)
#pragma unroll yes
#endif
      ACCUMULATE:
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(unsigned j = 0; j < M; j++)
         {
            d = d + A(0, j) * c[j];
         }
         return d;
      }
   };

   //=========================================================================
   // Specialization of determinant computation for 2*2 matrix:
   // This part simply computes determinant of 2x2 matrix and returns it back
   // ------------------------------------------------------------------------
   template <>
   struct determinant_matrix<2>
   {
      template <typename input_type, typename output_type, typename c_type, typename temp_type, typename d_type,
                typename base_type>
      static base_type determinant_compute(const ac_matrix<input_type, 2, 2>& a)
      {
         base_type result;
         base_type temp1 = a(0, 0) * a(1, 1);
         base_type temp2 = a(0, 1) * a(1, 0);
         result = (temp1 - temp2);
         return result;
      }
   };

   //========================================================================================================
   // Specialization of determinant computation for 1*1 matrix:
   // This part simply returns the sole element in a 1x1 matrix. It is triggered when M = 1 is passed to this
   // implementation.
   // -------------------------------------------------------------------------------------------------------
   template <>
   struct determinant_matrix<1>
   {
      template <typename input_type, typename output_type, typename c_type, typename temp_type, typename d_type,
                typename base_type>
      static base_type determinant_compute(const ac_matrix<input_type, 1, 1>& a)
      {
         base_type result;
         result = a(0, 0);
         return result;
      }
   };

   template <unsigned M, bool internal_adjust, typename T1, typename T2>
   struct types_params
   {
   };

   // This structure is used to define internal datatypes for ac_fixed implementation
   template <unsigned M, bool internal_adjust, int Wtemp, int Itemp, bool signtemp, ac_q_mode qtemp, ac_o_mode otemp,
             int W, int I, bool S, ac_q_mode q_mode, ac_o_mode o_mode>
   struct types_params<M, internal_adjust, ac_fixed<Wtemp, Itemp, signtemp, qtemp, otemp>,
                       ac_fixed<W, I, S, q_mode, o_mode>>
   {
      enum
      {
         c_width = (internal_adjust) ? Wtemp : (M * W + ac::log2_floor<Factorial<M>::value>::val),
         c_int = (internal_adjust) ? Itemp : (M * I + ac::log2_floor<Factorial<M>::value>::val),
         c_sign = (internal_adjust) ? signtemp : true,
         c_qmode = (internal_adjust) ? qtemp : AC_RND,
         c_omode = (internal_adjust) ? otemp : AC_SAT,

         temp_width = (internal_adjust) ? Wtemp : M * W,
         temp_int = (internal_adjust) ? Itemp : M * I,
         temp_sign = (internal_adjust) ? signtemp : true,
         temp_qmode = (internal_adjust) ? qtemp : AC_RND,
         temp_omode = (internal_adjust) ? otemp : AC_SAT,

         d_width = (internal_adjust) ? Wtemp : M * W,
         d_int = (internal_adjust) ? Itemp : M * I,
         d_sign = (internal_adjust) ? signtemp : true,
         d_qmode = (internal_adjust) ? qtemp : AC_RND,
         d_omode = (internal_adjust) ? otemp : AC_SAT,
      };

      typedef ac_fixed<c_width, c_int, c_sign, (ac_q_mode)c_qmode, (ac_o_mode)c_omode> c_type;
      typedef ac_fixed<temp_width, temp_int, temp_sign, (ac_q_mode)temp_qmode, (ac_o_mode)temp_omode> temp_type;
      typedef ac_fixed<2 * W + 1, 2 * I + 1, true, AC_RND, AC_SAT> base_type;
      typedef ac_fixed<d_width, d_int, d_sign, (ac_q_mode)d_qmode, (ac_o_mode)d_omode> d_type;
   };

   // This structure is used to define internal datatypes for ac_complex<ac_fixed> implementation
   template <unsigned M, bool internal_adjust, int Wtemp, int Itemp, bool signtemp, ac_q_mode qtemp, ac_o_mode otemp,
             int W, int I, bool S, ac_q_mode q_mode, ac_o_mode o_mode>
   struct types_params<M, internal_adjust, ac_complex<ac_fixed<Wtemp, Itemp, signtemp, qtemp, otemp>>,
                       ac_complex<ac_fixed<W, I, S, q_mode, o_mode>>>
   {
      enum
      {
         c_width = (internal_adjust) ? Wtemp : (M * W + ac::log2_floor<Factorial<M>::value>::val),
         c_int = (internal_adjust) ? Itemp : (M * I + ac::log2_floor<Factorial<M>::value>::val),
         c_sign = (internal_adjust) ? signtemp : true,
         c_qmode = (internal_adjust) ? qtemp : AC_RND,
         c_omode = (internal_adjust) ? otemp : AC_SAT,

         temp_width = (internal_adjust) ? Wtemp : M * W,
         temp_int = (internal_adjust) ? Itemp : M * I,
         temp_sign = (internal_adjust) ? signtemp : true,
         temp_qmode = (internal_adjust) ? qtemp : AC_RND,
         temp_omode = (internal_adjust) ? otemp : AC_SAT,

         d_width = (internal_adjust) ? Wtemp : M * W,
         d_int = (internal_adjust) ? Itemp : M * I,
         d_sign = (internal_adjust) ? signtemp : true,
         d_qmode = (internal_adjust) ? qtemp : AC_RND,
         d_omode = (internal_adjust) ? otemp : AC_SAT,

         base_width = 2 * W + 1,
         base_int = 2 * I + 1
      };
      // all the four typedefs for four internal variables
      typedef ac_complex<ac_fixed<c_width, c_int, c_sign, (ac_q_mode)c_qmode, (ac_o_mode)c_omode>> c_type;
      typedef ac_complex<ac_fixed<temp_width, temp_int, temp_sign, (ac_q_mode)temp_qmode, (ac_o_mode)temp_omode>>
          temp_type;
      typedef ac_complex<ac_fixed<base_width, base_int, true, q_mode, o_mode>> base_type;
      typedef ac_complex<ac_fixed<d_width, d_int, d_sign, (ac_q_mode)d_qmode, (ac_o_mode)d_omode>> d_type;
   };

   //==================================================================================================================
   // Function: ac_determinant
   //
   // Description:
   //    Recursive template implementation final wrapper function for ac_fixed and ac_complex <ac_fixed> datatype.
   //
   //    Function takes dimensions of input matrix(M), internal bit width limit(width_limit), internal
   //    integer bit limit(int_limit), input bit width(W1), input integer width(I1), input sign(S1), input rounding(q1)
   //    and saturation type(o1).
   //
   //    This function forms the typenames for calling the common template recursive implementation (defined
   //    above), by using typedefs. The typenames are passed as template parameters to the recursive template
   //    implementation. This is similar to ac_fixed implementation, only different being that datatypes are defined as
   //    ac_complex <ac_fixed> datatypes.
   //
   // Note:
   //     Internal bit-widths are adjustable using the override flag.
   //
   // -----------------------------------------------------------------------------------------------------------------
   template <unsigned M, bool override, typename input_type, typename output_type, typename internal_type>
   void ac_determinant_combined(const ac_matrix<input_type, M, M>& a, output_type& result)
   {
      // call the overloaded types_params structures to define internal datatypes with bitwidths.
      typedef typename types_params<M, override, internal_type, input_type>::c_type c_type1;
      typedef typename types_params<M, override, internal_type, input_type>::temp_type temp_type1;
      typedef typename types_params<M, override, internal_type, input_type>::d_type d_type1;
      typedef typename types_params<M, override, internal_type, input_type>::base_type base_type1;

      // get the final answer of template recursion with full precision
      d_type1 result1 = determinant_matrix<M>::template determinant_compute<input_type, output_type, c_type1,
                                                                            temp_type1, d_type1, base_type1>(a);
      // assign the full-precision output to the final output provided by the user
      result = result1;
   }

   template <bool override = false, int internal_width = 16, int internal_int = 8, bool internal_sign = true,
             ac_q_mode internal_rnd = AC_RND, ac_o_mode internal_sat = AC_SAT, unsigned M, int W1, int I1, bool S1,
             ac_q_mode q1, ac_o_mode o1, int W2, int I2, ac_q_mode q2, ac_o_mode o2>
   void ac_determinant(const ac_matrix<ac_fixed<W1, I1, S1, q1, o1>, M, M>& a, ac_fixed<W2, I2, true, q2, o2>& result)
   {
      typedef ac_fixed<internal_width, internal_int, internal_sign, internal_rnd, internal_sat> internal_type;
      typedef ac_fixed<W1, I1, S1, q1, o1> input_type;
      typedef ac_fixed<W2, I2, true, q2, o2> output_type;
      ac_determinant_combined<M, override, input_type, output_type, internal_type>(a, result);
#if !defined(__BAMBU__) && defined(AC_DETERMINANT_H_DEBUG)
      cout << "M = " << M << endl;
      cout << "input total bitwidth = " << W1 << endl;
      cout << "input integer bitwidth = " << I1 << endl;
      cout << "input matrix supplied :" << a << endl;
      cout << "Final output = " << result << endl;
#endif
   }

   template <bool override = false, int internal_width = 16, int internal_int = 8, bool internal_sign = true,
             ac_q_mode internal_rnd = AC_RND, ac_o_mode internal_sat = AC_SAT, unsigned M, int W1, int I1, bool S1,
             ac_q_mode q1, ac_o_mode o1, int W2, int I2, ac_q_mode q2, ac_o_mode o2>
   void ac_determinant(const ac_matrix<ac_complex<ac_fixed<W1, I1, S1, q1, o1>>, M, M>& a,
                       ac_complex<ac_fixed<W2, I2, true, q2, o2>>& result)
   {
      typedef ac_complex<ac_fixed<internal_width, internal_int, internal_sign, internal_rnd, internal_sat>>
          internal_type;
      typedef ac_complex<ac_fixed<W1, I1, S1, q1, o1>> input_type;
      typedef ac_complex<ac_fixed<W2, I2, true, q2, o2>> output_type;
      ac_determinant_combined<M, override, input_type, output_type, internal_type>(a, result);
#if !defined(__BAMBU__) && defined(AC_DETERMINANT_H_DEBUG)
      cout << "M = " << M << endl;
      cout << "input total bitwidth = " << W1 << endl;
      cout << "input integer bitwidth = " << I1 << endl;
      cout << "input matrix supplied :" << a << endl;
      cout << "Final output = " << result << endl;
#endif
   }

   // Define c-array function versions for determinant
   template <bool override = false, int internal_width = 16, int internal_int = 8, bool internal_sign = true,
             ac_q_mode internal_rnd = AC_RND, ac_o_mode internal_sat = AC_SAT, unsigned M, int W1, int I1, bool S1,
             ac_q_mode q1, ac_o_mode o1, int W2, int I2, ac_q_mode q2, ac_o_mode o2>
   void ac_determinant(const ac_fixed<W1, I1, S1, q1, o1> a[M][M], ac_fixed<W2, I2, true, q2, o2>& result)
   {
      ac_matrix<ac_fixed<W1, I1, S1, q1, o1>, M, M> a_temp;

#if !defined(__clang__)
#pragma unroll yes
#endif
   ROW_CPY:
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
      for(unsigned i = 0; i < M; i++)
      {
#if !defined(__clang__)
#pragma unroll yes
#endif
      COLUMN_CPY:
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(unsigned j = 0; j < M; j++)
         {
            a_temp(i, j) = a[i][j];
         }
      }
      ac_determinant<override, internal_width, internal_int, internal_sign, internal_rnd, internal_sat>(a_temp, result);
   }

   template <bool override = false, int internal_width = 16, int internal_int = 8, bool internal_sign = true,
             ac_q_mode internal_rnd = AC_RND, ac_o_mode internal_sat = AC_SAT, unsigned M, int W1, int I1, bool S1,
             ac_q_mode q1, ac_o_mode o1, int W2, int I2, ac_q_mode q2, ac_o_mode o2>
   void ac_determinant(const ac_complex<ac_fixed<W1, I1, S1, q1, o1>> a[M][M],
                       ac_complex<ac_fixed<W2, I2, true, q2, o2>>& result)
   {
      ac_matrix<ac_complex<ac_fixed<W1, I1, S1, q1, o1>>, M, M> a_temp;
#if !defined(__clang__)
#pragma unroll yes
#endif
   ROW_CPY:
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
      for(unsigned i = 0; i < M; i++)
      {
#if !defined(__clang__)
#pragma unroll yes
#endif
      COLUMN_CPY:
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
         for(unsigned j = 0; j < M; j++)
         {
            a_temp(i, j) = a[i][j];
         }
      }
      ac_determinant<override, internal_width, internal_int, internal_sign, internal_rnd, internal_sat>(a_temp, result);
   }

   // Version that allows returning of values for ac_matrix inputs.
   template <class T_out, bool override = false, int internal_width = 16, int internal_int = 8,
             bool internal_sign = true, ac_q_mode internal_rnd = AC_RND, ac_o_mode internal_sat = AC_SAT, unsigned M,
             class T_in>
   T_out ac_determinant(const ac_matrix<T_in, M, M>& input)
   {
      T_out output;
      ac_determinant<override, internal_width, internal_int, internal_sign, internal_rnd, internal_sat>(input, output);
      return output;
   }

   // Version that allows returning of values for c style array inputs.
   template <class T_out, bool override = false, int internal_width = 16, int internal_int = 8,
             bool internal_sign = true, ac_q_mode internal_rnd = AC_RND, ac_o_mode internal_sat = AC_SAT, unsigned M,
             class T_in>
   T_out ac_determinant(const T_in input[M][M])
   {
      T_out output;
      ac_determinant<override, internal_width, internal_int, internal_sign, internal_rnd, internal_sat>(input, output);
      return output;
   }
} // namespace ac_math

#endif
