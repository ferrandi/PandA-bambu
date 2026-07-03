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
 * @file BinaryOpNode.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "BinaryOpNode.hpp"

#include "NodeContainer.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "ir_helper.hpp"
#include "range_analysis_helper.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"

#ifndef NDEBUG
bool _ra_enable_add = true;
bool _ra_enable_sub = true;
bool _ra_enable_mul = true;
bool _ra_enable_sdiv = true;
bool _ra_enable_udiv = true;
bool _ra_enable_srem = true;
bool _ra_enable_urem = true;
bool _ra_enable_shl = true;
bool _ra_enable_shr = true;
bool _ra_enable_and = true;
bool _ra_enable_or = true;
bool _ra_enable_xor = true;
bool _ra_enable_min = true;
bool _ra_enable_max = true;

#define RETURN_DISABLED_OPTION(x, bw) \
   if(!_ra_enable_##x)                \
   {                                  \
      return Range(Regular, bw);      \
   }
#else
#define RETURN_DISABLED_OPTION(x, bw) void(0)
#endif

BinaryOpNode::BinaryOpNode(VarNode* _sink, VarNode* _source1, VarNode* _source2, const ir_nodeConstRef& _inst,
                           kind_R _opcode)
    : OpNode(_sink, _inst), source1(_source1), source2(_source2), opcode(_opcode)
{
   THROW_ASSERT(range_analysis::isValidType(_sink->getValue()),
                "Binary operation sink should be of valid type (" + _sink->getValue()->ToString() + ")");
}

OpNode::OpNodeType BinaryOpNode::getValueId() const
{
   return OpNodeType::OpNodeType_Binary;
}

std::vector<VarNode*> BinaryOpNode::getSources() const
{
   return {source1, source2};
}

void BinaryOpNode::replaceSource(const VarNode* _old, VarNode* _new)
{
   if(_old->getId() == source1->getId())
   {
      source1 = _new;
   }
   if(_old->getId() == source2->getId())
   {
      source2 = _new;
   }
}

static Range evaluate(kind_R opcode, Range::bw_t bw, const Range& op1, const Range& op2, bool opSigned)
{
   switch(opcode)
   {
      case gep_node_R:
      case add_node_R:
         RETURN_DISABLED_OPTION(add, bw);
         break;
      case sub_node_R:
         RETURN_DISABLED_OPTION(sub, bw);
         break;
      case mul_node_R:
      case widen_mul_node_R:
         RETURN_DISABLED_OPTION(mul, bw);
         break;
      case idiv_node_R:
         if(opSigned)
         {
            RETURN_DISABLED_OPTION(sdiv, bw);
         }
         else
         {
            RETURN_DISABLED_OPTION(udiv, bw);
         }
         break;
      case irem_node_R:
         if(opSigned)
         {
            RETURN_DISABLED_OPTION(srem, bw);
         }
         else
         {
            RETURN_DISABLED_OPTION(urem, bw);
         }
         break;
      case shl_node_R:
         RETURN_DISABLED_OPTION(shl, bw);
         break;
      case shr_node_R:
         RETURN_DISABLED_OPTION(shr, bw);
         break;
      case and_node_R:
         RETURN_DISABLED_OPTION(and, bw);
         break;
      case or_node_R:
         RETURN_DISABLED_OPTION(or, bw);
         break;
      case xor_node_R:
         RETURN_DISABLED_OPTION(xor, bw);
         break;
      case eq_node_R:
      case ne_node_R:
      case gt_node_R:
      case ge_node_R:
      case lt_node_R:
      case le_node_R:
         break;
      case min_node_R:
         RETURN_DISABLED_OPTION(min, bw);
         break;
      case max_node_R:
         RETURN_DISABLED_OPTION(max, bw);
         break;
      case add_sat_node_R:
         RETURN_DISABLED_OPTION(add, bw);
         break;
      case sub_sat_node_R:
         RETURN_DISABLED_OPTION(sub, bw);
         break;

      default:
         break;
   }
   return range_analysis::evaluate(opcode, bw, op1, op2, opSigned);
}

/// Computes the interval of the sink based on the interval of the sources,
/// the operation and the interval associated to the operation.
/// Basically, this function performs the operation indicated in its opcode
/// taking as its operands the source1 and the source2.
Range BinaryOpNode::eval() const
{
   const auto& op1 = getSource1()->getRange();
   const auto& op2 = getSource2()->getRange();
   // Instruction bitwidth
   const auto sinkBW = getSink()->getBitWidth();
   auto result = ir_helper::TypeRange(getSink()->getValue(), Unknown);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, ToString());

   // only evaluate if all operands are Regular
   if((op1.isRegular() || op1.isAnti()) && (op2.isRegular() || op2.isAnti()))
   {
      const auto opSigned = range_analysis::isSignedType(getSource1()->getValue());

      result = evaluate(getOpcode(), sinkBW, op1, op2, opSigned);

      // Bitvalue may consider only lower bits for some variables, thus it is necessary to perform evaluation on
      // truncated opernds to obtain valid results
      if(const auto* ssa = GetPointer<const ssa_node>(getSink()->getValue()))
      {
         const auto sinkSigned = range_analysis::isSignedType(getSink()->getValue());
         const auto bvRange = [&]() {
            if(ssa->bit_values.empty() || ssa->bit_values.front() == 'X')
            {
               return Range(Regular, sinkBW);
            }
            APInt bits(0);
            uint8_t i = 0;
            for(auto it = ssa->bit_values.crbegin(); it != ssa->bit_values.crend(); ++it, ++i)
            {
               if(*it != '0')
               {
                  bits |= APInt(1) << i;
               }
            }
            const auto r = Range(Regular, static_cast<Range::bw_t>(ssa->bit_values.size()), bits, bits);
            THROW_ASSERT(r.isConstant(), "Range derived from <" + ssa->bit_values + "> should be constant");
            return r;
         }();
         const auto op_code = getOpcode();
         if(bvRange.isConstant() && (bvRange.getSignedMax() != -1 || bvRange.getBitWidth() < result.getBitWidth()) &&
            (op_code == mul_node_R || op_code == widen_mul_node_R ||
             op_code == add_node_R /* || op_code == sub_node_R || op_code == gep_node_R */))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Result range " + result.ToString() + " filtered with mask " +
                               bitstring_to_string(bvRange.getBitValues(sinkSigned)) + "<" +
                               std::to_string(+bvRange.getBitWidth()) + "> from " + ssa->bit_values + "<" +
                               (sinkSigned ? "signed" : "unsigned") + "> " + bvRange.ToString());
            // #if HAVE_ASSERTS
            // const auto resEmpty = result.isEmpty();
            // #endif
            const auto truncRes = sinkSigned ?
                                      result.truncate(bvRange.getBitWidth()).sextOrTrunc(result.getBitWidth()) :
                                      result.truncate(bvRange.getBitWidth()).zextOrTrunc(result.getBitWidth());
            const auto maskRes = sinkSigned ? result.And(bvRange.zextOrTrunc(result.getBitWidth()))
                                                  .truncate(bvRange.getBitWidth())
                                                  .sextOrTrunc(result.getBitWidth()) :
                                              result.And(bvRange.zextOrTrunc(result.getBitWidth()));
            result = truncRes.getSpan() < maskRes.getSpan() ? truncRes : maskRes;
            // THROW_ASSERT(result.isEmpty() == resEmpty, "");
         }
      }

      if(result.getBitWidth() != sinkBW)
      {
         result = result.zextOrTrunc(sinkBW);
      }
   }
   else if(op1.isEmpty() || op2.isEmpty())
   {
      result = ir_helper::TypeRange(getSink()->getValue(), Empty);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---" + result.ToString() + " = " + op1.ToString() + " " + range_analysis::getString(getOpcode()) +
                      " " + op2.ToString());

   Range aux(Unknown, sinkBW);
   if(getIntersect()->tryGetRange(aux) && !aux.isFullSet())
   {
      auto _intersect = result.intersectWith(aux);
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

std::function<OpNode*(NodeContainer*)> BinaryOpNode::opCtorGenerator(const ir_nodeConstRef& stmt,
                                                                     const application_managerRef&)
{
   const auto ga = GetPointer<const assign_stmt>(stmt);
   if(ga == nullptr)
   {
      return nullptr;
   }
   const auto be = GetPointer<const binary_node>(ga->op1);
   if(be == nullptr)
   {
      return nullptr;
   }
   return [stmt, ga, be](NodeContainer* NC) {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                     "Analysing binary operation " + be->get_kind_text() + " " + ga->ToString());
      const auto function_id = ga->parent->index;

      // Create the sink.
      const auto sink = NC->addVarNode(ga->op0, function_id);
      const auto op_kind = range_analysis::op_convert(be->get_kind());

      // Create the sources.
      const auto _source1 = NC->addVarNode(be->op0, function_id);
      const auto _source2 = NC->addVarNode(be->op1, function_id);

      // Create the operation using the intersect to constrain sink's interval.
      auto op = new BinaryOpNode(sink, _source1, _source2, stmt, op_kind);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                     "---Added BinaryOp for " + range_analysis::getString(op_kind) + " with range " +
                         op->getIntersect()->ToString());
      return op;
   };
}

void BinaryOpNode::print(std::ostream& OS) const
{
   OS << getSink()->getValue() << " = (" << getSource1()->getValue() << ")"
      << range_analysis::getString(getOpcode()) + "(" << getSource2()->getValue() << ")";
}

std::string BinaryOpNode::getName() const
{
   return range_analysis::getString(opcode);
}

#pragma GCC diagnostic pop
