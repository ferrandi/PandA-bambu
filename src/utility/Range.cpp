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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file Range.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "Range.hpp"

#include "bit_lattice.hpp"
#include "exceptions.hpp"
#include "math_function.hpp"
#include "string_manipulation.hpp"

#define CASE_MISCELLANEOUS   \
   aggr_init_expr_K:         \
   case case_label_expr_K:   \
   case lut_expr_K:          \
   case target_expr_K:       \
   case target_mem_ref_K:    \
   case target_mem_ref461_K: \
   case binfo_K:             \
   case block_K:             \
   case constructor_K:       \
   case error_mark_K:        \
   case identifier_node_K:   \
   case ssa_name_K:          \
   case statement_list_K:    \
   case tree_list_K:         \
   case tree_vec_K:          \
   case call_expr_K

using bw_t = Range::bw_t;

// The number of bits needed to store the largest variable of the function (APInt).
const bw_t Range::max_digits = static_cast<bw_t>(std::numeric_limits<APInt::number>::digits);
const APInt Range::Min = APInt::getSignedMinValue(Range::max_digits);
const APInt Range::Max = APInt::getSignedMaxValue(Range::max_digits);
const APInt Range::MinDelta(1);

Range::Range(RangeType rType, bw_t rbw) : l(Min), u(Max), bw(rbw), type(rType)
{
   THROW_ASSERT(rbw > 0 && rbw <= max_digits, "Invalid bitwidth for range (bw = " + STR(rbw) + ")");
}

Range::Range(RangeType rType, bw_t rbw, const APInt& lb, const APInt& ub) : l(lb), u(ub), bw(rbw), type(rType)
{
   THROW_ASSERT(rbw > 0 && rbw <= max_digits, "Invalid bitwidth for range (bw = " + STR(rbw) + ")");
   normalizeRange(lb, ub, rType);
}

Range* Range::clone() const
{
   return new Range(*this);
}

void Range::normalizeRange(const APInt& lb, const APInt& ub, RangeType rType)
{
   type = rType;
   switch(rType)
   {
      case Empty:
      case Unknown:
      {
         l = Min;
         u = Max;
         break;
      }
      case Anti:
      {
         if(lb > ub)
         {
            return normalizeRange(ub + MinDelta, lb - MinDelta, Regular);
         }
         else if((lb == Min) && (ub == Max))
         {
            type = Empty;
            return;
         }
         else if(lb == Min)
         {
            return normalizeRange(ub + MinDelta, Max, Regular);
         }
         else if(ub == Max)
         {
            return normalizeRange(Min, lb - MinDelta, Regular);
         }
         else
         {
            THROW_ASSERT(ub >= lb, "");
            const auto maxS = APInt::getSignedMaxValue(bw);
            const auto minS = APInt::getSignedMinValue(bw);
            const bool lbgt = lb > maxS;
            const bool ubgt = ub > maxS;
            const bool lblt = lb < minS;
            const bool ublt = ub < minS;
            if(lbgt && ubgt)
            {
               if((ub - lb).abs() < APInt::getMaxValue(bw))
               {
                  l = lb.extOrTrunc(bw, true);
                  u = ub.extOrTrunc(bw, true);
               }
               else
               {
                  type = Regular;
                  l = APInt::getSignedMinValue(bw);
                  u = APInt::getSignedMaxValue(bw);
               }
            }
            else if(lblt && ublt)
            {
               if((ub - lb).abs() < APInt::getMaxValue(bw))
               {
                  l = ub.extOrTrunc(bw, true);
                  u = lb.extOrTrunc(bw, true);
               }
               else
               {
                  type = Regular;
                  l = APInt::getSignedMinValue(bw);
                  u = APInt::getSignedMaxValue(bw);
               }
            }
            else if(!lblt && ubgt)
            {
               const auto ubnew = ub.extOrTrunc(bw, true);
               type = Regular;
               if((ub - lb).abs() < APInt::getMaxValue(bw) && ubnew != (lb - MinDelta))
               {
                  l = ubnew + MinDelta;
                  u = lb - MinDelta;
               }
               else
               {
                  l = APInt::getSignedMinValue(bw);
                  u = APInt::getSignedMaxValue(bw);
               }
            }
            else if(lblt && !ubgt)
            {
               const auto lbnew = lb.extOrTrunc(bw, true);
               type = Regular;
               if((ub - lb).abs() < APInt::getMaxValue(bw) && lbnew != (ub + MinDelta))
               {
                  l = ub + MinDelta;
                  u = lbnew - MinDelta;
               }
               else
               {
                  l = APInt::getSignedMinValue(bw);
                  u = APInt::getSignedMaxValue(bw);
               }
            }
            else if(!lblt && !ubgt)
            {
               l = lb;
               u = ub;
            }
            else
            {
               THROW_UNREACHABLE("unexpected condition");
            }
         }
         break;
      }
      case Regular:
      {
         if((lb - MinDelta) == ub)
         {
            l = APInt::getSignedMinValue(bw);
            u = APInt::getSignedMaxValue(bw);
         }
         else if(lb > ub)
         {
            return normalizeRange(ub + MinDelta, lb - MinDelta, Anti);
         }
         else if(lb == Min && ub == Max)
         {
            l = APInt::getSignedMinValue(bw);
            u = APInt::getSignedMaxValue(bw);
         }
         else if(ub == Max)
         {
            const auto maxS = APInt::getSignedMaxValue(bw);
            const auto minS = APInt::getSignedMinValue(bw);
            if(lb < minS)
            {
               l = maxS;
               u = minS;
            }
            else if(lb >= minS && lb < 0)
            {
               l = lb;
               u = maxS;
            }
            else if(lb >= 0 && lb <= maxS)
            {
               l = lb;
               u = APInt::getSignedMaxValue(bw);
            }
            else if(lb > maxS && lb <= APInt::getMaxValue(bw))
            {
               l = lb.extOrTrunc(bw, true);
               u = -1;
            }
            else
            {
               l = maxS;
               u = minS;
            }
         }
         else if(lb == Min)
         {
            const auto maxS = APInt::getSignedMaxValue(bw);
            const auto minS = APInt::getSignedMinValue(bw);
            if(ub < minS)
            {
               l = maxS;
               u = minS;
            }
            else if(ub >= minS && ub < 0)
            {
               l = minS;
               u = ub;
            }
            else if(ub >= 0 && ub <= maxS)
            {
               l = APInt::getSignedMinValue(bw);
               u = ub;
            }
            else if(ub > maxS && ub < APInt::getMaxValue(bw))
            {
               type = Anti;
               u = -1;
               l = ub.extOrTrunc(bw, true) + MinDelta;
            }
            else
            {
               l = maxS;
               u = minS;
            }
         }
         else
         {
            THROW_ASSERT(ub >= lb, "");
            const auto maxS = APInt::getSignedMaxValue(bw);
            const auto minS = APInt::getSignedMinValue(bw);
            const bool lbgt = lb > maxS;
            const bool ubgt = ub > maxS;
            const bool lblt = lb < minS;
            const bool ublt = ub < minS;
            if(ubgt && lblt)
            {
               l = maxS;
               u = minS;
            }
            else if(lbgt && ubgt)
            {
               l = lb.extOrTrunc(bw, true);
               u = ub.extOrTrunc(bw, true);
            }
            else if(lblt && ublt)
            {
               l = ub.extOrTrunc(bw, true);
               u = lb.extOrTrunc(bw, true);
            }
            else if(!lblt && ubgt)
            {
               const auto ubnew = ub.extOrTrunc(bw, true);
               if((ub - lb).abs() < APInt::getMaxValue(bw) && ubnew != (lb - MinDelta))
               {
                  type = Anti;
                  l = ubnew + MinDelta;
                  u = lb - MinDelta;
               }
               else
               {
                  l = maxS;
                  u = minS;
               }
            }
            else if(lblt && !ubgt)
            {
               const auto lbnew = lb.extOrTrunc(bw, true);
               if((ub - lb).abs() < APInt::getMaxValue(bw) && lbnew != (ub + MinDelta))
               {
                  type = Anti;
                  l = ub + MinDelta;
                  u = lbnew - MinDelta;
               }
               else
               {
                  l = maxS;
                  u = minS;
               }
            }
            else if(!lblt && !ubgt)
            {
               l = lb;
               u = ub;
            }
            else
            {
               THROW_UNREACHABLE("unexpected condition");
            }
         }
         break;
      }
      default:
      {
         THROW_UNREACHABLE("Unknown range type");
         break;
      }
   }
   if(u < l)
   {
      l = Min;
      u = Max;
   }
}

bw_t Range::neededBits(const APInt& a, const APInt& b, bool sign)
{
   return std::max(a.minBitwidth(sign), b.minBitwidth(sign));
}

