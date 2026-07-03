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
 * @file TernaryOpNode.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "TernaryOpNode.hpp"

#include "NodeContainer.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "ir_helper.hpp"
#include "range_analysis_helper.hpp"

#ifndef NDEBUG
// TODO: disable because of problem with reduced precision fdiv/f64div operator (fix before enabling back)
bool _ra_enable_ternary = false;
#endif

TernaryOpNode::TernaryOpNode(VarNode* _sink, VarNode* _source1, VarNode* _source2, VarNode* _source3,
                             const ir_nodeConstRef& _inst, kind_R _opcode)
    : OpNode(_sink, _inst), source1(_source1), source2(_source2), source3(_source3), opcode(_opcode)
{
#if HAVE_ASSERTS
   const auto* ga = GetPointer<const assign_stmt>(_inst);
   THROW_ASSERT(ga, "TernaryOp associated statement should be a assign_stmt " + _inst->ToString());
   const auto* I = GetPointer<const ternary_node>(ga->op1);
   THROW_ASSERT(I, "TernaryOp operator should be a ternary_node");
   THROW_ASSERT(_sink->getBitWidth() >= _source2->getBitWidth(),
                std::string("Operator bitwidth overflow ") + ga->ToString() + " (sink= " +
                    std::to_string(+_sink->getBitWidth()) + ", op2= " + std::to_string(+_source2->getBitWidth()) + ")");
   THROW_ASSERT(_sink->getBitWidth() >= _source3->getBitWidth(),
                std::string("Operator bitwidth overflow ") + ga->ToString() + " (sink= " +
                    std::to_string(+_sink->getBitWidth()) + ", op3= " + std::to_string(+_source3->getBitWidth()) + ")");
#endif
}

OpNode::OpNodeType TernaryOpNode::getValueId() const
{
   return OpNodeType::OpNodeType_Ternary;
}

std::vector<VarNode*> TernaryOpNode::getSources() const
{
   return {source1, source2, source3};
}

void TernaryOpNode::replaceSource(const VarNode* _old, VarNode* _new)
{
   if(_old->getId() == source1->getId())
   {
      source1 = _new;
   }
   if(_old->getId() == source2->getId())
   {
      source2 = _new;
   }
   if(_old->getId() == source3->getId())
   {
      source3 = _new;
   }
}

Range TernaryOpNode::eval() const
{
   auto result = ir_helper::TypeRange(getSink()->getValue(), Regular);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, ToString());

#ifndef NDEBUG
   const auto& op1 = getSource1()->getRange();
   auto op2 = getSource2()->getRange();
   auto op3 = getSource3()->getRange();
   if(_ra_enable_ternary)
   {
      //    #endif
      // only evaluate if all operands are Regular
      if((op1.isRegular() || op1.isAnti()) && (op2.isRegular() || op2.isAnti()) && (op3.isRegular() || op3.isAnti()))
      {
         if(getOpcode() == select_node_R)
         {
            // Source1 is the selector
            if(op1.isSameRange(Range(Regular, op1.getBitWidth(), 1, 1)))
            {
               result = op2;
            }
            else if(op1.isSameRange(Range(Regular, op1.getBitWidth(), 0, 0)))
            {
               result = op3;
            }
            else
            {
               const auto* ga = GetPointer<const assign_stmt>(getInstruction());
               const auto* I = GetPointer<const ternary_node>(ga->op1);
               const auto BranchVar = range_analysis::castTraverse(I->op0);
               std::vector<const struct binary_node*> BranchConds;
               // Check if branch variable is correlated with op1 or op2
               if(GetPointer<const phi_stmt>(BranchVar) != nullptr)
               {
                  // TODO: find a way to propagate range from all phi edges when phi->res is one of the two result of
                  // the select_node
               }
               else if(const auto* BranchExpr = GetPointer<const binary_node>(BranchVar))
               {
                  BranchConds.push_back(BranchExpr);
               }

               for(const auto* be : BranchConds)
               {
                  if(range_analysis::isCompare(be))
                  {
                     const auto& CondOp0 = be->op0;
                     const auto& CondOp1 = be->op1;
                     if(CondOp0->get_kind() == constant_int_val_node_K ||
                        CondOp1->get_kind() == constant_int_val_node_K)
                     {
                        const auto& variable = CondOp0->get_kind() == constant_int_val_node_K ? CondOp1 : CondOp0;
                        const auto& constant = CondOp0->get_kind() == constant_int_val_node_K ? CondOp0 : CondOp1;
                        const auto& opV1 = I->op1;
                        const auto& opV2 = I->op2;
                        if(variable->index == opV1->index || variable->index == opV2->index)
                        {
                           const auto CR = ir_helper::NodeRange(constant);
                           THROW_ASSERT(CR.isConstant(), "Range from constant should be constant (" +
                                                             constant->ToString() + " => " + CR.ToString() + ")");
                           auto pred = range_analysis::isSignedType(CondOp0) ?
                                           range_analysis::op_convert(be->get_kind()) :
                                           range_analysis::op_unsigned(range_analysis::op_convert(be->get_kind()));
                           auto swappred = range_analysis::op_swap(pred);

                           auto tmpT = (variable == CondOp0) ? range_analysis::makeSatisfyingCmpRegion(pred, CR) :
                                                               range_analysis::makeSatisfyingCmpRegion(swappred, CR);
                           THROW_ASSERT(!tmpT.isFullSet(), "");

                           if(variable->index == opV2->index)
                           {
                              op3 = op3.intersectWith(tmpT.getAnti());
                           }
                           else
                           {
                              op2 = op2.intersectWith(tmpT);
                           }
                        }
                     }
                  }
               }
               result = op2.unionWith(op3);
            }
         }
      }
      else
      {
         if(op1.isEmpty() || op2.isEmpty() || op3.isEmpty())
         {
            result = ir_helper::TypeRange(getSink()->getValue(), Empty);
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---" + result.ToString() + " = " + op1.ToString() + " ? " + op2.ToString() + " : " + op3.ToString());
#endif

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
   return result;
}

std::function<OpNode*(NodeContainer*)> TernaryOpNode::opCtorGenerator(const ir_nodeConstRef& stmt,
                                                                      const application_managerRef&)
{
   if(stmt->get_kind() != assign_stmt_K)
   {
      return nullptr;
   }
   const auto ga = GetPointerS<const assign_stmt>(stmt);
   const auto te = GetPointer<const ternary_node>(ga->op1);
   if(te == nullptr)
   {
      return nullptr;
   }
   return [stmt, ga, te](NodeContainer* NC) {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                     "Analysing ternary operation " + te->get_kind_text() + " " + ga->ToString());
      const auto function_id = ga->parent->index;
      // Create the sink.
      const auto sink = NC->addVarNode(ga->op0, function_id);

      // Create the sources.
      const auto _source1 = NC->addVarNode(te->op0, function_id);
      const auto _source2 = NC->addVarNode(te->op1, function_id);
      const auto _source3 = NC->addVarNode(te->op2, function_id);

      auto op = new TernaryOpNode(sink, _source1, _source2, _source3, stmt, range_analysis::op_convert(te->get_kind()));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                     "---Added TernaryOp for " + te->get_kind_text() + " with range " + op->getIntersect()->ToString());
      return op;
   };
}

void TernaryOpNode::print(std::ostream& OS) const
{
   OS << getSink()->getValue() << " = " << getSource1()->getValue() << " ? " << getSource2()->getValue() << " : "
      << getSource3()->getValue();
}

std::string TernaryOpNode::getName() const
{
   return range_analysis::getString(getOpcode());
}
