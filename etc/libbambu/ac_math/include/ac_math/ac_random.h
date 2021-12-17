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
/*! /file This file provides random number generators ac_random(T&)
 *  for all types T Catapult considers as primitive types. These
 *  functions are used in the automatically generated testbench.
 */

#include <limits.h>
#include <stdlib.h>

#ifndef AC_RANDOM_H_INC
#define AC_RANDOM_H_INC

// Make sure that this library does not get synthesized by throwing a compilation error if synthesis directive is
// defined
#ifdef __BAMBU__
#error Synthesis directive defined for ac_random.h
#endif

// Check for macro definitions that will conflict with template parameter names in this file
#if defined(T)
#define T 0
#error The macro name 'T' conflicts with a Template Parameter name.
#error The compiler should have produced a warning about redefinition of 'T' giving the location of the previous definition.
#endif
#if defined(size)
#define size 0
#error The macro name 'size' conflicts with a Template Parameter name.
#error The compiler should have produced a warning about redefinition of 'size' giving the location of the previous definition.
#endif
#if defined(SIZE)
#define SIZE 0
#error The macro name 'SIZE' conflicts with a Template Parameter name.
#error The compiler should have produced a warning about redefinition of 'SIZE' giving the location of the previous definition.
#endif
#if defined(SMALL)
#define SMALL 0
#error The macro name 'SMALL' conflicts with a Template Parameter name.
#error The compiler should have produced a warning about redefinition of 'SMALL' giving the location of the previous definition.
#endif
#if defined(W)
#define W 0
#error The macro name 'W' conflicts with a Template Parameter name.
#error The compiler should have produced a warning about redefinition of 'W' giving the location of the previous definition.
#endif
#if defined(S)
#define S 0
#error The macro name 'S' conflicts with a Template Parameter name.
#error The compiler should have produced a warning about redefinition of 'S' giving the location of the previous definition.
#endif
#if defined(I)
#define I 0
#error The macro name 'I' conflicts with a Template Parameter name.
#error The compiler should have produced a warning about redefinition of 'I' giving the location of the previous definition.
#endif
#if defined(Q)
#define Q 0
#error The macro name 'Q' conflicts with a Template Parameter name.
#error The compiler should have produced a warning about redefinition of 'Q' giving the location of the previous definition.
#endif
#if defined(O)
#define O 0
#error The macro name 'O' conflicts with a Template Parameter name.
#error The compiler should have produced a warning about redefinition of 'O' giving the location of the previous definition.
#endif

template <typename T, int size>
struct ac_random_c_builtin_s
{
   void operator()(T& v);
};

template <typename T, int size>
void ac_random_c_builtin_s<T, size>::operator()(T& v)
{
   const int long mask = 0xffff;
   v = (rand() & mask);
   for(int i = 1; i < size; ++i)
   {
      v <<= 16;
      v |= (rand() & mask);
   }
}

template <typename T>
struct ac_random_c_builtin_s<T, 0>
{
   void operator()(T& v)
   {
      v = rand();
   }
};

template <typename T>
struct ac_random_c_builtin_s<T, 1>
{
   void operator()(T& v)
   {
      v = rand();
   }
};

template <typename T>
inline void ac_random_c_builtin(T& v)
{
   enum
   {
      size = (sizeof(T) * CHAR_BIT) / 16
   };
   ac_random_c_builtin_s<T, size>()(v);
}

inline void ac_random(long long int& v)
{
   ac_random_c_builtin(v);
}
inline void ac_random(long long unsigned& v)
{
   ac_random_c_builtin(v);
}
inline void ac_random(long int& v)
{
   ac_random_c_builtin(v);
}
inline void ac_random(long unsigned& v)
{
   ac_random_c_builtin(v);
}
inline void ac_random(int& v)
{
   ac_random_c_builtin(v);
}
inline void ac_random(unsigned& v)
{
   ac_random_c_builtin(v);
}
inline void ac_random(short int& v)
{
   ac_random_c_builtin(v);
}
inline void ac_random(short unsigned& v)
{
   ac_random_c_builtin(v);
}
inline void ac_random(char& v)
{
   ac_random_c_builtin(v);
}
inline void ac_random(signed char& v)
{
   ac_random_c_builtin(v);
}
inline void ac_random(unsigned char& v)
{
   ac_random_c_builtin(v);
}
inline void ac_random(bool& v)
{
   v = rand() & 1;
}
#endif // AC_RANDOM_H_INC

