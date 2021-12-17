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
#include <ac_complex.h>
#include <ac_fixed.h>
#include <ac_matrix.h>
#include <iomanip>
#include <iostream>

using namespace std;

// Helper macros for TEST_* macros
#define STRMACRO1(a) #a
#define STRMACRO2(a) STRMACRO1(a)

// Bit growth computations
//   ACTYPE + ACTYPE  ->  15bit I + 15bit I yields 16 bit Integer, plus 5 fractional for result 21,16
//   ACTYPE + S16TYPE ->  15bit I + 16 bit yields 17 Integer, plus 5 fractional for result 22,17

// Operand types
typedef ac_fixed<20, 15, true> ACTYPE;
typedef short S16TYPE;
typedef unsigned short U16TYPE;
typedef ac_complex<ac_fixed<20, 15, true>> cACTYPE;
typedef ac_complex<short> cS16TYPE;
typedef ac_complex<unsigned short> cU16TYPE;

// Result types (with bitgrowth)
typedef ac_fixed<21, 16, true> ACTYPEplusACTYPE;
typedef ac_fixed<22, 17, true> ACTYPEplusS16TYPE;
typedef ac_complex<ac_fixed<21, 16, true>> cACTYPEplusACTYPE;
typedef ac_complex<ac_fixed<22, 17, true>> cACTYPEplusS16TYPE;
typedef int S16TYPEplusS16TYPE;

typedef ac_fixed<40, 30, true> ACTYPEmultACTYPE;
typedef ac_complex<ac_fixed<41, 31, true>> cACTYPEmultACTYPE;

template <typename ACType>
void SET_VAL(ACType& obj, unsigned int val)
{
   obj = val;
}
template <typename ACType>
void SET_VAL(ac_complex<ACType>& obj, unsigned int val)
{
   obj.set_r(val);
   obj.set_i(1);
}

// Utility functions to initialize matrix contents
template <typename ACType, unsigned Rows, unsigned Cols>
void reset_matrix(ac_matrix<ACType, Rows, Cols>& mat, unsigned numCols, unsigned serial_number, unsigned scale = 1)
{
   for(unsigned i = 0; i < Rows; i++)
      for(unsigned j = 0; j < Cols; j++)
      {
         mat(i, j) = (ACType)(serial_number + (i * numCols + j) * scale);
      }
}

template <typename ACType, unsigned Rows, unsigned Cols>
void reset_matrix(ac_matrix<ac_complex<ACType>, Rows, Cols>& mat, unsigned numCols, unsigned serial_number,
                  unsigned scale = 1)
{
   for(unsigned i = 0; i < Rows; i++)
      for(unsigned j = 0; j < Cols; j++)
      {
         mat(i, j).set_r((ACType)(serial_number + (i * numCols + j) * scale));
         mat(i, j).set_i(1);
      }
}

template <class T, unsigned M, unsigned N>
int get_ac_width(const ac_matrix<T, M, N>& m)
{
   return m(0, 0).width;
}
template <class T, unsigned M, unsigned N>
int get_ac_i_width(const ac_matrix<T, M, N>& m)
{
   return m(0, 0).i_width;
}

template <class T, unsigned M, unsigned N>
int get_ac_width(const ac_matrix<ac_complex<T>, M, N>& m)
{
   return m(0, 0).real().width;
}
template <class T, unsigned M, unsigned N>
int get_ac_i_width(const ac_matrix<ac_complex<T>, M, N>& m)
{
   return m(0, 0).real().i_width;
}

#define fw 63

// Macro arguments:
//   type1,d1r,d1c - configuration of ac_matrix op1
//   type2,d2r,d2c - configuration of ac_matrix op2
//   rst1s,rst1m - parameters to reset_matrix for op1 (serial numer, multiplier)
//   rst2s,rst2m - parameters to reset_matrix for op2
//   rtype,eR,eC - expected return type and dimensions
//   dbg - non-zero turns on debug dump of input and output matrices

