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
 *              Copyright (C) 2019-2024 Politecnico di Milano
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
 * @file SymbValueRange.hpp
 * @brief This is an interval that contains a symbolic limit, which is given by the bounds of a program name, e.g.:
 * [-inf, ub(b) + 1].
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _RANGE_ANALYSIS_SYMB_VALUE_RANGE_HPP_
#define _RANGE_ANALYSIS_SYMB_VALUE_RANGE_HPP_
#include "ValueRange.hpp"
#include "VarNode.hpp"
#include "tree_common.hpp"

class SymbRange : public ValueRange
{
 private:
   /* The bound. It is a node which limits the interval of this range */
   VarNode* const bound;

   /**
    * @brief The predicate of the operation in which this interval takes part.
    * It is useful to know how we can constrain this interval after we fix the intersects.
    */
   kind pred;

 public:
   SymbRange(const RangeConstRef& range, VarNode* bound, kind pred);
   ~SymbRange() override = default;
   SymbRange(const SymbRange&) = delete;
   SymbRange(SymbRange&&) = delete;
   SymbRange& operator=(const SymbRange&) = delete;
   SymbRange& operator=(SymbRange&&) = delete;

   void print(std::ostream& OS) const override;

   ValueRangeType getValueId() const override;

   inline kind getOperation() const
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
    * @return RangeConstRef
    */
   RangeConstRef solveFuture(const VarNode* sink) const;

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