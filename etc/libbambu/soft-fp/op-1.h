/* Software floating-point emulation.
   Basic one-word fraction declaration and manipulation.
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

#ifndef SOFT_FP_OP_1_H
#define SOFT_FP_OP_1_H 1

#define _FP_FRAC_DECL_1(X) _FP_W_TYPE X##_f _FP_ZERO_INIT
#define _FP_FRAC_COPY_1(D, S) (D##_f = S##_f)
#define _FP_FRAC_SET_1(X, I) (X##_f = I)
#define _FP_FRAC_HIGH_1(X) (X##_f)
#define _FP_FRAC_LOW_1(X) (X##_f)
#define _FP_FRAC_WORD_1(X, w) (X##_f)

#define _FP_FRAC_ADDI_1(X, I) (X##_f += I)
#define _FP_FRAC_SLL_1(X, N)                  \
   do                                         \
   {                                          \
      if(__builtin_constant_p(N) && (N) == 1) \
         X##_f += X##_f;                      \
      else                                    \
         X##_f <<= (N);                       \
   } while(0)
#define _FP_FRAC_SRL_1(X, N) (X##_f >>= N)

/* Right shift with sticky-lsb.  */
#define _FP_FRAC_SRST_1(X, S, N, sz) __FP_FRAC_SRST_1(X##_f, S, (N), (sz))
#define _FP_FRAC_SRS_1(X, N, sz) __FP_FRAC_SRS_1(X##_f, (N), (sz))

#define __FP_FRAC_SRST_1(X, S, N, sz)                                                          \
   do                                                                                          \
   {                                                                                           \
      S = (__builtin_constant_p(N) && (N) == 1 ? X & 1 : (X << (_FP_W_TYPE_SIZE - (N))) != 0); \
      X = X >> (N);                                                                            \
   } while(0)

#define __FP_FRAC_SRS_1(X, N, sz) \
   (X = (X >> (N) | (__builtin_constant_p(N) && (N) == 1 ? X & 1 : (X << (_FP_W_TYPE_SIZE - (N))) != 0)))

#define _FP_FRAC_ADD_1(R, X, Y) (R##_f = X##_f + Y##_f)
#define _FP_FRAC_SUB_1(R, X, Y) (R##_f = X##_f - Y##_f)
#define _FP_FRAC_DEC_1(X, Y) (X##_f -= Y##_f)
#define _FP_FRAC_CLZ_1(z, X) __FP_CLZ((z), X##_f)

/* Predicates.  */
#define _FP_FRAC_NEGP_1(X) ((_FP_WS_TYPE)X##_f < 0)
#define _FP_FRAC_ZEROP_1(X) (X##_f == 0)
#define _FP_FRAC_OVERP_1(fs, X) (X##_f & _FP_OVERFLOW_##fs)
#define _FP_FRAC_CLEAR_OVERP_1(fs, X) (X##_f &= ~_FP_OVERFLOW_##fs)
#define _FP_FRAC_HIGHBIT_DW_1(fs, X) (X##_f & _FP_HIGHBIT_DW_##fs)
#define _FP_FRAC_EQ_1(X, Y) (X##_f == Y##_f)
#define _FP_FRAC_GE_1(X, Y) (X##_f >= Y##_f)
#define _FP_FRAC_GT_1(X, Y) (X##_f > Y##_f)

#define _FP_ZEROFRAC_1 0
#define _FP_MINFRAC_1 1
#define _FP_MAXFRAC_1 (~(_FP_WS_TYPE)0)

/* Unpack the raw bits of a native fp value.  Do not classify or
   normalize the data.  */

#define _FP_UNPACK_RAW_1(fs, X, val)             \
   do                                            \
   {                                             \
      union _FP_UNION_##fs _FP_UNPACK_RAW_1_flo; \
      _FP_UNPACK_RAW_1_flo.flt = (val);          \
                                                 \
      X##_f = _FP_UNPACK_RAW_1_flo.bits.frac;    \
      X##_e = _FP_UNPACK_RAW_1_flo.bits.exp;     \
      X##_s = _FP_UNPACK_RAW_1_flo.bits.sign;    \
   } while(0)

#define _FP_UNPACK_RAW_1_P(fs, X, val)                                             \
   do                                                                              \
   {                                                                               \
      union _FP_UNION_##fs* _FP_UNPACK_RAW_1_P_flo = (union _FP_UNION_##fs*)(val); \
                                                                                   \
      X##_f = _FP_UNPACK_RAW_1_P_flo->bits.frac;                                   \
      X##_e = _FP_UNPACK_RAW_1_P_flo->bits.exp;                                    \
      X##_s = _FP_UNPACK_RAW_1_P_flo->bits.sign;                                   \
   } while(0)

