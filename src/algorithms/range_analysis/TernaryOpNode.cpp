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
 * @file TernaryOpNode.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "TernaryOpNode.hpp"

#include "NodeContainer.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "range_analysis_helper.hpp"
#include "tree_helper.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#include <string>

#ifndef NDEBUG
// TODO: disable because of problem with reduced precision fdiv/f64div operator (fix before enabling back)
bool _ra_enable_ternary = false;
#endif

TernaryOpNode::TernaryOpNode(const ValueRangeRef& _intersect, VarNode* _sink, const tree_nodeConstRef& _inst,
                             VarNode* _source1, VarNode* _source2, VarNode* _source3, kind _opcode)
    : OpNode(_intersect, _sink, _inst), source1(_source1), source2(_source2), source3(_source3), opcode(_opcode)
{
#if HAVE_ASSERTS
   const auto* ga = GetPointer<const gimple_assign>(GET_CONST_NODE(_inst));
   THROW_ASSERT(ga, "TernaryOp associated statement should be a gimple_assign " + GET_CONST_NODE(_inst)->ToString());
   const auto* I = GetPointer<const ternary_expr>(GET_CONST_NODE(ga->op1));
   THROW_ASSERT(I, "TernaryOp operator should be a ternary_expr");
   THROW_ASSERT(_sink->getBitWidth() >= _source2->getBitWidth(),
                std::string("Operator bitwidth overflow ") + ga->ToString() + " (sink= " +
                    std::to_string(+_sink->getBitWidth()) + ", op2= " + std::to_string(+_source2->getBitWidth()) + ")");
   THROW_ASSERT(_sink->getBitWidth() >= _source3->getBitWidth(),
                std::string("Operator bitwidth overflow ") + ga->ToString() + " (sink= " +
                    std::to_string(+_sink->getBitWidth()) + ", op3= " + std::to_string(+_source3->getBitWidth()) + ")");
#endif
}

OpNode::OperationId TernaryOpNode::getValueId() const
{
   return OperationId::TernaryOpId;
}

std::vector<VarNode*> TernaryOpNode::getSources() const
{
   return {source1, source2, source3};
}

RangeRef TernaryOpNode::eval() const
{
   const auto op1 = getSource1()->getRange();
   auto op2 = getSource2()->getRange();
   auto op3 = getSource3()->getRange();

   auto result = tree_helper::TypeRange(getSink()->getValue(), Regular);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, ToString());

#ifndef NDEBUG
   if(_ra_enable_ternary)
   {
      //    #endif
      // only evaluate if all operands are Regular
      if((op1->isRegular() || op1->isAnti()) && (op2->isRegular() || op2->isAnti()) &&
         (op3->isRegular() || op3->isAnti()))
      {
         if(getOpcode() == cond_expr_K)
         {
            // Source1 is the selector
            if(op1->isSameRange(RangeRef(new Range(Regular, op1->getBitWidth(), 1, 1))))
            {
               result = RangeRef(op2->clone());
            }
            else if(op1->isSameRange(RangeRef(new Range(Regular, op1->getBitWidth(), 0, 0))))
            {
               result = RangeRef(op3->clone());
            }
            else
            {
               const auto* ga = GetPointer<const gimple_assign>(GET_CONST_NODE(getInstruction()));
               const auto* I = GetPointer<const ternary_expr>(GET_CONST_NODE(ga->op1));
               const auto BranchVar = range_analysis::branchOpRecurse(I->op0);
               std::vector<const struct binary_expr*> BranchConds;
               // Check if branch variable is correlated with op1 or op2
               if(GetPointer<const gimple_phi>(BranchVar) != nullptr)
               {
                  // TODO: find a way to propagate range from all phi edges when phi->res is one of the two result of
                  // the cond_expr
               }
               else if(const auto* BranchExpr = GetPointer<const binary_expr>(BranchVar))
               {
                  BranchConds.push_back(BranchExpr);
               }

               for(const auto* be : BranchConds)
               {
                  if(range_analysis::isCompare(be))
                  {
                     const auto& CondOp0 = be->op0;
                     const auto& CondOp1 = be->op1;
                     if(GET_CONST_NODE(CondOp0)->get_kind() == integer_cst_K ||
                        GET_CONST_NODE(CondOp1)->get_kind() == integer_cst_K)
                     {
                        const auto& variable = GET_CONST_NODE(CondOp0)->get_kind() == integer_cst_K ? CondOp1 : CondOp0;
                        const auto& constant = GET_CONST_NODE(CondOp0)->get_kind() == integer_cst_K ? CondOp0 : CondOp1;
                        const auto& opV1 = I->op1;
                        const auto& opV2 = I->op2;
                        if(GET_INDEX_CONST_NODE(variable) == GET_INDEX_CONST_NODE(opV1) ||
                           GET_INDEX_CONST_NODE(variable) == GET_INDEX_CONST_NODE(opV2))
                        {
                           const auto CR = tree_helper::Range(constant);
                           THROW_ASSERT(CR->isConstant(), "Range from constant should be constant (" +
                                                              GET_CONST_NODE(constant)->ToString() + " => " +
                                                              CR->ToString() + ")");
                           kind pred = range_analysis::isSignedType(CondOp0) ?
                                           be->get_kind() :
                                           range_analysis::op_unsigned(be->get_kind());
                           kind swappred = range_analysis::op_swap(pred);

                           auto tmpT = (variable == CondOp0) ? range_analysis::makeSatisfyingCmpRegion(pred, CR) :
                                                               range_analysis::makeSatisfyingCmpRegion(swappred, CR);
                           THROW_ASSERT(!tmpT->isFullSet(), "");

                           if(GET_INDEX_CONST_NODE(variable) == GET_INDEX_CONST_NODE(opV2))
                           {
                              RangeRef FValues(new Range(*tmpT->getAnti()));
                              op3 = op3->intersectWith(FValues);
                           }
                           else
                           {
                              op2 = op2->intersectWith(tmpT);
                           }
                        }
                     }
                  }
               }
               result = op2->unionWith(op3);
            }
         }
      }
      else
      {
         if(op1->isEmpty() || op2->isEmpty() || op3->isEmpty())
         {
            result = tree_helper::TypeRange(getSink()->getValue(), Empty);
         }
      }
   }
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---" + result->ToString() + " = " + op1->ToString() + " ? " + op2->ToString() + " : " +
                      op3->ToString());

   bool test = getIntersect()->getRange()->isFullSet();
   if(!test)
   {
      const auto aux = getIntersect()->getRange();
      auto _intersect = result->intersectWith(aux);
      if(!_intersect->isEmpty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---aux = " + aux->ToString() + " from " + getIntersect()->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---result = " + _intersect->ToString());
         result = _intersect;
      }
   }
   return result;
}

