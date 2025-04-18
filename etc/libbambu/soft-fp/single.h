/* Software floating-point emulation.
   Definitions for IEEE Single Precision.
   Copyright (C) 1997-2023 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   In addition to the permissions in the GNU Lesser General Public
   License, the Free Software Foundation gives you unlimited
   permission to link the compiled version of this file into
   combinations with other programs, and to distribute those
   combinations without any restriction coming from the use of this
   file.  (The Lesser General Public License restrictions do apply in
   other respects; for example, they cover modification of the file,
   and distribution when not linked into a combine executable.)

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#ifndef SOFT_FP_SINGLE_H
#define SOFT_FP_SINGLE_H 1

#if _FP_W_TYPE_SIZE < 32
#error "Here's a nickel kid.  Go buy yourself a real computer."
#endif

#define _FP_FRACTBITS_S _FP_W_TYPE_SIZE

#if _FP_W_TYPE_SIZE < 64
#define _FP_FRACTBITS_DW_S (2 * _FP_W_TYPE_SIZE)
#else
#define _FP_FRACTBITS_DW_S _FP_W_TYPE_SIZE
#endif

#define _FP_FRACBITS_S 24
#define _FP_FRACXBITS_S (_FP_FRACTBITS_S - _FP_FRACBITS_S)
#define _FP_WFRACBITS_S (_FP_WORKBITS + _FP_FRACBITS_S)
#define _FP_WFRACXBITS_S (_FP_FRACTBITS_S - _FP_WFRACBITS_S)
#define _FP_EXPBITS_S 8
#define _FP_EXPBIAS_S 127
#define _FP_EXPMAX_S 255
#define _FP_QNANBIT_S ((_FP_W_TYPE)1 << (_FP_FRACBITS_S - 2))
#define _FP_QNANBIT_SH_S ((_FP_W_TYPE)1 << (_FP_FRACBITS_S - 2 + _FP_WORKBITS))
#define _FP_IMPLBIT_S ((_FP_W_TYPE)1 << (_FP_FRACBITS_S - 1))
#define _FP_IMPLBIT_SH_S ((_FP_W_TYPE)1 << (_FP_FRACBITS_S - 1 + _FP_WORKBITS))
#define _FP_OVERFLOW_S ((_FP_W_TYPE)1 << (_FP_WFRACBITS_S))

#define _FP_WFRACBITS_DW_S (2 * _FP_WFRACBITS_S)
#define _FP_WFRACXBITS_DW_S (_FP_FRACTBITS_DW_S - _FP_WFRACBITS_DW_S)
#define _FP_HIGHBIT_DW_S ((_FP_W_TYPE)1 << (_FP_WFRACBITS_DW_S - 1) % _FP_W_TYPE_SIZE)

/* The implementation of _FP_MUL_MEAT_S and _FP_DIV_MEAT_S should be
   chosen by the target machine.  */

typedef float SFtype __attribute__((mode(SF)));

union _FP_UNION_S
{
   SFtype flt;
   unsigned coded;
   struct _FP_STRUCT_LAYOUT
   {
#if __BYTE_ORDER == __BIG_ENDIAN
      unsigned sign : 1;
      unsigned exp : _FP_EXPBITS_S;
      unsigned frac : _FP_FRACBITS_S - (_FP_IMPLBIT_S != 0);
#else
      unsigned frac : _FP_FRACBITS_S - (_FP_IMPLBIT_S != 0);
      unsigned exp : _FP_EXPBITS_S;
      unsigned sign : 1;
#endif
   } bits;
};

#define FP_DECL_S(X) _FP_DECL(1, X)
#define FP_UNPACK_RAW_S(X, val) _FP_UNPACK_RAW_1(S, X, (val))
#define FP_UNPACK_RAW_SP(X, val) _FP_UNPACK_RAW_1_P(S, X, (val))
#define FP_PACK_RAW_S(val, X) _FP_PACK_RAW_1(S, (val), X)
#define FP_PACK_RAW_SP(val, X)          \
   do                                   \
   {                                    \
      if(!FP_INHIBIT_RESULTS)           \
         _FP_PACK_RAW_1_P(S, (val), X); \
   } while(0)

#define FP_UNPACK_S(X, val)          \
   do                                \
   {                                 \
      _FP_UNPACK_RAW_1(S, X, (val)); \
      _FP_UNPACK_CANONICAL(S, 1, X); \
   } while(0)

