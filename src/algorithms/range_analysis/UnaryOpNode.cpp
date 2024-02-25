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
 * @file UnaryOpNode.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "UnaryOpNode.hpp"

#include "NodeContainer.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "range_analysis_helper.hpp"
#include "tree_helper.hpp"
#include "tree_reindex.hpp"

#ifndef NDEBUG
bool _ra_enable_abs = true;
bool _ra_enable_negate = true;
bool _ra_enable_sext = true;
bool _ra_enable_zext = true;

#define RESULT_DISABLED_OPTION(x, var, stdResult) _ra_enable_##x ? (stdResult) : tree_helper::TypeRange(var, Regular)
#else
#define RESULT_DISABLED_OPTION(x, var, stdResult) stdResult
#endif

UnaryOpNode::UnaryOpNode(VarNode* _sink, VarNode* _source, const tree_nodeConstRef& _inst, kind _opcode)
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

void UnaryOpNode::replaceSource(VarNode* _old, VarNode* _new)
{
   if(_old->getId() == source->getId())
   {
      source = _new;
   }
}

/// Computes the interval of the sink based on the interval of the sources,
/// the operation and the interval associated to the operation.
RangeRef UnaryOpNode::eval() const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, ToString());

   const auto bw = getSink()->getBitWidth();
   const auto oprnd = source->getRange();
   const auto resultType = tree_helper::CGetType(getSink()->getValue());
   const bool oprndSigned = range_analysis::isSignedType(source->getValue());
   auto result = tree_helper::TypeRange(getSink()->getValue(), Unknown);
   if(oprnd->isEmpty())
   {
      result = RangeRef(new Range(Empty, bw));
   }
   else if(oprnd->isRegular() || oprnd->isAnti())
   {
      switch(getOpcode())
      {
         case abs_expr_K:
         {
            THROW_ASSERT(oprndSigned, "Absolute value of unsigned operand should not happen");
            result = RESULT_DISABLED_OPTION(abs, getSink()->getValue(), oprnd->abs());
            break;
         }
         case bit_not_expr_K:
         {
            result = oprnd->Not();
            break;
         }
         case convert_expr_K:
         case nop_expr_K:
         {
            if(oprndSigned)
            {
               result = RESULT_DISABLED_OPTION(sext, getSink()->getValue(), oprnd->sextOrTrunc(bw));
            }
            else
            {
               result = RESULT_DISABLED_OPTION(zext, getSink()->getValue(), oprnd->zextOrTrunc(bw));
            }
            break;
         }
         case negate_expr_K:
         {
            result = RESULT_DISABLED_OPTION(negate, getSink()->getValue(), oprnd->negate());
            break;
         }
         case view_convert_expr_K:
         {
            if(GET_CONST_NODE(resultType)->get_kind() != real_type_K)
            {
               if(oprndSigned)
               {
                  result = RESULT_DISABLED_OPTION(sext, getSink()->getValue(), oprnd->sextOrTrunc(bw));
               }
               else
               {
                  result = RESULT_DISABLED_OPTION(zext, getSink()->getValue(), oprnd->zextOrTrunc(bw));
               }
            }
            break;
         }
         case addr_expr_K:
         case paren_expr_K:
         case alignof_expr_K:
         case arrow_expr_K:
         case buffer_ref_K:
         case card_expr_K:
         case cleanup_point_expr_K:
         case conj_expr_K:
         case exit_expr_K:
         case fix_ceil_expr_K:
         case fix_floor_expr_K:
         case fix_round_expr_K:
         case fix_trunc_expr_K:
         case float_expr_K:
         case imagpart_expr_K:
         case indirect_ref_K:
         case misaligned_indirect_ref_K:
         case loop_expr_K:
         case non_lvalue_expr_K:
         case realpart_expr_K:
         case reference_expr_K:
         case reinterpret_cast_expr_K:
         case sizeof_expr_K:
         case static_cast_expr_K:
         case throw_expr_K:
         case truth_not_expr_K:
         case unsave_expr_K:
         case va_arg_expr_K:
         case reduc_max_expr_K:
         case reduc_min_expr_K:
         case reduc_plus_expr_K:
         case vec_unpack_hi_expr_K:
         case vec_unpack_lo_expr_K:
         case vec_unpack_float_hi_expr_K:
         case vec_unpack_float_lo_expr_K:
         case CASE_BINARY_EXPRESSION:
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
            THROW_UNREACHABLE("Unhandled unary operation");
            break;
      }
   }
   THROW_ASSERT(result, "Result should be set now");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---" + result->ToString() + " = " + tree_node::GetString(getOpcode()) + "( " + oprnd->ToString() +
                      " )");

   auto test = getIntersect()->getRange()->isFullSet();
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