//----------------------------------------------------------------------
// TEST_OP_AUTO - test binary op on two ac_matrix operands of selected
//   types using C++11 "auto" for the result to confirm that the dimensions
//   of the result are correct and the expected bit growth is observed
#define TEST_OP_AUTO(op, type1, d1r, d1c, type2, d2r, d2c, rst1s, rst1m, rst2s, rst2m, rtype, eR, eC, dbg) \
   {                                                                                                       \
      bool flag = true;                                                                                    \
      ac_matrix<type1, d1r, d1c> X;                                                                        \
      ac_matrix<type2, d2r, d2c> Y;                                                                        \
      ac_matrix<rtype, eR, eC> Zmanual;                                                                    \
      reset_matrix(X, d1c, rst1s, rst1m);                                                                  \
      reset_matrix(Y, d2c, rst2s, rst2m);                                                                  \
      if(dbg)                                                                                              \
         cout << X << endl;                                                                                \
      if(dbg)                                                                                              \
         cout << Y << endl;                                                                                \
      auto Z = X op Y;                                                                                     \
      for(unsigned i = 0; i < eR; i++)                                                                     \
         for(unsigned j = 0; j < eC; j++)                                                                  \
            if(Z(i, j) != X(i, j) op Y(i, j))                                                              \
            {                                                                                              \
               flag = false;                                                                               \
            }                                                                                              \
      if(get_ac_width(Z) != get_ac_width(Zmanual))                                                         \
         flag = false;                                                                                     \
      if(get_ac_i_width(Z) != get_ac_i_width(Zmanual))                                                     \
         flag = false;                                                                                     \
      if(Z.rows != Zmanual.rows)                                                                           \
         flag = false;                                                                                     \
      if(Z.cols != Zmanual.cols)                                                                           \
         flag = false;                                                                                     \
      cout << "Testing: auto Z = " << std::left << setw(fw) << X.type_name() << " ";                       \
      cout << STRMACRO2(op) << "         " << std::left << setw(fw) << Y.type_name() << " RESULT: ";       \
      if(flag == false)                                                                                    \
      {                                                                                                    \
         cout << "FAILED" << endl;                                                                         \
         fail_count++;                                                                                     \
      }                                                                                                    \
      else                                                                                                 \
      {                                                                                                    \
         cout << "PASSED" << endl;                                                                         \
      }                                                                                                    \
      if(dbg)                                                                                              \
         cout << Z << endl;                                                                                \
   }

//----------------------------------------------------------------------
// TEST_OP_MANUAL - test binary op on two ac_matrix operands of selected
//   types using C++11 "auto" for the result to confirm that the dimensions
//   of the result are correct and the expected bit growth is observed
#define TEST_OP_MANUAL(op, type1, d1r, d1c, type2, d2r, d2c, rst1s, rst1m, rst2s, rst2m, rtype, eR, eC, dbg) \
   {                                                                                                         \
      bool flag = true;                                                                                      \
      ac_matrix<type1, d1r, d1c> X;                                                                          \
      ac_matrix<type2, d2r, d2c> Y;                                                                          \
      ac_matrix<rtype, eR, eC> Zmanual;                                                                      \
      reset_matrix(X, d1c, rst1s, rst1m);                                                                    \
      reset_matrix(Y, d2c, rst2s, rst2m);                                                                    \
      if(dbg)                                                                                                \
         cout << X << endl;                                                                                  \
      if(dbg)                                                                                                \
         cout << Y << endl;                                                                                  \
      Zmanual = X op Y;                                                                                      \
      for(unsigned i = 0; i < eR; i++)                                                                       \
         for(unsigned j = 0; j < eC; j++)                                                                    \
            if(Zmanual(i, j) != X(i, j) op Y(i, j))                                                          \
            {                                                                                                \
               flag = false;                                                                                 \
            }                                                                                                \
      cout << "Testing:      Z = " << std::left << setw(fw) << X.type_name() << " ";                         \
      cout << STRMACRO2(op) << "         " << std::left << setw(fw) << Y.type_name() << " RESULT: ";         \
      if(flag == false)                                                                                      \
      {                                                                                                      \
         cout << "FAILED" << endl;                                                                           \
         fail_count++;                                                                                       \
      }                                                                                                      \
      else                                                                                                   \
      {                                                                                                      \
         cout << "PASSED" << endl;                                                                           \
      }                                                                                                      \
      if(dbg)                                                                                                \
         cout << Zmanual << endl;                                                                            \
   }

