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
 * @file range_analysis_helper.cpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "range_analysis_helper.hpp"

#include "Range.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "tree_helper.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

kind range_analysis::op_unsigned(kind op)
{
   switch(op)
   {
      case ge_expr_K:
         return unge_expr_K;
      case gt_expr_K:
         return ungt_expr_K;
      case le_expr_K:
         return unle_expr_K;
      case lt_expr_K:
         return unlt_expr_K;
      case eq_expr_K:
         return uneq_expr_K;
      case unge_expr_K:
      case ungt_expr_K:
      case unle_expr_K:
      case unlt_expr_K:
      case uneq_expr_K:
      case ne_expr_K:
         return op;

      case assert_expr_K:
      case bit_and_expr_K:
      case bit_ior_expr_K:
      case bit_xor_expr_K:
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
      case lshift_expr_K:
      case max_expr_K:
      case mem_ref_K:
      case min_expr_K:
      case minus_expr_K:
      case modify_expr_K:
      case mult_expr_K:
      case mult_highpart_expr_K:
      case ordered_expr_K:
      case plus_expr_K:
      case pointer_plus_expr_K:
      case postdecrement_expr_K:
      case postincrement_expr_K:
      case predecrement_expr_K:
      case preincrement_expr_K:
      case range_expr_K:
      case rdiv_expr_K:
      case round_div_expr_K:
      case round_mod_expr_K:
      case rrotate_expr_K:
      case rshift_expr_K:
      case set_le_expr_K:
      case trunc_div_expr_K:
      case trunc_mod_expr_K:
      case truth_and_expr_K:
      case truth_andif_expr_K:
      case truth_or_expr_K:
      case truth_orif_expr_K:
      case truth_xor_expr_K:
      case try_catch_expr_K:
      case try_finally_K:
      case ltgt_expr_K:
      case unordered_expr_K:
      case widen_sum_expr_K:
      case widen_mult_expr_K:
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
      case sat_plus_expr_K:
      case sat_minus_expr_K:
      case extractvalue_expr_K:
      case extractelement_expr_K:
      case frem_expr_K:
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
         break;
   }
   THROW_UNREACHABLE("Unhandled predicate (" + tree_node::GetString(op) + ")");
   return static_cast<kind>(-1);
}

kind range_analysis::op_inv(kind op)
{
   switch(op)
   {
      case ge_expr_K:
         return lt_expr_K;
      case gt_expr_K:
         return le_expr_K;
      case le_expr_K:
         return gt_expr_K;
      case lt_expr_K:
         return ge_expr_K;
      case unge_expr_K:
         return unlt_expr_K;
      case ungt_expr_K:
         return unle_expr_K;
      case unle_expr_K:
         return ungt_expr_K;
      case unlt_expr_K:
         return unge_expr_K;
      case eq_expr_K:
      case uneq_expr_K:
         return ne_expr_K;
      case ne_expr_K:
         return eq_expr_K;

      case assert_expr_K:
      case bit_and_expr_K:
      case bit_ior_expr_K:
      case bit_xor_expr_K:
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
      case lshift_expr_K:
      case max_expr_K:
      case mem_ref_K:
      case min_expr_K:
      case minus_expr_K:
      case modify_expr_K:
      case mult_expr_K:
      case mult_highpart_expr_K:
      case ordered_expr_K:
      case plus_expr_K:
      case pointer_plus_expr_K:
      case postdecrement_expr_K:
      case postincrement_expr_K:
      case predecrement_expr_K:
      case preincrement_expr_K:
      case range_expr_K:
      case rdiv_expr_K:
      case round_div_expr_K:
      case round_mod_expr_K:
      case rrotate_expr_K:
      case rshift_expr_K:
      case set_le_expr_K:
      case trunc_div_expr_K:
      case trunc_mod_expr_K:
      case truth_and_expr_K:
      case truth_andif_expr_K:
      case truth_or_expr_K:
      case truth_orif_expr_K:
      case truth_xor_expr_K:
      case try_catch_expr_K:
      case try_finally_K:
      case ltgt_expr_K:
      case unordered_expr_K:
      case widen_sum_expr_K:
      case widen_mult_expr_K:
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
      case sat_plus_expr_K:
      case sat_minus_expr_K:
      case extractvalue_expr_K:
      case extractelement_expr_K:
      case frem_expr_K:
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
         break;
   }

   THROW_UNREACHABLE("Unhandled predicate (" + tree_node::GetString(op) + ")");
   return static_cast<kind>(-1);
}