std::function<OpNode*(NodeContainer*)> TernaryOpNode::opCtorGenerator(const tree_nodeConstRef& stmt,
                                                                      const application_managerRef&)
{
   const auto* ga = GetPointer<const gimple_assign>(GET_CONST_NODE(stmt));
   if(ga == nullptr)
   {
      return nullptr;
   }
   const auto te = GetPointer<const ternary_expr>(GET_CONST_NODE(ga->op1));
   if(te == nullptr)
   {
      return nullptr;
   }
   return [stmt, ga, te](NodeContainer* NC) {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                     "Analysing ternary operation " + te->get_kind_text() + " " + ga->ToString());
      const auto function_id = GET_INDEX_CONST_NODE(ga->scpe);
      // Create the sink.
      const auto sink = NC->addVarNode(ga->op0, function_id, ga->bb_index);

      // Create the sources.
      const auto _source1 = NC->addVarNode(te->op0, function_id, ga->bb_index);
      const auto _source2 = NC->addVarNode(te->op1, function_id, ga->bb_index);
      const auto _source3 = NC->addVarNode(te->op2, function_id, ga->bb_index);

      // Create the operation using the intersect to constrain sink's interval.
      auto BI = ValueRangeRef(new ValueRange(tree_helper::Range(stmt)));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                     "---Added TernaryOp for " + te->get_kind_text() + " with range " + BI->ToString());
      return new TernaryOpNode(BI, sink, stmt, _source1, _source2, _source3, te->get_kind());
   };
}

void TernaryOpNode::print(std::ostream& OS) const
{
   OS << getSink()->getValue() << " = " << getSource1()->getValue() << " ? " << getSource2()->getValue() << " : "
      << getSource3()->getValue();
}

void TernaryOpNode::printDot(std::ostream& OS) const
{
   std::string opcodeName = tree_node::GetString(getOpcode());
   OS << " \"" << this << "\" [label=\"" << opcodeName << "\"]\n";

   const auto& V1 = getSource1()->getValue();
   if(tree_helper::IsConstant(V1))
   {
      OS << " " << tree_helper::GetConstValue(V1) << " -> \"" << this << "\"\n";
   }
   else
   {
      OS << " \"" << V1 << "\" -> \"" << this << "\"\n";
   }
   const auto& V2 = getSource2()->getValue();
   if(tree_helper::IsConstant(V2))
   {
      OS << " " << tree_helper::GetConstValue(V2) << " -> \"" << this << "\"\n";
   }
   else
   {
      OS << " \"" << V2 << "\" -> \"" << this << "\"\n";
   }

   const auto& V3 = getSource3()->getValue();
   if(tree_helper::IsConstant(V3))
   {
      OS << " " << tree_helper::GetConstValue(V3) << " -> \"" << this << "\"\n";
   }
   else
   {
      OS << " \"" << V3 << "\" -> \"" << this << "\"\n";
   }
   const auto& VS = getSink()->getValue();
   OS << " \"" << this << "\" -> \"" << VS << "\"\n";
}
