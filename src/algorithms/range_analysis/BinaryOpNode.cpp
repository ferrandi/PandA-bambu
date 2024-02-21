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
 * @file BinaryOpNode.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "BinaryOpNode.hpp"

#include "NodeContainer.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "range_analysis_helper.hpp"
#include "tree_helper.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

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

#define RETURN_DISABLED_OPTION(x, bw)          \
   if(!_ra_enable_##x)                         \
   {                                           \
      return RangeRef(new Range(Regular, bw)); \
   }
#else
#define RETURN_DISABLED_OPTION(x, bw) void(0)
#endif

BinaryOpNode::BinaryOpNode(VarNode* _sink, VarNode* _source1, VarNode* _source2, const tree_nodeConstRef& _inst,
                           kind _opcode)
    : OpNode(_sink, _inst), source1(_source1), source2(_source2), opcode(_opcode)
{
   THROW_ASSERT(range_analysis::isValidType(_sink->getValue()), "Binary operation sink should be of valid type (" +
                                                                    GET_CONST_NODE(_sink->getValue())->ToString() +
                                                                    ")");
}

OpNode::OpNodeType BinaryOpNode::getValueId() const
{
   return OpNodeType::OpNodeType_Binary;
}

std::vector<VarNode*> BinaryOpNode::getSources() const
{
   return {source1, source2};
}

void BinaryOpNode::replaceSource(VarNode* _old, VarNode* _new)
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

RangeRef BinaryOpNode::evaluate(kind opcode, Range::bw_t bw, const RangeConstRef& op1, const RangeConstRef& op2,
                                bool opSigned)
{
   switch(opcode)
   {
      case pointer_plus_expr_K:
      case plus_expr_K:
         RETURN_DISABLED_OPTION(add, bw);
         return op1->add(op2);
      case minus_expr_K:
         RETURN_DISABLED_OPTION(sub, bw);
         return op1->sub(op2);
      case mult_expr_K:
         RETURN_DISABLED_OPTION(mul, bw);
         return op1->mul(op2);
      case widen_mult_expr_K:
         RETURN_DISABLED_OPTION(mul, bw);
         return opSigned ? op1->sextOrTrunc(bw)->mul(op2->sextOrTrunc(bw)) :
                           op1->zextOrTrunc(bw)->mul(op2->sextOrTrunc(bw));
      case trunc_div_expr_K:
         if(opSigned)
         {
            RETURN_DISABLED_OPTION(sdiv, bw);
            return op1->sdiv(op2);
         }
         else
         {
            RETURN_DISABLED_OPTION(udiv, bw);
            return op1->udiv(op2);
         }
      case trunc_mod_expr_K:
         if(opSigned)
         {
            RETURN_DISABLED_OPTION(srem, bw);
            const auto res = op1->srem(op2);
            if(!res->isUnknown() && !res->isEmpty() && res->getSignedMin() == 0)
            {
               const auto consRes = res->unionWith(res->negate());
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Being conservative on signed modulo operator: " + res->ToString() + " -> " +
                                  consRes->ToString());
               return consRes;
            }
            return res;
         }
         else
         {
            RETURN_DISABLED_OPTION(urem, bw);
            return op1->urem(op2);
         }
      case lshift_expr_K:
         RETURN_DISABLED_OPTION(shl, bw);
         return opSigned ? op1->sextOrTrunc(bw)->shl(op2) : op1->zextOrTrunc(bw)->shl(op2);
      case rshift_expr_K:
         RETURN_DISABLED_OPTION(shr, bw);
         return opSigned ? op1->shr(op2, true)->sextOrTrunc(bw) : op1->shr(op2, false)->zextOrTrunc(bw);
      case bit_and_expr_K:
         RETURN_DISABLED_OPTION(and, bw);
         return op1->And(op2);
      case bit_ior_expr_K:
         RETURN_DISABLED_OPTION(or, bw);
         return op1->Or(op2);
      case bit_xor_expr_K:
         RETURN_DISABLED_OPTION(xor, bw);
         return op1->Xor(op2);
      case eq_expr_K:
         if(op1->getBitWidth() < op2->getBitWidth())
         {
            return opSigned ? op1->sextOrTrunc(op2->getBitWidth())->Eq(op2, bw) :
                              op1->zextOrTrunc(op2->getBitWidth())->Eq(op2, bw);
         }
         else if(op2->getBitWidth() < op1->getBitWidth())
         {
            return opSigned ? op2->sextOrTrunc(op1->getBitWidth())->Eq(op1, bw) :
                              op2->zextOrTrunc(op1->getBitWidth())->Eq(op1, bw);
         }
         return op1->Eq(op2, bw);
      case ne_expr_K:
         return op1->Ne(op2, bw);
      case gt_expr_K:
         return opSigned ? op1->Sgt(op2, bw) : op1->Ugt(op2, bw);
      case ge_expr_K:
         return opSigned ? op1->Sge(op2, bw) : op1->Uge(op2, bw);
      case lt_expr_K:
         return opSigned ? op1->Slt(op2, bw) : op1->Ult(op2, bw);
      case le_expr_K:
         return opSigned ? op1->Sle(op2, bw) : op1->Ule(op2, bw);
      case min_expr_K:
         RETURN_DISABLED_OPTION(min, bw);
         return opSigned ? op1->SMin(op2) : op1->UMin(op2);
      case max_expr_K:
         RETURN_DISABLED_OPTION(max, bw);
         return opSigned ? op1->SMax(op2) : op1->UMax(op2);
      case sat_plus_expr_K:
         RETURN_DISABLED_OPTION(add, bw);
         return opSigned ? op1->sat_add(op2) : op1->usat_add(op2);
      case sat_minus_expr_K:
         RETURN_DISABLED_OPTION(sub, bw);
         return opSigned ? op1->sat_sub(op2) : op1->usat_sub(op2);

      case assert_expr_K:
      case catch_expr_K:
      case ceil_div_expr_K:
      case ceil_mod_expr_K:
      case complex_expr_K:
      case compound_expr_K:
      case eh_filter_expr_K:
      case exact_div_expr_K:
      case fdesc_expr_K:
      case floor_div_expr_K:
      case floor_mod_expr_K:
      case goto_subroutine_K:
      case in_expr_K:
      case init_expr_K:
      case lrotate_expr_K:
      case mem_ref_K:
      case modify_expr_K:
      case mult_highpart_expr_K:
      case ordered_expr_K:
      case postdecrement_expr_K:
      case postincrement_expr_K:
      case predecrement_expr_K:
      case preincrement_expr_K:
      case range_expr_K:
      case rdiv_expr_K:
      case frem_expr_K:
      case round_div_expr_K:
      case round_mod_expr_K:
      case rrotate_expr_K:
      case set_le_expr_K:
      case truth_and_expr_K:
      case truth_andif_expr_K:
      case truth_or_expr_K:
      case truth_orif_expr_K:
      case truth_xor_expr_K:
      case try_catch_expr_K:
      case try_finally_K:
      case ltgt_expr_K:
      case uneq_expr_K:
      case unge_expr_K:
      case ungt_expr_K:
      case unlt_expr_K:
      case unle_expr_K:
      case unordered_expr_K:
      case widen_sum_expr_K:
      case with_size_expr_K:
      case vec_lshift_expr_K:
      case vec_rshift_expr_K:
      case widen_mult_hi_expr_K:
      case widen_mult_lo_expr_K:
      case vec_pack_trunc_expr_K:
      case vec_pack_sat_expr_K:
      case vec_pack_fix_trunc_expr_K:
      case vec_extracteven_expr_K:
      case vec_extractodd_expr_K:
      case vec_interleavehigh_expr_K:
      case vec_interleavelow_expr_K:
      case extract_bit_expr_K:
      case extractvalue_expr_K:
      case extractelement_expr_K:
      case CASE_UNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_CPP_NODES:
      case CASE_MISCELLANEOUS:
      default:
         THROW_UNREACHABLE("Unhandled binary operation (" + tree_node::GetString(opcode) + ")");
         break;
   }
   return nullptr;
}

