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
 * @file VarNode.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "VarNode.hpp"

#include "range_analysis_helper.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_reindex.hpp"

VarNode::VarNode(const tree_nodeConstRef& _V, unsigned int _function_id, unsigned int _use_bbi)
    : id(makeId(_V, _use_bbi)), V(_V), function_id(_function_id), abstractState(0)
{
   THROW_ASSERT(_V != nullptr, "Variable cannot be null");
   THROW_ASSERT(_V->get_kind() == tree_reindex_K, "Variable should be a tree_reindex node");
   interval = tree_helper::TypeRange(_V, Unknown);
}

void VarNode::init(bool outside)
{
   THROW_ASSERT(tree_helper::TypeSize(V), "Bitwidth not valid");
   THROW_ASSERT(interval, "Interval should be initialized during VarNode construction");
   if(interval->isUnknown()) // Ranges already initialized come from user defined hints and shouldn't be overwritten
   {
      if(tree_helper::IsConstant(V))
      {
         interval = tree_helper::Range(V);
      }
      else
      {
         interval = tree_helper::TypeRange(V, outside ? Regular : Unknown);
      }
   }
}

RangeRef VarNode::getMaxRange() const
{
   return tree_helper::TypeRange(V, Regular);
}

void VarNode::storeAbstractState()
{
   THROW_ASSERT(!interval->isUnknown(), "storeAbstractState doesn't handle empty set");

   if(interval->getLower() == Range::Min)
   {
      if(interval->getUpper() == Range::Max)
      {
         abstractState = '?';
      }
      else
      {
         abstractState = '-';
      }
   }
   else if(interval->getUpper() == Range::Max)
   {
      abstractState = '+';
   }
   else
   {
      abstractState = '0';
   }
}

void VarNode::print(std::ostream& OS) const
{
   if(tree_helper::IsConstant(V))
   {
      OS << tree_helper::GetConstValue(V);
   }
   else
   {
      OS << V;
   }
   OS << " ";
   getRange()->print(OS);
}

std::string VarNode::ToString() const
{
   std::stringstream ss;
   print(ss);
   return ss.str();
}

VarNode::key_type VarNode::makeId(const tree_nodeConstRef& V, unsigned int use_bbi)
{
   return static_cast<unsigned long long>(GET_INDEX_CONST_NODE(V)) << 32 |
          (tree_helper::IsConstant(V) ? BB_ENTRY : use_bbi);
   // return V;
}