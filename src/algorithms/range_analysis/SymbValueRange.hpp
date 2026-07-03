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
 *              Copyright (C) 2019-2026 Politecnico di Milano
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
 * @file SymbValueRange.hpp
 * @brief This is an interval that contains a symbolic limit, which is given by the bounds of a program name, e.g.:
 * [-inf, ub(b) + 1].
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef _RANGE_ANALYSIS_SYMB_VALUE_RANGE_HPP_
#define _RANGE_ANALYSIS_SYMB_VALUE_RANGE_HPP_

#include "ValueRange.hpp"
#include "VarNode.hpp"
#include "range_analysis_helper.hpp"

class SymbRange : public ValueRange
{
 private:
   /* The bound. It is a node which limits the interval of this range */
   VarNode* const bound;

   /**
    * @brief The predicate of the operation in which this interval takes part.
    * It is useful to know how we can constrain this interval after we fix the intersects.
    */
   kind_R pred;

 public:
   SymbRange(const Range& range, VarNode* bound, kind_R pred);
   SymbRange(const SymbRange&) = delete;
   SymbRange(SymbRange&&) = delete;
   SymbRange& operator=(const SymbRange&) = delete;
   SymbRange& operator=(SymbRange&&) = delete;

   void print(std::ostream& OS) const override;

   ValueRangeType getValueId() const override;

   inline kind_R getOperation() const
   {
      return pred;
   }

   inline VarNode* getBound() const
   {
      return bound;
   }

   /**
    * @brief Replace symbolic bound with hard-wired constants.
    *
    * @param sink
    * @return Range
    */
   Range solveFuture(const VarNode* sink) const;

   static inline bool classof(SymbRange const*)
   {
      return true;
   }

   static inline bool classof(ValueRange const* BI)
   {
      return BI->getValueId() == SymbRangeId;
   }
};

#endif // _RANGE_ANALYSIS_VALUE_RANGE_HPP_
