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
 * @file SymbValueRange.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "SymbValueRange.hpp"

#include "exceptions.hpp"
#include "ir_helper.hpp"
#include "range_analysis_helper.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"

SymbRange::SymbRange(const Range& _range, VarNode* _bound, kind_R _pred)
    : ValueRange(_range), bound(_bound), pred(_pred)
{
}

void SymbRange::print(std::ostream& OS) const
{
   const auto bnd = getBound()->getValue();
   switch(getOperation())
   {
      case unsigned_eq_node_R:
      case eq_node_R: // equal
         OS << "[lb(" << bnd << "), ub(" << bnd << ")]";
         break;
      case unsigned_le_node_R:
         OS << "[0, ub(" << bnd << ")]";
         break;
      case le_node_R: // sign less or equal
         OS << "[-inf, ub(" << bnd << ")]";
         break;
      case unsigned_lt_node_R:
         OS << "[0, ub(" << bnd << ") - 1]";
         break;
      case lt_node_R: // sign less than
         OS << "[-inf, ub(" << bnd << ") - 1]";
         break;
      case unsigned_ge_node_R:
      case ge_node_R: // sign greater or equal
         OS << "[lb(" << bnd << "), +inf]";
         break;
      case unsigned_gt_node_R:
      case gt_node_R: // sign greater than
         OS << "[lb(" << bnd << " - 1), +inf]";
         break;
      case ne_node_R:
         OS << ")b(" << bnd << ")(";
         break;
      default:
         THROW_UNREACHABLE("Unexpected operation: " + range_analysis::getString(getOperation()));
   }
}

ValueRangeType SymbRange::getValueId() const
{
   return SymbRangeId;
}

Range SymbRange::solveFuture(const VarNode* _sink) const
{
   // Get the lower and the upper bound of the
   // node which bounds this intersection.
   const auto boundRange = bound->getRange();
   const auto sinkRange = _sink->getRange();
   THROW_ASSERT(!boundRange.isEmpty(), "Bound range should not be empty");
   THROW_ASSERT(!sinkRange.isEmpty(), "Sink range should not be empty");

   auto IsAnti = boundRange.isAnti() || sinkRange.isAnti();
   const auto l = IsAnti ? (boundRange.isUnknown() ? Range::Min : boundRange.getUnsignedMin()) : boundRange.getLower();
   const auto u = IsAnti ? (boundRange.isUnknown() ? Range::Max : boundRange.getUnsignedMax()) : boundRange.getUpper();

   // Get the lower and upper bound of the interval of this operation
   const auto lower = IsAnti ? (sinkRange.isUnknown() ? Range::Min : sinkRange.getUnsignedMin()) : sinkRange.getLower();
   const auto upper = IsAnti ? (sinkRange.isUnknown() ? Range::Max : sinkRange.getUnsignedMax()) : sinkRange.getUpper();

   const auto bw = getRange().getBitWidth();
   switch(getOperation())
   {
      case unsigned_eq_node_R:
      case eq_node_R: // equal
         return Range(Regular, bw, l, u);
      case le_node_R: // signed less or equal
         if(lower > u)
         {
            return Range(Empty, bw);
         }
         else
         {
            return Range(Regular, bw, lower, u);
         }
      case lt_node_R: // signed less than
         if(u != Range::Max && u != APInt::getSignedMaxValue(bw))
         {
            if(lower > (u - 1))
            {
               return Range(Empty, bw);
            }

            return Range(Regular, bw, lower, u - 1);
         }
         else
         {
            if(lower > u)
            {
               return Range(Empty, bw);
            }

            return Range(Regular, bw, lower, u);
         }
      case ge_node_R: // signed greater or equal
         if(l > upper)
         {
            return Range(Empty, bw);
         }
         else
         {
            return Range(Regular, bw, l, upper);
         }
      case gt_node_R: // signed greater than
         if(l != Range::Min && l != APInt::getSignedMinValue(bw))
         {
            if((l + 1) > upper)
            {
               return Range(Empty, bw);
            }

            return Range(Regular, bw, l + 1, upper);
         }
         else
         {
            if(l > upper)
            {
               return Range(Empty, bw);
            }

            return Range(Regular, bw, l, upper);
         }
      case ne_node_R:
      case unsigned_ge_node_R:
      case unsigned_gt_node_R:
      case unsigned_le_node_R:
      case unsigned_lt_node_R:
         break;
      default:
         THROW_UNREACHABLE("Unexpected operation: " + range_analysis::getString(getOperation()));
         break;
   }
   return ir_helper::TypeRange(_sink->getValue(), Regular);
}

#pragma GCC diagnostic pop