RangeRef Range::fromBitValues(const std::deque<bit_lattice>& bv, bw_t bitwidth, bool isSigned)
{
   THROW_ASSERT(bv.size() <= bitwidth, "BitValues size not appropriate");
   auto bitstring_to_int = [&](const std::deque<bit_lattice>& bv_in) {
      long long out = isSigned && bv_in.front() == bit_lattice::ONE ? std::numeric_limits<long long>::min() : 0LL;
      auto bv_it = bv_in.crbegin();
      const auto bv_end = bv_in.crend();
      for(size_t i = 0; bv_it != bv_end; ++i, ++bv_it)
      {
         if(*bv_it == bit_lattice::ONE)
         {
            out |= 1LL << i;
         }
         else
         {
            out &= ~(1LL << i);
         }
      }
      return out;
   };
   auto manip = [&](const std::deque<bit_lattice>& bv_in) {
      if(bv_in.size() < bitwidth)
      {
         return APInt(bitstring_to_int(sign_extend_bitstring(bv_in, isSigned, bitwidth)))
             .extOrTrunc(bitwidth, isSigned);
      }
      return APInt(bitstring_to_int(bv_in)).extOrTrunc(bitwidth, isSigned);
   };
   const auto max = [&]() {
      std::deque<bit_lattice> bv_out;
      bv_out.push_back((bv.front() == bit_lattice::U || bv.front() == bit_lattice::X) ?
                           (isSigned ? bit_lattice::ZERO : bit_lattice::ONE) :
                           bv.front());
      for(auto it = ++(bv.begin()); it != bv.end(); ++it)
      {
         bv_out.push_back((*it == bit_lattice::U || *it == bit_lattice::X) ? bit_lattice::ONE : *it);
      }
      return manip(bv_out);
   }();
   const auto min = [&]() {
      std::deque<bit_lattice> bv_out;
      bv_out.push_back((bv.front() == bit_lattice::U || bv.front() == bit_lattice::X) ?
                           (isSigned ? bit_lattice::ONE : bit_lattice::ZERO) :
                           bv.front());
      for(auto it = ++(bv.begin()); it != bv.end(); ++it)
      {
         bv_out.push_back((*it == bit_lattice::U || *it == bit_lattice::X) ? bit_lattice::ZERO : *it);
      }
      return manip(bv_out);
   }();

   THROW_ASSERT(min <= max, "");
   return RangeRef(new Range(Regular, bitwidth, min, max));
}

std::deque<bit_lattice> Range::getBitValues(bool isSigned) const
{
   if(isEmpty() || isAnti() || isUnknown())
   {
      return create_u_bitstring(bw);
   }
   if(isConstant())
   {
      return create_bitstring_from_constant(isSigned ? getSignedMin() : getUnsignedMin(), bw, isSigned);
   }

   auto min = create_bitstring_from_constant(isSigned ? getSignedMin() : getUnsignedMin(), bw, isSigned);
   auto max = create_bitstring_from_constant(isSigned ? getSignedMax() : getUnsignedMax(), bw, isSigned);
   auto& longer = min.size() >= max.size() ? min : max;
   auto& shorter = min.size() >= max.size() ? max : min;
   if(shorter.size() < longer.size())
   {
      shorter = sign_extend_bitstring(shorter, isSigned, longer.size());
   }

   std::deque<bit_lattice> range_bv;
   auto s_it = shorter.cbegin();
   auto l_it = longer.cbegin();
   const auto s_end = shorter.cend();
   const auto l_end = longer.cend();
   for(; s_it != s_end && l_it != l_end; s_it++, l_it++)
   {
      if(*s_it == *l_it)
      {
         range_bv.push_back(*s_it);
      }
      else
      {
         break;
      }
   }
   while(range_bv.size() < longer.size())
   {
      range_bv.push_back(bit_lattice::U);
   }
   sign_reduce_bitstring(range_bv, isSigned);
   return range_bv;
}

RangeRef Range::getAnti() const
{
   if(type == Anti)
   {
      return RangeRef(new Range(Regular, bw, l, u));
   }
   if(type == Regular)
   {
      return RangeRef(new Range(Anti, bw, l, u));
   }
   if(type == Empty)
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(type == Unknown)
   {
      return RangeRef(this->clone());
   }
   THROW_UNREACHABLE("unexpected condition");
   return nullptr;
}

bw_t Range::getBitWidth() const
{
   return bw;
}

const APInt& Range::getLower() const
{
   THROW_ASSERT(!isAnti(), "Lower bound not valid for Anti range");
   return l;
}

const APInt& Range::getUpper() const
{
   THROW_ASSERT(!isAnti(), "Upper bound not valid for Anti range");
   return u;
}

APInt Range::getSignedMax() const
{
   THROW_ASSERT(!isUnknown() && !isEmpty(), "Max not valid for Unknown/Empty range");
   const auto maxS = APInt::getSignedMaxValue(bw);
   if(type == Regular)
   {
      return u > maxS ? maxS : u;
   }
   return maxS;
}

APInt Range::getSignedMin() const
{
   THROW_ASSERT(!isUnknown() && !isEmpty(), "Min not valid for Unknown/Empty range");
   const auto minS = APInt::getSignedMinValue(bw);
   if(type == Regular)
   {
      return l < minS ? minS : l;
   }
   return minS;
}

APInt Range::getUnsignedMax() const
{
   THROW_ASSERT(!isUnknown() && !isEmpty(), "UMax not valid for Unknown/Empty range");
   if(isAnti())
   {
      auto lb = l - MinDelta;
      auto ub = u + MinDelta;
      return (lb >= 0 || ub < 0) ? APInt::getMaxValue(bw) : lb.extOrTrunc(bw, false);
   }
   return (u < 0 || l >= 0) ? u.extOrTrunc(bw, false) : APInt::getMaxValue(bw);
}

APInt Range::getUnsignedMin() const
{
   THROW_ASSERT(!isUnknown() && !isEmpty(), "UMin not valid for Unknown/Empty range");
   if(isAnti())
   {
      return (l > 0 || u < 0) ? APInt::getMinValue(bw) : (u + MinDelta);
   }
   return (l > 0 || u < 0) ? l.extOrTrunc(bw, false) : APInt::getMinValue(bw);
}

APInt Range::getSpan() const
{
   if(isEmpty())
   {
      return 0;
   }
   if(isUnknown() || isFullSet())
   {
      return APInt::getMaxValue(bw) + 1;
   }
   if(isAnti())
   {
      return APInt::getMaxValue(bw) - (u - l);
   }
   const auto uLimit = u == Max ? APInt::getSignedMaxValue(bw) : u;
   const auto lLimit = l == Min ? APInt::getSignedMinValue(bw) : l;
   return 1 + uLimit - lLimit;
}

bool Range::isUnknown() const
{
   return type == Unknown;
}

void Range::setUnknown()
{
   type = Unknown;
}

bool Range::isRegular() const
{
   return type == Regular;
}

bool Range::isAnti() const
{
   return type == Anti;
}

bool Range::isEmpty() const
{
   return type == Empty;
}

bool Range::isSameType(const RangeConstRef& other) const
{
   return type == other->type;
}

bool Range::isSameRange(const RangeConstRef& other) const
{
   return this->bw == other->bw && isSameType(other) && (l == other->l) && (u == other->u);
}

bool Range::isFullSet() const
{
   THROW_ASSERT(!isUnknown(), "");
   if(isEmpty() || isAnti())
   {
      return false;
   }
   return APInt::getSignedMaxValue(bw) <= getUpper() && APInt::getSignedMinValue(bw) >= getLower();
}

bool Range::isSingleElement() const
{
   if(isUnknown() || isEmpty())
   {
      return false;
   }
   return l == u;
}

bool Range::isConstant() const
{
   return isRegular() && l == u;
}

#define RETURN_EMPTY_ON_EMPTY(b)            \
   if(this->isEmpty() || other->isEmpty())  \
   {                                        \
      return RangeRef(new Range(Empty, b)); \
   }
#define RETURN_UNKNOWN_ON_UNKNOWN(b)           \
   if(this->isUnknown() || other->isUnknown()) \
   {                                           \
      return RangeRef(new Range(Unknown, b));  \
   }

RangeRef Range::add(const RangeConstRef& other) const
{
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isFullSet() || other->isFullSet())
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(isAnti() && other->isConstant())
   {
      auto ol = other->getLower();
      if(ol >= (Max - u))
      {
         return RangeRef(new Range(Regular, bw));
      }
      return RangeRef(new Range(Anti, bw, l + ol, u + ol));
   }
   if(this->isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(other->isConstant())
   {
      auto ol = other->getLower();
      if(l == Min)
      {
         THROW_ASSERT(u != Max, "");
         return RangeRef(new Range(Regular, bw, l, u + ol));
      }
      if(u == Max)
      {
         THROW_ASSERT(l != Min, "");
         return RangeRef(new Range(Regular, bw, l + ol, u));
      }

      return RangeRef(new Range(Regular, bw, l + ol, u + ol));
   }

   const auto min = getLower() + other->getLower();
   const auto max = getUpper() + other->getUpper();
   RangeRef res(new Range(Regular, bw, min, max));
   if(res->getSpan() <= getSpan() || res->getSpan() <= other->getSpan())
   {
      return RangeRef(new Range(Regular, bw));
   }
   return res;
}

