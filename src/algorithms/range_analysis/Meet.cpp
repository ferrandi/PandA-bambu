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
 * @file Meet.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "Meet.hpp"

#include "dbgPrintHelper.hpp"

#ifndef NDEBUG
int Meet::debug_level = DEBUG_LEVEL_NONE;
#endif

const APInt& Meet::getFirstGreaterFromVector(const std::vector<APInt>& constantvector, const APInt& val)
{
   for(const auto& vapint : constantvector)
   {
      if(vapint >= val)
      {
         return vapint;
      }
   }
   return Range::Max;
}

/*
 * Get the first constant from vector less than val
 */
const APInt& Meet::getFirstLessFromVector(const std::vector<APInt>& constantvector, const APInt& val)
{
   for(auto vit = constantvector.rbegin(), vend = constantvector.rend(); vit != vend; ++vit)
   {
      const auto& vapint = *vit;
      if(vapint <= val)
      {
         return vapint;
      }
   }
   return Range::Min;
}

bool Meet::fixed(OpNode* op)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "FIXED:: " + op->ToString() + ":");
   const auto oldInterval = op->getSink()->getRange();
   const auto newInterval = op->eval();

   op->getSink()->setRange(newInterval);
   const auto modified = !oldInterval.isSameRange(newInterval);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  std::string(modified ? "    new " : "        ") + oldInterval.ToString() + " -> " +
                      newInterval.ToString());
   return modified;
}

bool Meet::widen(OpNode* op, const std::vector<APInt>& constantvector)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "WIDEN:: " + op->ToString() + ":");
   const auto oldRange = op->getSink()->getRange();
   const auto newRange = op->eval();

   auto intervalWiden = [&](const Range& oldInterval, const Range& newInterval) {
      const auto bw = oldInterval.getBitWidth();
      if(oldInterval.isUnknown() || oldInterval.isEmpty() || oldInterval.isAnti() || newInterval.isEmpty() ||
         newInterval.isAnti())
      {
         if(oldInterval.isAnti() && newInterval.isAnti() && !newInterval.isSameRange(oldInterval))
         {
            const auto oldAnti = oldInterval.getAnti();
            const auto newAnti = newInterval.getAnti();
            const auto& oldLower = oldAnti.getLower();
            const auto& oldUpper = oldAnti.getUpper();
            const auto& newLower = newAnti.getLower();
            const auto& newUpper = newAnti.getUpper();
            const auto& nlconstant = getFirstGreaterFromVector(constantvector, newLower);
            const auto& nuconstant = getFirstLessFromVector(constantvector, newUpper);

            if(newLower > oldLower || newUpper < oldUpper)
            {
               const auto& l = newLower > oldLower ? nlconstant : oldLower;
               const auto& u = newUpper < oldUpper ? nuconstant : oldUpper;
               if(l > u)
               {
                  return Range(Regular, bw);
               }
               return Range(Anti, bw, l, u);
            }
         }
         else
         {
            // Sometimes sigma operation could cause confusion after maximum widening has been reached and generate
            // loops
            if(!oldInterval.isUnknown() && oldInterval.isFullSet() && newInterval.isAnti())
            {
               return oldInterval;
            }
            if(oldInterval.isRegular() && newInterval.isAnti())
            {
               return oldInterval.unionWith(newInterval);
            }
            return newInterval;
         }
      }
      else
      {
         const auto& oldLower = oldInterval.getLower();
         const auto& oldUpper = oldInterval.getUpper();
         const auto& newLower = newInterval.getLower();
         const auto& newUpper = newInterval.getUpper();

         // Jump-set
         const auto& nlconstant = getFirstLessFromVector(constantvector, newLower);
         const auto& nuconstant = getFirstGreaterFromVector(constantvector, newUpper);

         if(newLower < oldLower || newUpper > oldUpper)
         {
            return Range(Regular, bw, newLower < oldLower ? nlconstant : oldLower,
                         newUpper > oldUpper ? nuconstant : oldUpper);
         }
      }
      //    THROW_UNREACHABLE("Meet::widen unreachable state");
      return oldInterval;
   };

   const auto widen = intervalWiden(oldRange, newRange);
   //    THROW_ASSERT(oldRange.getSpan() <= widen->getSpan(), "Widening should produce bigger range: " +
   //    oldRange.ToString() + " > " + widen->ToString());
   op->getSink()->setRange(widen);

   const auto modified = !oldRange.isSameRange(widen);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  std::string(modified ? "    new " : "        ") + oldRange.ToString() + " -> " + newRange.ToString() +
                      " -> " + widen.ToString());
   return modified;
}

bool Meet::growth(OpNode* op)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "GROWTH:: " + op->ToString() + ":");
   const auto oldRange = op->getSink()->getRange();
   const auto newRange = op->eval();

   auto intervalGrowth = [](const Range& oldInterval, const Range& newInterval) {
      if(oldInterval.isUnknown() || oldInterval.isEmpty() || oldInterval.isAnti() || newInterval.isEmpty() ||
         newInterval.isAnti())
      {
         return newInterval;
      }
      auto bw = oldInterval.getBitWidth();
      const auto& oldLower = oldInterval.getLower();
      const auto& oldUpper = oldInterval.getUpper();
      const auto& newLower = newInterval.getLower();
      const auto& newUpper = newInterval.getUpper();

      if(newLower < oldLower || newUpper > oldUpper)
      {
         return Range(Regular, bw, newLower < oldLower ? Range::Min : oldLower,
                      newUpper > oldUpper ? Range::Max : oldUpper);
      }
      //    THROW_UNREACHABLE("Meet::growth unreachable state");
      return oldInterval;
   };

   op->getSink()->setRange(intervalGrowth(oldRange, newRange));

   const auto sinkRange = op->getSink()->getRange();
   const auto modified = !oldRange.isSameRange(sinkRange);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  std::string(modified ? "     new " : "         ") + oldRange.ToString() + " -> " +
                      sinkRange.ToString());
   return modified;
}

