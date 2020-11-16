/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (C) 2015-2020 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/**
 * @file bambu_macros.h
 * @brief macros to restrict the bitsize of a variable.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef BAMBU_MACROS_H
#define BAMBU_MACROS_H

#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/arithmetic/mod.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/comparison/greater.hpp>
#include <boost/preprocessor/comparison/greater_equal.hpp>
#include <boost/preprocessor/comparison/less.hpp>
#include <boost/preprocessor/comparison/less_equal.hpp>
#include <boost/preprocessor/comparison/not_equal.hpp>
#include <boost/preprocessor/debug/assert.hpp>
#include <boost/preprocessor/empty.hpp>
#include <boost/preprocessor/if.hpp>
#include <boost/preprocessor/logical.hpp>
#include <boost/preprocessor/repetition/for.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/preprocessor/tuple/elem.hpp>

#define POW2_0 1
#define POW2_1 2
#define POW2_2 4
#define POW2_3 8
#define POW2_4 16
#define POW2_5 32
#define POW2_6 64
#define POW2_7 128
#define POW2_8 256
#define POW2_9 512
#define POW2_10 1024
#define POW2_11 2048
#define POW2_12 4096
#define POW2_13 8192
#define POW2_14 16384
#define POW2_15 32768
#define POW2_16 65536
#define POW2_17 131072
#define POW2_18 262144
#define POW2_19 524288
#define POW2_20 1048576
#define POW2_21 2097152
#define POW2_22 4194304
#define POW2_23 8388608
#define POW2_24 16777216
#define POW2_25 33554432
#define POW2_26 67108864
#define POW2_27 134217728
#define POW2_28 268435456
#define POW2_29 536870912
#define POW2_30 1073741824
#define POW2_31 2147483648
#define POW2_32 4294967296
#define POW2_33 8589934592
#define POW2_34 17179869184
#define POW2_35 34359738368
#define POW2_36 68719476736
#define POW2_37 137438953472
#define POW2_38 274877906944
#define POW2_39 549755813888
#define POW2_40 1099511627776
#define POW2_41 2199023255552
#define POW2_42 4398046511104
#define POW2_43 8796093022208
#define POW2_44 17592186044416
#define POW2_45 35184372088832
#define POW2_46 70368744177664
#define POW2_47 140737488355328
#define POW2_48 281474976710656
#define POW2_49 562949953421312
#define POW2_50 1125899906842624
#define POW2_51 2251799813685248
#define POW2_52 4503599627370496
#define POW2_53 9007199254740992
#define POW2_54 18014398509481984
#define POW2_55 36028797018963968
#define POW2_56 72057594037927936
#define POW2_57 144115188075855872
#define POW2_58 288230376151711744
#define POW2_59 576460752303423488
#define POW2_60 1152921504606846976
#define POW2_61 2305843009213693952
#define POW2_62 4611686018427387904
#define POW2_63 9223372036854775808
#define POW2(exp) BOOST_PP_CAT(POW2_, exp)

#define CEIL_LOG2_0 0
#define CEIL_LOG2_1 0
#define CEIL_LOG2_2 1
#define CEIL_LOG2_3 2
#define CEIL_LOG2_4 2
#define CEIL_LOG2_5 3
#define CEIL_LOG2_6 3
#define CEIL_LOG2_7 3
#define CEIL_LOG2_8 3
#define CEIL_LOG2_9 4
#define CEIL_LOG2_10 4
#define CEIL_LOG2_11 4
#define CEIL_LOG2_12 4
#define CEIL_LOG2_13 4
#define CEIL_LOG2_14 4
#define CEIL_LOG2_15 4
#define CEIL_LOG2_16 4
#define CEIL_LOG2_17 5
#define CEIL_LOG2_18 5
#define CEIL_LOG2_19 5
#define CEIL_LOG2_20 5
#define CEIL_LOG2_21 5
#define CEIL_LOG2_22 5
#define CEIL_LOG2_23 5
#define CEIL_LOG2_24 5
#define CEIL_LOG2_25 5
#define CEIL_LOG2_26 5
#define CEIL_LOG2_27 5
#define CEIL_LOG2_28 5
#define CEIL_LOG2_29 5
#define CEIL_LOG2_30 5
#define CEIL_LOG2_31 5
#define CEIL_LOG2_32 5
#define CEIL_LOG2_33 6
#define CEIL_LOG2_34 6
#define CEIL_LOG2_35 6
#define CEIL_LOG2_36 6
#define CEIL_LOG2_37 6
#define CEIL_LOG2_38 6
#define CEIL_LOG2_39 6
#define CEIL_LOG2_40 6
#define CEIL_LOG2_41 6
#define CEIL_LOG2_42 6
#define CEIL_LOG2_43 6
#define CEIL_LOG2_44 6
#define CEIL_LOG2_45 6
#define CEIL_LOG2_46 6
#define CEIL_LOG2_47 6
#define CEIL_LOG2_48 6
#define CEIL_LOG2_49 6
#define CEIL_LOG2_50 6
#define CEIL_LOG2_51 6
#define CEIL_LOG2_52 6
#define CEIL_LOG2_53 6
#define CEIL_LOG2_54 6
#define CEIL_LOG2_55 6
#define CEIL_LOG2_56 6
#define CEIL_LOG2_57 6
#define CEIL_LOG2_58 6
#define CEIL_LOG2_59 6
#define CEIL_LOG2_60 6
#define CEIL_LOG2_61 6
#define CEIL_LOG2_62 6
#define CEIL_LOG2_63 6
#define CEIL_LOG2_64 6
#define CEIL_LOG2(exp) BOOST_PP_CAT(CEIL_LOG2_, exp)

#define CHECK_N(x, n, ...) n
#define CHECK(...) CHECK_N(__VA_ARGS__, 0, )
#define PROBE(x) x, 1,
#define IS_CONSTANT(x) CHECK(BOOST_PP_CAT(IS_CONSTANT_, x))
#define IS_CONSTANT_0 PROBE(~)
#define IS_CONSTANT_1 PROBE(~)
#define IS_CONSTANT_2 PROBE(~)
#define IS_CONSTANT_3 PROBE(~)
#define IS_CONSTANT_4 PROBE(~)
#define IS_CONSTANT_5 PROBE(~)
#define IS_CONSTANT_6 PROBE(~)
#define IS_CONSTANT_7 PROBE(~)
#define IS_CONSTANT_8 PROBE(~)
#define IS_CONSTANT_9 PROBE(~)
#define IS_CONSTANT_10 PROBE(~)
#define IS_CONSTANT_11 PROBE(~)
#define IS_CONSTANT_12 PROBE(~)
#define IS_CONSTANT_13 PROBE(~)
#define IS_CONSTANT_14 PROBE(~)
#define IS_CONSTANT_15 PROBE(~)
#define IS_CONSTANT_16 PROBE(~)
#define IS_CONSTANT_17 PROBE(~)
#define IS_CONSTANT_18 PROBE(~)
#define IS_CONSTANT_19 PROBE(~)
#define IS_CONSTANT_20 PROBE(~)
#define IS_CONSTANT_21 PROBE(~)
#define IS_CONSTANT_22 PROBE(~)
#define IS_CONSTANT_23 PROBE(~)
#define IS_CONSTANT_24 PROBE(~)
#define IS_CONSTANT_25 PROBE(~)
#define IS_CONSTANT_26 PROBE(~)
#define IS_CONSTANT_27 PROBE(~)
#define IS_CONSTANT_28 PROBE(~)
#define IS_CONSTANT_29 PROBE(~)
#define IS_CONSTANT_30 PROBE(~)
#define IS_CONSTANT_31 PROBE(~)
#define IS_CONSTANT_32 PROBE(~)
#define IS_CONSTANT_33 PROBE(~)
#define IS_CONSTANT_34 PROBE(~)
#define IS_CONSTANT_35 PROBE(~)
#define IS_CONSTANT_36 PROBE(~)
#define IS_CONSTANT_37 PROBE(~)
#define IS_CONSTANT_38 PROBE(~)
#define IS_CONSTANT_39 PROBE(~)
#define IS_CONSTANT_40 PROBE(~)
#define IS_CONSTANT_41 PROBE(~)
#define IS_CONSTANT_42 PROBE(~)
#define IS_CONSTANT_43 PROBE(~)
#define IS_CONSTANT_44 PROBE(~)
#define IS_CONSTANT_45 PROBE(~)
#define IS_CONSTANT_46 PROBE(~)
#define IS_CONSTANT_47 PROBE(~)
#define IS_CONSTANT_48 PROBE(~)
#define IS_CONSTANT_49 PROBE(~)
#define IS_CONSTANT_50 PROBE(~)
#define IS_CONSTANT_51 PROBE(~)
#define IS_CONSTANT_52 PROBE(~)
#define IS_CONSTANT_53 PROBE(~)
#define IS_CONSTANT_54 PROBE(~)
#define IS_CONSTANT_55 PROBE(~)
#define IS_CONSTANT_56 PROBE(~)
#define IS_CONSTANT_57 PROBE(~)
#define IS_CONSTANT_58 PROBE(~)
#define IS_CONSTANT_59 PROBE(~)
#define IS_CONSTANT_60 PROBE(~)
#define IS_CONSTANT_61 PROBE(~)
#define IS_CONSTANT_62 PROBE(~)
#define IS_CONSTANT_63 PROBE(~)
#define IS_CONSTANT_64 PROBE(~)
#define IS_CONSTANT_65 PROBE(~)
#define IS_CONSTANT_66 PROBE(~)
#define IS_CONSTANT_67 PROBE(~)
#define IS_CONSTANT_68 PROBE(~)
#define IS_CONSTANT_69 PROBE(~)
#define IS_CONSTANT_70 PROBE(~)
#define IS_CONSTANT_71 PROBE(~)
#define IS_CONSTANT_72 PROBE(~)
#define IS_CONSTANT_73 PROBE(~)
#define IS_CONSTANT_74 PROBE(~)
#define IS_CONSTANT_75 PROBE(~)
#define IS_CONSTANT_76 PROBE(~)
#define IS_CONSTANT_77 PROBE(~)
#define IS_CONSTANT_78 PROBE(~)
#define IS_CONSTANT_79 PROBE(~)
#define IS_CONSTANT_80 PROBE(~)
#define IS_CONSTANT_81 PROBE(~)
#define IS_CONSTANT_82 PROBE(~)
#define IS_CONSTANT_83 PROBE(~)
#define IS_CONSTANT_84 PROBE(~)
#define IS_CONSTANT_85 PROBE(~)
#define IS_CONSTANT_86 PROBE(~)
#define IS_CONSTANT_87 PROBE(~)
#define IS_CONSTANT_88 PROBE(~)
#define IS_CONSTANT_89 PROBE(~)
#define IS_CONSTANT_90 PROBE(~)
#define IS_CONSTANT_91 PROBE(~)
#define IS_CONSTANT_92 PROBE(~)
#define IS_CONSTANT_93 PROBE(~)
#define IS_CONSTANT_94 PROBE(~)
#define IS_CONSTANT_95 PROBE(~)
#define IS_CONSTANT_96 PROBE(~)
#define IS_CONSTANT_97 PROBE(~)
#define IS_CONSTANT_98 PROBE(~)
#define IS_CONSTANT_99 PROBE(~)
#define IS_CONSTANT_100 PROBE(~)
#define IS_CONSTANT_101 PROBE(~)
#define IS_CONSTANT_102 PROBE(~)
#define IS_CONSTANT_103 PROBE(~)
#define IS_CONSTANT_104 PROBE(~)
#define IS_CONSTANT_105 PROBE(~)
#define IS_CONSTANT_106 PROBE(~)
#define IS_CONSTANT_107 PROBE(~)
#define IS_CONSTANT_108 PROBE(~)
#define IS_CONSTANT_109 PROBE(~)
#define IS_CONSTANT_110 PROBE(~)
#define IS_CONSTANT_111 PROBE(~)
#define IS_CONSTANT_112 PROBE(~)
#define IS_CONSTANT_113 PROBE(~)
#define IS_CONSTANT_114 PROBE(~)
#define IS_CONSTANT_115 PROBE(~)
#define IS_CONSTANT_116 PROBE(~)
#define IS_CONSTANT_117 PROBE(~)
#define IS_CONSTANT_118 PROBE(~)
#define IS_CONSTANT_119 PROBE(~)
#define IS_CONSTANT_120 PROBE(~)
#define IS_CONSTANT_121 PROBE(~)
#define IS_CONSTANT_122 PROBE(~)
#define IS_CONSTANT_123 PROBE(~)
#define IS_CONSTANT_124 PROBE(~)
#define IS_CONSTANT_125 PROBE(~)
#define IS_CONSTANT_126 PROBE(~)
#define IS_CONSTANT_127 PROBE(~)
#define IS_CONSTANT_128 PROBE(~)
#define IS_CONSTANT_129 PROBE(~)
#define IS_CONSTANT_130 PROBE(~)
#define IS_CONSTANT_131 PROBE(~)
#define IS_CONSTANT_132 PROBE(~)
#define IS_CONSTANT_133 PROBE(~)
#define IS_CONSTANT_134 PROBE(~)
#define IS_CONSTANT_135 PROBE(~)
#define IS_CONSTANT_136 PROBE(~)
#define IS_CONSTANT_137 PROBE(~)
#define IS_CONSTANT_138 PROBE(~)
#define IS_CONSTANT_139 PROBE(~)
#define IS_CONSTANT_140 PROBE(~)
#define IS_CONSTANT_141 PROBE(~)
#define IS_CONSTANT_142 PROBE(~)
#define IS_CONSTANT_143 PROBE(~)
#define IS_CONSTANT_144 PROBE(~)
#define IS_CONSTANT_145 PROBE(~)
#define IS_CONSTANT_146 PROBE(~)
#define IS_CONSTANT_147 PROBE(~)
#define IS_CONSTANT_148 PROBE(~)
#define IS_CONSTANT_149 PROBE(~)
#define IS_CONSTANT_150 PROBE(~)
#define IS_CONSTANT_151 PROBE(~)
#define IS_CONSTANT_152 PROBE(~)
#define IS_CONSTANT_153 PROBE(~)
#define IS_CONSTANT_154 PROBE(~)
#define IS_CONSTANT_155 PROBE(~)
#define IS_CONSTANT_156 PROBE(~)
#define IS_CONSTANT_157 PROBE(~)
#define IS_CONSTANT_158 PROBE(~)
#define IS_CONSTANT_159 PROBE(~)
#define IS_CONSTANT_160 PROBE(~)
#define IS_CONSTANT_161 PROBE(~)
#define IS_CONSTANT_162 PROBE(~)
#define IS_CONSTANT_163 PROBE(~)
#define IS_CONSTANT_164 PROBE(~)
#define IS_CONSTANT_165 PROBE(~)
#define IS_CONSTANT_166 PROBE(~)
#define IS_CONSTANT_167 PROBE(~)
#define IS_CONSTANT_168 PROBE(~)
#define IS_CONSTANT_169 PROBE(~)
#define IS_CONSTANT_170 PROBE(~)
#define IS_CONSTANT_171 PROBE(~)
#define IS_CONSTANT_172 PROBE(~)
#define IS_CONSTANT_173 PROBE(~)
#define IS_CONSTANT_174 PROBE(~)
#define IS_CONSTANT_175 PROBE(~)
#define IS_CONSTANT_176 PROBE(~)
#define IS_CONSTANT_177 PROBE(~)
#define IS_CONSTANT_178 PROBE(~)
#define IS_CONSTANT_179 PROBE(~)
#define IS_CONSTANT_180 PROBE(~)
#define IS_CONSTANT_181 PROBE(~)
#define IS_CONSTANT_182 PROBE(~)
#define IS_CONSTANT_183 PROBE(~)
#define IS_CONSTANT_184 PROBE(~)
#define IS_CONSTANT_185 PROBE(~)
#define IS_CONSTANT_186 PROBE(~)
#define IS_CONSTANT_187 PROBE(~)
#define IS_CONSTANT_188 PROBE(~)
#define IS_CONSTANT_189 PROBE(~)
#define IS_CONSTANT_190 PROBE(~)
#define IS_CONSTANT_191 PROBE(~)
#define IS_CONSTANT_192 PROBE(~)
#define IS_CONSTANT_193 PROBE(~)
#define IS_CONSTANT_194 PROBE(~)
#define IS_CONSTANT_195 PROBE(~)
#define IS_CONSTANT_196 PROBE(~)
#define IS_CONSTANT_197 PROBE(~)
#define IS_CONSTANT_198 PROBE(~)
#define IS_CONSTANT_199 PROBE(~)
#define IS_CONSTANT_200 PROBE(~)
#define IS_CONSTANT_201 PROBE(~)
#define IS_CONSTANT_202 PROBE(~)
#define IS_CONSTANT_203 PROBE(~)
#define IS_CONSTANT_204 PROBE(~)
#define IS_CONSTANT_205 PROBE(~)
#define IS_CONSTANT_206 PROBE(~)
#define IS_CONSTANT_207 PROBE(~)
#define IS_CONSTANT_208 PROBE(~)
#define IS_CONSTANT_209 PROBE(~)
#define IS_CONSTANT_210 PROBE(~)
#define IS_CONSTANT_211 PROBE(~)
#define IS_CONSTANT_212 PROBE(~)
#define IS_CONSTANT_213 PROBE(~)
#define IS_CONSTANT_214 PROBE(~)
#define IS_CONSTANT_215 PROBE(~)
#define IS_CONSTANT_216 PROBE(~)
#define IS_CONSTANT_217 PROBE(~)
#define IS_CONSTANT_218 PROBE(~)
#define IS_CONSTANT_219 PROBE(~)
#define IS_CONSTANT_220 PROBE(~)
#define IS_CONSTANT_221 PROBE(~)
#define IS_CONSTANT_222 PROBE(~)
#define IS_CONSTANT_223 PROBE(~)
#define IS_CONSTANT_224 PROBE(~)
#define IS_CONSTANT_225 PROBE(~)
#define IS_CONSTANT_226 PROBE(~)
#define IS_CONSTANT_227 PROBE(~)
#define IS_CONSTANT_228 PROBE(~)
#define IS_CONSTANT_229 PROBE(~)
#define IS_CONSTANT_230 PROBE(~)
#define IS_CONSTANT_231 PROBE(~)
#define IS_CONSTANT_232 PROBE(~)
#define IS_CONSTANT_233 PROBE(~)
#define IS_CONSTANT_234 PROBE(~)
#define IS_CONSTANT_235 PROBE(~)
#define IS_CONSTANT_236 PROBE(~)
#define IS_CONSTANT_237 PROBE(~)
#define IS_CONSTANT_238 PROBE(~)
#define IS_CONSTANT_239 PROBE(~)
#define IS_CONSTANT_240 PROBE(~)
#define IS_CONSTANT_241 PROBE(~)
#define IS_CONSTANT_242 PROBE(~)
#define IS_CONSTANT_243 PROBE(~)
#define IS_CONSTANT_244 PROBE(~)
#define IS_CONSTANT_245 PROBE(~)
#define IS_CONSTANT_246 PROBE(~)
#define IS_CONSTANT_247 PROBE(~)
#define IS_CONSTANT_248 PROBE(~)
#define IS_CONSTANT_249 PROBE(~)
#define IS_CONSTANT_250 PROBE(~)
#define IS_CONSTANT_251 PROBE(~)
#define IS_CONSTANT_252 PROBE(~)
#define IS_CONSTANT_253 PROBE(~)
#define IS_CONSTANT_254 PROBE(~)
#define IS_CONSTANT_255 PROBE(~)
#define IS_CONSTANT_256 PROBE(~)
#define IIF(cond) BOOST_PP_CAT(IIF_, cond)
#define IIF_0(t, f) f
#define IIF_1(t, f) t
#define EAT(...)
#define EXPAND(...) __VA_ARGS__
#define WHEN(c) IIF(c)(EXPAND, EAT)

#if !defined(UDATATYPE_BITSIZE)
#define UDATATYPE_BITSIZE 64
#endif

#define UDATATYPE unsigned long long
#define BOOLTYPE _Bool
#define VAL_RESIZE(VAR, nbit) ((UDATATYPE)(BOOST_PP_IF(BOOST_PP_LESS(nbit, UDATATYPE_BITSIZE), ((VAR) & ((UDATATYPE)(BOOST_PP_CAT(POW2(nbit), ULL) - 1))), VAR)))

#define MACRO_BIT_RESIZE(VAR, nbit) VAR = VAL_RESIZE(VAR, nbit)
#define BIT_RESIZE(VAR, nbit) WHEN(BOOST_PP_NOT(IS_CONSTANT(VAR)))(MACRO_BIT_RESIZE(VAR, nbit))
#define SELECT_BIT(VAR, bit) (((VAR) >> (bit)) & 1)
#define SET_BIT(VAR, bit, value) VAR = ((UDATATYPE)((VAR) & ~(BOOST_PP_CAT(POW2(bit), ULL)))) | ((UDATATYPE)((((UDATATYPE)(value)) & 1ULL) << (bit)))
#define SELECT_RANGE(var, high, low) VAL_RESIZE(((UDATATYPE)(var)) >> (low), BOOST_PP_ADD(BOOST_PP_SUB(high, low), 1))
#define SET_RANGE(VAR, val, high, low) VAR = BOOST_PP_IF(BOOST_PP_LESS(BITSIZE(high, low), UDATATYPE_BITSIZE), (VAR & ~(UDATATYPE)((BOOST_PP_CAT(POW2(BITSIZE(high, low)), ULL) - 1) << low)) | ((VAL_RESIZE(val, BITSIZE(high, low))) << low), val)
#define CONCAT(var1, var2, var2bitsize) ((((UDATATYPE)(var1)) << (var2bitsize)) | VAL_RESIZE(var2, var2bitsize))

#define MACRO_DEVECTORIZE_VALUE(z, n, text) _Bool BOOST_PP_CAT(text, BOOST_PP_CAT(_, n)) = (text >> n) & 1;
#define DEVECTORIZE_VALUE(var, nbits) BOOST_PP_REPEAT_FROM_TO(0, nbits, MACRO_DEVECTORIZE_VALUE, var)

#define MACRO_VECTORIZE_VALUE(z, n, text) text = text | (((UDATATYPE)(BOOST_PP_CAT(text, BOOST_PP_CAT(_, n)))) << n);
#define VECTORIZE_VALUE(var, nbits) BOOST_PP_REPEAT_FROM_TO(0, nbits, MACRO_VECTORIZE_VALUE, var)

#define MACRO_VECTORIZE_DECL(z, n, text) _Bool BOOST_PP_CAT(text, BOOST_PP_CAT(_, n));
#define VECTORIZE_DECL(var, nbits) BOOST_PP_REPEAT_FROM_TO(0, nbits, MACRO_VECTORIZE_DECL, var)

#define count_leading_zero_macro(man_bits, MAN_IN_ORIG, SHIFT)                                                                                                                                            \
   {                                                                                                                                                                                                      \
      BOOLTYPE _result_5 = 0;                                                                                                                                                                             \
      BOOLTYPE _result_4, _result_3, _result_2, _result_1, _result_0;                                                                                                                                     \
      UDATATYPE _val4, _val8;                                                                                                                                                                             \
      UDATATYPE _result = 0;                                                                                                                                                                              \
      UDATATYPE MAN_IN;                                                                                                                                                                                   \
      if(man_bits <= 32)                                                                                                                                                                                  \
      {                                                                                                                                                                                                   \
         MAN_IN = ((UDATATYPE)MAN_IN_ORIG) << BOOST_PP_SUB(32, man_bits);                                                                                                                                 \
         BIT_RESIZE(MAN_IN, 32);                                                                                                                                                                          \
      }                                                                                                                                                                                                   \
      else                                                                                                                                                                                                \
      {                                                                                                                                                                                                   \
         MAN_IN = ((UDATATYPE)MAN_IN_ORIG) << BOOST_PP_SUB(64, man_bits);                                                                                                                                 \
      }                                                                                                                                                                                                   \
      _result_5 = man_bits <= 32 || SELECT_RANGE(MAN_IN, 63, 32) == 0;                                                                                                                                    \
      _result_4 = (_result_5 ? SELECT_RANGE(MAN_IN, 31, 16) : SELECT_RANGE(MAN_IN, 63, 48)) == 0;                                                                                                         \
      _result_3 = (_result_4 ? (_result_5 ? SELECT_RANGE(MAN_IN, 15, 8) : SELECT_RANGE(MAN_IN, 47, 40)) : (_result_5 ? SELECT_RANGE(MAN_IN, 31, 24) : SELECT_RANGE(MAN_IN, 63, 56))) == 0;                \
      _result_2 = (_result_3 ? (_result_4 ? (_result_5 ? SELECT_RANGE(MAN_IN, 7, 4) : SELECT_RANGE(MAN_IN, 39, 36)) : (_result_5 ? SELECT_RANGE(MAN_IN, 23, 20) : SELECT_RANGE(MAN_IN, 55, 52))) :        \
                               (_result_4 ? (_result_5 ? SELECT_RANGE(MAN_IN, 15, 12) : SELECT_RANGE(MAN_IN, 47, 44)) : (_result_5 ? SELECT_RANGE(MAN_IN, 31, 28) : SELECT_RANGE(MAN_IN, 63, 60)))) == 0; \
      _val8 = _result_3 ? (_result_4 ? (_result_5 ? SELECT_RANGE(MAN_IN, 7, 0) : SELECT_RANGE(MAN_IN, 39, 32)) : (_result_5 ? SELECT_RANGE(MAN_IN, 23, 16) : SELECT_RANGE(MAN_IN, 55, 48))) :             \
                          (_result_4 ? (_result_5 ? SELECT_RANGE(MAN_IN, 15, 8) : SELECT_RANGE(MAN_IN, 47, 40)) : (_result_5 ? SELECT_RANGE(MAN_IN, 31, 24) : SELECT_RANGE(MAN_IN, 63, 56)));             \
      _val4 = _result_2 ? _val8 & 15 : (_val8 >> 4) & 15;                                                                                                                                                 \
      _result_1 = _val4 >> 2 == 0;                                                                                                                                                                        \
      _result_0 = _result_1 ? (~((_val4 & 2) >> 1)) & 1 : (~((_val4 & 8) >> 3)) & 1;                                                                                                                      \
      if(man_bits <= 32)                                                                                                                                                                                  \
      {                                                                                                                                                                                                   \
         VECTORIZE_VALUE(_result, 5)                                                                                                                                                                      \
         SHIFT = _result;                                                                                                                                                                                 \
      }                                                                                                                                                                                                   \
      else                                                                                                                                                                                                \
      {                                                                                                                                                                                                   \
         VECTORIZE_VALUE(_result, 6)                                                                                                                                                                      \
         SHIFT = _result;                                                                                                                                                                                 \
      }                                                                                                                                                                                                   \
      BIT_RESIZE(SHIFT, CEIL_LOG2(man_bits));                                                                                                                                                             \
   }

#define count_leading_zero_macro_lshift(man_bits, MAN_IN_ORIG, COUNT, SHIFTED)                                                                                                                            \
   {                                                                                                                                                                                                      \
      BOOLTYPE _result_5 = 0;                                                                                                                                                                             \
      BOOLTYPE _result_4, _result_3, _result_2, _result_1, _result_0;                                                                                                                                     \
      UDATATYPE _val4, _val8;                                                                                                                                                                             \
      UDATATYPE _result = 0, _result_lshifted;                                                                                                                                                            \
      UDATATYPE MAN_IN;                                                                                                                                                                                   \
      if(man_bits <= 32)                                                                                                                                                                                  \
      {                                                                                                                                                                                                   \
         MAN_IN = ((UDATATYPE)MAN_IN_ORIG) << BOOST_PP_SUB(32, man_bits);                                                                                                                                 \
         BIT_RESIZE(MAN_IN, 32);                                                                                                                                                                          \
      }                                                                                                                                                                                                   \
      else                                                                                                                                                                                                \
      {                                                                                                                                                                                                   \
         MAN_IN = ((UDATATYPE)MAN_IN_ORIG) << BOOST_PP_SUB(64, man_bits);                                                                                                                                 \
      }                                                                                                                                                                                                   \
      _result_lshifted = MAN_IN;                                                                                                                                                                          \
      _result_5 = man_bits <= 32 || SELECT_RANGE(MAN_IN, 63, 32) == 0;                                                                                                                                    \
      _result_lshifted = _result_5 ? (_result_lshifted << 32) : _result_lshifted;                                                                                                                         \
      _result_4 = (_result_5 ? SELECT_RANGE(MAN_IN, 31, 16) : SELECT_RANGE(MAN_IN, 63, 48)) == 0;                                                                                                         \
      _result_lshifted = _result_4 ? _result_lshifted << 16 : _result_lshifted;                                                                                                                           \
      _result_3 = (_result_4 ? (_result_5 ? SELECT_RANGE(MAN_IN, 15, 8) : SELECT_RANGE(MAN_IN, 47, 40)) : (_result_5 ? SELECT_RANGE(MAN_IN, 31, 24) : SELECT_RANGE(MAN_IN, 63, 56))) == 0;                \
      _result_lshifted = _result_3 ? _result_lshifted << 8 : _result_lshifted;                                                                                                                            \
      _result_2 = (_result_3 ? (_result_4 ? (_result_5 ? SELECT_RANGE(MAN_IN, 7, 4) : SELECT_RANGE(MAN_IN, 39, 36)) : (_result_5 ? SELECT_RANGE(MAN_IN, 23, 20) : SELECT_RANGE(MAN_IN, 55, 52))) :        \
                               (_result_4 ? (_result_5 ? SELECT_RANGE(MAN_IN, 15, 12) : SELECT_RANGE(MAN_IN, 47, 44)) : (_result_5 ? SELECT_RANGE(MAN_IN, 31, 28) : SELECT_RANGE(MAN_IN, 63, 60)))) == 0; \
      _result_lshifted = _result_2 ? _result_lshifted << 4 : _result_lshifted;                                                                                                                            \
      _val8 = _result_3 ? (_result_4 ? (_result_5 ? SELECT_RANGE(MAN_IN, 7, 0) : SELECT_RANGE(MAN_IN, 39, 32)) : (_result_5 ? SELECT_RANGE(MAN_IN, 23, 16) : SELECT_RANGE(MAN_IN, 55, 48))) :             \
                          (_result_4 ? (_result_5 ? SELECT_RANGE(MAN_IN, 15, 8) : SELECT_RANGE(MAN_IN, 47, 40)) : (_result_5 ? SELECT_RANGE(MAN_IN, 31, 24) : SELECT_RANGE(MAN_IN, 63, 56)));             \
      _val4 = _result_2 ? _val8 & 15 : (_val8 >> 4) & 15;                                                                                                                                                 \
      _result_1 = _val4 >> 2 == 0;                                                                                                                                                                        \
      _result_lshifted = _result_1 ? _result_lshifted << 2 : _result_lshifted;                                                                                                                            \
      _result_0 = _result_1 ? (~((_val4 & 2) >> 1)) & 1 : (~((_val4 & 8) >> 3)) & 1;                                                                                                                      \
      _result_lshifted = _result_0 ? _result_lshifted << 1 : _result_lshifted;                                                                                                                            \
      if(man_bits <= 32)                                                                                                                                                                                  \
      {                                                                                                                                                                                                   \
         VECTORIZE_VALUE(_result, 5)                                                                                                                                                                      \
         COUNT = _result;                                                                                                                                                                                 \
      }                                                                                                                                                                                                   \
      else                                                                                                                                                                                                \
      {                                                                                                                                                                                                   \
         VECTORIZE_VALUE(_result, 6)                                                                                                                                                                      \
         COUNT = _result;                                                                                                                                                                                 \
      }                                                                                                                                                                                                   \
      SHIFTED = _result_lshifted >> (BOOST_PP_SUB(64, man_bits));                                                                                                                                         \
      BIT_RESIZE(COUNT, CEIL_LOG2(man_bits));                                                                                                                                                             \
   }

#define count_leading_zero_macro_lshift64(man_bits, MAN_IN_ORIG, COUNT, SHIFTED)                              \
   {                                                                                                          \
      BOOLTYPE _result_5 = 0;                                                                                 \
      BOOLTYPE _result_4, _result_3, _result_2, _result_1, _result_0;                                         \
      UDATATYPE _result = 0, _result_lshifted;                                                                \
      UDATATYPE MAN_IN;                                                                                       \
      MAN_IN = ((UDATATYPE)MAN_IN_ORIG);                                                                      \
      _result_lshifted = MAN_IN;                                                                              \
      _result_5 = SELECT_RANGE(MAN_IN, BOOST_PP_SUB(man_bits, 1), BOOST_PP_SUB(man_bits, 32)) == 0;           \
      _result_lshifted = _result_5 ? (_result_lshifted << 32) : _result_lshifted;                             \
      _result_4 = SELECT_RANGE(_result_lshifted, BOOST_PP_SUB(man_bits, 1), BOOST_PP_SUB(man_bits, 16)) == 0; \
      _result_lshifted = _result_4 ? _result_lshifted << 16 : _result_lshifted;                               \
      _result_3 = SELECT_RANGE(_result_lshifted, BOOST_PP_SUB(man_bits, 1), BOOST_PP_SUB(man_bits, 8)) == 0;  \
      _result_lshifted = _result_3 ? _result_lshifted << 8 : _result_lshifted;                                \
      _result_2 = SELECT_RANGE(_result_lshifted, BOOST_PP_SUB(man_bits, 1), BOOST_PP_SUB(man_bits, 4)) == 0;  \
      _result_lshifted = _result_2 ? _result_lshifted << 4 : _result_lshifted;                                \
      _result_1 = SELECT_RANGE(_result_lshifted, BOOST_PP_SUB(man_bits, 1), BOOST_PP_SUB(man_bits, 2)) == 0;  \
      _result_lshifted = _result_1 ? _result_lshifted << 2 : _result_lshifted;                                \
      _result_0 = SELECT_BIT(_result_lshifted, BOOST_PP_SUB(man_bits, 1)) == 0;                               \
      SHIFTED = _result_0 ? _result_lshifted << 1 : _result_lshifted;                                         \
      VECTORIZE_VALUE(_result, 6)                                                                             \
      COUNT = _result;                                                                                        \
      BIT_RESIZE(COUNT, CEIL_LOG2(man_bits));                                                                 \
   }

#endif