RangeRef Range::sat_add(const RangeConstRef& other) const
{
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isFullSet() || other->isFullSet())
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(isAnti() && other->isConstant())
   {
      auto ol = other->getLower();
      if(ol == 0)
      {
         return RangeRef(this->clone());
      }
      if(ol > 0)
      {
         return RangeRef(new Range(Regular, bw, APInt::getSignedMinValue(bw) + ol, APInt::getSignedMaxValue(bw)));
      }
      return RangeRef(new Range(Regular, bw, APInt::getSignedMinValue(bw), APInt::getSignedMaxValue(bw) + ol));
   }
   if(this->isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(other->isConstant())
   {
      auto ol = other->getLower();
      if(ol == 0)
      {
         return RangeRef(this->clone());
      }
      if(l == Min)
      {
         THROW_ASSERT(u != Max, "");
         if(ol > 0)
         {
            return RangeRef(new Range(Regular, bw, APInt::getSignedMinValue(bw) + ol,
                                      std::min(APInt::getSignedMaxValue(bw), u + ol)));
         }
         return RangeRef(
             new Range(Regular, bw, APInt::getSignedMinValue(bw), std::max(APInt::getSignedMinValue(bw), u + ol)));
      }
      if(u == Max)
      {
         THROW_ASSERT(l != Min, "");
         if(ol > 0)
         {
            return RangeRef(
                new Range(Regular, bw, std::min(APInt::getSignedMaxValue(bw), l + ol), APInt::getSignedMaxValue(bw)));
         }
         return RangeRef(
             new Range(Regular, bw, std::max(APInt::getSignedMinValue(bw), l + ol), APInt::getSignedMaxValue(bw) + ol));
      }
   }

   const auto min =
       std::max(APInt::getSignedMinValue(bw), std::min(APInt::getSignedMaxValue(bw), getLower() + other->getLower()));
   const auto max =
       std::max(APInt::getSignedMinValue(bw), std::min(APInt::getSignedMaxValue(bw), getUpper() + other->getUpper()));
   RangeRef res(new Range(Regular, bw, min, max));
   if(res->getSpan() <= getSpan() || res->getSpan() <= other->getSpan())
   {
      return RangeRef(new Range(Regular, bw));
   }
   return res;
}

RangeRef Range::usat_add(const RangeConstRef& other) const
{
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isFullSet() || other->isFullSet())
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(isAnti() && other->isConstant())
   {
      auto ol = other->getLower();
      if(l + ol >= APInt::getMaxValue(bw))
      {
         return RangeRef(new Range(Anti, bw, 0, u + ol));
      }
      return RangeRef(new Range(Anti, bw, l + ol, u + ol));
   }
   if(this->isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(other->isConstant())
   {
      auto ol = other->getLower();
      if(l == Min)
      {
         THROW_ASSERT(u != Max, "");
         return RangeRef(new Range(Regular, bw, 0, std::min(APInt::getMaxValue(bw), u + ol)));
      }
      if(u == Max)
      {
         THROW_ASSERT(l != Min, "");
         return RangeRef(new Range(Regular, bw, std::min(APInt::getMaxValue(bw), l + ol), APInt::getMaxValue(bw)));
      }

      return RangeRef(new Range(Regular, bw, l + ol, std::min(APInt::getMaxValue(bw), u + ol)));
   }

   const auto min = std::min(APInt::getMaxValue(bw), getLower() + other->getLower());
   const auto max = std::min(APInt::getMaxValue(bw), getUpper() + other->getUpper());
   RangeRef res(new Range(Regular, bw, min, max));
   if(res->getSpan() <= getSpan() || res->getSpan() <= other->getSpan())
   {
      return RangeRef(new Range(Regular, bw));
   }
   return res;
}

RangeRef Range::sub(const RangeConstRef& other) const
{
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isFullSet() || other->isFullSet())
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(this->isAnti() && other->isConstant())
   {
      auto ol = other->getLower();
      if(l <= (Min + ol))
      {
         return RangeRef(new Range(Regular, bw));
      }

      return RangeRef(new Range(Anti, bw, l - ol, u - ol));
   }
   if(this->isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(other->isConstant())
   {
      auto ol = other->getLower();
      if(l == Min)
      {
         THROW_ASSERT(u != Max, "");
         auto minValue = APInt::getSignedMinValue(bw);
         auto upper = u - ol;
         if(minValue > upper)
         {
            return RangeRef(new Range(Regular, bw));
         }

         return RangeRef(new Range(Regular, bw, l, upper));
      }
      if(u == Max)
      {
         THROW_ASSERT(l != Min, "");
         auto maxValue = APInt::getSignedMaxValue(bw);
         auto lower = l - ol;
         if(maxValue < lower)
         {
            return RangeRef(new Range(Regular, bw));
         }

         return RangeRef(new Range(Regular, bw, l - ol, u));
      }

      auto lower = (l - ol).extOrTrunc(bw, true);
      auto upper = (u - ol).extOrTrunc(bw, true);
      if(lower <= upper)
      {
         return RangeRef(new Range(Regular, bw, lower, upper));
      }
      return RangeRef(new Range(Anti, bw, upper + 1, lower - 1));
   }

   const auto min = getLower() - other->getUpper();
   const auto max = getUpper() - other->getLower();
   RangeRef res(new Range(Regular, bw, min, max));
   if(res->getSpan() < getSpan() || res->getSpan() < other->getSpan())
   {
      return RangeRef(new Range(Regular, bw));
   }
   return res;
}

RangeRef Range::sat_sub(const RangeConstRef& other) const
{
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isFullSet() || other->isFullSet())
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(this->isAnti() && other->isConstant())
   {
      auto ol = other->getLower();
      if(ol == 0)
      {
         return RangeRef(this->clone());
      }
      if(ol < 0)
      {
         return RangeRef(new Range(Regular, bw, APInt::getSignedMinValue(bw) - ol, APInt::getSignedMaxValue(bw)));
      }
      return RangeRef(new Range(Anti, bw, APInt::getSignedMinValue(bw), APInt::getSignedMaxValue(bw) - ol));
   }
   if(this->isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(other->isConstant())
   {
      auto ol = other->getLower();
      if(l == Min)
      {
         THROW_ASSERT(u != Max, "");
         if(ol < 0)
         {
            return RangeRef(new Range(Regular, bw, APInt::getSignedMinValue(bw) - ol,
                                      std::min(APInt::getSignedMaxValue(bw), u - ol)));
         }
         return RangeRef(
             new Range(Regular, bw, APInt::getSignedMinValue(bw), std::max(APInt::getSignedMinValue(bw), u - ol)));
      }
      if(u == Max)
      {
         THROW_ASSERT(l != Min, "");
         if(ol < 0)
         {
            return RangeRef(
                new Range(Regular, bw, std::min(APInt::getSignedMaxValue(bw), l - ol), APInt::getSignedMaxValue(bw)));
         }
         return RangeRef(
             new Range(Regular, bw, std::max(APInt::getSignedMinValue(bw), l - ol), APInt::getSignedMaxValue(bw) - ol));
      }
   }

   const auto min =
       std::max(APInt::getSignedMinValue(bw), std::min(APInt::getSignedMaxValue(bw), getLower() - other->getUpper()));
   const auto max =
       std::max(APInt::getSignedMinValue(bw), std::min(APInt::getSignedMaxValue(bw), getUpper() - other->getLower()));
   RangeRef res(new Range(Regular, bw, min, max));
   if(res->getSpan() < getSpan() || res->getSpan() < other->getSpan())
   {
      return RangeRef(new Range(Regular, bw));
   }
   return res;
}