/* Repack the raw bits of a native fp value.  */

#define _FP_PACK_RAW_1(fs, val, X)             \
   do                                          \
   {                                           \
      union _FP_UNION_##fs _FP_PACK_RAW_1_flo; \
                                               \
      _FP_PACK_RAW_1_flo.bits.frac = X##_f;    \
      _FP_PACK_RAW_1_flo.bits.exp = X##_e;     \
      _FP_PACK_RAW_1_flo.bits.sign = X##_s;    \
                                               \
      (val) = _FP_PACK_RAW_1_flo.flt;          \
   } while(0)

#define _FP_PACK_RAW_1_P(fs, val, X)                                             \
   do                                                                            \
   {                                                                             \
      union _FP_UNION_##fs* _FP_PACK_RAW_1_P_flo = (union _FP_UNION_##fs*)(val); \
                                                                                 \
      _FP_PACK_RAW_1_P_flo->bits.frac = X##_f;                                   \
      _FP_PACK_RAW_1_P_flo->bits.exp = X##_e;                                    \
      _FP_PACK_RAW_1_P_flo->bits.sign = X##_s;                                   \
   } while(0)

/* Multiplication algorithms: */

/* Basic.  Assuming the host word size is >= 2*FRACBITS, we can do the
   multiplication immediately.  */

#define _FP_MUL_MEAT_DW_1_imm(wfracbits, R, X, Y) \
   do                                             \
   {                                              \
      R##_f = X##_f * Y##_f;                      \
   } while(0)

#define _FP_MUL_MEAT_1_imm(wfracbits, R, X, Y)                      \
   do                                                               \
   {                                                                \
      _FP_MUL_MEAT_DW_1_imm((wfracbits), R, X, Y);                  \
      /* Normalize since we know where the msb of the multiplicands \
    were (bit B), we know that the msb of the of the product is     \
    at either 2B or 2B-1.  */                                       \
      _FP_FRAC_SRS_1(R, (wfracbits)-1, 2 * (wfracbits));            \
   } while(0)

/* Given a 1W * 1W => 2W primitive, do the extended multiplication.  */

