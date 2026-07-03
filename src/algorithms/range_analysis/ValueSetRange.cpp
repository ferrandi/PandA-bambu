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
 *              Copyright (C) 2026 Politecnico di Milano
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
 * @file ValueSetRange.cpp
 * @brief Implements helpers for converting disjoint interval sets to/from ranges.
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 * This source builds the logic that clamps value sets to variable domains,
 * folds contiguous intervals into anti-ranges when possible, and reconciles
 * conversions with the `Unknown` preference for inexact representations.
 */

#include "ValueSetRange.hpp"

#include <boost/icl/concept/interval.hpp>
#include <boost/icl/concept/interval_bounds.hpp>

namespace range_analysis
{
   namespace
   {
      APInt getDomainMin(Range::bw_t bw, bool isSigned)
      {
         return isSigned ? APInt::getSignedMinValue(bw) : APInt::getMinValue(bw);
      }

      APInt getDomainMax(Range::bw_t bw, bool isSigned)
      {
         return isSigned ? APInt::getSignedMaxValue(bw) : APInt::getMaxValue(bw);
      }

      ValueSet clampToDomain(const ValueSet& set, const ValueSet& domain)
      {
         return intersectValueSets(set, domain);
      }

      ValueSet domainMinusClosed(const ValueSet& domain, const APInt& lb, const APInt& ub)
      {
         if(domain.empty())
         {
            return ValueSet{};
         }
         ValueSet result = domain;
         result -= ValueInterval::closed(lb, ub);
         return result;
      }

   } // namespace

   ValueSet domainForVar(const VarNode* var)
   {
      const auto bw = var->getBitWidth();
      const bool isSigned = range_analysis::isSignedType(var->getValue());
      const auto minV = getDomainMin(bw, isSigned);
      const auto maxV = getDomainMax(bw, isSigned);
      ValueSet domain;
      domain += ValueInterval::closed(minV, maxV);
      return domain;
   }

   ValueSet singletonSet(const APInt& value)
   {
      ValueSet set;
      set += ValueInterval::closed(value, value);
      return set;
   }

   ValueSet intersectValueSets(const ValueSet& lhs, const ValueSet& rhs)
   {
      return lhs & rhs;
   }

   size_t intervalCount(const ValueSet& set)
   {
      return set.iterative_size();
   }

   ValueSet rangeToValueSet(const Range& range, const VarNode* var)
   {
      const auto domain = domainForVar(var);
      if(range.isUnknown() || range.isFullSet())
      {
         return domain;
      }
      if(range.isEmpty())
      {
         return ValueSet{};
      }

      if(range.isRegular())
      {
         ValueSet result;
         result += ValueInterval::closed(range.getLower(), range.getUpper());
         return clampToDomain(result, domain);
      }
      return clampToDomain(domainMinusClosed(domain, range.getLower(), range.getUpper()), domain);
   }

   refcount<ValueRange> toValueRange(const VarNode* var, const ValueSet& set, unsigned int maxIntervals,
                                     bool preferUnknown)
   {
      const auto bw = var->getBitWidth();
      if(intervalCount(set) > maxIntervals && maxIntervals > 0)
      {
         return refcount<ValueRange>(new ValueRange(Range(Unknown, bw)));
      }

      if(set.empty())
      {
         return refcount<ValueRange>(new ValueRange(Range(Empty, bw)));
      }

      ValueSetRange tmp(set, domainForVar(var), bw, range_analysis::isSignedType(var->getValue()), preferUnknown);
      Range approx(Unknown, bw);
      if(tmp.tryGetRange(approx))
      {
         return refcount<ValueRange>(new ValueRange(approx));
      }
      return refcount<ValueRange>(
          new ValueSetRange(set, domainForVar(var), bw, range_analysis::isSignedType(var->getValue()), preferUnknown));
   }

   bool tryGetRangeFromValueRange(const ValueRange& vr, Range& out)
   {
      return vr.tryGetRange(out);
   }
} // namespace range_analysis

namespace
{
   bool intervalIsClosed(const range_analysis::ValueInterval& interval)
   {
      return boost::icl::is_left_closed(interval.bounds()) && boost::icl::is_right_closed(interval.bounds());
   }
} // namespace