RangeRef Range::usat_sub(const RangeConstRef& other) const
{
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isFullSet() || other->isFullSet())
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(this->isAnti() && other->isConstant())
   {
      auto ol = other->getLower();
      if(ol == 0)
      {
         return RangeRef(this->clone());
      }
      if(ol < 0)
      {
         return RangeRef(new Range(Regular, bw, 0 - ol, APInt::getMaxValue(bw)));
      }
      return RangeRef(new Range(Regular, bw, 0, APInt::getMaxValue(bw) - ol));
   }
   if(this->isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(other->isConstant())
   {
      auto ol = other->getLower();
      if(l == Min)
      {
         THROW_ASSERT(u != Max, "");
         if(ol < 0)
         {
            return RangeRef(new Range(Regular, bw, 0 - ol, std::min(APInt::getMaxValue(bw), u - ol)));
         }
         return RangeRef(new Range(Regular, bw, 0, std::max(APInt(0), u - ol)));
      }
      if(u == Max)
      {
         THROW_ASSERT(l != Min, "");
         if(ol < 0)
         {
            return RangeRef(new Range(Regular, bw, std::min(APInt::getMaxValue(bw), l - ol), APInt::getMaxValue(bw)));
         }
         return RangeRef(new Range(Regular, bw, std::max(APInt(0), l - ol), APInt::getMaxValue(bw) - ol));
      }
   }

   const auto min = std::max(APInt(0), std::min(APInt::getMaxValue(bw), getLower() - other->getUpper()));
   const auto max = std::max(APInt(0), std::min(APInt::getMaxValue(bw), getUpper() - other->getLower()));
   RangeRef res(new Range(Regular, bw, min, max));
   if(res->getSpan() < getSpan() || res->getSpan() < other->getSpan())
   {
      return RangeRef(new Range(Regular, bw));
   }
   return res;
}

RangeRef Range::mul(const RangeConstRef& other) const
{
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isFullSet() || other->isFullSet() || this->isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }

   // Multiplication is signedness-independent. However different ranges can be
   // obtained depending on how the input ranges are treated. These different
   // ranges are all conservatively correct, but one might be better than the
   // other. We calculate two ranges; one treating the inputs as unsigned
   // and the other signed, then return the smallest of these ranges.

   // Unsigned range first.
   auto this_min = getUnsignedMin();
   auto this_max = getUnsignedMax();
   auto Other_min = other->getUnsignedMin();
   auto Other_max = other->getUnsignedMax();

   const auto Result_zext = Range(Regular, static_cast<bw_t>(bw * 2), this_min * Other_min, this_max * Other_max);
   const auto UR = Result_zext.truncate(bw);

   // Now the signed range. Because we could be dealing with negative numbers
   // here, the lower bound is the smallest of the Cartesian product of the
   // lower and upper ranges; for example:
   //   [-1,4] * [-2,3] = min(-1*-2, -1*-3, 4*-2, 4*3) = -8.
   // Similarly for the upper bound, swapping min for max.

   this_min = getSignedMin();
   this_max = getSignedMax();
   Other_min = other->getSignedMin();
   Other_max = other->getSignedMax();

   const auto res =
       std::minmax({this_min * Other_min, this_min * Other_max, this_max * Other_min, this_max * Other_max});
   const auto Result_sext = Range(Regular, static_cast<bw_t>(bw * 2), res.first, res.second);
   const auto SR = Result_sext.truncate(bw);

   return UR->getSpan() < SR->getSpan() ? UR : SR;
}

RangeRef Range::udiv(const RangeConstRef& other) const
{
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isFullSet())
   {
      return RangeRef(new Range(Regular, bw));
   }

   auto a = getUnsignedMin();
   auto b = getUnsignedMax();
   auto c = other->getUnsignedMin();
   auto d = other->getUnsignedMax();

   // Deal with division by 0 exception
   if((c == 0) && (d == 0))
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(c == 0)
   {
      c = 1;
   }
   RangeRef res(new Range(Regular, bw, a / d, b / c));
   return res;
}

#define DIV_HELPER(x, y)                            \
   ((x) == Max) ?                                   \
       (((y) < 0) ? Min : (((y) == 0) ? 0 : Max)) : \
       (((y) == Max) ? 0 :                          \
                       (((x) == Min) ? (((y) < 0) ? Max : (((y) == 0) ? 0 : Min)) : (((y) == Min) ? 0 : ((x) / (y)))))

RangeRef Range::sdiv(const RangeConstRef& other) const
{
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isFullSet() || this->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }

   const APInt& a = this->getLower();
   const APInt& b = this->getUpper();
   APInt c1, d1, c2, d2;
   bool is_zero_in = false;
   if(other->isAnti())
   {
      auto antiRange = other->getAnti();
      c1 = Min;
      d1 = antiRange->getLower() - 1;
      if(d1 == 0)
      {
         d1 = -1;
      }
      else if(d1 > 0)
      {
         return RangeRef(new Range(Regular, bw)); /// could be improved
      }
      c2 = antiRange->getUpper() + 1;
      if(c2 == 0)
      {
         c2 = 1;
      }
      else if(c2 < 0)
      {
         return RangeRef(new Range(Regular, bw)); /// could be improved
      }
      d2 = Max;
   }
   else
   {
      c1 = other->getLower();
      d1 = other->getUpper();
      // Deal with division by 0 exception
      if((c1 == 0) && (d1 == 0))
      {
         return RangeRef(new Range(Regular, bw));
      }
      is_zero_in = (c1 < 0) && (d1 > 0);
      if(is_zero_in)
      {
         d1 = -1;
         c2 = 1;
      }
      else
      {
         c2 = other->getLower();
         if(c2 == 0)
         {
            c1 = c2 = 1;
         }
      }
      d2 = other->getUpper();
      if(d2 == 0)
      {
         d1 = d2 = -1;
      }
   }
   auto n_iters = (is_zero_in || other->isAnti()) ? 8u : 4u;

   APInt candidates[8];
   candidates[0] = DIV_HELPER(a, c1);
   candidates[1] = DIV_HELPER(a, d1);
   candidates[2] = DIV_HELPER(b, c1);
   candidates[3] = DIV_HELPER(b, d1);
   if(n_iters == 8)
   {
      candidates[4] = DIV_HELPER(a, c2);
      candidates[5] = DIV_HELPER(a, d2);
      candidates[6] = DIV_HELPER(b, c2);
      candidates[7] = DIV_HELPER(b, d2);
   }
   // Lower bound is the min value from the vector, while upper bound is the max value
   auto res = std::minmax_element(candidates, candidates + n_iters);
   return RangeRef(new Range(Regular, bw, *res.first, *res.second));
}

RangeRef Range::urem(const RangeConstRef& other) const
{
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(other->isConstant())
   {
      if(other->getLower() == 0)
      {
         return RangeRef(new Range(Empty, bw));
      }
      else if(other->getUnsignedMin() == 1)
      {
         return RangeRef(new Range(Regular, bw, 0, 0));
      }
   }

   const APInt& a = this->getUnsignedMin();
   const APInt& b = this->getUnsignedMax();
   APInt c = other->getUnsignedMin();
   const APInt& d = other->getUnsignedMax();

   // Deal with mod 0 exception
   if((c == 0) && (d == 0))
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(c == 0)
   {
      c = 1;
   }

   APInt candidates[8];

   candidates[0] = a < c ? a : 0;
   candidates[1] = a < d ? a : 0;
   candidates[2] = b < c ? b : 0;
   candidates[3] = b < d ? b : 0;
   candidates[4] = a < c ? a : c - 1;
   candidates[5] = a < d ? a : d - 1;
   candidates[6] = b < c ? b : c - 1;
   candidates[7] = b < d ? b : d - 1;

   // Lower bound is the min value from the vector, while upper bound is the max value
   auto res = std::minmax_element(candidates, candidates + 8);
   return RangeRef(new Range(Regular, bw, *res.first, *res.second));
}

RangeRef Range::srem(const RangeConstRef& other) const
{
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isFullSet() || this->isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }

   const auto& a = this->getLower();
   const auto& b = this->getUpper();
   const auto& c = other->getLower();
   const auto& d = other->getUpper();

   // Deal with mod 0 exception
   if(c <= 0 && d >= 0)
   {
      return RangeRef(new Range(Regular, bw));
   }

   const auto dmin = std::min(c.abs(), d.abs());
   const auto dmax = std::max(c.abs(), d.abs());
   const auto abs_min = std::min(a.abs(), b.abs());
   const auto abs_max = std::max(a.abs(), b.abs());
   if((abs_min < dmin && dmin < abs_max) || (abs_min < dmax && dmax < abs_max))
   {
      return RangeRef(new Range(Regular, bw, a >= 0 ? 0 : (a.abs() < dmax ? a : -(dmax - 1)),
                                b <= 0 ? 0 : (b.abs() < dmax ? b : (dmax - 1))));
   }
   else if(abs_max < dmin)
   {
      return RangeRef(this->clone());
   }
   return RangeRef(new Range(Regular, bw, a < 0 ? -(dmax - 1) : 0, b > 0 ? (dmax - 1) : 0));
}

