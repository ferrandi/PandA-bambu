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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file Range.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef RANGE_HPP
#define RANGE_HPP

#include "APInt.hpp"
#include "bit_lattice.hpp"

enum RangeType
{
   Empty,
   Unknown,
   Regular,
   Anti
};

class Range
{
 public:
   using bw_t = APInt::bw_t;

 private:
   /// The lower bound of the range.
   APInt l;
   /// The upper bound of the range.
   APInt u;
   /// the range bit-width
   bw_t bw;
   /// the range type
   RangeType type;

   void normalizeRange(const APInt& lb, const APInt& ub, RangeType rType);

 public:
   Range(RangeType type, bw_t bw);
   Range(RangeType rType, bw_t bw, const APInt& lb, const APInt& ub);
   Range(const Range& other) = default;
   Range(Range&&) = default;

   Range& operator=(const Range& other) = default;
   Range& operator=(Range&&) = default;

   bw_t getBitWidth() const;
   const APInt& getLower() const;
   const APInt& getUpper() const;
   APInt getSignedMax() const;
   APInt getSignedMin() const;
   APInt getUnsignedMax() const;
   APInt getUnsignedMin() const;
   APInt getSpan() const;
   std::deque<bit_lattice> getBitValues(bool isSigned) const;
   Range getAnti() const;

   bool isUnknown() const;
   void setUnknown();
   bool isRegular() const;
   bool isAnti() const;
   bool isEmpty() const;
   bool operator==(const Range& other) const = delete;
   bool operator!=(const Range& other) const = delete;
   bool isSameType(const Range& other) const;
   bool isSameRange(const Range& other) const;
   bool isFullSet() const;
   bool isSingleElement() const;
   bool isConstant() const;

   void print(std::ostream& OS) const;
   std::string ToString() const;

   /* Arithmetic operations */
   Range add(const Range& other) const;
   Range sat_add(const Range& other) const;
   Range usat_add(const Range& other) const;
   Range sub(const Range& other) const;
   Range sat_sub(const Range& other) const;
   Range usat_sub(const Range& other) const;
   Range mul(const Range& other) const;
   Range udiv(const Range& other) const;
   Range sdiv(const Range& other) const;
   Range urem(const Range& other) const;
   Range srem(const Range& other) const;
   Range shl(const Range& other) const;
   Range shr(const Range& other, bool sign) const;
   Range abs() const;
   Range negate() const;

   /* Bitwise operations */
   Range Not() const;
   Range And(const Range& other) const;
   Range Or(const Range& other) const;
   Range Xor(const Range& other) const;

   /* Comparators */
   Range Eq(const Range& other, bw_t bw) const;
   Range Ne(const Range& other, bw_t bw) const;
   Range Ugt(const Range& other, bw_t bw) const;
   Range Uge(const Range& other, bw_t bw) const;
   Range Ult(const Range& other, bw_t bw) const;
   Range Ule(const Range& other, bw_t bw) const;
   Range UMin(const Range& other) const;
   Range UMax(const Range& other) const;
   Range Sgt(const Range& other, bw_t bw) const;
   Range Sge(const Range& other, bw_t bw) const;
   Range Slt(const Range& other, bw_t bw) const;
   Range Sle(const Range& other, bw_t bw) const;
   Range SMin(const Range& other) const;
   Range SMax(const Range& other) const;

   Range sextOrTrunc(bw_t bitwidth) const;
   Range zextOrTrunc(bw_t bitwidth) const;
   Range truncate(bw_t bitwidth) const;
   Range intersectWith(const Range& other) const;
   Range unionWith(const Range& other) const;

   static const bw_t max_digits;
   static const APInt Min;
   static const APInt Max;
   static const APInt MinDelta;
   static bw_t neededBits(const APInt& a, const APInt& b, bool sign);
   static Range fromBitValues(const std::deque<bit_lattice>& bv, bw_t bitwidth, bool isSigned);
};

std::ostream& operator<<(std::ostream& OS, const Range& R);

#endif
