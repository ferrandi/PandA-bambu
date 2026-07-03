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
 * @file range_analysis_helper.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "range_analysis_helper.hpp"

#include "Range.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "ir_helper.hpp"
#include "ir_node.hpp"

#define INTEGER_PTR // Pointers are considered as integers

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"

kind_R range_analysis::op_unsigned(kind_R op)
{
   switch(op)
   {
      case ge_node_R:
         return unsigned_ge_node_R;
      case gt_node_R:
         return unsigned_gt_node_R;
      case le_node_R:
         return unsigned_le_node_R;
      case lt_node_R:
         return unsigned_lt_node_R;
      case eq_node_R:
         return unsigned_eq_node_R;
      case unsigned_ge_node_R:
      case unsigned_gt_node_R:
      case unsigned_le_node_R:
      case unsigned_lt_node_R:
      case unsigned_eq_node_R:
      case ne_node_R:
         return op;

      default:
         break;
   }
   THROW_UNREACHABLE("Unhandled predicate");
   return error_R;
}

kind_R range_analysis::op_inv(kind_R op)
{
   switch(op)
   {
      case ge_node_R:
         return lt_node_R;
      case gt_node_R:
         return le_node_R;
      case le_node_R:
         return gt_node_R;
      case lt_node_R:
         return ge_node_R;
      case unsigned_ge_node_R:
         return unsigned_lt_node_R;
      case unsigned_gt_node_R:
         return unsigned_le_node_R;
      case unsigned_le_node_R:
         return unsigned_gt_node_R;
      case unsigned_lt_node_R:
         return unsigned_ge_node_R;
      case eq_node_R:
      case unsigned_eq_node_R:
         return ne_node_R;
      case ne_node_R:
         return eq_node_R;

      default:
         break;
   }

   THROW_UNREACHABLE("Unhandled predicate");
   return error_R;
}

kind_R range_analysis::op_swap(kind_R op)
{
   switch(op)
   {
      case ge_node_R:
         return le_node_R;
      case gt_node_R:
         return lt_node_R;
      case le_node_R:
         return ge_node_R;
      case lt_node_R:
         return gt_node_R;
      case unsigned_ge_node_R:
         return unsigned_le_node_R;
      case unsigned_gt_node_R:
         return unsigned_lt_node_R;
      case unsigned_le_node_R:
         return unsigned_ge_node_R;
      case unsigned_lt_node_R:
         return unsigned_gt_node_R;

      case eq_node_R:
      case ne_node_R:
      case unsigned_eq_node_R:
         return op;

      default:
         break;
   }

   THROW_UNREACHABLE("Unhandled predicate " + getString(op));
   return error_R;
}

kind_R range_analysis::op_convert(kind op)
{
   switch(op)
   {
#define RANGECODE(SYM) \
   case SYM##_K:       \
      return SYM##_R;
#include "range.def"
#undef RANGECODE
      default:
         break;
   }
   THROW_UNREACHABLE("Unhandled predicate: " + ir_node::GetString(op));
   return error_R;
}

std::string range_analysis::getString(kind_R op)
{
   switch(op)
   {
#define RANGECODE(SYM) \
   case SYM##_R:       \
      return #SYM;
      RANGE_SPECIFIC_CODE_LIST
#include "range.def"
#undef RANGECODE
      case error_R:
      default:
         break;
   }
   THROW_UNREACHABLE("Unhandled predicate");
   return "";
}

bool range_analysis::isCompare(kind_R c_type)
{
   return c_type == eq_node_R || c_type == ne_node_R || c_type == gt_node_R || c_type == lt_node_R ||
          c_type == ge_node_R || c_type == le_node_R;
}

bool range_analysis::isCompare(const struct binary_node* condition)
{
   return isCompare(op_convert(condition->get_kind()));
}

ir_nodeConstRef range_analysis::castTraverse(ir_nodeConstRef op)
{
   if(op->get_kind() == ssa_node_K)
   {
      const auto ssa = GetPointerS<const ssa_node>(op);
      const auto def_node = ssa->GetDefStmt();
      if(def_node->get_kind() == assign_stmt_K)
      {
         const auto ga = GetPointerS<const assign_stmt>(def_node);
         if(ga->op1->get_kind() == ssa_node_K)
         {
            return castTraverse(ga->op1);
         }
         else if(const auto nop = GetPointer<const nop_node>(ga->op1))
         {
            return castTraverse(nop->op);
         }
         return def_node;
      }
      else if(def_node->get_kind() == phi_stmt_K)
      {
         const auto gp = GetPointerS<const phi_stmt>(def_node);
         const auto& defEdges = gp->CGetDefEdgesList();
         auto op_node = defEdges.front().first;
         for(auto& [var, prev_bbi] : defEdges)
         {
            if(op_node->index != var->index)
            {
               return def_node;
            }
         }
         return castTraverse(op_node);
      }
      else if(def_node->get_kind() == nop_stmt_K)
      {
         // Variable is a function parameter
         return def_node;
      }
      THROW_UNREACHABLE("Definition statement not handled (" + def_node->get_kind_text() + " " + def_node->ToString() +
                        ")");
   }
   return op;
}