#define FP_UNPACK_SP(X, val)           \
   do                                  \
   {                                   \
      _FP_UNPACK_RAW_1_P(S, X, (val)); \
      _FP_UNPACK_CANONICAL(S, 1, X);   \
   } while(0)

#define FP_UNPACK_SEMIRAW_S(X, val)  \
   do                                \
   {                                 \
      _FP_UNPACK_RAW_1(S, X, (val)); \
      _FP_UNPACK_SEMIRAW(S, 1, X);   \
   } while(0)

#define FP_UNPACK_SEMIRAW_SP(X, val)   \
   do                                  \
   {                                   \
      _FP_UNPACK_RAW_1_P(S, X, (val)); \
      _FP_UNPACK_SEMIRAW(S, 1, X);     \
   } while(0)

#define FP_PACK_S(val, X)          \
   do                              \
   {                               \
      _FP_PACK_CANONICAL(S, 1, X); \
      _FP_PACK_RAW_1(S, (val), X); \
   } while(0)

#define FP_PACK_SP(val, X)              \
   do                                   \
   {                                    \
      _FP_PACK_CANONICAL(S, 1, X);      \
      if(!FP_INHIBIT_RESULTS)           \
         _FP_PACK_RAW_1_P(S, (val), X); \
   } while(0)

#define FP_PACK_SEMIRAW_S(val, X)  \
   do                              \
   {                               \
      _FP_PACK_SEMIRAW(S, 1, X);   \
      _FP_PACK_RAW_1(S, (val), X); \
   } while(0)

#define FP_PACK_SEMIRAW_SP(val, X)      \
   do                                   \
   {                                    \
      _FP_PACK_SEMIRAW(S, 1, X);        \
      if(!FP_INHIBIT_RESULTS)           \
         _FP_PACK_RAW_1_P(S, (val), X); \
   } while(0)

#define FP_ISSIGNAN_S(X) _FP_ISSIGNAN(S, 1, X)
#define FP_NEG_S(R, X) _FP_NEG(S, 1, R, X)
#define FP_ADD_S(R, X, Y) _FP_ADD(S, 1, R, X, Y)
#define FP_SUB_S(R, X, Y) _FP_SUB(S, 1, R, X, Y)
#define FP_MUL_S(R, X, Y) _FP_MUL(S, 1, R, X, Y)
#define FP_DIV_S(R, X, Y) _FP_DIV(S, 1, R, X, Y)
#define FP_SQRT_S(R, X) _FP_SQRT(S, 1, R, X)
#define _FP_SQRT_MEAT_S(R, S, T, X, Q) _FP_SQRT_MEAT_1(R, S, T, X, (Q))

#if _FP_W_TYPE_SIZE < 64
#define FP_FMA_S(R, X, Y, Z) _FP_FMA(S, 1, 2, R, X, Y, Z)
#else
#define FP_FMA_S(R, X, Y, Z) _FP_FMA(S, 1, 1, R, X, Y, Z)
#endif

#define FP_CMP_S(r, X, Y, un, ex) _FP_CMP(S, 1, (r), X, Y, (un), (ex))
#define FP_CMP_EQ_S(r, X, Y, ex) _FP_CMP_EQ(S, 1, (r), X, Y, (ex))
#define FP_CMP_UNORD_S(r, X, Y, ex) _FP_CMP_UNORD(S, 1, (r), X, Y, (ex))

#define FP_TO_INT_S(r, X, rsz, rsg) _FP_TO_INT(S, 1, (r), X, (rsz), (rsg))
#define FP_TO_INT_ROUND_S(r, X, rsz, rsg) _FP_TO_INT_ROUND(S, 1, (r), X, (rsz), (rsg))
#define FP_FROM_INT_S(X, r, rs, rt) _FP_FROM_INT(S, 1, X, (r), (rs), rt)

#define _FP_FRAC_HIGH_S(X) _FP_FRAC_HIGH_1(X)
#define _FP_FRAC_HIGH_RAW_S(X) _FP_FRAC_HIGH_1(X)

#if _FP_W_TYPE_SIZE < 64
#define _FP_FRAC_HIGH_DW_S(X) _FP_FRAC_HIGH_2(X)
#else
#define _FP_FRAC_HIGH_DW_S(X) _FP_FRAC_HIGH_1(X)
#endif

#endif /* !SOFT_FP_SINGLE_H */