#if(defined(SYSTEMC_INCLUDED) || defined(SYSTEMC_H)) && !defined(MC_SYSTEMC)
#define AC_SYSTEMC
#endif

#if defined(AC_SYSTEMC) && !defined(AC_RANDOM_H_INC_SYSTEMC_INCLUDED)
#define AC_RANDOM_H_INC_SYSTEMC_INCLUDED
template <typename T>
void ac_systemc_builtin_it(T& v, const int size)
{
   const int long mask = 0xffff;
   int lo = 0;
   int hi = 15;
   for(; hi < size; lo += 16, hi += 16)
   {
      v.range(hi, lo) = (rand() & mask);
   }
   if(lo < size)
   {
      v.range(size - 1, lo) = (rand() & mask);
   }
}

inline void ac_random(sc_int_base& v)
{
   ac_systemc_builtin_it(v, v.length());
}
inline void ac_random(sc_uint_base& v)
{
   ac_systemc_builtin_it(v, v.length());
}
inline void ac_random(sc_signed& v)
{
   ac_systemc_builtin_it(v, v.length());
}
inline void ac_random(sc_unsigned& v)
{
   ac_systemc_builtin_it(v, v.length());
}
template <int Tlen>
inline void ac_random(sc_bv<Tlen>& v)
{
   ac_systemc_builtin_it(v, Tlen);
}
template <int Tlen>
inline void ac_random(sc_lv<Tlen>& v)
{
   ac_systemc_builtin_it(v, Tlen);
}
#endif // AC_RANDOM_H_INC_SYSTEMC_INCLUDED

#if defined(SC_INCLUDE_FX) && defined(AC_SYSTEMC) && !defined(AC_RANDOM_H_INC_SC_INCLUDE_FX)
#define AC_RANDOM_H_INC_SC_INCLUDE_FX

void ac_random(sc_fxnum& v)
{
   ac_systemc_builtin_it(v, v.wl());
}

#endif // AC_RANDOM_H_INC_SC_INCLUDE_FX

#if defined(__AC_INT_H) && !defined(AC_RANDOM_H_INC__AC_INT_H)
#define AC_RANDOM_H_INC__AC_INT_H

template <class T, int SIZE, bool SMALL>
struct ac_random_ac_s
{
   void operator()(T& v);
};

template <class T, int SIZE, bool SMALL>
void ac_random_ac_s<T, SIZE, SMALL>::operator()(T& v)
{
   const ac_int<6> ac_16 = 16;
   ac_int<16> rv = rand();
   v.set_slc(0, rv);
   for(int i = 16; i < SIZE; i += 16)
   {
      v <<= ac_16;
      rv = rand();
      v.set_slc(0, rv);
   }
}

template <class T, int SIZE>
struct ac_random_ac_s<T, SIZE, true>
{
   void operator()(T& v)
   {
      v.set_slc(0, ac_int<SIZE>(rand()));
   }
};

template <int W, bool S>
inline void ac_random(ac_int<W, S>& v)
{
   ac_random_ac_s<ac_int<W, S>, W, W <= 16>()(v);
}

#endif // AC_RANDOM_H_INC__AC_INT_H

#if defined(__AC_FIXED_H) && !defined(AC_RANDOM_H_INC__AC_FIXED_H)
#define AC_RANDOM_H_INC__AC_FIXED_H

template <int W, int I, bool S, ac_q_mode Q, ac_o_mode O>
inline void ac_random(ac_fixed<W, I, S, Q, O>& v)
{
   ac_random_ac_s<ac_fixed<W, I, S, Q, O>, W, W <= 16>()(v);
}

#endif // AC_RANDOM_H_INC__AC_FIXED_H

#if defined(__AC_CHANNEL_H) && !defined(AC_RANDOM_H_INC__AC_CHANNEL_H)
#define AC_RANDOM_H_INC__AC_CHANNEL_H

// Used to extract the sub type of the channel
template <class Tchantype>
struct ac_extract_chan_subtype;
template <class Tsubtype>
struct ac_extract_chan_subtype<ac_channel<Tsubtype>>
{
   typedef Tsubtype chan_subtype;
};

#endif // AC_RANDOM_H_INC__AC_CHANNEL_H