ir_nodeConstRef range_analysis::castTraverseSSA(ir_nodeConstRef op)
{
   const auto def = castTraverse(op);
   if(def->get_kind() == assign_stmt_K)
   {
      return GetPointerS<const assign_stmt>(def)->op0;
   }
   else if(def->get_kind() == phi_stmt_K)
   {
      return GetPointerS<const phi_stmt>(def)->res;
   }
   return op;
}

Range range_analysis::evaluate(kind_R opcode, Range::bw_t bw, const Range& op1, const Range& op2, bool opSigned)
{
   switch(opcode)
   {
      case gep_node_R:
      case add_node_R:
         return op1.add(op2);
      case sub_node_R:
         return op1.sub(op2);
      case mul_node_R:
         return op1.mul(op2);
      case widen_mul_node_R:
         return opSigned ? op1.sextOrTrunc(bw).mul(op2.sextOrTrunc(bw)) : op1.zextOrTrunc(bw).mul(op2.sextOrTrunc(bw));
      case idiv_node_R:
         return opSigned ? op1.sdiv(op2) : op1.udiv(op2);
      case irem_node_R:
         if(opSigned)
         {
            auto res = op1.srem(op2);
            if(!res.isUnknown() && !res.isEmpty() && res.getSignedMin() == 0)
            {
               return res.unionWith(res.negate());
            }
            return res;
         }
         return op1.urem(op2);
      case shl_node_R:
         return opSigned ? op1.sextOrTrunc(bw).shl(op2) : op1.zextOrTrunc(bw).shl(op2);
      case shr_node_R:
         return opSigned ? op1.shr(op2, true).sextOrTrunc(bw) : op1.shr(op2, false).zextOrTrunc(bw);
      case and_node_R:
         return op1.And(op2);
      case or_node_R:
         return op1.Or(op2);
      case xor_node_R:
         return op1.Xor(op2);
      case eq_node_R:
         if(op1.getBitWidth() < op2.getBitWidth())
         {
            return opSigned ? op1.sextOrTrunc(op2.getBitWidth()).Eq(op2, bw) :
                              op1.zextOrTrunc(op2.getBitWidth()).Eq(op2, bw);
         }
         else if(op2.getBitWidth() < op1.getBitWidth())
         {
            return opSigned ? op2.sextOrTrunc(op1.getBitWidth()).Eq(op1, bw) :
                              op2.zextOrTrunc(op1.getBitWidth()).Eq(op1, bw);
         }
         return op1.Eq(op2, bw);
      case ne_node_R:
         return op1.Ne(op2, bw);
      case gt_node_R:
         return opSigned ? op1.Sgt(op2, bw) : op1.Ugt(op2, bw);
      case ge_node_R:
         return opSigned ? op1.Sge(op2, bw) : op1.Uge(op2, bw);
      case lt_node_R:
         return opSigned ? op1.Slt(op2, bw) : op1.Ult(op2, bw);
      case le_node_R:
         return opSigned ? op1.Sle(op2, bw) : op1.Ule(op2, bw);
      case min_node_R:
         return opSigned ? op1.SMin(op2) : op1.UMin(op2);
      case max_node_R:
         return opSigned ? op1.SMax(op2) : op1.UMax(op2);
      case add_sat_node_R:
         return opSigned ? op1.sat_add(op2) : op1.usat_add(op2);
      case sub_sat_node_R:
         return opSigned ? op1.sat_sub(op2) : op1.usat_sub(op2);

      default:
         THROW_UNREACHABLE("Unhandled binary operation");
         break;
   }
   return Range(Regular, bw);
}