/// This is the meet operator of the cropping analysis. Whereas the growth
/// analysis expands the bounds of each variable, regardless of intersections
/// in the constraint graph, the cropping analysis shrinks these bounds back
/// to ranges that respect the intersections.
bool Meet::narrow(OpNode* op, const std::vector<APInt>& constantvector)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "NARROW:: " + op->ToString() + ":");
   const auto oldRange = op->getSink()->getRange();
   const auto newRange = op->eval();

   auto intervalNarrow = [&](const Range& oldInterval, const Range& newInterval) {
      if(newInterval.isConstant())
      {
         return newInterval;
      }
      const auto bw = oldInterval.getBitWidth();
      if(oldInterval.isAnti() || newInterval.isAnti() || oldInterval.isEmpty() || newInterval.isEmpty())
      {
         if(oldInterval.isAnti() && newInterval.isAnti() && !newInterval.isSameRange(oldInterval))
         {
            const auto oldAnti = oldInterval.getAnti();
            const auto newAnti = newInterval.getAnti();
            const auto& oldLower = oldAnti.getLower();
            const auto& oldUpper = oldAnti.getUpper();
            const auto& newLower = newAnti.getLower();
            const auto& newUpper = newAnti.getUpper();
            const auto& nlconstant = getFirstGreaterFromVector(constantvector, newLower);
            const auto& nuconstant = getFirstLessFromVector(constantvector, newUpper);

            if(oldLower < nlconstant && oldUpper > nuconstant)
            {
               if(nlconstant <= nuconstant)
               {
                  return Range(Anti, bw, nlconstant, nuconstant);
               }
               return Range(Regular, bw);
            }
            if(oldLower < nlconstant)
            {
               return Range(Anti, bw, nlconstant, oldUpper);
            }
            if(oldUpper > nuconstant)
            {
               return Range(Anti, bw, oldLower, nuconstant);
            }
         }
         else if(newInterval.isUnknown() || !newInterval.isFullSet())
         {
            return newInterval;
         }
      }
      else
      {
         const auto& oLower = oldInterval.isFullSet() ? Range::Min : oldInterval.getLower();
         const auto& oUpper = oldInterval.isFullSet() ? Range::Max : oldInterval.getUpper();
         const auto& nLower = newInterval.isFullSet() ? Range::Min : newInterval.getLower();
         const auto& nUpper = newInterval.isFullSet() ? Range::Max : newInterval.getUpper();
         auto sinkInterval = oldInterval;
         if((oLower == Range::Min) && (nLower != Range::Min))
         {
            sinkInterval = Range(Regular, bw, nLower, oUpper);
         }
         else if(nLower < oLower)
         {
            sinkInterval = Range(Regular, bw, nLower, oUpper);
         }
         if(!sinkInterval.isAnti())
         {
            if((oUpper == Range::Max) && (nUpper != Range::Max))
            {
               sinkInterval = Range(Regular, bw, sinkInterval.getLower(), nUpper);
            }
            else if(oUpper < nUpper)
            {
               sinkInterval = Range(Regular, bw, sinkInterval.getLower(), nUpper);
            }
         }
         return sinkInterval;
      }
      return oldInterval;
   };

   const auto narrow = intervalNarrow(oldRange, newRange);
   //    THROW_ASSERT(oldRange.getSpan() >= narrow->getSpan(), "Narrowing should produce smaller range: " +
   //    oldRange.ToString() + " < " + narrow->ToString());
   op->getSink()->setRange(narrow);

   const auto modified = !oldRange.isSameRange(narrow);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  std::string(modified ? "     new " : "         ") + oldRange.ToString() + " -> " + narrow.ToString());
   return modified;
}

bool Meet::crop(OpNode* op)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "CROP:: " + op->ToString() + ":");
   const auto oldRange = op->getSink()->getRange();
   const auto newRange = op->eval();
   const char _abstractState = op->getSink()->getAbstractState();

   auto intervalCrop = [](const Range& oldInterval, const Range& newInterval, char abstractState) {
      if(oldInterval.isAnti() || newInterval.isAnti() || oldInterval.isEmpty() || newInterval.isEmpty())
      {
         return newInterval;
      }

      const auto bw = oldInterval.getBitWidth();
      if((abstractState == '-' || abstractState == '?') && (newInterval.getLower() > oldInterval.getLower()))
      {
         return Range(Regular, bw, newInterval.getLower(), oldInterval.getUpper());
      }

      if((abstractState == '+' || abstractState == '?') && (newInterval.getUpper() < oldInterval.getUpper()))
      {
         return Range(Regular, bw, oldInterval.getLower(), newInterval.getUpper());
      }
      return oldInterval;
   };

   op->getSink()->setRange(intervalCrop(oldRange, newRange, _abstractState));

   const auto sinkRange = op->getSink()->getRange();
   const auto modified = !oldRange.isSameRange(sinkRange);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  std::string(modified ? "   new " : "       ") + oldRange.ToString() + " -> " + sinkRange.ToString());
   return modified;
}