//----------------------------------------------------------------------
// TEST_OPEQ_MANUAL - test binary op on two ac_matrix operands of selected
//   types using C++11 "auto" for the result to confirm that the dimensions
//   of the result are correct and the expected bit growth is observed
#define TEST_OPEQ_MANUAL(op, type1, d1r, d1c, type2, d2r, d2c, rst1s, rst1m, rst2s, rst2m, dbg)     \
   {                                                                                                \
      bool flag = true;                                                                             \
      ac_matrix<type1, d1r, d1c> X;                                                                 \
      ac_matrix<type2, d2r, d2c> Y;                                                                 \
      reset_matrix(X, d1c, rst1s, rst1m);                                                           \
      reset_matrix(Y, d2c, rst2s, rst2m);                                                           \
      if(dbg)                                                                                       \
         cout << X << endl;                                                                         \
      if(dbg)                                                                                       \
         cout << Y << endl;                                                                         \
      X op Y;                                                                                       \
      for(unsigned i = 0; i < d1r; i++)                                                             \
      {                                                                                             \
         for(unsigned j = 0; j < d1c; j++)                                                          \
         {                                                                                          \
            type1 tmpX;                                                                             \
            SET_VAL(tmpX, (rst1s + (i * d1c + j) * rst1m));                                         \
            type2 tmpY;                                                                             \
            SET_VAL(tmpY, (rst2s + (i * d2c + j) * rst2m));                                         \
            tmpX op tmpY;                                                                           \
            if(X(i, j) != tmpX)                                                                     \
            {                                                                                       \
               flag = false;                                                                        \
            }                                                                                       \
         }                                                                                          \
      }                                                                                             \
      cout << "Testing:      Z = " << std::left << setw(fw) << X.type_name() << " ";                \
      cout << STRMACRO2(op) << "        " << std::left << setw(fw) << Y.type_name() << " RESULT: "; \
      if(flag == false)                                                                             \
      {                                                                                             \
         cout << "FAILED" << endl;                                                                  \
         fail_count++;                                                                              \
      }                                                                                             \
      else                                                                                          \
      {                                                                                             \
         cout << "PASSED" << endl;                                                                  \
      }                                                                                             \
      if(dbg)                                                                                       \
         cout << X << endl;                                                                         \
   }