ValueSetRange::ValueSetRange(const range_analysis::ValueSet& set, const range_analysis::ValueSet& domain,
                             Range::bw_t bw, bool isSigned, bool preferUnknown)
    : ValueRange(Range(Unknown, bw)),
      set_(range_analysis::intersectValueSets(set, domain)),
      domain_(domain),
      bw_(bw),
      isSigned_(isSigned),
      preferUnknown_(preferUnknown)
{
   updateApproxRange();
}

const range_analysis::ValueSet& ValueSetRange::getSet() const
{
   return set_;
}

const range_analysis::ValueSet& ValueSetRange::getDomain() const
{
   return domain_;
}

bool ValueSetRange::tryGetRange(Range& out) const
{
   // Convert to a single Range only when the set can be represented exactly.
   // Otherwise signal failure so callers can keep the full ValueSetRange.
   if(set_.empty())
   {
      out = Range(Empty, bw_);
      return true;
   }
   if(hasSingleClosedInterval(out))
   {
      return true;
   }
   if(domain_.empty())
   {
      return false;
   }
   const auto domMin = boost::icl::lower(domain_);
   const auto domMax = boost::icl::upper(domain_);

   auto toClosedBounds = [&](const range_analysis::ValueInterval& interval, APInt& lb, APInt& ub) -> bool {
      if(boost::icl::is_empty(interval))
      {
         return false;
      }
      lb = boost::icl::first(interval);
      ub = boost::icl::last(interval);
      return ub >= lb;
   };

   size_t missingCount = 0;
   APInt missingL;
   APInt missingU;
   APInt current = domMin;
   for(const auto& interval : set_)
   {
      APInt lb;
      APInt ub;
      if(!toClosedBounds(interval, lb, ub))
      {
         continue;
      }
      if(lb > current)
      {
         const auto gapU = lb - Range::MinDelta;
         if(gapU >= current)
         {
            missingL = current;
            missingU = gapU;
            ++missingCount;
            if(missingCount > 1)
            {
               return false;
            }
         }
      }
      const auto next = ub + Range::MinDelta;
      if(next > current)
      {
         current = next;
      }
      if(current > domMax)
      {
         break;
      }
   }
   if(current <= domMax)
   {
      missingL = current;
      missingU = domMax;
      ++missingCount;
   }

   if(missingCount == 0)
   {
      out = Range(Regular, bw_, domMin, domMax);
      return true;
   }
   if(missingCount == 1)
   {
      out = Range(Anti, bw_, missingL, missingU);
      return true;
   }
   return false;
}

ValueRangeType ValueSetRange::getValueId() const
{
   return ValueSetRangeId;
}

void ValueSetRange::print(std::ostream& OS) const
{
   if(set_.empty())
   {
      Range(Empty, bw_).print(OS);
      return;
   }

   Range single(Unknown, bw_);
   if(hasSingleClosedInterval(single))
   {
      single.print(OS);
      return;
   }

   OS << "{";
   bool first = true;
   for(const auto& interval : set_)
   {
      if(!first)
      {
         OS << ", ";
      }
      first = false;
      const auto bounds = interval.bounds();
      OS << (boost::icl::is_left_closed(bounds) ? "[" : "(");
      OS << boost::icl::lower(interval) << "," << boost::icl::upper(interval);
      OS << (boost::icl::is_right_closed(bounds) ? "]" : ")");
   }
   OS << "}";
}

bool ValueSetRange::hasSingleClosedInterval(Range& out) const
{
   if(set_.empty())
   {
      out = Range(Empty, bw_);
      return true;
   }
   if(set_.iterative_size() != 1)
   {
      return false;
   }
   const auto& interval = *set_.begin();
   if(!intervalIsClosed(interval))
   {
      return false;
   }
   const auto inf = boost::icl::lower(interval);
   const auto sup = boost::icl::upper(interval);
   out = Range(Regular, bw_, inf, sup);
   return true;
}

void ValueSetRange::updateApproxRange()
{
   Range approx(Unknown, bw_);
   if(!tryGetRange(approx))
   {
      if(!preferUnknown_)
      {
         const auto minV = isSigned_ ? APInt::getSignedMinValue(bw_) : APInt::getMinValue(bw_);
         const auto maxV = isSigned_ ? APInt::getSignedMaxValue(bw_) : APInt::getMaxValue(bw_);
         approx = Range(Regular, bw_, minV, maxV);
      }
   }
   setRange(approx);
}