/// Computes the interval of the sink based on the interval of the sources,
/// the operation and the interval associated to the operation.
/// Basically, this function performs the operation indicated in its opcode
/// taking as its operands the source1 and the source2.
RangeRef BinaryOpNode::eval() const
{
   const auto op1 = getSource1()->getRange();
   const auto op2 = getSource2()->getRange();
   // Instruction bitwidth
   const auto sinkBW = getSink()->getBitWidth();
   auto result = tree_helper::TypeRange(getSink()->getValue(), Unknown);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, ToString());

   // only evaluate if all operands are Regular
   if((op1->isRegular() || op1->isAnti()) && (op2->isRegular() || op2->isAnti()))
   {
      const auto opSigned = range_analysis::isSignedType(getSource1()->getValue());

      result = evaluate(getOpcode(), sinkBW, op1, op2, opSigned);

      // Bitvalue may consider only lower bits for some variables, thus it is necessary to perform evaluation on
      // truncated opernds to obtain valid results
      if(const auto* ssa = GetPointer<const ssa_name>(GET_CONST_NODE(getSink()->getValue())))
      {
         const auto sinkSigned = range_analysis::isSignedType(getSink()->getValue());
         const auto bvRange = [&]() {
            if(ssa->bit_values.empty() || ssa->bit_values.front() == 'X')
            {
               return RangeRef(new Range(Regular, sinkBW));
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
            const auto r = RangeRef(new Range(Regular, static_cast<Range::bw_t>(ssa->bit_values.size()), bits, bits));
            THROW_ASSERT(r->isConstant(), "Range derived from <" + ssa->bit_values + "> should be constant");
            return r;
         }();
         const auto op_code = getOpcode();
         if(bvRange->isConstant() &&
            (bvRange->getSignedMax() != -1 || bvRange->getBitWidth() < result->getBitWidth()) &&
            (op_code == mult_expr_K || op_code == widen_mult_expr_K ||
             op_code == plus_expr_K /* || op_code == minus_expr_K || op_code == pointer_plus_expr_K */))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Result range " + result->ToString() + " filtered with mask " +
                               bitstring_to_string(bvRange->getBitValues(sinkSigned)) + "<" +
                               std::to_string(+bvRange->getBitWidth()) + "> from " + ssa->bit_values + "<" +
                               (sinkSigned ? "signed" : "unsigned") + "> " + bvRange->ToString());
            // #if HAVE_ASSERTS
            // const auto resEmpty = result->isEmpty();
            // #endif
            const auto truncRes = sinkSigned ?
                                      result->truncate(bvRange->getBitWidth())->sextOrTrunc(result->getBitWidth()) :
                                      result->truncate(bvRange->getBitWidth())->zextOrTrunc(result->getBitWidth());
            const auto maskRes = sinkSigned ? result->And(bvRange->zextOrTrunc(result->getBitWidth()))
                                                  ->truncate(bvRange->getBitWidth())
                                                  ->sextOrTrunc(result->getBitWidth()) :
                                              result->And(bvRange->zextOrTrunc(result->getBitWidth()));
            result = truncRes->getSpan() < maskRes->getSpan() ? truncRes : maskRes;
            // THROW_ASSERT(result->isEmpty() == resEmpty, "");
         }
      }

      if(result->getBitWidth() != sinkBW)
      {
         result = result->zextOrTrunc(sinkBW);
      }
   }
   else if(op1->isEmpty() || op2->isEmpty())
   {
      result = tree_helper::TypeRange(getSink()->getValue(), Empty);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---" + result->ToString() + " = " + op1->ToString() + " " + tree_node::GetString(getOpcode()) + " " +
                      op2->ToString());

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