RangeRef Range::shl(const RangeConstRef& other) const
{
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isFullSet() || other->isFullSet() || this->isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(this->isConstant() && other->isConstant())
   {
      const auto c = (this->getLower() << other->getLower().extOrTrunc(static_cast<APInt::bw_t>(ceil_log2(bw)), false))
                         .extOrTrunc(bw, true);
      return RangeRef(new Range(Regular, bw, c, c));
   }

   const auto a = this->getLower();
   const auto b = this->getUpper();
   const auto fix = other->zextOrTrunc(static_cast<bw_t>(ceil_log2(bw)));
   const auto c = fix->getUnsignedMin();
   const auto d = fix->getUnsignedMax();

   if(c >= bw)
   {
      return RangeRef(new Range(Regular, bw, 0, 0));
   }
   if(d >= bw)
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(a < 0 && b < 0)
   {
      if(d > a.leadingOnes(bw))
      {
         return RangeRef(new Range(Regular, bw));
      }
      return RangeRef(new Range(Regular, bw, a << d, b << c));
   }
   if(a < 0 && b >= 0)
   {
      if(d > std::min(a.leadingOnes(bw), b.leadingZeros(bw)))
      {
         return RangeRef(new Range(Regular, bw));
      }
      return RangeRef(new Range(Regular, bw, a << d, b << d));
   }
   if(d > b.leadingZeros(bw))
   {
      return RangeRef(new Range(Regular, bw));
   }
   return RangeRef(new Range(Regular, bw, a << c, b << d));
}

RangeRef Range::shr(const RangeConstRef& other, bool sign) const
{
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }

   const auto fix = other->zextOrTrunc(static_cast<bw_t>(ceil_log2(bw)));
   const auto c = fix->getUnsignedMin();
   const auto d = fix->getUnsignedMax();

   if(sign)
   {
      const auto a = getSignedMin();
      const auto b = getSignedMax();
      const auto min = a >= 0 ? a >> d : a >> c;
      const auto max = b >= 0 ? b >> c : b >> d;

      return RangeRef(new Range(Regular, bw, min, max));
   }
   else
   {
      const auto a = getUnsignedMin();
      const auto b = getUnsignedMax();

      return RangeRef(new Range(Regular, bw, a >> d, b >> c));
   }
}

/*
 * 	This and operations are coded following Hacker's Delight algorithm.
 * 	According to the author, they provide tight results.
 */
namespace
{
   APInt minOR(bw_t bw, APInt a, const APInt& b, APInt c, const APInt& d)
   {
      APInt temp;
      APInt m = APInt(1) << (bw - 1U);
      APInt nm = APInt(-1) << (bw - 1U);
      for(auto i = 0; i < bw; ++i)
      {
         if(~a & c & m)
         {
            temp = (a | m) & nm;
            if(temp <= b)
            {
               a = temp;
               break;
            }
         }
         else if(a & ~c & m)
         {
            temp = (c | m) & nm;
            if(temp <= d)
            {
               c = temp;
               break;
            }
         }
         m >>= 1;
         nm >>= 1;
      }
      return a | c;
   }

   APInt maxOR(bw_t bw, const APInt& a, APInt b, const APInt& c, APInt d)
   {
      APInt temp;
      APInt m = APInt(1) << (bw - 1);
      APInt mm = m - 1;
      for(auto i = 0; i < bw; ++i)
      {
         if(b & d & m)
         {
            temp = (b - m) | mm;
            if(temp >= a)
            {
               b = temp;
               break;
            }
            temp = (d - m) | mm;
            if(temp >= c)
            {
               d = temp;
               break;
            }
         }
         m >>= 1;
         mm >>= 1;
      }
      return b | d;
   }

   std::pair<APInt, APInt> OR(bw_t bw, const APInt& a, const APInt& b, const APInt& c, const APInt& d)
   {
      auto abcd = ((a >= 0) << 3) | ((b >= 0) << 2) | ((c >= 0) << 1) | (d >= 0);

      APInt res_l = 0, res_u = 0;
      switch(abcd)
      {
         case 0:
         case 3:
         case 12:
         case 15:
            res_l = minOR(bw, a, b, c, d);
            res_u = maxOR(bw, a, b, c, d);
            break;
         case 1:
            return std::make_pair(a, -1);
         case 4:
            return std::make_pair(c, -1);
         case 5:
            res_l = std::min({a, c});
            res_u = maxOR(bw, 0, b, 0, d);
            break;
         case 7:
            res_l = minOR(bw, a, -1, c, d);
            res_u = maxOR(bw, 0, b, c, d);
            break;
         case 13:
            res_l = minOR(bw, a, b, c, -1);
            res_u = maxOR(bw, a, b, 0, d);
            break;
         default:
            THROW_UNREACHABLE("OR unreachable state " + STR(abcd));
      }
      return std::make_pair(res_l, res_u);
   }

   APInt minAND(bw_t bw, APInt a, const APInt& b, APInt c, const APInt& d)
   {
      APInt temp;
      APInt m = APInt(1) << (bw - 1U);
      APInt nm = APInt(-1) << (bw - 1U);
      for(auto i = 0; i < bw; ++i)
      {
         if(~a & ~c & m)
         {
            temp = (a | m) & nm;
            if(temp <= b)
            {
               a = temp;
               break;
            }
            temp = (c | m) & nm;
            if(temp <= d)
            {
               c = temp;
               break;
            }
         }
         m >>= 1;
         nm >>= 1;
      }
      return a & c;
   }

   APInt maxAND(bw_t bw, const APInt& a, APInt b, const APInt& c, APInt d)
   {
      APInt temp;
      APInt m = APInt(1) << (bw - 1);
      APInt mm = m - 1;
      for(auto i = 0; i < bw; ++i)
      {
         if(b & ~d & m)
         {
            temp = (b & ~m) | mm;
            if(temp >= a)
            {
               b = temp;
               break;
            }
         }
         else if(~b & d & m)
         {
            temp = (d & ~m) | mm;
            if(temp >= c)
            {
               d = temp;
               break;
            }
         }
         m >>= 1;
         mm >>= 1;
      }
      return b & d;
   }

   std::pair<APInt, APInt> AND(bw_t bw, const APInt& a, const APInt& b, const APInt& c, const APInt& d)
   {
      auto abcd = ((a >= 0) << 3) | ((b >= 0) << 2) | ((c >= 0) << 1) | (d >= 0);

      APInt res_l = 0, res_u = 0;
      switch(abcd)
      {
         case 0:
         case 3:
         case 12:
         case 15:
            res_l = minAND(bw, a, b, c, d);
            res_u = maxAND(bw, a, b, c, d);
            break;
         case 1:
            res_l = minAND(bw, a, b, c, -1);
            res_u = maxAND(bw, a, b, 0, d);
            break;
         case 4:
            res_l = minAND(bw, a, -1, c, d);
            res_u = maxAND(bw, 0, b, c, d);
            break;
         case 5:
            res_l = minAND(bw, a, -1, c, -1);
            res_u = std::max({b, d});
            break;
         case 7:
            return std::make_pair(0, d);
         case 13:
            return std::make_pair(0, b);
         default:
            THROW_UNREACHABLE("AND unreachable state " + STR(abcd));
      }
      return std::make_pair(res_l, res_u);
   }

   APInt minXOR(bw_t bw, APInt a, const APInt& b, APInt c, const APInt& d)
   {
      APInt temp;
      APInt m = APInt(1) << (bw - 1U);
      APInt nm = APInt(-1) << (bw - 1U);
      for(auto i = 0; i < bw; ++i)
      {
         if(~a & c & m)
         {
            temp = (a | m) & nm;
            if(temp <= b)
            {
               a = temp;
            }
         }
         else if(a & ~c & m)
         {
            temp = (c | m) & nm;
            if(temp <= d)
            {
               c = temp;
            }
         }
         m >>= 1;
         nm >>= 1;
      }
      return a ^ c;
   }

   APInt maxXOR(bw_t bw, const APInt& a, APInt b, const APInt& c, APInt d)
   {
      APInt temp;
      APInt m = APInt(1) << (bw - 1);
      APInt mm = m - 1;
      for(auto i = 0; i < bw; ++i)
      {
         if(b & d & m)
         {
            temp = (b - m) | mm;
            if(temp >= a)
            {
               b = temp;
            }
            else
            {
               temp = (d - m) | mm;
               if(temp >= c)
               {
                  d = temp;
               }
            }
         }
         m >>= 1;
         mm >>= 1;
      }
      return b ^ d;
   }

   std::pair<APInt, APInt> uXOR(bw_t bw, const APInt& a, const APInt& b, const APInt& c, const APInt& d)
   {
      return std::make_pair(minXOR(bw, a, b, c, d), maxXOR(bw, a, b, c, d));
   }

} // namespace

RangeRef Range::Or(const RangeConstRef& other) const
{
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isConstant() && this->getSignedMax() == 0)
   {
      return RangeRef(other->clone());
   }
   if(other->isConstant() && other->getSignedMax() == 0)
   {
      return RangeRef(this->clone());
   }

   const auto& a = this->isAnti() ? Min : this->getLower();
   const auto& b = this->isAnti() ? Max : this->getUpper();
   const auto& c = other->isAnti() ? Min : other->getLower();
   const auto& d = other->isAnti() ? Max : other->getUpper();

   const auto res = OR(bw, a, b, c, d);
   return RangeRef(new Range(Regular, bw, res.first, res.second));
}

