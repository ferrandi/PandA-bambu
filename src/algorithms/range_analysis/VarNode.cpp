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
 * @file VarNode.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "VarNode.hpp"

#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "range_analysis_helper.hpp"

VarNode::VarNode(const ir_nodeConstRef& _V, unsigned int _function_id, unsigned int _use_bbi)
    : id(makeId(_V, _use_bbi)),
      V(_V),
      function_id(_function_id),
      interval(ir_helper::TypeRange(_V, Unknown)),
      abstractState(0)
{
   THROW_ASSERT(_V, "Variable cannot be null");
}

void VarNode::init(bool outside)
{
   THROW_ASSERT(ir_helper::TypeSize(V), "Bitwidth not valid");
   if(interval.isUnknown()) // Ranges already initialized come from user defined hints and shouldn't be overwritten
   {
      if(ir_helper::IsConstant(V))
      {
         interval = ir_helper::NodeRange(V);
      }
      else
      {
         interval = ir_helper::TypeRange(V, outside ? Regular : Unknown);
      }
   }
}

Range VarNode::getMaxRange() const
{
   return ir_helper::TypeRange(V, Regular);
}

void VarNode::storeAbstractState()
{
   THROW_ASSERT(!interval.isUnknown(), "storeAbstractState doesn't handle empty set");

   if(interval.getLower() == Range::Min)
   {
      if(interval.getUpper() == Range::Max)
      {
         abstractState = '?';
      }
      else
      {
         abstractState = '-';
      }
   }
   else if(interval.getUpper() == Range::Max)
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
   if(ir_helper::IsConstant(V))
   {
      OS << ir_helper::GetConstValue(V);
   }
   else
   {
      OS << V;
   }
   OS << " ";
   getRange().print(OS);
}

std::string VarNode::ToString() const
{
   std::stringstream ss;
   print(ss);
   return ss.str();
}

VarNode::key_type VarNode::makeId(const ir_nodeConstRef& V, unsigned int use_bbi)
{
   return (static_cast<VarNode::key_type>(V->index) << 32) |
          static_cast<VarNode::key_type>(ir_helper::IsConstant(V) ? BB_ENTRY : use_bbi);
}