std::function<OpNode*(NodeContainer*)> BinaryOpNode::opCtorGenerator(const tree_nodeConstRef& stmt,
                                                                     const application_managerRef&)
{
   const auto ga = GetPointer<const gimple_assign>(GET_CONST_NODE(stmt));
   if(ga == nullptr)
   {
      return nullptr;
   }
   const auto be = GetPointer<const binary_expr>(GET_CONST_NODE(ga->op1));
   if(be == nullptr)
   {
      return nullptr;
   }
   return [stmt, ga, be](NodeContainer* NC) {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                     "Analysing binary operation " + be->get_kind_text() + " " + ga->ToString());
      const auto function_id = GET_INDEX_CONST_NODE(ga->scpe);

      // Create the sink.
      const auto sink = NC->addVarNode(ga->op0, function_id);
      const auto op_kind = be->get_kind();

      // Create the sources.
      const auto _source1 = NC->addVarNode(be->op0, function_id);
      const auto _source2 = NC->addVarNode(be->op1, function_id);

      // Create the operation using the intersect to constrain sink's interval.
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                     "---Added BinaryOp for " + tree_node::GetString(op_kind) + " with range " +
                         tree_helper::Range(stmt)->ToString());
      return new BinaryOpNode(sink, _source1, _source2, stmt, op_kind);
   };
}

void BinaryOpNode::print(std::ostream& OS) const
{
   OS << getSink()->getValue() << " = (" << getSource1()->getValue() << ")" << tree_node::GetString(getOpcode()) + "("
      << getSource2()->getValue() << ")";
}

void BinaryOpNode::printDot(std::ostream& OS) const
{
   std::string opcodeName = tree_node::GetString(opcode);
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
   const auto& VS = getSink()->getValue();
   OS << " \"" << this << "\" -> \"" << VS << "\"\n";
}