Range range_analysis::staticCompare(kind_R _compare_op, Range::bw_t bw, const ir_nodeConstRef& op0,
                                    const ir_nodeConstRef& op1)
{
   THROW_ASSERT(isCompare(_compare_op), "_compare_op is not a compare operator.");
   const auto isSigned = range_analysis::isSignedType(op0);
   const auto compare_op = isSigned ? _compare_op : op_unsigned(_compare_op);

   if(ir_helper::IsConstant(op0) && ir_helper::IsConstant(op1))
   {
      const auto range0 = ir_helper::NodeRange(op0);
      const auto range1 = ir_helper::NodeRange(op1);
      return evaluate(compare_op, bw, range0, range1, isSigned);
   }
   if(op0->index == op1->index)
   {
      if(compare_op == eq_node_R || compare_op == ge_node_R || compare_op == le_node_R ||
         compare_op == unsigned_eq_node_R || compare_op == unsigned_ge_node_R || compare_op == unsigned_le_node_R)
      {
         return Range(Regular, bw, 1, 1);
      }
      return Range(Regular, bw, 0, 0);
   }

   return Range(Regular, bw, 0, 1);
}

bool range_analysis::isValidType(const ir_nodeConstRef& tn)
{
   switch(tn->get_kind())
   {
      case integer_ty_node_K:
#ifdef INTEGER_PTR
      case pointer_ty_node_K:
#endif
         return true;
      case array_ty_node_K:
         return isValidType(ir_helper::CGetElements(tn));
      case constant_int_val_node_K:
      case CASE_DECL_NODES:
      case ssa_node_K:
         return isValidType(ir_helper::CGetType(tn));
      case function_ty_node_K:
#ifndef INTEGER_PTR
      case pointer_ty_node_K:
#endif
      case constant_fp_val_node_K:
      case real_ty_node_K:
      case struct_ty_node_K:
      case constant_vector_val_node_K:
      case vector_ty_node_K:
      case void_ty_node_K:
         return false;
      case call_node_K:
      case constructor_node_K:
      case identifier_node_K:
      case lut_node_K:
      case statement_list_node_K:
      case CASE_FAKE_NODES:
      case CASE_UNARY_NODES:
      case CASE_BINARY_NODES:
      case CASE_TERNARY_NODES:
      case CASE_NODE_STMTS:
      default:
         THROW_UNREACHABLE("Unhandled node type (" + tn->get_kind_text() + " " + tn->ToString() + ")");
   }
   return false;
}

bool range_analysis::isValidInstruction(const ir_nodeConstRef& stmt, const FunctionBehaviorConstRef& FB)
{
   ir_nodeConstRef Type = nullptr;
   switch(stmt->get_kind())
   {
      case assign_stmt_K:
      {
         const auto* ga = GetPointerS<const assign_stmt>(stmt);
         Type = ir_helper::CGetType(ga->op0);
         if(Type->get_kind() == vector_ty_node_K)
         {
            // Vector arithmetic not yet supported
            return false;
         }
         if(ir_helper::IsStore(stmt, FB->get_function_mem()))
         {
            return false;
         }
         if(ir_helper::IsLoad(stmt, FB->get_function_mem()))
         {
            break;
         }

         switch(ga->op1->get_kind())
         {
            /// cst_node cases
            case constant_int_val_node_K:
               break;

            /// unary_node cases
            case abs_node_K:
            case not_node_K:
            case neg_node_K:
            case nop_node_K:
            case bitcast_node_K:
            {
               const auto* ue = GetPointerS<const unary_node>(ga->op1);
               if(GetPointer<const expr_node>(ue->op))
               {
                  // Nested operations not supported
                  return false;
               }
               break;
            }

            /// binary_node cases
            case and_node_K:
            case or_node_K:
            case xor_node_K:
            case eq_node_K:
            case ge_node_K:
            case gt_node_K:
            case le_node_K:
            case shl_node_K:
            case lt_node_K:
            case max_node_K:
            case min_node_K:
            case sub_node_K:
            case mul_node_K:
            case ne_node_K:
            case add_node_K:
#ifdef INTEGER_PTR
            case gep_node_K:
#endif
            case shr_node_K:
            case sub_sat_node_K:
            case add_sat_node_K:
            case idiv_node_K:
            case irem_node_K:
            case widen_mul_node_K:
            {
               const auto* be = GetPointerS<const binary_node>(ga->op1);
               if(!isValidType(be->op0) || !isValidType(be->op1))
               {
                  return false;
               }
               break;
            }

            /// ternary_node case
            case select_node_K:
            {
               const auto* te = GetPointerS<const ternary_node>(ga->op1);
               if(!isValidType(te->op0) || !isValidType(te->op1) || !isValidType(te->op2))
               {
                  return false;
               }
               break;
            }

            case ssa_node_K:
            {
               if(!isValidType(ga->op1))
               {
                  return false;
               }
               break;
            }

            // Unary case
            case addr_node_K:
            case fptoi_node_K:
            case itofp_node_K:
            case mem_access_node_K:
            case unaligned_mem_access_node_K:
// Binary case
#ifndef INTEGER_PTR
            case gep_node_K:
#endif
            case fdiv_node_K:
            case frem_node_K:
            case extract_bit_node_K:
            case extractvalue_node_K:
            case extractelement_node_K:

            // Ternary case
            case concat_bit_node_K:
            case shufflevector_node_K:
            case ternary_add_node_K:
            case ternary_as_node_K:
            case ternary_sa_node_K:
            case ternary_ss_node_K:
            case fshl_node_K:
            case fshr_node_K:
            case CASE_TYPE_NODES:
            case constant_fp_val_node_K:
            case CASE_DECL_NODES:
            case CASE_FAKE_NODES:
            case CASE_NODE_STMTS:
            case lut_node_K:
            case constructor_node_K:
            case identifier_node_K:
            case statement_list_node_K:
            case call_node_K:
            case constant_vector_val_node_K:
            case insertvalue_node_K:
            case insertelement_node_K:
            default:
               return false;
         }
         break;
      }

      case phi_stmt_K:
      {
         const auto* gp = GetPointerS<const phi_stmt>(stmt);
         Type = ir_helper::CGetType(gp->res);
         break;
      }

      case call_stmt_K:
      case multi_way_if_stmt_K:
      case nop_stmt_K:
      case return_stmt_K:
      case CASE_UNARY_NODES:
      case CASE_BINARY_NODES:
      case CASE_TERNARY_NODES:
      case CASE_TYPE_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_MISCELLANEOUS:
      default:
         return false;
   }
   return isValidType(Type);
}