kind range_analysis::op_swap(kind op)
{
   switch(op)
   {
      case ge_expr_K:
         return le_expr_K;
      case gt_expr_K:
         return lt_expr_K;
      case le_expr_K:
         return ge_expr_K;
      case lt_expr_K:
         return gt_expr_K;
      case unge_expr_K:
         return unle_expr_K;
      case ungt_expr_K:
         return unlt_expr_K;
      case unle_expr_K:
         return unge_expr_K;
      case unlt_expr_K:
         return ungt_expr_K;

      case bit_and_expr_K:
      case bit_ior_expr_K:
      case bit_xor_expr_K:
      case eq_expr_K:
      case ne_expr_K:
      case uneq_expr_K:
         return op;

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
      case lshift_expr_K:
      case max_expr_K:
      case mem_ref_K:
      case min_expr_K:
      case minus_expr_K:
      case modify_expr_K:
      case mult_expr_K:
      case mult_highpart_expr_K:
      case ordered_expr_K:
      case plus_expr_K:
      case pointer_plus_expr_K:
      case postdecrement_expr_K:
      case postincrement_expr_K:
      case predecrement_expr_K:
      case preincrement_expr_K:
      case range_expr_K:
      case rdiv_expr_K:
      case round_div_expr_K:
      case round_mod_expr_K:
      case rrotate_expr_K:
      case rshift_expr_K:
      case set_le_expr_K:
      case trunc_div_expr_K:
      case trunc_mod_expr_K:
      case truth_and_expr_K:
      case truth_andif_expr_K:
      case truth_or_expr_K:
      case truth_orif_expr_K:
      case truth_xor_expr_K:
      case try_catch_expr_K:
      case try_finally_K:
      case ltgt_expr_K:
      case unordered_expr_K:
      case widen_sum_expr_K:
      case widen_mult_expr_K:
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
      case sat_plus_expr_K:
      case sat_minus_expr_K:
      case extractvalue_expr_K:
      case extractelement_expr_K:
      case frem_expr_K:
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
         break;
   }

   THROW_UNREACHABLE("Unhandled predicate (" + tree_node::GetString(op) + ")");
   return static_cast<kind>(-1);
}

bool range_analysis::isCompare(kind c_type)
{
   return c_type == eq_expr_K || c_type == ne_expr_K || c_type == gt_expr_K || c_type == lt_expr_K ||
          c_type == ge_expr_K || c_type == le_expr_K;
}

bool range_analysis::isCompare(const struct binary_expr* condition)
{
   return isCompare(condition->get_kind());
}

tree_nodeConstRef range_analysis::branchOpRecurse(const tree_nodeConstRef op)
{
   if(const auto* nop = GetPointer<const nop_expr>(op))
   {
      return branchOpRecurse(nop->op);
   }
   else if(const auto* ce = GetPointer<const convert_expr>(op))
   {
      return branchOpRecurse(ce->op);
   }
   else if(const auto* ssa = GetPointer<const ssa_name>(op))
   {
      const auto DefStmt = GET_CONST_NODE(ssa->CGetDefStmt());
      if(const auto* gp = GetPointer<const gimple_phi>(DefStmt))
      {
         const auto& defEdges = gp->CGetDefEdgesList();
         THROW_ASSERT(not defEdges.empty(), "Branch variable definition from nowhere");
         return defEdges.size() > 1 ? DefStmt : branchOpRecurse(defEdges.front().first);
      }
      else if(const auto* ga = GetPointer<const gimple_assign>(DefStmt))
      {
         return branchOpRecurse(ga->op1);
      }
      else if(GetPointer<const gimple_nop>(DefStmt) != nullptr)
      {
         // Branch variable is a function parameter
         return DefStmt;
      }
      THROW_UNREACHABLE("Branch var definition statement not handled (" + DefStmt->get_kind_text() + " " +
                        DefStmt->ToString() + ")");
   }
   else if(op->get_kind() == tree_reindex_K)
   {
      return branchOpRecurse(GET_CONST_NODE(op));
   }
   return op;
}

