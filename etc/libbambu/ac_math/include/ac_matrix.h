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
//***************************************************************************
// File: ac_matrix.h
//
// Description: Provides a container class representing a matrix
//    with overloaded operators for basic math functions.
//
// Usage:
//    A sample testbench and its implementation look like
//    this:
//
//    #include <ac_fixed.h>
//    #include <ac_complex.h>
//    #include <ac_matrix.h>
//
//    #pragma design top
//    void test_function(ac_matrix<int,3,3> &m) {
//      return determinant(m);
//    }
//
//    #ifndef __BAMBU__
//    #include <iostream>
//    using namespace std;
//
//    CCS_MAIN(int arg, char **argc)
//    {
//      ac_matrix<int,2,2> m2x2;
//      m2x2(0,0) = 3;
//      m2x2(0,1) = 8;
//      m2x2(1,0) = 4;
//      m2x2(1,1) = 6;
//
//      int det = CCS_DESIGN(determinant)(m2x2);
//      cout << "det = " << det << endl;
//      CCS_RETURN (0);
//    }
//    #endif
//
// Notes:
//    Classic C++ 2-d array access notation of arr[m][n] needs to
//    be changed to arr(m,n) for this class.
//
// Revision History:
//
//***************************************************************************

#ifndef _INCLUDED_AC_MATRIX_H_
#define _INCLUDED_AC_MATRIX_H_

// Include headers for data types supported by these implementations
#include <ac_complex.h>
#include <ac_fixed.h>
#include <ac_float.h>
#include <ac_int.h>

//#include <ac_assert.h>

//=========================================================================
// Class: ac_matrix
//
//-------------------------------------------------------------------------

template <class T, unsigned M, unsigned N>
class ac_matrix
{
 public:
   // Traits for computing internals of ac_matrix
   typedef T type;
   static const unsigned dim1 = M;
   static const unsigned dim2 = N;
   enum
   {
      rows = M
   };
   enum
   {
      cols = N
   };
   enum
   {
      length = M * N
   };

   // Constructor: new (empty) MxN matrix
   ac_matrix()
   {
   }

   // Constructor: new MxN matrix initialized with value v
   ac_matrix(const T& v)
   {
      initialize(v);
   }

   // Copy constructor
   ac_matrix(const ac_matrix<T, M, N>& other)
   {
      copy_contents(other);
   }

   // Destructor
   ~ac_matrix()
   {
   }

   inline static std::string type_name()
   {
      typedef typename ac_private::map<T>::t map_T;
      std::string r = "ac_matrix<";
      r += map_T::type_name();
      r += ",";
      r += ac_int<32, true>(M).to_string(AC_DEC);
      r += ",";
      r += ac_int<32, true>(N).to_string(AC_DEC);
      r += ">";
      return r;
   }

 public: // Class Member Functions
   // Set matrix equal to matrix other
   /* ac_matrix<T,M,N> & operator=(const ac_matrix<T,M,N> &other)
    {
      // Equal to itself
      if ( &other != this ) {
        copy_contents(other);
      }
      return *this;
    }*/

   // Set all elements of matrix equal to constant
   ac_matrix<T, M, N>& operator=(const T& v)
   {
      initialize(v);
      return *this;
   }

   // Get (modifiable) element
   T& operator()(const unsigned r, const unsigned c)
   {
      // assert( (r<M) && (c<N) );
      return m_data[r][c];
   }

   // Get element value
   const T& operator()(const unsigned r, const unsigned c) const
   {
      // assert( (r<M) && (c<N) );
      return m_data[r][c];
   }

   // Return a new read-only submatrix of this matrix
   // row,col args select starting position
   template <unsigned R, unsigned S>
   const ac_matrix<T, R, S> operator()(unsigned row, unsigned col) const
   {
      ac_matrix<T, R, S> submatrix;
      // assert(row+R<M);
      // assert(col+S<N);
      for(unsigned i = 0; i < R; i++)
      {
         for(unsigned j = 0; j < S; j++)
         {
            submatrix(i, j) = (*this)(i + row, j + col);
         }
      }
      return submatrix;
   }

