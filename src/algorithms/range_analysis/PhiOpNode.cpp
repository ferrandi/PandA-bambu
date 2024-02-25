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
 * @file PhiOpNode.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "PhiOpNode.hpp"

#include "NodeContainer.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "tree_helper.hpp"
#include "tree_reindex.hpp"

PhiOpNode::PhiOpNode(VarNode* _sink, const tree_nodeConstRef& _inst) : OpNode(_sink, _inst)
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

void PhiOpNode::replaceSource(VarNode* _old, VarNode* _new)
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
RangeRef PhiOpNode::eval() const
{
   THROW_ASSERT(sources.size() > 0, "Phi operation sources list empty");
   auto result = tree_helper::TypeRange(getSink()->getValue(), Empty);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, GET_CONST_NODE(getSink()->getValue())->ToString() + " = PHI");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   // Iterate over the sources of the phiop
   for(const auto varNode : sources)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  ->" + varNode->ToString());
      result = result->unionWith(varNode->getRange());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--  = " + result->ToString());

   if(!getIntersect()->getRange()->isFullSet())
   {
      const auto aux = getIntersect()->getRange();
      const auto _intersect = result->intersectWith(aux);
      if(!_intersect->isEmpty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---aux = " + aux->ToString() + " from " + getIntersect()->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---result = " + _intersect->ToString());
         result = _intersect;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---res = " + result->ToString());
   return result;
}

std::function<OpNode*(NodeContainer*)> PhiOpNode::opCtorGenerator(const tree_nodeConstRef& stmt,
                                                                  const application_managerRef&)
{
   const auto gp = GetPointer<const gimple_phi>(GET_CONST_NODE(stmt));
   if(!gp || gp->CGetDefEdgesList().size() <= 1)
   {
      return nullptr;
   }
   return [stmt, gp](NodeContainer* NC) {
      if(gp->virtual_flag)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---This is a virtual phi, skipping...");
         return static_cast<PhiOpNode*>(nullptr);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                     "Analysing phi operation " + gp->ToString());
      const auto function_id = GET_INDEX_CONST_NODE(gp->scpe);
      // Create the sink.
      const auto sink = NC->addVarNode(gp->res, function_id);
      const auto phiOp = new PhiOpNode(sink, stmt);

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                     "---Added PhiOp with range " + tree_helper::Range(stmt)->ToString() + " and " +
                         std::to_string(gp->CGetDefEdgesList().size()) + " sources");

      // Create the sources.
      for(const auto& [var, bbi] : gp->CGetDefEdgesList())
      {
         const auto source = NC->addVarNode(var, function_id);
         phiOp->addSource(source);
      }
      return phiOp;
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

void PhiOpNode::printDot(std::ostream& OS) const
{
   OS << " \"" << this << "\" [label=\"phi\"]\n";
   for(const VarNode* varNode : sources)
   {
      const auto& V = varNode->getValue();
      if(tree_helper::IsConstant(V))
      {
         OS << " " << tree_helper::GetConstValue(V) << " -> \"" << this << "\"\n";
      }
      else
      {
         OS << " \"" << V << "\" -> \"" << this << "\"\n";
      }
   }
   const auto& VS = getSink()->getValue();
   OS << " \"" << this << "\" -> \"" << VS << "\"\n";
}
