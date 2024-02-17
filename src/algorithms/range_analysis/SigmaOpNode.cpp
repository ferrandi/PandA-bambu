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
 * @file SigmaOpNode.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "SigmaOpNode.hpp"

#include "NodeContainer.hpp"
#include "SymbValueRange.hpp"
#include "dbgPrintHelper.hpp"
#include "tree_helper.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

SigmaOpNode::SigmaOpNode(const ValueRangeRef& _intersect, VarNode* _sink, const tree_nodeConstRef& _inst,
                         VarNode* _source, VarNode* _SymbolicSource, kind _opcode)
    : UnaryOpNode(_intersect, _sink, _inst, _source, _opcode), SymbolicSource(_SymbolicSource), unresolved(false)
{
}

OpNode::OperationId SigmaOpNode::getValueId() const
{
   return OperationId::SigmaOpId;
}

std::vector<VarNode*> SigmaOpNode::getSources() const
{
   auto s = UnaryOpNode::getSources();
   if(SymbolicSource != nullptr)
   {
      s.push_back(SymbolicSource);
   }
   return s;
}

/// Computes the interval of the sink based on the interval of the sources,
/// the operation and the interval associated to the operation.
RangeRef SigmaOpNode::eval() const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, ToString());

   RangeRef result(getSource()->getRange()->clone());
   const auto aux = getIntersect()->getRange();
   if(!aux->isUnknown())
   {
      auto _intersect = result->intersectWith(aux);
      if(!_intersect->isEmpty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---aux = " + aux->ToString() + " from " + getIntersect()->ToString());
         // Sigma operations are used to enhance live range split after conditional statements,
         // thus it is useful to intersect their range only if it actually produces tighter interval
         if(_intersect->getSpan() < result->getSpan())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---result = " + _intersect->ToString());
            result = _intersect;
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---result not changed because not improved");
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---" + result->ToString() + " = SIGMA< " + getSource()->getRange()->ToString() + " >");
   return result;
}

std::function<OpNode*(NodeContainer*)> SigmaOpNode::opCtorGenerator(const tree_nodeConstRef& stmt,
                                                                    const application_managerRef&)
{
   const auto gp = GetPointer<const gimple_phi>(GET_CONST_NODE(stmt));
   if(!gp || gp->CGetDefEdgesList().size() != 1)
   {
      return nullptr;
   }
   return [stmt, gp](NodeContainer* NC) {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                     "Analysing sigma operation " + gp->ToString());
      const auto BBI = gp->bb_index;
      const auto function_id = GET_INDEX_CONST_NODE(gp->scpe);
      const auto& [sourceTN, sourceBBI] = gp->CGetDefEdgesList().front();

      // Create the sink.
      const auto sink = NC->addVarNode(gp->res, function_id, gp->bb_index);
      const auto source = NC->addVarNode(sourceTN, function_id, sourceBBI);

      auto vsmit = NC->getCVR().find(sourceTN);
      if(vsmit == NC->getCVR().end())
      {
         return static_cast<SigmaOpNode*>(nullptr);
      }

      auto condRangeIt = vsmit->second.getVR().find(BBI);
      if(condRangeIt != vsmit->second.getVR().end())
      {
         const auto& CondRange = condRangeIt->second;
         VarNode* SymbSrc = nullptr;
         if(auto symb = RefcountCast<SymbRange>(CondRange))
         {
            SymbSrc = symb->getBound();
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                        "---Added SigmaOp with " + std::string(SymbSrc ? "symbolic " : "") + "range " +
                            CondRange->ToString());
         return new SigmaOpNode(CondRange, sink, stmt, source, SymbSrc, gp->get_kind());
      }
      else
      {
         auto BI = ValueRangeRef(new ValueRange(tree_helper::Range(stmt)));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                        "---Added SigmaOp with range " + BI->ToString());
         return new SigmaOpNode(BI, sink, stmt, source, nullptr, gp->get_kind());
      }
   };
}

void SigmaOpNode::print(std::ostream& OS) const
{
   OS << getSink()->getValue() << " = SIGMA< " << getSource()->getValue() << " >";
}

void SigmaOpNode::printDot(std::ostream& OS) const
{
   OS << " \"" << this << "\" [label=\"SigmaOp:";
   getIntersect()->print(OS);
   OS << "\"]\n";
   const auto& V = getSource()->getValue();
   if(tree_helper::IsConstant(V))
   {
      OS << " " << tree_helper::GetConstValue(V) << " -> \"" << this << "\"\n";
   }
   else
   {
      OS << " \"" << V << "\" -> \"" << this << "\"\n";
   }
   if(SymbolicSource)
   {
      const auto& _V = SymbolicSource->getValue();
      if(tree_helper::IsConstant(_V))
      {
         OS << " " << tree_helper::GetConstValue(_V) << " -> \"" << this << "\"\n";
      }
      else
      {
         OS << " \"" << _V << "\" -> \"" << this << "\"\n";
      }
   }

   const auto& VS = getSink()->getValue();
   OS << " \"" << this << "\" -> \"" << VS << "\"\n";
}