 public: // Relational operators
   //----------------------------------------------------------------------
   // OPERATOR !=
   template <class T2, unsigned M2, unsigned N2>
   bool operator!=(const ac_matrix<T2, M2, N2>& other) const
   {
      if(M2 != M)
      {
         return true;
      }
      if(N2 != N)
      {
         return true;
      }
      for(unsigned i = 0; i < M; i++)
      {
         for(unsigned j = 0; j < N; j++)
         {
            if((*this)(i, j) != other(i, j))
            {
               return true;
            }
         }
      }
      return false;
   }

   //----------------------------------------------------------------------
   // OPERATOR ==
   template <class T2, unsigned M2, unsigned N2>
   bool operator==(const ac_matrix<T2, M2, N2>& other) const
   {
      return (!this->operator!=(other));
   }

 public: // Math operators
   //----------------------------------------------------------------------
   // OPERATOR + (piecewise matrix addition)
   template <typename T2>
   ac_matrix<typename ac::rt_2T<T, T2>::plus, M, N> operator+(const ac_matrix<T2, M, N>& op2) const
   {
      ac_matrix<typename ac::rt_2T<T, T2>::plus, M, N> result;
      for(unsigned i = 0; i < M; i++)
      {
         for(unsigned j = 0; j < N; j++)
         {
            result(i, j) = (*this)(i, j) + op2(i, j);
         }
      }
      return result;
   }

   //----------------------------------------------------------------------
   // OPERATOR +=
   ac_matrix<T, M, N>& operator+=(const ac_matrix<T, M, N>& op2)
   {
      for(unsigned i = 0; i < M; i++)
      {
         for(unsigned j = 0; j < N; j++)
         {
            (*this)(i, j) += op2(i, j);
         }
      }
      return *this;
   }

   //----------------------------------------------------------------------
   // OPERATOR -
   template <typename T2>
   ac_matrix<typename ac::rt_2T<T, T2>::minus, M, N> operator-(const ac_matrix<T2, M, N>& op2) const
   {
      ac_matrix<typename ac::rt_2T<T, T2>::minus, M, N> result;
      for(unsigned i = 0; i < M; i++)
      {
         for(unsigned j = 0; j < N; j++)
         {
            result(i, j) = (*this)(i, j) - op2(i, j);
         }
      }
      return result;
   }

   //----------------------------------------------------------------------
   // OPERATOR -=
   ac_matrix<T, M, N>& operator-=(const ac_matrix<T, M, N>& op2)
   {
      for(unsigned i = 0; i < M; i++)
      {
         for(unsigned j = 0; j < N; j++)
         {
            (*this)(i, j) -= op2(i, j);
         }
      }
      return *this;
   }

 public: // Matrix math functions
   //----------------------------------------------------------------------
   // Function: pwisemult
   // Description: piecewise multiplication of two matrices
   // Return: matrix of the same dimension as this
   template <typename T2>
   ac_matrix<typename ac::rt_2T<T, T2>::mult, M, N> pwisemult(const ac_matrix<T2, M, N>& op2) const
   {
      ac_matrix<typename ac::rt_2T<T, T2>::mult, M, N> result;
      for(unsigned i = 0; i < M; i++)
      {
         for(unsigned j = 0; j < N; j++)
         {
            result(i, j) = (*this)(i, j) * op2(i, j);
         }
      }
      return result;
   }

   //----------------------------------------------------------------------
   // Function: mult
   // Description: matrix multiplication
   //    result[M][Q] = this[M][N] X op2[N][Q]
   // Return: matrix sized MxQ
   template <typename T2, unsigned Q>
   ac_matrix<typename ac::rt_2T<T, T2>::mult, M, Q> operator*(const ac_matrix<T2, N, Q>& op2) const
   {
      ac_matrix<typename ac::rt_2T<T, T2>::mult, M, Q> result;
      for(unsigned i = 0; i < M; i++)
      {
         for(unsigned j = 0; j < Q; j++)
         {
            T tsum = 0;
            for(unsigned k = 0; k < N; k++)
            {
               tsum += (*this)(i, k) * op2(k, j);
            }
            result(i, j) = tsum;
         }
      }
      return result;
   }

   //----------------------------------------------------------------------
   // Function: transpose
   // Description: compute transpose of this matrix
   // Return: matrix sized NxM
   ac_matrix<T, N, M> transpose() const
   {
      ac_matrix<T, N, M> result;
      for(unsigned i = 0; i < M; i++)
      {
         for(unsigned j = 0; j < N; j++)
         {
            result(i, j) = (*this)(i, j);
         }
      }
      return result;
   }