bool range_analysis::isValidType(const tree_nodeConstRef& _tn)
{
   const auto tn = _tn->get_kind() == tree_reindex_K ? GET_CONST_NODE(_tn) : _tn;
   switch(tn->get_kind())
   {
      case boolean_type_K:
      case enumeral_type_K:
      case integer_type_K:
#ifdef INTEGER_PTR
      case pointer_type_K:
#endif
         return true;
      case array_type_K:
         return isValidType(tree_helper::CGetElements(tn));
      case integer_cst_K:
      case string_cst_K:
      case CASE_DECL_NODES:
      case ssa_name_K:
         return isValidType(tree_helper::CGetType(tn));
      case real_type_K:
      case real_cst_K:
      case vector_type_K:
      case CharType_K:
      case nullptr_type_K:
      case type_pack_expansion_K:
      case complex_type_K:
      case function_type_K:
      case lang_type_K:
      case method_type_K:
      case offset_type_K:
#ifndef INTEGER_PTR
      case pointer_type_K:
#endif
      case qual_union_type_K:
      case record_type_K:
      case reference_type_K:
      case set_type_K:
      case template_type_parm_K:
      case typename_type_K:
      case type_argument_pack_K:
      case union_type_K:
      case void_type_K:
         return false;
      case complex_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case lut_expr_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case binfo_K:
      case block_K:
      case constructor_K:
      case error_mark_K:
      case identifier_node_K:
      case statement_list_K:
      case tree_list_K:
      case tree_vec_K:
      case call_expr_K:
      case CASE_FAKE_NODES:
      case CASE_UNARY_EXPRESSION:
      case CASE_BINARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_PRAGMA_NODES:
      case CASE_CPP_NODES:
      case CASE_GIMPLE_NODES:
      default:
         THROW_UNREACHABLE("Unhandled node type (" + tn->get_kind_text() + " " + tn->ToString() + ")");
   }
   return false;
}