//----------------------------------------------------------------------
// TEST_FUNC_AUTO - test binary op on two ac_matrix operands of selected
//   types using C++11 "auto" for the result to confirm that the dimensions
//   of the result are correct and the expected bit growth is observed
#define TEST_FUNC_AUTO(func, op, type1, d1r, d1c, type2, d2r, d2c, rst1s, rst1m, rst2s, rst2m, rtype, eR, eC, dbg) \
   {                                                                                                               \
      bool flag = true;                                                                                            \
      ac_matrix<type1, d1r, d1c> X;                                                                                \
      ac_matrix<type2, d2r, d2c> Y;                                                                                \
      ac_matrix<rtype, eR, eC> Zmanual;                                                                            \
      reset_matrix(X, d1c, rst1s, rst1m);                                                                          \
      reset_matrix(Y, d2c, rst2s, rst2m);                                                                          \
      if(dbg)                                                                                                      \
         cout << X << endl;                                                                                        \
      if(dbg)                                                                                                      \
         cout << Y << endl;                                                                                        \
      auto Z = X.func(Y);                                                                                          \
      for(unsigned i = 0; i < eR; i++)                                                                             \
         for(unsigned j = 0; j < eC; j++)                                                                          \
            if(Z(i, j) != X(i, j) op Y(i, j))                                                                      \
            {                                                                                                      \
               flag = false;                                                                                       \
            }                                                                                                      \
      if(get_ac_width(Z) != get_ac_width(Zmanual))                                                                 \
         flag = false;                                                                                             \
      if(get_ac_i_width(Z) != get_ac_i_width(Zmanual))                                                             \
         flag = false;                                                                                             \
      if(Z.rows != Zmanual.rows)                                                                                   \
         flag = false;                                                                                             \
      if(Z.cols != Zmanual.cols)                                                                                   \
         flag = false;                                                                                             \
      cout << "Testing: auto Z = " << std::left << setw(fw) << X.type_name() << " ";                               \
      cout << STRMACRO2(func) << " " << std::left << setw(fw) << Y.type_name() << " RESULT: ";                     \
      if(flag == false)                                                                                            \
      {                                                                                                            \
         cout << "FAILED" << endl;                                                                                 \
         fail_count++;                                                                                             \
      }                                                                                                            \
      else                                                                                                         \
      {                                                                                                            \
         cout << "PASSED" << endl;                                                                                 \
      }                                                                                                            \
      if(dbg)                                                                                                      \
         cout << Z << endl;                                                                                        \
   }

//----------------------------------------------------------------------
// TEST_MATMULT_AUTO - test matrix multiplication on two ac_matrix operands of selected
//   types using C++11 "auto" for the result to confirm that the dimensions
//   of the result are correct and the expected bit growth is observed
#define TEST_MATMULT_AUTO(ref, type1, d1r, d1c, type2, d2r, d2c, rst1s, rst1m, rst2s, rst2m, rtype, eR, eC, dbg) \
   {                                                                                                             \
      bool flag = true;                                                                                          \
      ac_matrix<type1, d1r, d1c> A;                                                                              \
      ac_matrix<type2, d2r, d2c> B;                                                                              \
      ac_matrix<rtype, eR, eC> Zmanual;                                                                          \
      reset_matrix(A, d1c, rst1s, rst1m);                                                                        \
      reset_matrix(B, d2c, rst2s, rst2m);                                                                        \
      if(dbg)                                                                                                    \
         cout << A << endl;                                                                                      \
      if(dbg)                                                                                                    \
         cout << B << endl;                                                                                      \
      auto Z = A * B;                                                                                            \
      if(get_ac_width(Z) != get_ac_width(Zmanual))                                                               \
         flag = false;                                                                                           \
      if(get_ac_i_width(Z) != get_ac_i_width(Zmanual))                                                           \
         flag = false;                                                                                           \
      if(Z.rows != Zmanual.rows)                                                                                 \
         flag = false;                                                                                           \
      if(Z.cols != Zmanual.cols)                                                                                 \
         flag = false;                                                                                           \
      for(unsigned i = 0; i < eR; i++)                                                                           \
         for(unsigned j = 0; j < eC; j++)                                                                        \
            if(Z(i, j) != ref(i, j))                                                                             \
            {                                                                                                    \
               flag = false;                                                                                     \
            }                                                                                                    \
      cout << "Testing: auto Z = " << std::left << setw(fw) << A.type_name() << " ";                             \
      cout << "*         " << std::left << setw(fw) << B.type_name() << " RESULT: ";                             \
      if(flag == false)                                                                                          \
      {                                                                                                          \
         cout << "FAILED" << endl;                                                                               \
         fail_count++;                                                                                           \
      }                                                                                                          \
      else                                                                                                       \
      {                                                                                                          \
         cout << "PASSED" << endl;                                                                               \
      }                                                                                                          \
      if(dbg)                                                                                                    \
         cout << Z << endl;                                                                                      \
   }

