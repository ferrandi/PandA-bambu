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
 * @file PhiOpNode.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "PhiOpNode.hpp"

#include "NodeContainer.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "ir_helper.hpp"

PhiOpNode::PhiOpNode(VarNode* _sink, const ir_nodeConstRef& _inst) : OpNode(_sink, _inst)
{
}

OpNode::OpNodeType PhiOpNode::getValueId() const
{
   return OpNodeType::OpNodeType_Phi;
}

std::vector<VarNode*> PhiOpNode::getSources() const
{
   return sources;
}

void PhiOpNode::replaceSource(const VarNode* _old, VarNode* _new)
{
   for(auto& src : sources)
   {
      if(_old->getId() == src->getId())
      {
         src = _new;
      }
   }
}

/// Computes the interval of the sink based on the interval of the sources.
/// The result of evaluating a phi-function is the union of the ranges of
/// every variable used in the phi.
Range PhiOpNode::eval() const
{
   THROW_ASSERT(sources.size() > 0, "Phi operation sources list empty");
   auto result = ir_helper::TypeRange(getSink()->getValue(), Empty);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, getSink()->getValue()->ToString() + " = PHI");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   // Iterate over the sources of the phiop
   for(const auto varNode : sources)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  ->" + varNode->ToString());
      result = result.unionWith(varNode->getRange());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--  = " + result.ToString());

   Range aux(Unknown, result.getBitWidth());
   if(getIntersect()->tryGetRange(aux) && !aux.isFullSet())
   {
      const auto _intersect = result.intersectWith(aux);
      if(!_intersect.isEmpty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---aux = " + aux.ToString() + " from " + getIntersect()->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---res = " + _intersect.ToString());
         result = _intersect;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---res = " + result.ToString());
   return result;
}

std::function<OpNode*(NodeContainer*)> PhiOpNode::opCtorGenerator(const ir_nodeConstRef& stmt,
                                                                  const application_managerRef&)
{
   if(stmt->get_kind() != phi_stmt_K)
   {
      return nullptr;
   }
   return [stmt](NodeContainer* NC) {
      const auto gp = GetPointer<const phi_stmt>(stmt);
      if(gp->virtual_flag)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---This is a virtual phi, skipping...");
         return static_cast<PhiOpNode*>(nullptr);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                     "Analysing phi operation " + gp->ToString());
      const auto function_id = gp->parent->index;
      // Create the sink.
      const auto sink = NC->addVarNode(gp->res, function_id);
      const auto op = new PhiOpNode(sink, stmt);

      // Create the sources.
      for(const auto& [var, bbi] : gp->CGetDefEdgesList())
      {
         const auto source = NC->addVarNode(var, function_id);
         op->addSource(source);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                     "---Added PhiOp with range " + op->getIntersect()->ToString() + " and " +
                         std::to_string(op->getNumSources()) + " sources");
      return op;
   };
}

void PhiOpNode::print(std::ostream& OS) const
{
   OS << getSink()->getValue() << " = PHI<";
   size_t i = 0;
   for(; i < (sources.size() - 1); ++i)
   {
      OS << sources.at(i)->getValue() << ", ";
   }
   OS << sources.at(i)->getValue() << ">";
}

std::string PhiOpNode::getName() const
{
   return ir_node::GetString(phi_stmt_K);
}