bool range_analysis::isValidInstruction(const tree_nodeConstRef& stmt, const FunctionBehaviorConstRef& FB)
{
   tree_nodeConstRef Type = nullptr;
   switch(GET_CONST_NODE(stmt)->get_kind())
   {
      case gimple_assign_K:
      {
         auto* ga = GetPointer<const gimple_assign>(GET_CONST_NODE(stmt));
         if(GET_CONST_NODE(tree_helper::CGetType(ga->op0))->get_kind() == vector_type_K)
         {
            // Vector arithmetic not yet supported
            return false;
         }
         if(tree_helper::IsLoad(stmt, FB->get_function_mem()))
         {
            Type = tree_helper::CGetType(ga->op0);
            break;
         }
         else if(tree_helper::IsStore(stmt, FB->get_function_mem()))
         {
            Type = tree_helper::CGetType(ga->op1);
            break;
         }
         Type = tree_helper::CGetType(ga->op0);

         switch(GET_CONST_NODE(ga->op1)->get_kind())
         {
            /// cst_node cases
            case integer_cst_K:
            case string_cst_K:
               break;

            /// unary_expr cases
            case nop_expr_K:
            case abs_expr_K:
            case bit_not_expr_K:
            case convert_expr_K:
            case negate_expr_K:
            case view_convert_expr_K:
            {
               const auto* ue = GetPointer<const unary_expr>(GET_CONST_NODE(ga->op1));
               if(GetPointer<const expr_node>(GET_CONST_NODE(ue->op)))
               {
                  // Nested operations not supported
                  return false;
               }
               break;
            }

            /// binary_expr cases
            case plus_expr_K:
            case minus_expr_K:
            case mult_expr_K:
            case widen_mult_expr_K:
            case trunc_div_expr_K:
            case trunc_mod_expr_K:
            case lshift_expr_K:
            case rshift_expr_K:
            case bit_and_expr_K:
            case bit_ior_expr_K:
            case bit_xor_expr_K:
            case eq_expr_K:
            case ne_expr_K:
            case gt_expr_K:
            case ge_expr_K:
            case lt_expr_K:
            case le_expr_K:
#ifdef INTEGER_PTR
            case pointer_plus_expr_K:
#endif
            case min_expr_K:
            case max_expr_K:
            case sat_plus_expr_K:
            case sat_minus_expr_K:
            {
               const auto bin_op = GetPointer<const binary_expr>(GET_CONST_NODE(ga->op1));
               if(!isValidType(bin_op->op0) || !isValidType(bin_op->op1))
               {
                  return false;
               }
               break;
            }

            /// ternary_expr case
            case cond_expr_K:
               break;

            case ssa_name_K:
            {
               if(!isValidType(GET_CONST_NODE(ga->op1)))
               {
                  return false;
               }
               break;
            }

            // Unary case
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
// Binary case
#ifndef INTEGER_PTR
            case pointer_plus_expr_K:
#endif
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
            case unge_expr_K:
            case ungt_expr_K:
            case unlt_expr_K:
            case unle_expr_K:
            case uneq_expr_K:
            case ltgt_expr_K:
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

            // Ternary case
            case component_ref_K:
            case bit_field_ref_K:
            case bit_ior_concat_expr_K:
            case vtable_ref_K:
            case with_cleanup_expr_K:
            case obj_type_ref_K:
            case save_expr_K:
            case vec_cond_expr_K:
            case vec_perm_expr_K:
            case dot_prod_expr_K:
            case ternary_plus_expr_K:
            case ternary_pm_expr_K:
            case ternary_mp_expr_K:
            case ternary_mm_expr_K:
            case fshl_expr_K:
            case fshr_expr_K:
            case CASE_QUATERNARY_EXPRESSION:
            case CASE_TYPE_NODES:
            case complex_cst_K:
            case real_cst_K:
            case void_cst_K:
            case CASE_DECL_NODES:
            case CASE_FAKE_NODES:
            case CASE_GIMPLE_NODES:
            case CASE_PRAGMA_NODES:
            case CASE_CPP_NODES:
            case aggr_init_expr_K:
            case case_label_expr_K:
            case lut_expr_K:
            case target_expr_K:
            case target_mem_ref_K:
            case target_mem_ref461_K:
            case binfo_K:
            case block_K:
            case constructor_K:
            case error_mark_K:
            case identifier_node_K:
            case statement_list_K:
            case tree_list_K:
            case tree_vec_K:
            case call_expr_K:
            case vector_cst_K:
            case insertvalue_expr_K:
            case insertelement_expr_K:
            default:
               return false;
         }
      }
      break;

      case gimple_phi_K:
      {
         const auto* phi = GetPointer<const gimple_phi>(GET_CONST_NODE(stmt));
         Type = tree_helper::CGetType(phi->res);
      }
      break;

      case gimple_asm_K:
      case gimple_bind_K:
      case gimple_call_K:
      case gimple_cond_K:
      case gimple_for_K:
      case gimple_goto_K:
      case gimple_label_K:
      case gimple_multi_way_if_K:
      case gimple_nop_K:
      case gimple_pragma_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case gimple_return_K:
      case gimple_switch_K:
      case gimple_while_K:
      case CASE_UNARY_EXPRESSION:
      case CASE_BINARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_CPP_NODES:
      case CASE_MISCELLANEOUS:
      default:
         return false;
   }
   return isValidType(Type);
}

bool range_analysis::isSignedType(const tree_nodeConstRef& _tn)
{
   const auto tn = _tn->get_kind() == tree_reindex_K ? GET_CONST_NODE(_tn) : _tn;
   switch(tn->get_kind())
   {
      case enumeral_type_K:
         return !GetPointer<const enumeral_type>(tn)->unsigned_flag;
      case integer_type_K:
         return !GetPointer<const integer_type>(tn)->unsigned_flag;
      case real_type_K:
         return true;
      case boolean_type_K:
      case array_type_K:
      case CharType_K:
      case nullptr_type_K:
      case type_pack_expansion_K:
      case complex_type_K:
      case function_type_K:
      case lang_type_K:
      case method_type_K:
      case offset_type_K:
      case pointer_type_K:
      case qual_union_type_K:
      case record_type_K:
      case reference_type_K:
      case set_type_K:
      case template_type_parm_K:
      case typename_type_K:
      case union_type_K:
      case vector_type_K:
      case void_type_K:
      case type_argument_pack_K:
         return false;
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case ssa_name_K:
         return isSignedType(tree_helper::CGetType(tn));
      case aggr_init_expr_K:
      case case_label_expr_K:
      case lut_expr_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case binfo_K:
      case block_K:
      case constructor_K:
      case error_mark_K:
      case identifier_node_K:
      case statement_list_K:
      case tree_list_K:
      case tree_vec_K:
      case call_expr_K:
      case CASE_FAKE_NODES:
      case CASE_UNARY_EXPRESSION:
      case CASE_BINARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_PRAGMA_NODES:
      case CASE_CPP_NODES:
      case CASE_GIMPLE_NODES:
      default:
         THROW_UNREACHABLE("Unhandled node type (" + tn->get_kind_text() + " " + tn->ToString() + ")");
   }
   return true;
}

