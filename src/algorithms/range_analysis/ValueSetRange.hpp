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
 * @file ValueSetRange.hpp
 * @brief Wrapper for disjoint interval sets used by range analysis predicates.
 *
 * Provides conversions between `Range` objects and disjoint interval
 * sets, including helpers that respect domain limits and the preferred `Unknown`
 * fallback when the set cannot be represented compactly.
 */
#ifndef _RANGE_ANALYSIS_VALUE_SET_RANGE_HPP_
#define _RANGE_ANALYSIS_VALUE_SET_RANGE_HPP_

#include "ValueRange.hpp"
#include "VarNode.hpp"
#include "range_analysis_helper.hpp"
#include "refcount.hpp"

#include <boost/icl/interval.hpp>
#include <boost/icl/interval_set.hpp>
#include <ostream>

namespace range_analysis
{
   namespace icl = boost::icl;
   using ValueInterval = icl::interval<APInt>::type;
   using ValueSet = icl::interval_set<APInt>;

   ValueSet intersectValueSets(const ValueSet& lhs, const ValueSet& rhs);
   ValueSet domainForVar(const VarNode* var);
   ValueSet singletonSet(const APInt& value);
   size_t intervalCount(const ValueSet& set);
   ValueSet rangeToValueSet(const Range& range, const VarNode* var);
   refcount<ValueRange> toValueRange(const VarNode* var, const ValueSet& set, unsigned int maxIntervals,
                                     bool preferUnknown);
   bool tryGetRangeFromValueRange(const ValueRange& vr, Range& out);
} // namespace range_analysis

// Represents a disjoint set of intervals when a single Range cannot be exact.
// The base ValueRange::range stores a best-effort approximation.
class ValueSetRange : public ValueRange
{
 public:
   ValueSetRange(const range_analysis::ValueSet& set, const range_analysis::ValueSet& domain, Range::bw_t bw,
                 bool isSigned, bool preferUnknown);

   const range_analysis::ValueSet& getSet() const;
   const range_analysis::ValueSet& getDomain() const;

   // Try to express the set as a single Range (exact or anti-range).
   // Returns false when the set cannot be represented without losing precision.
   bool tryGetRange(Range& out) const override;
   ValueRangeType getValueId() const override;
   void print(std::ostream& OS) const override;

   static inline bool classof(ValueRange const* BI)
   {
      return BI->getValueId() == ValueSetRangeId;
   }

 private:
   range_analysis::ValueSet set_;
   range_analysis::ValueSet domain_;
   Range::bw_t bw_;
   bool isSigned_;
   bool preferUnknown_;

   bool hasSingleClosedInterval(Range& out) const;
   void updateApproxRange();
};

#endif // _RANGE_ANALYSIS_VALUE_SET_RANGE_HPP_