RangeRef Range::And(const RangeConstRef& other) const
{
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(this->isConstant() && this->getSignedMax() == -1)
   {
      return RangeRef(other->clone());
   }
   if(other->isConstant() && other->getSignedMax() == -1)
   {
      return RangeRef(this->clone());
   }

   const auto& a = this->isAnti() ? Min : this->getLower();
   const auto& b = this->isAnti() ? Max : this->getUpper();
   const auto& c = other->isAnti() ? Min : other->getLower();
   const auto& d = other->isAnti() ? Max : other->getUpper();

   const auto res = AND(bw, a, b, c, d);
   return RangeRef(new Range(Regular, bw, res.first, res.second));
}

RangeRef Range::Xor(const RangeConstRef& other) const
{
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);

   const auto& a = this->isAnti() ? Min : this->getLower();
   const auto& b = this->isAnti() ? Max : this->getUpper();
   const auto& c = other->isAnti() ? Min : other->getLower();
   const auto& d = other->isAnti() ? Max : other->getUpper();

   if(a >= 0 && b >= 0 && c >= 0 && d >= 0)
   {
      const auto res = uXOR(bw, a, b, c, d);
      return RangeRef(new Range(Regular, bw, res.first, res.second));
   }
   else if(a == -1 && b == -1 && c >= 0 && d >= 0)
   {
      return this->sub(other);
   }
   else if(c == -1 && d == -1 && a >= 0 && b >= 0)
   {
      return other->sub(RangeRef(this->clone()));
   }
   return RangeRef(new Range(Regular, bw));
}

RangeRef Range::Not() const
{
   if(isEmpty() || isUnknown())
   {
      return RangeRef(new Range(this->type, bw));
   }

   const auto min = ~this->u;
   const auto max = ~this->l;

   return RangeRef(new Range(this->type, bw, min, max));
}

RangeRef Range::Eq(const RangeConstRef& other, bw_t _bw) const
{
   RETURN_EMPTY_ON_EMPTY(_bw)
   RETURN_UNKNOWN_ON_UNKNOWN(_bw)
   if(this->isAnti() && other->isAnti())
   {
      return RangeRef(new Range(Regular, _bw, 0, 1));
   }
   if(!this->isAnti() && !other->isAnti())
   {
      if((l == Min) || (u == Max) || (other->l == Min) || (other->u == Max))
      {
         return RangeRef(new Range(Regular, _bw, 0, 1));
      }
   }
   bool areTheyEqual = !this->intersectWith(other)->isEmpty();
   bool areTheyDifferent = !((l == u) && this->isSameRange(other));

   if(areTheyEqual && areTheyDifferent)
   {
      return RangeRef(new Range(Regular, _bw, 0, 1));
   }
   if(areTheyEqual && !areTheyDifferent)
   {
      return RangeRef(new Range(Regular, _bw, 1, 1));
   }
   if(!areTheyEqual && areTheyDifferent)
   {
      return RangeRef(new Range(Regular, _bw, 0, 0));
   }

   THROW_UNREACHABLE("condition unexpected");
   return nullptr;
}

RangeRef Range::Ne(const RangeConstRef& other, bw_t _bw) const
{
   RETURN_EMPTY_ON_EMPTY(_bw)
   RETURN_UNKNOWN_ON_UNKNOWN(_bw)
   if(this->isAnti() && other->isAnti())
   {
      return RangeRef(new Range(Regular, _bw, 0, 1));
   }
   if(!this->isAnti() && !other->isAnti())
   {
      if((l == Min) || (u == Max) || (other->l == Min) || (other->u == Max))
      {
         return RangeRef(new Range(Regular, _bw, 0, 1));
      }
   }
   bool areTheyEqual = !this->intersectWith(other)->isEmpty();
   bool areTheyDifferent = !((l == u) && this->isSameRange(other));
   if(areTheyEqual && areTheyDifferent)
   {
      return RangeRef(new Range(Regular, _bw, 0, 1));
   }
   if(areTheyEqual && !areTheyDifferent)
   {
      return RangeRef(new Range(Regular, _bw, 0, 0));
   }
   if(!areTheyEqual && areTheyDifferent)
   {
      return RangeRef(new Range(Regular, _bw, 1, 1));
   }

   THROW_UNREACHABLE("condition unexpected");
   return nullptr;
}

RangeRef Range::Ugt(const RangeConstRef& other, bw_t _bw) const
{
   RETURN_EMPTY_ON_EMPTY(_bw)
   RETURN_UNKNOWN_ON_UNKNOWN(_bw)
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, _bw, 0, 1));
   }

   const auto a = this->getUnsignedMin();
   const auto b = this->getUnsignedMax();
   const auto c = other->getUnsignedMin();
   const auto d = other->getUnsignedMax();
   if(a > d)
   {
      return RangeRef(new Range(Regular, _bw, 1, 1));
   }
   if(c >= b)
   {
      return RangeRef(new Range(Regular, _bw, 0, 0));
   }

   return RangeRef(new Range(Regular, _bw, 0, 1));
}

RangeRef Range::Uge(const RangeConstRef& other, bw_t _bw) const
{
   RETURN_EMPTY_ON_EMPTY(_bw)
   RETURN_UNKNOWN_ON_UNKNOWN(_bw)
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, _bw, 0, 1));
   }

   const auto a = this->getUnsignedMin();
   const auto b = this->getUnsignedMax();
   const auto c = other->getUnsignedMin();
   const auto d = other->getUnsignedMax();
   if(a >= d)
   {
      return RangeRef(new Range(Regular, _bw, 1, 1));
   }
   if(c > b)
   {
      return RangeRef(new Range(Regular, _bw, 0, 0));
   }

   return RangeRef(new Range(Regular, _bw, 0, 1));
}

RangeRef Range::Ult(const RangeConstRef& other, bw_t _bw) const
{
   RETURN_EMPTY_ON_EMPTY(_bw)
   RETURN_UNKNOWN_ON_UNKNOWN(_bw)
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, _bw, 0, 1));
   }

   const auto a = this->getUnsignedMin();
   const auto b = this->getUnsignedMax();
   const auto c = other->getUnsignedMin();
   const auto d = other->getUnsignedMax();
   if(b < c)
   {
      return RangeRef(new Range(Regular, _bw, 1, 1));
   }
   if(d <= a)
   {
      return RangeRef(new Range(Regular, _bw, 0, 0));
   }

   return RangeRef(new Range(Regular, _bw, 0, 1));
}

RangeRef Range::Ule(const RangeConstRef& other, bw_t _bw) const
{
   RETURN_EMPTY_ON_EMPTY(_bw)
   RETURN_UNKNOWN_ON_UNKNOWN(_bw)
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, _bw, 0, 1));
   }

   const auto a = this->getUnsignedMin();
   const auto b = this->getUnsignedMax();
   const auto c = other->getUnsignedMin();
   const auto d = other->getUnsignedMax();
   if(b <= c)
   {
      return RangeRef(new Range(Regular, _bw, 1, 1));
   }
   if(d < a)
   {
      return RangeRef(new Range(Regular, _bw, 0, 0));
   }

   return RangeRef(new Range(Regular, _bw, 0, 1));
}

RangeRef Range::Sgt(const RangeConstRef& other, bw_t _bw) const
{
   RETURN_EMPTY_ON_EMPTY(_bw)
   RETURN_UNKNOWN_ON_UNKNOWN(_bw)
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, _bw, 0, 1));
   }

   const auto a = this->getSignedMin();
   const auto b = this->getSignedMax();
   const auto c = other->getSignedMin();
   const auto d = other->getSignedMax();
   if(a > d)
   {
      return RangeRef(new Range(Regular, _bw, 1, 1));
   }
   if(c >= b)
   {
      return RangeRef(new Range(Regular, _bw, 0, 0));
   }

   return RangeRef(new Range(Regular, _bw, 0, 1));
}

RangeRef Range::Sge(const RangeConstRef& other, bw_t _bw) const
{
   RETURN_EMPTY_ON_EMPTY(_bw)
   RETURN_UNKNOWN_ON_UNKNOWN(_bw)
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, _bw, 0, 1));
   }

   const auto a = this->getSignedMin();
   const auto b = this->getSignedMax();
   const auto c = other->getSignedMin();
   const auto d = other->getSignedMax();
   if(a >= d)
   {
      return RangeRef(new Range(Regular, _bw, 1, 1));
   }
   if(c > b)
   {
      return RangeRef(new Range(Regular, _bw, 0, 0));
   }

   return RangeRef(new Range(Regular, _bw, 0, 1));
}