std::function<OpNode*(NodeContainer*)> UnaryOpNode::opCtorGenerator(const tree_nodeConstRef& stmt,
                                                                    const application_managerRef&)
{
   const auto* ga = GetPointer<const gimple_assign>(GET_CONST_NODE(stmt));
   if(ga == nullptr)
   {
      return nullptr;
   }
   if(GetPointer<const ssa_name>(GET_CONST_NODE(ga->op1)) != nullptr ||
      GetPointer<const cst_node>(GET_CONST_NODE(ga->op1)))
   {
      return [stmt, ga](NodeContainer* NC) {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                        "Analysing assign operation " + ga->ToString());
         const auto function_id = GET_INDEX_CONST_NODE(ga->scpe);
         const auto sink = NC->addVarNode(ga->op0, function_id);
         const auto _source = NC->addVarNode(ga->op1, function_id);

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                        "---Added assign operation with range " + tree_helper::Range(stmt)->ToString());
         return new UnaryOpNode(sink, _source, stmt, nop_expr_K);
      };
   }
   const auto ue = GetPointer<const unary_expr>(GET_CONST_NODE(ga->op1));
   if(ue == nullptr)
   {
      return nullptr;
   }
   return [stmt, ga, ue](NodeContainer* NC) {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                     "Analysing unary operation " + ue->get_kind_text() + " " + ga->ToString());

      const auto function_id = GET_INDEX_CONST_NODE(ga->scpe);
      const auto sink = NC->addVarNode(ga->op0, function_id);
      const auto _source = NC->addVarNode(ue->op, function_id);
      const auto op_kind = ue->get_kind();

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level,
                     "---Added UnaryOp for " + ue->get_kind_text() + " with range " +
                         tree_helper::Range(stmt)->ToString());
      return new UnaryOpNode(sink, _source, stmt, op_kind);
   };
}

void UnaryOpNode::print(std::ostream& OS) const
{
   OS << getSink()->getValue() << " = " << tree_node::GetString(getOpcode()) << "( " << getSource()->getValue() << " )";
}

void UnaryOpNode::printDot(std::ostream& OS) const
{
   OS << " \"" << this << "\" [label=\"";

   // Instruction bitwidth
   const auto bw = getSink()->getBitWidth();
   const bool oprndSigned = range_analysis::isSignedType(source->getValue());

   if(opcode == nop_expr_K || opcode == convert_expr_K)
   {
      if(bw < getSource()->getBitWidth())
      {
         OS << "trunc i" << bw;
      }
      else
      {
         if(tree_helper::IsPointerType(getSource()->getValue()))
         {
            OS << "ptr_cast i" << bw;
         }
         else
         {
            if(oprndSigned)
            {
               OS << "sext i" << bw;
            }
            else
            {
               OS << "zext i" << bw;
            }
         }
      }
   }
   else if(opcode == fix_trunc_expr_K)
   {
      const auto type = tree_helper::CGetType(getSink()->getValue());
      if(const auto* int_type = GetPointer<const integer_type>(GET_CONST_NODE(type)))
      {
         if(int_type->unsigned_flag)
         {
            OS << "fptoui i" << bw;
         }
         else
         {
            OS << "fptosi i" << bw;
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
      getIntersect()->print(OS);
   }

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

   const auto& VS = getSink()->getValue();
   OS << " \"" << this << "\" -> \"" << VS << "\"\n";
}