#define _FP_MUL_MEAT_DW_1_wide(wfracbits, R, X, Y, doit) \
   do                                                    \
   {                                                     \
      doit(R##_f1, R##_f0, X##_f, Y##_f);                \
   } while(0)

#define _FP_MUL_MEAT_1_wide(wfracbits, R, X, Y, doit)                         \
   do                                                                         \
   {                                                                          \
      _FP_FRAC_DECL_2(_FP_MUL_MEAT_1_wide_Z);                                 \
      _FP_MUL_MEAT_DW_1_wide((wfracbits), _FP_MUL_MEAT_1_wide_Z, X, Y, doit); \
      /* Normalize since we know where the msb of the multiplicands           \
    were (bit B), we know that the msb of the of the product is               \
    at either 2B or 2B-1.  */                                                 \
      _FP_FRAC_SRS_2(_FP_MUL_MEAT_1_wide_Z, (wfracbits)-1, 2 * (wfracbits));  \
      R##_f = _FP_MUL_MEAT_1_wide_Z_f0;                                       \
   } while(0)

/* Finally, a simple widening multiply algorithm.  What fun!  */

#define _FP_MUL_MEAT_DW_1_hard(wfracbits, R, X, Y)                                                   \
   do                                                                                                \
   {                                                                                                 \
      _FP_W_TYPE _FP_MUL_MEAT_DW_1_hard_xh, _FP_MUL_MEAT_DW_1_hard_xl;                               \
      _FP_W_TYPE _FP_MUL_MEAT_DW_1_hard_yh, _FP_MUL_MEAT_DW_1_hard_yl;                               \
      _FP_FRAC_DECL_2(_FP_MUL_MEAT_DW_1_hard_a);                                                     \
                                                                                                     \
      /* Split the words in half.  */                                                                \
      _FP_MUL_MEAT_DW_1_hard_xh = X##_f >> (_FP_W_TYPE_SIZE / 2);                                    \
      _FP_MUL_MEAT_DW_1_hard_xl = X##_f & (((_FP_W_TYPE)1 << (_FP_W_TYPE_SIZE / 2)) - 1);            \
      _FP_MUL_MEAT_DW_1_hard_yh = Y##_f >> (_FP_W_TYPE_SIZE / 2);                                    \
      _FP_MUL_MEAT_DW_1_hard_yl = Y##_f & (((_FP_W_TYPE)1 << (_FP_W_TYPE_SIZE / 2)) - 1);            \
                                                                                                     \
      /* Multiply the pieces.  */                                                                    \
      R##_f0 = _FP_MUL_MEAT_DW_1_hard_xl * _FP_MUL_MEAT_DW_1_hard_yl;                                \
      _FP_MUL_MEAT_DW_1_hard_a_f0 = _FP_MUL_MEAT_DW_1_hard_xh * _FP_MUL_MEAT_DW_1_hard_yl;           \
      _FP_MUL_MEAT_DW_1_hard_a_f1 = _FP_MUL_MEAT_DW_1_hard_xl * _FP_MUL_MEAT_DW_1_hard_yh;           \
      R##_f1 = _FP_MUL_MEAT_DW_1_hard_xh * _FP_MUL_MEAT_DW_1_hard_yh;                                \
                                                                                                     \
      /* Reassemble into two full words.  */                                                         \
      if((_FP_MUL_MEAT_DW_1_hard_a_f0 += _FP_MUL_MEAT_DW_1_hard_a_f1) < _FP_MUL_MEAT_DW_1_hard_a_f1) \
         R##_f1 += (_FP_W_TYPE)1 << (_FP_W_TYPE_SIZE / 2);                                           \
      _FP_MUL_MEAT_DW_1_hard_a_f1 = _FP_MUL_MEAT_DW_1_hard_a_f0 >> (_FP_W_TYPE_SIZE / 2);            \
      _FP_MUL_MEAT_DW_1_hard_a_f0 = _FP_MUL_MEAT_DW_1_hard_a_f0 << (_FP_W_TYPE_SIZE / 2);            \
      _FP_FRAC_ADD_2(R, R, _FP_MUL_MEAT_DW_1_hard_a);                                                \
   } while(0)

#define _FP_MUL_MEAT_1_hard(wfracbits, R, X, Y)                              \
   do                                                                        \
   {                                                                         \
      _FP_FRAC_DECL_2(_FP_MUL_MEAT_1_hard_z);                                \
      _FP_MUL_MEAT_DW_1_hard((wfracbits), _FP_MUL_MEAT_1_hard_z, X, Y);      \
                                                                             \
      /* Normalize.  */                                                      \
      _FP_FRAC_SRS_2(_FP_MUL_MEAT_1_hard_z, (wfracbits)-1, 2 * (wfracbits)); \
      R##_f = _FP_MUL_MEAT_1_hard_z_f0;                                      \
   } while(0)

/* Division algorithms: */

/* Basic.  Assuming the host word size is >= 2*FRACBITS, we can do the
   division immediately.  Give this macro either _FP_DIV_HELP_imm for
   C primitives or _FP_DIV_HELP_ldiv for the ISO function.  Which you
   choose will depend on what the compiler does with divrem4.  */

#define _FP_DIV_MEAT_1_imm(fs, R, X, Y, doit)                                           \
   do                                                                                   \
   {                                                                                    \
      _FP_W_TYPE _FP_DIV_MEAT_1_imm_q, _FP_DIV_MEAT_1_imm_r;                            \
      X##_f <<= (X##_f < Y##_f ? R##_e--, _FP_WFRACBITS_##fs : _FP_WFRACBITS_##fs - 1); \
      doit(_FP_DIV_MEAT_1_imm_q, _FP_DIV_MEAT_1_imm_r, X##_f, Y##_f);                   \
      R##_f = _FP_DIV_MEAT_1_imm_q | (_FP_DIV_MEAT_1_imm_r != 0);                       \
   } while(0)

/* GCC's longlong.h defines a 2W / 1W => (1W,1W) primitive udiv_qrnnd
   that may be useful in this situation.  This first is for a primitive
   that requires normalization, the second for one that does not.  Look
   for UDIV_NEEDS_NORMALIZATION to tell which your machine needs.  */

#define _FP_DIV_MEAT_1_udiv_norm(fs, R, X, Y)                                                         \
   do                                                                                                 \
   {                                                                                                  \
      _FP_W_TYPE _FP_DIV_MEAT_1_udiv_norm_nh;                                                         \
      _FP_W_TYPE _FP_DIV_MEAT_1_udiv_norm_nl;                                                         \
      _FP_W_TYPE _FP_DIV_MEAT_1_udiv_norm_q;                                                          \
      _FP_W_TYPE _FP_DIV_MEAT_1_udiv_norm_r;                                                          \
      _FP_W_TYPE _FP_DIV_MEAT_1_udiv_norm_y;                                                          \
                                                                                                      \
      /* Normalize Y -- i.e. make the most significant bit set.  */                                   \
      _FP_DIV_MEAT_1_udiv_norm_y = Y##_f << _FP_WFRACXBITS_##fs;                                      \
                                                                                                      \
      /* Shift X op correspondingly high, that is, up one full word.  */                              \
      if(X##_f < Y##_f)                                                                               \
      {                                                                                               \
         R##_e--;                                                                                     \
         _FP_DIV_MEAT_1_udiv_norm_nl = 0;                                                             \
         _FP_DIV_MEAT_1_udiv_norm_nh = X##_f;                                                         \
      }                                                                                               \
      else                                                                                            \
      {                                                                                               \
         _FP_DIV_MEAT_1_udiv_norm_nl = X##_f << (_FP_W_TYPE_SIZE - 1);                                \
         _FP_DIV_MEAT_1_udiv_norm_nh = X##_f >> 1;                                                    \
      }                                                                                               \
                                                                                                      \
      udiv_qrnnd(_FP_DIV_MEAT_1_udiv_norm_q, _FP_DIV_MEAT_1_udiv_norm_r, _FP_DIV_MEAT_1_udiv_norm_nh, \
                 _FP_DIV_MEAT_1_udiv_norm_nl, _FP_DIV_MEAT_1_udiv_norm_y);                            \
      R##_f = (_FP_DIV_MEAT_1_udiv_norm_q | (_FP_DIV_MEAT_1_udiv_norm_r != 0));                       \
   } while(0)

#define _FP_DIV_MEAT_1_udiv(fs, R, X, Y)                                                                               \
   do                                                                                                                  \
   {                                                                                                                   \
      _FP_W_TYPE _FP_DIV_MEAT_1_udiv_nh, _FP_DIV_MEAT_1_udiv_nl;                                                       \
      _FP_W_TYPE _FP_DIV_MEAT_1_udiv_q, _FP_DIV_MEAT_1_udiv_r;                                                         \
      if(X##_f < Y##_f)                                                                                                \
      {                                                                                                                \
         R##_e--;                                                                                                      \
         _FP_DIV_MEAT_1_udiv_nl = X##_f << _FP_WFRACBITS_##fs;                                                         \
         _FP_DIV_MEAT_1_udiv_nh = X##_f >> _FP_WFRACXBITS_##fs;                                                        \
      }                                                                                                                \
      else                                                                                                             \
      {                                                                                                                \
         _FP_DIV_MEAT_1_udiv_nl = X##_f << (_FP_WFRACBITS_##fs - 1);                                                   \
         _FP_DIV_MEAT_1_udiv_nh = X##_f >> (_FP_WFRACXBITS_##fs + 1);                                                  \
      }                                                                                                                \
      udiv_qrnnd(_FP_DIV_MEAT_1_udiv_q, _FP_DIV_MEAT_1_udiv_r, _FP_DIV_MEAT_1_udiv_nh, _FP_DIV_MEAT_1_udiv_nl, Y##_f); \
      R##_f = _FP_DIV_MEAT_1_udiv_q | (_FP_DIV_MEAT_1_udiv_r != 0);                                                    \
   } while(0)

/* Square root algorithms:
   We have just one right now, maybe Newton approximation
   should be added for those machines where division is fast.  */

#define _FP_SQRT_MEAT_1(R, S, T, X, q) \
   do                                  \
   {                                   \
      while((q) != _FP_WORK_ROUND)     \
      {                                \
         T##_f = S##_f + (q);          \
         if(T##_f <= X##_f)            \
         {                             \
            S##_f = T##_f + (q);       \
            X##_f -= T##_f;            \
            R##_f += (q);              \
         }                             \
         _FP_FRAC_SLL_1(X, 1);         \
         (q) >>= 1;                    \
      }                                \
      if(X##_f)                        \
      {                                \
         if(S##_f < X##_f)             \
            R##_f |= _FP_WORK_ROUND;   \
         R##_f |= _FP_WORK_STICKY;     \
      }                                \
   } while(0)

/* Assembly/disassembly for converting to/from integral types.
   No shifting or overflow handled here.  */

#define _FP_FRAC_ASSEMBLE_1(r, X, rsize) ((r) = X##_f)
#define _FP_FRAC_DISASSEMBLE_1(X, r, rsize) (X##_f = (r))

/* Convert FP values between word sizes.  */

#define _FP_FRAC_COPY_1_1(D, S) (D##_f = S##_f)

#endif /* !SOFT_FP_OP_1_H */