RangeRef range_analysis::makeSatisfyingCmpRegion(kind pred, const RangeConstRef& Other)
{
   const auto bw = Other->getBitWidth();
   if(Other->isUnknown() || Other->isEmpty())
   {
      return RangeRef(Other->clone());
   }
   if(Other->isAnti() && pred != eq_expr_K && pred != ne_expr_K && pred != uneq_expr_K)
   {
      THROW_UNREACHABLE("Invalid request " + tree_node::GetString(pred) + " " + Other->ToString());
      return RangeRef(new Range(Empty, bw));
   }

   switch(pred)
   {
      case ge_expr_K:
         return RangeRef(new Range(Regular, bw, Other->getSignedMax(), APInt::getSignedMaxValue(bw)));
      case gt_expr_K:
         return RangeRef(new Range(Regular, bw, Other->getSignedMax() + Range::MinDelta, APInt::getSignedMaxValue(bw)));
      case le_expr_K:
         return RangeRef(new Range(Regular, bw, APInt::getSignedMinValue(bw), Other->getSignedMin()));
      case lt_expr_K:
         return RangeRef(new Range(Regular, bw, APInt::getSignedMinValue(bw), Other->getSignedMin() - Range::MinDelta));
      case unge_expr_K:
         return RangeRef(new Range(Regular, bw, Other->getUnsignedMax(), APInt::getMaxValue(bw)));
      case ungt_expr_K:
         return RangeRef(new Range(Regular, bw, Other->getUnsignedMax() + Range::MinDelta, APInt::getMaxValue(bw)));
      case unle_expr_K:
         return RangeRef(new Range(Regular, bw, APInt::getMinValue(bw), Other->getUnsignedMin()));
      case unlt_expr_K:
         return RangeRef(new Range(Regular, bw, APInt::getMinValue(bw), Other->getUnsignedMin() - Range::MinDelta));
      case uneq_expr_K:
      case eq_expr_K:
         return RangeRef(Other->clone());
      case ne_expr_K:
         return Other->getAnti();

      case assert_expr_K:
      case bit_and_expr_K:
      case bit_ior_expr_K:
      case bit_xor_expr_K:
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
      case lshift_expr_K:
      case max_expr_K:
      case mem_ref_K:
      case min_expr_K:
      case minus_expr_K:
      case modify_expr_K:
      case mult_expr_K:
      case mult_highpart_expr_K:
      case ordered_expr_K:
      case plus_expr_K:
      case pointer_plus_expr_K:
      case postdecrement_expr_K:
      case postincrement_expr_K:
      case predecrement_expr_K:
      case preincrement_expr_K:
      case range_expr_K:
      case rdiv_expr_K:
      case round_div_expr_K:
      case round_mod_expr_K:
      case rrotate_expr_K:
      case rshift_expr_K:
      case set_le_expr_K:
      case trunc_div_expr_K:
      case trunc_mod_expr_K:
      case truth_and_expr_K:
      case truth_andif_expr_K:
      case truth_or_expr_K:
      case truth_orif_expr_K:
      case truth_xor_expr_K:
      case try_catch_expr_K:
      case try_finally_K:
      case ltgt_expr_K:
      case unordered_expr_K:
      case widen_sum_expr_K:
      case widen_mult_expr_K:
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
      case sat_plus_expr_K:
      case sat_minus_expr_K:
      case extractvalue_expr_K:
      case extractelement_expr_K:
      case frem_expr_K:
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
         break;
   }
   THROW_UNREACHABLE("Unhandled compare operation (" + tree_node::GetString(pred) + ")");
   return nullptr;
}