   //----------------------------------------------------------------------
   // Function: sum
   // Description: Calculate the sum of all elements of this matrix
   // Return: scalar value
   T sum() const
   {
      T t = (T)0;
      for(unsigned i = 0; i < M; i++)
      {
         for(unsigned j = 0; j < N; j++)
         {
            t += (*this)(i, j);
         }
      }
      return t;
   }

   //----------------------------------------------------------------------
   // Function: scale
   // Description: Scale all elements of this matrix with scale factor
   // Return: matrix sized MxN
   template <class TF>
   ac_matrix<T, M, N> scale(TF scale) const
   {
      ac_matrix<T, M, N> result;
      for(unsigned i = 0; i < M; i++)
      {
         for(unsigned j = 0; j < N; j++)
         {
            result(i, j) = (T)(scale * (*this)(i, j));
         }
      }
      return result;
   }

   // Send ac_matrix inputs to cholesky decomposition as 2D arrays instead.
   template <bool use_pwl, int del_w, int del_i, ac_q_mode int_Q, ac_o_mode int_O, class T1, unsigned M1, class T2>
   friend void indirect_chol_d(const ac_matrix<T1, M1, M1>& input, ac_matrix<T2, M1, M1>& output);

   // Send ac_matrix inputs to matrix multiplication as 2D arrays instead.
   template <class TA, class TB, class TC>
   friend void ac_matrix_matrixmul(const TA& A, const TB& B, TC& C);

   // Send ac_matrix inputs to cholesky inverse as 2D arrays instead.
   template <bool use_pwl1, bool use_pwl2, int add2w, int add2i, ac_q_mode temp_Q, ac_o_mode temp_O, unsigned M1,
             class T1, class T2>
   friend void ac_matrix_cholinv(const ac_matrix<T1, M1, M1>& input, ac_matrix<T2, M1, M1>& output);

#ifdef CCS_SCVERIFY
 public: // Class Member Data (must be public for SCVerify)
#else
 private: // Class Member Data
#endif
         // Use static memory allocation. If you compile this way
   // for execution on a processor, watch out for stack overflows.
   T m_data[M][N];

 private: // Internal class member functions
   void copy_contents(const ac_matrix<T, M, N>& other)
   {
      for(unsigned x = 0; x < M; x++)
      {
         for(unsigned y = 0; y < N; y++)
         {
            (*this)(x, y) = other(x, y);
         }
      }
   }

   void initialize(const T& v)
   {
      for(unsigned x = 0; x < M; x++)
      {
         for(unsigned y = 0; y < N; y++)
         {
            (*this)(x, y) = v;
         }
      }
   }
};

//----------------------------------------------------------------------
// Function: determinant
// Description: Calculate the determinant of the square matrix m
//

// Generic templatized function prototype
template <class T, unsigned M>
T determinant(ac_matrix<T, M, M>& m);

// Generic templatized function implementation
template <class T, unsigned M>
T determinant(ac_matrix<T, M, M>& m)
{
   // ac_assert(false); // not yet implemented
   T det = 1.0;
   return det;
}

// Specialized template version for 3x3 matrix
template <class T>
T determinant(ac_matrix<T, 3, 3>& m)
{
   return m(0, 0) * (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1)) - m(0, 1) * (m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0)) +
          m(0, 2) * (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0));
}

// Specialized template version for 2x2 matrix
template <class T>
T determinant(ac_matrix<T, 2, 2>& m)
{
   return m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0);
}

#ifndef __BAMBU__
//#include <iostream>

//----------------------------------------------------------------------
// Function: ostream operator<<
// Description: print the contents of an ac_matrix to cout

template <class T, unsigned M, unsigned N>
std::ostream& operator<<(std::ostream& os, const ac_matrix<T, M, N>& m)
{
   // column header
   os << "   ";
   for(unsigned j = 0; j < N; j++)
   {
      os << "[" << j << "]  ";
   }
   os << std::endl;
   for(unsigned i = 0; i < M; i++)
   {
      os << "[" << i << "] ";
      for(unsigned j = 0; j < N; j++)
      {
         os << m(i, j) << "  ";
      }
      os << std::endl;
   }
   return os;
}
#endif // NOT __BAMBU__

#endif // _INCLUDED_AC_MATRIX_H_
