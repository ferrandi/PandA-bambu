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
 * @file UnaryOpNode.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "UnaryOpNode.hpp"

#include "NodeContainer.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "ir_helper.hpp"
#include "range_analysis_helper.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"

#ifndef NDEBUG
bool _ra_enable_abs = true;
bool _ra_enable_negate = true;
bool _ra_enable_sext = true;
bool _ra_enable_zext = true;
bool _ra_enable_tnot = true;

#define RESULT_DISABLED_OPTION(x, var, stdResult) _ra_enable_##x ? (stdResult) : ir_helper::TypeRange(var, Regular)
#else
#define RESULT_DISABLED_OPTION(x, var, stdResult) stdResult
#endif

UnaryOpNode::UnaryOpNode(VarNode* _sink, VarNode* _source, const ir_nodeConstRef& _inst, kind_R _opcode)
    : OpNode(_sink, _inst), source(_source), opcode(_opcode)
{
}

OpNode::OpNodeType UnaryOpNode::getValueId() const
{
   return OpNodeType::OpNodeType_Unary;
}

std::vector<VarNode*> UnaryOpNode::getSources() const
{
   return {source};
}

void UnaryOpNode::replaceSource(const VarNode* _old, VarNode* _new)
{
   if(_old->getId() == source->getId())
   {
      source = _new;
   }
}

/// Computes the interval of the sink based on the interval of the sources,
/// the operation and the interval associated to the operation.
Range UnaryOpNode::eval() const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, ToString());

   const auto bw = getSink()->getBitWidth();
   const auto& oprnd = source->getRange();
   const auto resultType = ir_helper::CGetType(getSink()->getValue());
   const bool oprndSigned = range_analysis::isSignedType(source->getValue());
   auto result = ir_helper::TypeRange(getSink()->getValue(), Unknown);
   if(oprnd.isEmpty())
   {
      result = Range(Empty, bw);
   }
   else if(oprnd.isRegular() || oprnd.isAnti())
   {
      switch(getOpcode())
      {
         case abs_node_R:
         {
            THROW_ASSERT(oprndSigned, "Absolute value of unsigned operand should not happen");
            result = RESULT_DISABLED_OPTION(abs, getSink()->getValue(), oprnd.abs());
            break;
         }
         case not_node_R:
         {
            result = oprnd.Not();
            break;
         }
         case nop_node_R:
         {
            const auto sinkSigned = range_analysis::isSignedType(getSink()->getValue());
            result = oprndSigned ? oprnd.sextOrTrunc(bw) : oprnd.zextOrTrunc(bw);
            if(oprndSigned != sinkSigned)
            {
               result = sinkSigned ? result.sextOrTrunc(bw) : result.zextOrTrunc(bw);
            }
            if(oprndSigned)
            {
               result = RESULT_DISABLED_OPTION(sext, getSink()->getValue(), result);
            }
            else
            {
               result = RESULT_DISABLED_OPTION(zext, getSink()->getValue(), result);
            }
            break;
         }
         case neg_node_R:
         {
            result = RESULT_DISABLED_OPTION(negate, getSink()->getValue(), oprnd.negate());
            break;
         }
         case bitcast_node_R:
         {
            if(resultType->get_kind() != real_ty_node_K)
            {
               if(oprndSigned)
               {
                  result = RESULT_DISABLED_OPTION(sext, getSink()->getValue(), oprnd.sextOrTrunc(bw));
               }
               else
               {
                  result = RESULT_DISABLED_OPTION(zext, getSink()->getValue(), oprnd.zextOrTrunc(bw));
               }
            }
            break;
         }
         default:
            THROW_UNREACHABLE("Unhandled unary operation");
            break;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---" + result.ToString() + " = " + range_analysis::getString(getOpcode()) + "( " + oprnd.ToString() +
                      " )");

   Range aux(Unknown, bw);
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

std::function<OpNode*(NodeContainer*)> UnaryOpNode::opCtorGenerator(const ir_nodeConstRef& stmt,
                                                                    const application_managerRef&)
{
   if(stmt->get_kind() != assign_stmt_K)
   {
      return nullptr;
   }
   const auto ga = GetPointerS<const assign_stmt>(stmt);
   if(GetPointer<const ssa_node>(ga->op1) != nullptr || ir_helper::IsConstant(ga->op1))
   {
      return [stmt, ga](NodeContainer* NC) {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                        "Analysing assign operation " + ga->ToString());
         const auto function_id = ga->parent->index;
         const auto sink = NC->addVarNode(ga->op0, function_id);
         const auto _source = NC->addVarNode(ga->op1, function_id);

         auto op = new UnaryOpNode(sink, _source, stmt, range_analysis::op_convert(nop_node_K));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                        "---Added assign operation with range " + op->getIntersect()->ToString());
         return op;
      };
   }
   const auto ue = GetPointer<const unary_node>(ga->op1);
   if(ue == nullptr)
   {
      return nullptr;
   }
   return [stmt, ga, ue](NodeContainer* NC) {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                     "Analysing unary operation " + ue->get_kind_text() + " " + ga->ToString());

      const auto function_id = ga->parent->index;
      const auto sink = NC->addVarNode(ga->op0, function_id);
      const auto _source = NC->addVarNode(ue->op, function_id);
      const auto op_kind = range_analysis::op_convert(ue->get_kind());

      auto op = new UnaryOpNode(sink, _source, stmt, op_kind);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                     "---Added UnaryOp for " + ue->get_kind_text() + " with range " + op->getIntersect()->ToString());
      return op;
   };
}

void UnaryOpNode::print(std::ostream& OS) const
{
   OS << getSink()->getValue() << " = " << range_analysis::getString(getOpcode()) << "( " << getSource()->getValue()
      << " )";
}

std::string UnaryOpNode::getName() const
{
   std::stringstream ss;
   // Instruction bitwidth
   const auto bw = getSink()->getBitWidth();
   const bool oprndSigned = range_analysis::isSignedType(source->getValue());

   if(opcode == nop_node_R)
   {
      if(bw < getSource()->getBitWidth())
      {
         ss << "trunc i" << bw;
      }
      else
      {
         if(ir_helper::IsPointerType(getSource()->getValue()))
         {
            ss << "ptr_cast i" << bw;
         }
         else
         {
            if(oprndSigned)
            {
               ss << "sext i" << bw;
            }
            else
            {
               ss << "zext i" << bw;
            }
         }
      }
   }
   else if(opcode == fptoi_node_R)
   {
      const auto type = ir_helper::CGetType(getSink()->getValue());
      if(const auto* int_type = GetPointer<const integer_ty_node>(type))
      {
         if(int_type->unsigned_flag)
         {
            ss << "fptoui i" << bw;
         }
         else
         {
            ss << "fptosi i" << bw;
         }
      }
      else
      {
         THROW_UNREACHABLE("Sink should be of type integer");
      }
   }
   else
   {
      // Phi functions, Loads and Stores are handled here.
      getIntersect()->print(ss);
   }

   return ss.str();
}

#pragma GCC diagnostic pop
