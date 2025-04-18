/* Soft-FP definitions for bambu.
   Copyright (C) 2014-2024 Politecnico di Milano

This file is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

This file is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */

#include "bambu-arch.h"

#define NO_ASM 1
#define NO_DENORM 1

#define _FP_W_TYPE_SIZE 64
#define _FP_W_TYPE unsigned long long
#define _FP_WS_TYPE signed long long
#define _FP_I_TYPE long long

#define _FP_MUL_MEAT_H(R, X, Y) _FP_MUL_MEAT_1_imm(_FP_WFRACBITS_H, R, X, Y)
#define _FP_MUL_MEAT_S(R, X, Y) _FP_MUL_MEAT_1_imm(_FP_WFRACBITS_S, R, X, Y)
#define _FP_MUL_MEAT_D(R, X, Y) _FP_MUL_MEAT_1_wide(_FP_WFRACBITS_D, R, X, Y, umul_ppmm)
#define _FP_MUL_MEAT_Q(R, X, Y) _FP_MUL_MEAT_2_wide_3mul(_FP_WFRACBITS_Q, R, X, Y, umul_ppmm)

#define _FP_DIV_MEAT_H(R, X, Y) _FP_DIV_MEAT_1_imm(H, R, X, Y, _FP_DIV_HELP_imm)
#define _FP_DIV_MEAT_S(R, X, Y) _FP_DIV_MEAT_1_imm(S, R, X, Y, _FP_DIV_HELP_imm)
#define _FP_DIV_MEAT_D(R, X, Y) _FP_DIV_MEAT_1_udiv_norm(D, R, X, Y)
#define _FP_DIV_MEAT_Q(R, X, Y) _FP_DIV_MEAT_2_udiv(Q, R, X, Y)

#define _FP_NANFRAC_H ((_FP_QNANBIT_H << 1) - 1)
#define _FP_NANFRAC_S ((_FP_QNANBIT_S << 1) - 1)
#define _FP_NANFRAC_D ((_FP_QNANBIT_D << 1) - 1), -1
#define _FP_NANFRAC_Q ((_FP_QNANBIT_Q << 1) - 1), -1, -1, -1
#define _FP_NANSIGN_H 0
#define _FP_NANSIGN_S 0
#define _FP_NANSIGN_D 0
#define _FP_NANSIGN_Q 0

#define _FP_KEEPNANFRACP 1
#define _FP_QNANNEGATEDP 0

/* Someone please check this.  */
#define _FP_CHOOSENAN(fs, wc, R, X, Y, OP)                                                                  \
   do                                                                                                       \
   {                                                                                                        \
      if((_FP_FRAC_HIGH_RAW_##fs(X) & _FP_QNANBIT_##fs) && !(_FP_FRAC_HIGH_RAW_##fs(Y) & _FP_QNANBIT_##fs)) \
      {                                                                                                     \
         R##_s = Y##_s;                                                                                     \
         _FP_FRAC_COPY_##wc(R, Y);                                                                          \
      }                                                                                                     \
      else                                                                                                  \
      {                                                                                                     \
         R##_s = X##_s;                                                                                     \
         _FP_FRAC_COPY_##wc(R, X);                                                                          \
      }                                                                                                     \
      R##_c = FP_CLS_NAN;                                                                                   \
   } while(0)

/* Not checked.  */
#define _FP_TININESS_AFTER_ROUNDING 0

#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321

/* Define ALIASNAME as a strong alias for NAME.  */
#define strong_alias(name, aliasname) _strong_alias(name, aliasname)
#ifdef __APPLE__
#define _strong_alias(name, aliasname)
#else
#define _strong_alias(name, aliasname) extern __typeof(name) aliasname __attribute__((alias(#name)));
#endif