RangeRef Range::Slt(const RangeConstRef& other, bw_t _bw) const
{
   RETURN_EMPTY_ON_EMPTY(_bw)
   RETURN_UNKNOWN_ON_UNKNOWN(_bw)
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, _bw, 0, 1));
   }

   const auto a = this->getSignedMin();
   const auto b = this->getSignedMax();
   const auto c = other->getSignedMin();
   const auto d = other->getSignedMax();
   if(b < c)
   {
      return RangeRef(new Range(Regular, _bw, 1, 1));
   }
   if(d <= a)
   {
      return RangeRef(new Range(Regular, _bw, 0, 0));
   }

   return RangeRef(new Range(Regular, _bw, 0, 1));
}

RangeRef Range::Sle(const RangeConstRef& other, bw_t _bw) const
{
   RETURN_EMPTY_ON_EMPTY(_bw)
   RETURN_UNKNOWN_ON_UNKNOWN(_bw)
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, _bw, 0, 1));
   }

   const auto a = this->getSignedMin();
   const auto b = this->getSignedMax();
   const auto c = other->getSignedMin();
   const auto d = other->getSignedMax();
   if(b <= c)
   {
      return RangeRef(new Range(Regular, _bw, 1, 1));
   }
   if(d < a)
   {
      return RangeRef(new Range(Regular, _bw, 0, 0));
   }

   return RangeRef(new Range(Regular, _bw, 0, 1));
}

RangeRef Range::SMin(const RangeConstRef& other) const
{
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }

   const auto thisMin = this->Slt(other, 1);
   if(thisMin->isConstant())
   {
      return thisMin->getUnsignedMin() ? RangeRef(this->clone()) : RangeRef(other->clone());
   }
   const auto min = std::min({this->getSignedMin(), other->getSignedMin()});
   const auto max = std::min({this->getSignedMax(), other->getSignedMax()});
   return RangeRef(new Range(Regular, bw, min, max));
}

RangeRef Range::SMax(const RangeConstRef& other) const
{
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }

   const auto thisMax = this->Sgt(other, 1);
   if(thisMax->isConstant())
   {
      return thisMax->getUnsignedMin() ? RangeRef(this->clone()) : RangeRef(other->clone());
   }
   const auto min = std::max({this->getSignedMin(), other->getSignedMin()});
   const auto max = std::max({this->getSignedMax(), other->getSignedMax()});
   return RangeRef(new Range(Regular, bw, min, max));
}

RangeRef Range::UMin(const RangeConstRef& other) const
{
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }

   const auto thisMin = this->Ult(other, 1);
   if(thisMin->isConstant())
   {
      return thisMin->getUnsignedMin() ? RangeRef(this->clone()) : RangeRef(other->clone());
   }
   const auto min = std::min({this->getUnsignedMin(), other->getUnsignedMin()});
   const auto max = std::min({this->getUnsignedMax(), other->getUnsignedMax()});
   return RangeRef(new Range(Regular, bw, min, max));
}

RangeRef Range::UMax(const RangeConstRef& other) const
{
   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);
   if(isAnti() || other->isAnti())
   {
      return RangeRef(new Range(Regular, bw));
   }

   const auto thisMax = this->Ugt(other, 1);
   if(thisMax->isConstant())
   {
      return thisMax->getUnsignedMin() ? RangeRef(this->clone()) : RangeRef(other->clone());
   }
   const auto min = std::max({this->getUnsignedMin(), other->getUnsignedMin()});
   const auto max = std::max({this->getUnsignedMax(), other->getUnsignedMax()});
   return RangeRef(new Range(Regular, bw, min, max));
}

RangeRef Range::abs() const
{
   if(isEmpty() || isUnknown())
   {
      return RangeRef(this->clone());
   }
   if(isAnti())
   {
      if(u < 0)
      {
         if(l == APInt::getSignedMinValue(bw))
         {
            return RangeRef(new Range(Regular, bw, 0, APInt::getSignedMaxValue(bw)));
         }
         return RangeRef(new Range(Anti, bw, APInt::getSignedMinValue(bw) + 1, -1));
      }
      if(l < 0)
      {
         if(l == APInt::getSignedMinValue(bw))
         {
            return RangeRef(new Range(Regular, bw, u + 1, APInt::getSignedMaxValue(bw)));
         }
         const auto min = std::min({-l, u});
         return RangeRef(new Range(Anti, bw, APInt::getSignedMinValue(bw) + 1, min));
      }
      return RangeRef(new Range(Anti, bw, APInt::getSignedMinValue(bw) + 1, l == 0 ? 0 : -1));
   }

   const auto smin = getSignedMin();
   const auto smax = getSignedMax();
   if(smax < 0)
   {
      if(smin == APInt::getSignedMinValue(bw))
      {
         return RangeRef(new Range(Anti, bw, smin + 1, -smax - 1));
      }
      return RangeRef(new Range(Regular, bw, -smax, -smin));
   }
   if(smin < 0)
   {
      if(smin == APInt::getSignedMinValue(bw))
      {
         return RangeRef(new Range(Anti, bw, smin + 1, -1));
      }
      const auto max = std::max({smax, -smin});
      return RangeRef(new Range(Regular, bw, 0, max));
   }
   return RangeRef(this->clone());
}

RangeRef Range::negate() const
{
   if(isEmpty() || isUnknown())
   {
      return RangeRef(this->clone());
   }
   if(isAnti())
   {
      return RangeRef(new Range(Anti, bw, -u, -l));
   }
   return RangeRef(new Range(Regular, bw, -u, -l));
}

// Truncate
// - if the source range is entirely inside max bit range, it is the result
// - else, the result is the max bit range
RangeRef Range::truncate(bw_t bitwidth) const
{
   if(isEmpty())
   {
      return RangeRef(new Range(Empty, bitwidth));
   }
   if(isUnknown())
   {
      return RangeRef(new Range(Unknown, bitwidth));
   }
   if(bitwidth == bw)
   {
      return RangeRef(this->clone());
   }
   const auto a = this->getSignedMin();
   const auto b = this->getSignedMax();
   if(isFullSet() || isAnti() || (b - a).abs() > APInt::getMaxValue(bitwidth))
   {
      return RangeRef(new Range(Regular, bitwidth));
   }

   const auto stmin = a.extOrTrunc(bitwidth, true);
   const auto stmax = b.extOrTrunc(bitwidth, true);

   if(a < 0 && b >= 0) // Zero crossing range
   {
      if(stmax < 0) // overflow
      {
         if(stmax >= stmin)
         {
            return RangeRef(new Range(Regular, bitwidth));
         }
         return RangeRef(new Range(Anti, bitwidth, stmax + 1, (stmin < 0 ? stmin : 0) - 1));
      }
      if(stmin > 0) // underflow
      {
         if(stmax >= stmin)
         {
            return RangeRef(new Range(Regular, bitwidth));
         }
         return RangeRef(new Range(Anti, bitwidth, stmax + 1, stmin - 1));
      }
   }

   if(stmin > stmax)
   {
      if(a.sign() == 1)
      {
         return RangeRef(new Range(Regular, bitwidth, stmax, stmin));
      }
      return RangeRef(new Range(Anti, bitwidth, stmax + 1, stmin - 1));
   }
   return RangeRef(new Range(Regular, bitwidth, stmin, stmax));
}

RangeRef Range::sextOrTrunc(bw_t bitwidth) const
{
   if(bitwidth <= bw)
   {
      return truncate(bitwidth);
   }

   if(isEmpty())
   {
      return RangeRef(new Range(Empty, bitwidth));
   }
   if(isUnknown())
   {
      return RangeRef(new Range(Unknown, bitwidth));
   }

   const auto this_min = this->getSignedMin();
   const auto this_max = this->getSignedMax();

   const auto res = std::minmax({this_min.extOrTrunc(bitwidth, true), this_max.extOrTrunc(bitwidth, true)});
   RangeRef sextRes(new Range(Regular, bitwidth, res.first, res.second));
   if(sextRes->isFullSet())
   {
      return RangeRef(new Range(Regular, bitwidth));
   }
   return sextRes;
}

RangeRef Range::zextOrTrunc(bw_t bitwidth) const
{
   if(bitwidth <= bw)
   {
      return truncate(bitwidth);
   }

   if(isEmpty())
   {
      return RangeRef(new Range(Empty, bitwidth));
   }
   if(isUnknown())
   {
      return RangeRef(new Range(Unknown, bitwidth));
   }
   if(isAnti())
   {
      if(getUnsignedMax() < APInt::getMaxValue(bw))
      {
         return RangeRef(new Range(Regular, bitwidth, u + 1, getUnsignedMax()));
      }
      return RangeRef(new Range(Regular, bitwidth, 0, APInt::getMaxValue(bw)));
   }

   if(this->getSignedMin() < 0 && this->getSignedMax() >= 0)
   {
      return RangeRef(new Range(Regular, bitwidth, 0, APInt::getMaxValue(bw)));
   }

   return RangeRef(new Range(Regular, bitwidth, this->getSignedMin().extOrTrunc(bw, false),
                             this->getSignedMax().extOrTrunc(bw, false)));
}

