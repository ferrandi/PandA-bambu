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
 * @file ConditionalValueRange.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _RANGE_ANALYSIS_CONDITIONAL_VALUE_RANGE_HPP_
#define _RANGE_ANALYSIS_CONDITIONAL_VALUE_RANGE_HPP_
#include "refcount.hpp"

#include <map>

CONSTREF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(ValueRange);

class ConditionalValueRange
{
 private:
   const tree_nodeConstRef V;

   std::map<unsigned int, ValueRangeRef> bbVR;

 public:
   ConditionalValueRange(const tree_nodeConstRef& _V, const std::map<unsigned int, ValueRangeRef>& _bbVR);
   ConditionalValueRange(const tree_nodeConstRef& _V, unsigned int TrueBBI, unsigned int FalseBBI,
                         const ValueRangeRef& TrueVR, const ValueRangeRef& FalseVR);

   inline const std::map<unsigned int, ValueRangeRef>& getVR() const
   {
      return bbVR;
   }

   inline const tree_nodeConstRef& getVar() const
   {
      return V;
   }

   /**
    * @brief Add an interval associated to a new basic block
    *
    * @param bbi
    * @param cvr
    */
   void addVR(unsigned int bbi, const ValueRangeRef& cvr);
};

#endif // _RANGE_ANALYSIS_CONDITIONAL_VALUE_RANGE_HPP_