bool range_analysis::isSignedType(const ir_nodeConstRef& tn)
{
   switch(tn->get_kind())
   {
      case integer_ty_node_K:
         return !GetPointerS<const integer_ty_node>(tn)->unsigned_flag;
      case real_ty_node_K:
         return true;
      case array_ty_node_K:
      case function_ty_node_K:
      case pointer_ty_node_K:
      case struct_ty_node_K:
      case vector_ty_node_K:
      case void_ty_node_K:
         return false;
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case ssa_node_K:
         return isSignedType(ir_helper::CGetType(tn));
      case lut_node_K:
      case constructor_node_K:
      case identifier_node_K:
      case statement_list_node_K:
      case call_node_K:
      case CASE_FAKE_NODES:
      case CASE_UNARY_NODES:
      case CASE_BINARY_NODES:
      case CASE_TERNARY_NODES:
      case CASE_NODE_STMTS:
      default:
         THROW_UNREACHABLE("Unhandled node type (" + tn->get_kind_text() + " " + tn->ToString() + ")");
   }
   return true;
}

Range range_analysis::makeSatisfyingCmpRegion(kind_R pred, const Range& Other)
{
   const auto bw = Other.getBitWidth();
   if(Other.isUnknown() || Other.isEmpty())
   {
      return Other;
   }
   if(Other.isAnti() && pred != eq_node_R && pred != ne_node_R && pred != unsigned_eq_node_R)
   {
      THROW_UNREACHABLE("Invalid request " + getString(pred) + " " + Other.ToString());
      return Range(Empty, bw);
   }

   switch(pred)
   {
      case ge_node_R:
         return Range(Regular, bw, Other.getSignedMax(), APInt::getSignedMaxValue(bw));
      case gt_node_R:
         return Range(Regular, bw, Other.getSignedMax() + Range::MinDelta, APInt::getSignedMaxValue(bw));
      case le_node_R:
         return Range(Regular, bw, APInt::getSignedMinValue(bw), Other.getSignedMin());
      case lt_node_R:
         return Range(Regular, bw, APInt::getSignedMinValue(bw), Other.getSignedMin() - Range::MinDelta);
      case unsigned_ge_node_R:
         return Range(Regular, bw, Other.getUnsignedMax(), APInt::getMaxValue(bw));
      case unsigned_gt_node_R:
         return Range(Regular, bw, Other.getUnsignedMax() + Range::MinDelta, APInt::getMaxValue(bw));
      case unsigned_le_node_R:
         return Range(Regular, bw, APInt::getMinValue(bw), Other.getUnsignedMin());
      case unsigned_lt_node_R:
         return Range(Regular, bw, APInt::getMinValue(bw), Other.getUnsignedMin() - Range::MinDelta);
      case unsigned_eq_node_R:
      case eq_node_R:
         return Other;
      case ne_node_R:
         return Other.getAnti();

      default:
         break;
   }
   THROW_UNREACHABLE("Unhandled compare operation (" + getString(pred) + ")");
   return Range(Regular, bw);
}

#pragma GCC diagnostic pop