RangeRef Range::intersectWith(const RangeConstRef& other) const
{
#ifdef DEBUG_RANGE_OP
   PRINT_MSG("intersectWith-this: " << *this << std::endl << "intersectWith-other: " << *other);
#endif

   RETURN_EMPTY_ON_EMPTY(bw);
   RETURN_UNKNOWN_ON_UNKNOWN(bw);

   if(!this->isAnti() && !other->isAnti())
   {
      APInt res_l = getLower() > other->getLower() ? getLower() : other->getLower();
      APInt res_u = getUpper() < other->getUpper() ? getUpper() : other->getUpper();
      if(res_u < res_l)
      {
         return RangeRef(new Range(Empty, bw));
      }

      return RangeRef(new Range(Regular, bw, res_l, res_u));
   }
   if(this->isAnti() && !other->isAnti())
   {
      auto antiRange = this->getAnti();
      auto antil = antiRange->getLower();
      auto antiu = antiRange->getUpper();
      if(antil <= other->getLower())
      {
         if(other->getUpper() <= antiu)
         {
            return RangeRef(new Range(Empty, bw));
         }
         APInt res_l = other->getLower() > antiu ? other->getLower() : antiu + 1;
         APInt res_u = other->getUpper();
         return RangeRef(new Range(Regular, bw, res_l, res_u));
      }
      if(antiu >= other->getUpper())
      {
         THROW_ASSERT(other->getLower() < antil, "");
         APInt res_l = other->getLower();
         APInt res_u = other->getUpper() < antil ? other->getUpper() : antil - 1;
         return RangeRef(new Range(Regular, bw, res_l, res_u));
      }
      if(other->getLower() == Min && other->getUpper() == Max)
      {
         return RangeRef(this->clone());
      }
      if(antil > other->getUpper() || antiu < other->getLower())
      {
         return RangeRef(other->clone());
      }

      // we approximate to the range of other
      return RangeRef(other->clone());
   }
   if(!this->isAnti() && other->isAnti())
   {
      auto antiRange = other->getAnti();
      auto antil = antiRange->getLower();
      auto antiu = antiRange->getUpper();
      if(antil <= this->getLower())
      {
         if(this->getUpper() <= antiu)
         {
            return RangeRef(new Range(Empty, bw));
         }
         APInt res_l = this->getLower() > antiu ? this->getLower() : antiu + 1;
         APInt res_u = this->getUpper();
         return RangeRef(new Range(Regular, bw, res_l, res_u));
      }
      if(antiu >= this->getUpper())
      {
         THROW_ASSERT(this->getLower() < antil, "");
         APInt res_l = this->getLower();
         APInt res_u = this->getUpper() < antil ? this->getUpper() : antil - 1;
         return RangeRef(new Range(Regular, bw, res_l, res_u));
      }
      if(this->getLower() == Min && this->getUpper() == Max)
      {
         return RangeRef(other->clone());
      }
      if(antil > this->getUpper() || antiu < this->getLower())
      {
         return RangeRef(this->clone());
      }

      // we approximate to the range of this
      return RangeRef(this->clone());
   }

   auto antiRange_a = this->getAnti();
   auto antiRange_b = other->getAnti();
   auto antil_a = antiRange_a->getLower();
   auto antiu_a = antiRange_a->getUpper();
   auto antil_b = antiRange_b->getLower();
   auto antiu_b = antiRange_b->getUpper();
   if(antil_a > antil_b)
   {
      std::swap(antil_a, antil_b);
      std::swap(antiu_a, antiu_b);
   }
   if(antil_b > (antiu_a + 1))
   {
      return RangeRef(new Range(Anti, bw, antil_a, antiu_a));
   }
   auto res_l = antil_a;
   auto res_u = antiu_a > antiu_b ? antiu_a : antiu_b;
   if(res_l == Min && res_u == Max)
   {
      return RangeRef(new Range(Empty, bw));
   }

   return RangeRef(new Range(Anti, bw, res_l, res_u));
}

RangeRef Range::unionWith(const RangeConstRef& other) const
{
#ifdef DEBUG_RANGE_OP
   PRINT_MSG("unionWith-this: " << *this << std::endl << "unionWith-other: " << *other);
#endif

   if(this->isEmpty() || this->isUnknown())
   {
      return RangeRef(other->clone());
   }
   if(other->isEmpty() || other->isUnknown())
   {
      return RangeRef(this->clone());
   }
   if(!this->isAnti() && !other->isAnti())
   {
      APInt res_l = getLower() < other->getLower() ? getLower() : other->getLower();
      APInt res_u = getUpper() > other->getUpper() ? getUpper() : other->getUpper();
      return RangeRef(new Range(Regular, bw, res_l, res_u));
   }
   if(this->isAnti() && !other->isAnti())
   {
      auto antiRange = this->getAnti();
      auto antil = antiRange->getLower();
      auto antiu = antiRange->getUpper();
      THROW_ASSERT(antil != Min, "");
      THROW_ASSERT(antiu != Max, "");
      if(antil > other->getUpper() || antiu < other->getLower())
      {
         return RangeRef(this->clone());
      }
      if(antil > other->getLower() && antiu < other->getUpper())
      {
         return RangeRef(new Range(Regular, bw));
      }
      if(antil >= other->getLower() && antiu > other->getUpper())
      {
         return RangeRef(new Range(Anti, bw, other->getUpper() + 1, antiu));
      }
      if(antil < other->getLower() && antiu <= other->getUpper())
      {
         return RangeRef(new Range(Anti, bw, antil, other->getLower() - 1));
      }

      return RangeRef(new Range(Regular, bw)); // approximate to the full set
   }
   if(!this->isAnti() && other->isAnti())
   {
      auto antiRange = other->getAnti();
      auto antil = antiRange->getLower();
      auto antiu = antiRange->getUpper();
      THROW_ASSERT(antil != Min, "");
      THROW_ASSERT(antiu != Max, "");
      if(antil > this->getUpper() || antiu < this->getLower())
      {
         return RangeRef(other->clone());
      }
      if(antil > this->getLower() && antiu < this->getUpper())
      {
         return RangeRef(new Range(Regular, bw));
      }
      if(antil >= this->getLower() && antiu > this->getUpper())
      {
         return RangeRef(new Range(Anti, bw, this->getUpper() + 1, antiu));
      }
      if(antil < this->getLower() && antiu <= this->getUpper())
      {
         return RangeRef(new Range(Anti, bw, antil, this->getLower() - 1));
      }

      return RangeRef(new Range(Regular, bw)); // approximate to the full set
   }

   auto antiRange_a = this->getAnti();
   auto antiRange_b = other->getAnti();
   auto antil_a = antiRange_a->getLower();
   auto antiu_a = antiRange_a->getUpper();
   THROW_ASSERT(antil_a != Min, "");
   THROW_ASSERT(antiu_a != Max, "");
   auto antil_b = antiRange_b->getLower();
   auto antiu_b = antiRange_b->getUpper();
   THROW_ASSERT(antil_b != Min, "");
   THROW_ASSERT(antiu_b != Max, "");
   if(antil_a > antiu_b || antiu_a < antil_b)
   {
      return RangeRef(new Range(Regular, bw));
   }
   if(antil_a > antil_b && antiu_a < antiu_b)
   {
      return RangeRef(this->clone());
   }
   if(antil_b > antil_a && antiu_b < antiu_a)
   {
      return RangeRef(this->clone());
   }
   if(antil_a >= antil_b && antiu_b <= antiu_a)
   {
      return RangeRef(new Range(Anti, bw, antil_a, antiu_b));
   }
   if(antil_b >= antil_a && antiu_a <= antiu_b)
   {
      return RangeRef(new Range(Anti, bw, antil_b, antiu_a));
   }

   THROW_UNREACHABLE("unsupported condition");
   return nullptr;
}

void Range::print(std::ostream& OS) const
{
   if(this->isUnknown())
   {
      OS << "[Unknown," << +bw << "]";
   }
   else if(this->isEmpty())
   {
      OS << "[Empty," << +bw << "]";
   }
   else if(this->isAnti())
   {
      if(l == Min)
      {
         OS << ")-inf,";
      }
      else
      {
         OS << ")" << l << ",";
      }
      OS << +bw << ",";
      if(u == Max)
      {
         OS << "+inf(";
      }
      else
      {
         OS << u << "(";
      }
   }
   else
   {
      if(l == Min)
      {
         OS << "[-inf,";
      }
      else
      {
         OS << "[" << l << ",";
      }
      OS << +bw << ",";
      if(u == Max)
      {
         OS << "+inf]";
      }
      else
      {
         OS << u << "]";
      }
   }
}

std::string Range::ToString() const
{
   std::stringstream ss;
   print(ss);
   return ss.str();
}

std::ostream& operator<<(std::ostream& OS, const Range& R)
{
   R.print(OS);
   return OS;
}