int main(int argc, char* argv[])
{
   int fail_count = 0;

   // reference for matrix multiply
   ac_matrix<ACTYPE, 4, 4> ref1;
   ref1(0, 0) = 175;
   ref1(0, 1) = 190;
   ref1(0, 2) = 205;
   ref1(0, 3) = 220;
   ref1(1, 0) = 400;
   ref1(1, 1) = 440;
   ref1(1, 2) = 480;
   ref1(1, 3) = 520;
   ref1(2, 0) = 625;
   ref1(2, 1) = 690;
   ref1(2, 2) = 755;
   ref1(2, 3) = 820;
   ref1(3, 0) = 850;
   ref1(3, 1) = 940;
   ref1(3, 2) = 1030;
   ref1(3, 3) = 1120;

   // reference for complex matrix multiply
   ac_matrix<cACTYPE, 4, 4> ref2;
   ref2(0, 0).set_r(170);
   ref2(0, 0).set_i(60);
   ref2(0, 1).set_r(185);
   ref2(0, 1).set_i(65);
   ref2(0, 2).set_r(200);
   ref2(0, 2).set_i(70);
   ref2(0, 3).set_r(215);
   ref2(0, 3).set_i(75);
   ref2(1, 0).set_r(395);
   ref2(1, 0).set_i(85);
   ref2(1, 1).set_r(435);
   ref2(1, 1).set_i(90);
   ref2(1, 2).set_r(475);
   ref2(1, 2).set_i(95);
   ref2(1, 3).set_r(515);
   ref2(1, 3).set_i(100);
   ref2(2, 0).set_r(620);
   ref2(2, 0).set_i(110);
   ref2(2, 1).set_r(685);
   ref2(2, 1).set_i(115);
   ref2(2, 2).set_r(750);
   ref2(2, 2).set_i(120);
   ref2(2, 3).set_r(815);
   ref2(2, 3).set_i(125);
   ref2(3, 0).set_r(845);
   ref2(3, 0).set_i(135);
   ref2(3, 1).set_r(935);
   ref2(3, 1).set_i(140);
   ref2(3, 2).set_r(1025);
   ref2(3, 2).set_i(145);
   ref2(3, 3).set_r(1115);
   ref2(3, 3).set_i(150);

   cout << "=============================================================================" << endl;
   cout << "Test class: ac_matrix" << endl;
   cout << endl;

   TEST_OP_AUTO(+, ACTYPE, 4, 5, ACTYPE, 4, 5, 5, 100, 5, 200, ACTYPEplusACTYPE, 4, 5, false)
   TEST_OP_AUTO(+, ACTYPE, 4, 5, S16TYPE, 4, 5, 5, 100, 5, 200, ACTYPEplusS16TYPE, 4, 5, false)
   TEST_OP_AUTO(-, ACTYPE, 4, 5, ACTYPE, 4, 5, 5, 100, 5, 200, ACTYPEplusACTYPE, 4, 5, false)
   TEST_OP_AUTO(-, ACTYPE, 4, 5, S16TYPE, 4, 5, 5, 100, 5, 200, ACTYPEplusS16TYPE, 4, 5, false)

   TEST_OP_MANUAL(+, ACTYPE, 4, 5, ACTYPE, 4, 5, 5, 100, 5, 200, ACTYPEplusACTYPE, 4, 5, false)
   TEST_OP_MANUAL(+, ACTYPE, 4, 5, S16TYPE, 4, 5, 5, 100, 5, 200, ACTYPEplusS16TYPE, 4, 5, false)
   TEST_OP_MANUAL(-, ACTYPE, 4, 5, ACTYPE, 4, 5, 5, 100, 5, 200, ACTYPEplusACTYPE, 4, 5, false)
   TEST_OP_MANUAL(-, ACTYPE, 4, 5, S16TYPE, 4, 5, 5, 100, 5, 200, ACTYPEplusS16TYPE, 4, 5, false)

   TEST_OPEQ_MANUAL(+=, ACTYPE, 4, 5, ACTYPE, 4, 5, 5, 100, 5, 200, false)
   //    TEST_OPEQ_MANUAL(+=,     ACTYPE,4,5,     S16TYPE,4,5,     5,100,    5,200,                               false)
   //    // not supported
   TEST_OPEQ_MANUAL(-=, ACTYPE, 4, 5, ACTYPE, 4, 5, 5, 100, 5, 200, false)
   //    TEST_OPEQ_MANUAL(-=,     ACTYPE,4,5,     S16TYPE,4,5,     5,100,    5,200,                               false)
   //    // not supported

   TEST_FUNC_AUTO(pwisemult, *, ACTYPE, 4, 5, ACTYPE, 4, 5, 5, 100, 5, 200, ACTYPEmultACTYPE, 4, 5, false)

   TEST_MATMULT_AUTO(ref1, ACTYPE, 4, 5, ACTYPE, 5, 4, 1, 1, 1, 1, ACTYPEmultACTYPE, 4, 4, false)

   TEST_OP_AUTO(+, cACTYPE, 4, 5, cACTYPE, 4, 5, 5, 100, 5, 200, cACTYPEplusACTYPE, 4, 5, false)
   TEST_OP_AUTO(+, cACTYPE, 4, 5, cS16TYPE, 4, 5, 5, 100, 5, 200, cACTYPEplusS16TYPE, 4, 5, false)
   TEST_OP_AUTO(-, cACTYPE, 4, 5, cACTYPE, 4, 5, 5, 100, 5, 200, cACTYPEplusACTYPE, 4, 5, false)
   TEST_OP_AUTO(-, cACTYPE, 4, 5, cS16TYPE, 4, 5, 5, 100, 5, 200, cACTYPEplusS16TYPE, 4, 5, false)

   TEST_OP_MANUAL(+, cACTYPE, 4, 5, cACTYPE, 4, 5, 5, 100, 5, 200, cACTYPEplusACTYPE, 4, 5, false)
   TEST_OP_MANUAL(+, cACTYPE, 4, 5, cS16TYPE, 4, 5, 5, 100, 5, 200, cACTYPEplusS16TYPE, 4, 5, false)
   TEST_OP_MANUAL(-, cACTYPE, 4, 5, cACTYPE, 4, 5, 5, 100, 5, 200, cACTYPEplusACTYPE, 4, 5, false)
   TEST_OP_MANUAL(-, cACTYPE, 4, 5, cS16TYPE, 4, 5, 5, 100, 5, 200, cACTYPEplusS16TYPE, 4, 5, false)

   TEST_OPEQ_MANUAL(+=, cACTYPE, 4, 5, cACTYPE, 4, 5, 5, 100, 5, 200, false)
   //    TEST_OPEQ_MANUAL(+=,    cACTYPE,4,5,    cS16TYPE,4,5,     5,100,    5,200,                               false)
   //    // not supported
   TEST_OPEQ_MANUAL(-=, cACTYPE, 4, 5, cACTYPE, 4, 5, 5, 100, 5, 200, false)
   //    TEST_OPEQ_MANUAL(-=,    cACTYPE,4,5,    cS16TYPE,4,5,     5,100,    5,200,                               false)
   //    // not supported

   TEST_FUNC_AUTO(pwisemult, *, cACTYPE, 4, 5, cACTYPE, 4, 5, 5, 100, 5, 200, cACTYPEmultACTYPE, 4, 5, false)

   TEST_MATMULT_AUTO(ref2, cACTYPE, 4, 5, cACTYPE, 5, 4, 1, 1, 1, 1, cACTYPEmultACTYPE, 4, 4, false)
   if(fail_count)
   {
      cout << "    Error: One or more unit tests failed" << endl;
   }
   else
   {
      cout << "    All unit tests passed" << endl;
   }
   return fail_count;
}
