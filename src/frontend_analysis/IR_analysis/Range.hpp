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
 *              Copyright (C) 2004-2019 Politecnico di Milano
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
 * @file Range.hpp
 * @brief
 *
 * @author Michele Fiorito <michele2.fiorito@mail.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef RANGE_HPP
#define RANGE_HPP

#include "APInt.hpp"
#include "bit_lattice.hpp"
#include "refcount.hpp"
#include "tree_common.hpp"

REF_FORWARD_DECL(Range);
CONSTREF_FORWARD_DECL(Range);

enum RangeType
{
   Empty,
   Unknown,
   Regular,
   Anti,
   Real
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
   virtual ~Range() = default;
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
   virtual std::deque<bit_lattice> getBitValues(bool isSigned) const;
   virtual RangeRef getAnti() const;

   virtual bool isUnknown() const;
   virtual void setUnknown();
   bool isRegular() const;
   bool isAnti() const;
   virtual bool isEmpty() const;
   virtual bool isReal() const;
   bool operator==(const Range& other) const = delete;
   bool operator!=(const Range& other) const = delete;
   bool isSameType(const RangeConstRef& other) const;
   virtual bool isSameRange(const RangeConstRef& other) const;
   virtual bool isFullSet() const;
   virtual bool isSingleElement() const;
   virtual bool isConstant() const;
   virtual Range* clone() const;
   virtual void print(std::ostream& OS) const;
   std::string ToString() const;

   /* Arithmetic operations */
   RangeRef add(const RangeConstRef& other) const;
   RangeRef sat_add(const RangeConstRef& other) const;
   RangeRef usat_add(const RangeConstRef& other) const;
   RangeRef sub(const RangeConstRef& other) const;
   RangeRef sat_sub(const RangeConstRef& other) const;
   RangeRef usat_sub(const RangeConstRef& other) const;
   RangeRef mul(const RangeConstRef& other) const;
   RangeRef udiv(const RangeConstRef& other) const;
   RangeRef sdiv(const RangeConstRef& other) const;
   RangeRef urem(const RangeConstRef& other) const;
   RangeRef srem(const RangeConstRef& other) const;
   RangeRef shl(const RangeConstRef& other) const;
   RangeRef shr(const RangeConstRef& other, bool sign) const;
   virtual RangeRef abs() const;
   virtual RangeRef negate() const;

   /* Bitwise operations */
   RangeRef Not() const;
   RangeRef And(const RangeConstRef& other) const;
   RangeRef Or(const RangeConstRef& other) const;
   RangeRef Xor(const RangeConstRef& other) const;

   /* Comparators */
   virtual RangeRef Eq(const RangeConstRef& other, bw_t bw) const;
   virtual RangeRef Ne(const RangeConstRef& other, bw_t bw) const;
   RangeRef Ugt(const RangeConstRef& other, bw_t bw) const;
   RangeRef Uge(const RangeConstRef& other, bw_t bw) const;
   RangeRef Ult(const RangeConstRef& other, bw_t bw) const;
   RangeRef Ule(const RangeConstRef& other, bw_t bw) const;
   RangeRef UMin(const RangeConstRef& other) const;
   RangeRef UMax(const RangeConstRef& other) const;
   RangeRef Sgt(const RangeConstRef& other, bw_t bw) const;
   RangeRef Sge(const RangeConstRef& other, bw_t bw) const;
   RangeRef Slt(const RangeConstRef& other, bw_t bw) const;
   RangeRef Sle(const RangeConstRef& other, bw_t bw) const;
   RangeRef SMin(const RangeConstRef& other) const;
   RangeRef SMax(const RangeConstRef& other) const;

   RangeRef sextOrTrunc(bw_t bitwidth) const;
   RangeRef zextOrTrunc(bw_t bitwidth) const;
   RangeRef truncate(bw_t bitwidth) const;
   virtual RangeRef intersectWith(const RangeConstRef& other) const;
   virtual RangeRef unionWith(const RangeConstRef& other) const;

   static const bw_t MAX_BIT_INT;
   static const APInt Min;
   static const APInt Max;
   static const APInt MinDelta;
   static RangeRef makeSatisfyingCmpRegion(kind pred, const RangeConstRef& Other);
   static bw_t neededBits(const APInt& a, const APInt& b, bool sign);
   static RangeRef fromBitValues(const std::deque<bit_lattice>& bv, bw_t bitwidth, bool isSigned);
};

std::ostream& operator<<(std::ostream& OS, const Range& R);

class RealRange : public Range
{
 private:
   RangeRef sign;
   RangeRef exponent;
   RangeRef significand;

 public:
   RealRange(const Range& s, const Range& e, const Range& f);
   RealRange(const RangeConstRef& s, const RangeConstRef& e, const RangeConstRef& f);
   explicit RealRange(const RangeConstRef& vc);
   ~RealRange() = default;
   RealRange(const RealRange& other) = default;
   RealRange(RealRange&&) = default;
   RealRange& operator=(const RealRange& other) = default;
   RealRange& operator=(RealRange&&) = default;
   RangeRef getRange() const;

   RangeRef getSign() const;
   RangeRef getExponent() const;
   RangeRef getSignificand() const;
   std::deque<bit_lattice> getBitValues(bool) const override;
   RangeRef getAnti() const override;
   void setSign(const RangeConstRef& s);
   void setExponent(const RangeConstRef& e);
   void setSignificand(const RangeConstRef& f);
   bool isSameRange(const RangeConstRef& other) const override;
   bool isFullSet() const override;
   bool isUnknown() const override;
   void setUnknown() override;
   bool isSingleElement() const override;
   bool isConstant() const override;
   bool isEmpty() const override;
   bool isReal() const override;
   Range* clone() const override;
   void print(std::ostream& OS) const override;

   RangeRef abs() const override;
   RangeRef negate() const override;

   RangeRef Eq(const RangeConstRef& other, bw_t bw) const override;
   RangeRef Ne(const RangeConstRef& other, bw_t bw) const override;

   RangeRef intersectWith(const RangeConstRef& other) const override;
   RangeRef unionWith(const RangeConstRef& other) const override;
   RangeRef toFloat64() const;
   RangeRef toFloat32() const;

   static refcount<RealRange> fromBitValues(const std::deque<bit_lattice>& bv);
};

#endif