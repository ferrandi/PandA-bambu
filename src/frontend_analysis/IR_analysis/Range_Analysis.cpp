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
 *              Copyright (C) 2004-2019 Politecnico di Milano
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
 * @file Range_Analysis.cpp
 * @brief
 *
 * @author Michele Fiorito <michele2.fiorito@mail.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "Range_Analysis.hpp"

#include "config_HAVE_ASSERTS.hpp"

///. include
#include "Parameter.hpp"

/// behavior includes
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "op_graph.hpp"
#include "var_pp_functor.hpp"

#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "function_frontend_flow_step.hpp"

#include "dead_code_elimination.hpp"

/// HLS include
#include "hls_manager.hpp"
#include "hls_target.hpp"

/// HLS/memory include
#include "memory.hpp"

/// stl
#include "custom_map.hpp"
#include <map>
#include <set>
#include <sstream>
#include <vector>

/// Tree includes
#include "ext_tree_node.hpp"
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_reindex.hpp"

#include "bit_lattice.hpp"

#include "dbgPrintHelper.hpp"              // for DEBUG_LEVEL_
#include "string_manipulation.hpp"         // for GET_CLASS
#include <boost/filesystem/operations.hpp> // for create_directories

#define RA_JUMPSET
//    #define EARLY_DEAD_CODE_RESTART     // Abort analysis when dead code is detected instead of waiting step's end
#define INTEGER_PTR     // Pointers are considered as integers
#define BITVALUE_UPDATE // Read/write bitvalue information during the analysis
#ifdef BITVALUE_UPDATE
#define BITVALUE_PHI_MASK // Propagate bit_values through phis for floating-point variables
#endif

#define RA_EXEC_NORMAL 0
#define RA_EXEC_READONLY 1
#define RA_EXEC_SKIP 2
#ifndef NDEBUG
//    #define DEBUG_RANGE_OP
//    #define SCC_DEBUG
#endif

#define CASE_MISCELLANEOUS   \
   aggr_init_expr_K:         \
   case case_label_expr_K:   \
   case lut_expr_K:          \
   case target_expr_K:       \
   case target_mem_ref_K:    \
   case target_mem_ref461_K: \
   case binfo_K:             \
   case block_K:             \
   case constructor_K:       \
   case error_mark_K:        \
   case identifier_node_K:   \
   case ssa_name_K:          \
   case statement_list_K:    \
   case tree_list_K:         \
   case tree_vec_K:          \
   case call_expr_K

using bw_t = Range::bw_t;

union vcFloat
{
   float flt;
   struct _FP_STRUCT_LAYOUT
   {
#if __BYTE_ORDER == __BIG_ENDIAN
      uint32_t sign : 1;
      uint32_t exp : 8;
      uint32_t frac : 23;
#else
      uint32_t coded;
#endif
   } bits __attribute__((packed));
};

union vcDouble
{
   double dub;
   struct _FP_STRUCT_LAYOUT
   {
#if __BYTE_ORDER == __BIG_ENDIAN
      uint64_t sign : 1;
      uint64_t exp : 11;
      uint64_t frac : 52;
#else
      uint64_t coded;
#endif
   } bits __attribute__((packed));
};

bool tree_reindexCompare::operator()(const tree_nodeConstRef& lhs, const tree_nodeConstRef& rhs) const
{
   return static_cast<const tree_reindex*>(lhs.get())->actual_tree_node->index < static_cast<const tree_reindex*>(rhs.get())->actual_tree_node->index;
}

namespace
{
   // ========================================================================== //
   // Static global functions and definitions
   // ========================================================================== //

   // Used to print pseudo-edges in the Constraint Graph dot
   std::string pestring;
   std::stringstream pseudoEdgesString(pestring);

   kind op_unsigned(kind op)
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
      THROW_UNREACHABLE("Unhandled predicate (" + STR(op) + ")");
      return static_cast<kind>(-1);
   }

   kind op_inv(kind op)
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

      THROW_UNREACHABLE("Unhandled predicate (" + STR(op) + ")");
      return static_cast<kind>(-1);
   }

   kind op_swap(kind op)
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

      THROW_UNREACHABLE("Unhandled predicate (" + STR(op) + ")");
      return static_cast<kind>(-1);
   }

   bool isCompare(kind c_type)
   {
      return c_type == eq_expr_K || c_type == ne_expr_K || c_type == ltgt_expr_K || c_type == uneq_expr_K || c_type == gt_expr_K || c_type == lt_expr_K || c_type == ge_expr_K || c_type == le_expr_K || c_type == unlt_expr_K || c_type == ungt_expr_K ||
             c_type == unle_expr_K || c_type == unge_expr_K;
   }

   bool isCompare(const struct binary_expr* condition)
   {
      return isCompare(condition->get_kind());
   }

   tree_nodeConstRef branchOpRecurse(const tree_nodeConstRef op)
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
         THROW_UNREACHABLE("Branch var definition statement not handled (" + DefStmt->get_kind_text() + " " + DefStmt->ToString() + ")");
      }
      else if(op->get_kind() == tree_reindex_K)
      {
         return branchOpRecurse(GET_CONST_NODE(op));
      }
      return op;
   }

   // Print name of variable according to its type
   void printVarName(const tree_nodeConstRef& V, std::ostream& OS)
   {
      OS << GET_CONST_NODE(V)->ToString();
   }

   bool isValidType(const tree_nodeConstRef& tn)
   {
      switch(tn->get_kind())
      {
         case boolean_type_K:
         case enumeral_type_K:
         case integer_type_K:
#ifdef INTEGER_PTR
         case pointer_type_K:
#endif
         case real_type_K:
            return true;
         case array_type_K:
         case vector_type_K:
            return isValidType(tree_helper::CGetElements(tn));
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
         case tree_reindex_K:
            return isValidType(GET_CONST_NODE(tn));
         case CASE_CST_NODES:
         case CASE_DECL_NODES:
         case ssa_name_K:
            return isValidType(tree_helper::CGetType(tn));
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
         case last_tree_K:
         case none_K:
         case placeholder_expr_K:
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

   bool isValidInstruction(const tree_nodeConstRef& stmt, const FunctionBehaviorConstRef& FB, const tree_managerConstRef& TM)
   {
      tree_nodeConstRef Type = nullptr;
      switch(GET_CONST_NODE(stmt)->get_kind())
      {
         case gimple_assign_K:
         {
            auto* ga = GetPointer<const gimple_assign>(GET_CONST_NODE(stmt));
            if(tree_helper::CGetType(GET_CONST_NODE(ga->op0))->get_kind() == vector_type_K)
            {
               // Vector arithmetic not yet supported
               return false;
            }
            if(tree_helper::IsLoad(TM, stmt, FB->get_function_mem()))
            {
               Type = tree_helper::CGetType(GET_CONST_NODE(ga->op0));
               break;
            }
            else if(tree_helper::IsStore(TM, stmt, FB->get_function_mem()))
            {
               Type = tree_helper::CGetType(GET_CONST_NODE(ga->op1));
               break;
            }
            Type = tree_helper::CGetType(GET_CONST_NODE(ga->op0));

            switch(GET_CONST_NODE(ga->op1)->get_kind())
            {
               /// cst_node cases
               case integer_cst_K:
               case real_cst_K:
               case string_cst_K:
               case vector_cst_K:
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
               case unge_expr_K:
               case ungt_expr_K:
               case unlt_expr_K:
               case unle_expr_K:
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
               case CASE_QUATERNARY_EXPRESSION:
               case CASE_TYPE_NODES:
               case complex_cst_K:
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
               default:
                  return false;
            }
         }
         break;

         case gimple_phi_K:
         {
            const auto* phi = GetPointer<const gimple_phi>(GET_CONST_NODE(stmt));
            Type = tree_helper::CGetType(GET_CONST_NODE(phi->res));
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

   bool isSignedType(const tree_nodeConstRef& tn)
   {
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
         case tree_reindex_K:
            return isSignedType(GET_CONST_NODE(tn));
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
         case last_tree_K:
         case none_K:
         case placeholder_expr_K:
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

   tree_nodeConstRef getGIMPLE_Type(const tree_nodeConstRef& tn)
   {
      if(tn->get_kind() == tree_reindex_K)
      {
         return getGIMPLE_Type(GET_CONST_NODE(tn));
      }
      if(const auto* gp = GetPointer<const gimple_phi>(tn))
      {
         return getGIMPLE_Type(GET_CONST_NODE(gp->res));
      }
      if(const auto* gr = GetPointer<const gimple_return>(tn))
      {
         return getGIMPLE_Type(GET_CONST_NODE(gr->op));
      }
      return tree_helper::CGetType(tn);
   }

   bw_t getGIMPLE_BW(const tree_nodeConstRef& tn)
   {
      const auto type = getGIMPLE_Type(tn);
      const auto size = BitLatticeManipulator::Size(type);
      THROW_ASSERT(static_cast<bool>(size), "Unhandled type (" + type->get_kind_text() + ") for " + tn->get_kind_text() + " " + tn->ToString());
      return static_cast<bw_t>(size);
   }

   RangeRef getRangeFor(const tree_nodeConstRef& tn, RangeType rType)
   {
      THROW_ASSERT(rType != Anti, "");
      if(tn->get_kind() == tree_reindex_K)
      {
         return getRangeFor(GET_CONST_NODE(tn), rType);
      }

      const auto type = getGIMPLE_Type(tn);
      const auto bw = static_cast<bw_t>(BitLatticeManipulator::Size(type));
      THROW_ASSERT(static_cast<bool>(bw), "Unhandled type (" + type->get_kind_text() + ") for " + tn->get_kind_text() + " " + tn->ToString());

      if(GetPointer<const real_type>(type) != nullptr)
      {
         return RangeRef(new RealRange(RangeRef(new Range(rType, bw))));
      }
      return RangeRef(new Range(rType, bw));
   }

   RangeRef getGIMPLE_range(const tree_nodeConstRef& tn)
   {
      if(tn->get_kind() == tree_reindex_K)
      {
         return getGIMPLE_range(GET_CONST_NODE(tn));
      }

      const auto type = getGIMPLE_Type(tn);
      bw_t bw = static_cast<bw_t>(BitLatticeManipulator::Size(type));
      THROW_ASSERT(static_cast<bool>(bw) || tn->get_kind() == string_cst_K, "Unhandled type (" + type->get_kind_text() + ") for " + tn->get_kind_text() + " " + tn->ToString());
#ifdef BITVALUE_UPDATE
      bool sign = false;
#endif
      APInt min, max;
      if(const auto* ic = GetPointer<const integer_cst>(tn))
      {
         min = max = tree_helper::get_integer_cst_value(ic);
         return RangeRef(new Range(Regular, bw, min, max));
      }
      else if(const auto* rc = GetPointer<const real_cst>(tn))
      {
         THROW_ASSERT(bw == 64 || bw == 32, "Floating point variable with unhandled bitwidth (" + STR(bw) + ")");
         if(rc->valx.front() == '-' && rc->valr.front() != rc->valx.front())
         {
            return RealRange::fromBitValues(string_to_bitstring(convert_fp_to_string("-" + rc->valr, bw)));
         }
         else
         {
            return RealRange::fromBitValues(string_to_bitstring(convert_fp_to_string(rc->valr, bw)));
         }
      }
      else if(const auto* sc = GetPointer<const string_cst>(tn))
      {
         bw = 8;
         RangeRef r(new Range(Regular, bw, 0, 0)); // Zero is the string terminator, thus it should always be included
         for(const auto& c : sc->strg)
         {
            r = r->unionWith(RangeRef(new Range(Regular, bw, c, c)));
         }
         return r;
      }
      else if(const auto* vc = GetPointer<const vector_cst>(tn))
      {
         const auto el_bw = getGIMPLE_BW(vc->list_of_valu.front());
         RangeRef r(new Range(Empty, bw));
         const auto stride = static_cast<size_t>(bw / el_bw);
         const auto strides = vc->list_of_valu.size() / stride;
         THROW_ASSERT(strides * stride == vc->list_of_valu.size(), "");
         for(size_t i = 0; i < strides; ++i)
         {
            auto curr_el = getGIMPLE_range(vc->list_of_valu.at(i * stride));
            for(size_t j = 1; j < stride; ++j)
            {
               curr_el = getGIMPLE_range(vc->list_of_valu.at(i * stride + j))->zextOrTrunc(bw)->shl(RangeRef(new Range(Regular, bw, el_bw * j, el_bw * j)))->Or(curr_el);
            }
            r = r->unionWith(curr_el);
         }
         return r;
      }
      else if(const auto* it = GetPointer<const integer_type>(type))
      {
#ifdef BITVALUE_UPDATE
         sign = !it->unsigned_flag;
#endif
         min = it->unsigned_flag ? APInt::getMinValue(bw) : APInt::getSignedMinValue(bw);
         max = it->unsigned_flag ? APInt::getMaxValue(bw) : APInt::getSignedMaxValue(bw);
      }
      else if(const auto* et = GetPointer<const enumeral_type>(type))
      {
#ifdef BITVALUE_UPDATE
         sign = !et->unsigned_flag;
#endif
         min = et->unsigned_flag ? APInt::getMinValue(bw) : APInt::getSignedMinValue(bw);
         max = et->unsigned_flag ? APInt::getMaxValue(bw) : APInt::getSignedMaxValue(bw);
      }
      else if(type->get_kind() == boolean_type_K)
      {
         min = 0;
         max = 1;
         bw = 1;
      }
      else if(GetPointer<const real_type>(type) != nullptr)
      {
         if(const auto* ssa = GetPointer<const ssa_name>(tn))
         {
            auto bv = string_to_bitstring(ssa->bit_values);
            if(bv.size() < bw)
            {
               bv = BitLatticeManipulator::sign_extend_bitstring(bv, false, bw);
            }
            return RealRange::fromBitValues(bv);
         }
         if(bw == 32)
         {
            return RangeRef(new RealRange(Range(Regular, 1), Range(Regular, 8), Range(Regular, 23)));
         }
         if(bw == 64)
         {
            return RangeRef(new RealRange(Range(Regular, 1), Range(Regular, 11), Range(Regular, 52)));
         }
         THROW_UNREACHABLE("Floating point variable with unhandled bitwidth (" + STR(bw) + ")");
      }
#ifdef INTEGER_PTR
      else if(GetPointer<const pointer_type>(type) != nullptr)
      {
         min = APInt::getMinValue(bw);
         max = APInt::getMaxValue(bw);
      }
#endif
      else if(type->get_kind() == vector_type_K || type->get_kind() == array_type_K)
      {
         bw = static_cast<bw_t>(BitLatticeManipulator::Size(tree_helper::CGetElements(type)));
         return RangeRef(new Range(Regular, bw));
      }
      else if(const auto* rt = GetPointer<const record_type>(type))
      {
         THROW_ASSERT(GetPointer<const integer_cst>(GET_CONST_NODE(rt->size)), "record_type has no size");
         bw = static_cast<bw_t>(tree_helper::get_integer_cst_value(GetPointer<const integer_cst>(GET_CONST_NODE(rt->size))));
         THROW_ASSERT(bw, "Invalid bitwidth");
         return RangeRef(new Range(Regular, bw));
      }
      else
      {
         THROW_UNREACHABLE("Unable to define range for type " + type->get_kind_text() + " of " + tn->ToString());
      }

#ifdef BITVALUE_UPDATE
      if(const auto* ssa = GetPointer<const ssa_name>(tn))
      {
         if(!ssa->bit_values.empty())
         {
            const auto bvSize = static_cast<bw_t>(ssa->bit_values.size());
            const auto bvRange = Range::fromBitValues(string_to_bitstring(ssa->bit_values), bvSize, sign);
            const auto varRange = RangeRef(new Range(Regular, bw, min, max))->truncate(bvSize)->intersectWith(bvRange);
            return sign ? varRange->sextOrTrunc(bw) : varRange->zextOrTrunc(bw);
         }
      }
#endif
      return RangeRef(new Range(Regular, bw, min, max));
   }
} // namespace

// ========================================================================== //
// VarNode
// ========================================================================== //
enum updateType
{
   ut_None = 0,
   ut_Range = 1,
   ut_BitValue = 2,
   ut_Constant = 4
};

class VarNode
{
   /// The program variable
   const tree_nodeConstRef V;
   /// ID of the associated function
   unsigned int function_id;
   /// A Range associated to the variable, that is,
   /// its interval inferred by the analysis.
   RangeConstRef interval;

   /// Used by the crop meet operator
   char abstractState;

 public:
   explicit VarNode(const tree_nodeConstRef& _V, unsigned int _function_id);
   ~VarNode() = default;
   VarNode(const VarNode&) = delete;
   VarNode(VarNode&&) = delete;
   VarNode& operator=(const VarNode&) = delete;
   VarNode& operator=(VarNode&&) = delete;

   /// Initializes the value of the node.
   void init(bool outside);
   /// Returns the range of the variable represented by this node.
   RangeConstRef getRange() const
   {
      return interval;
   }
   /// Returns the variable represented by this node.
   const tree_nodeConstRef& getValue() const
   {
      return V;
   }
   unsigned int getFunctionId() const
   {
      return function_id;
   }
   bw_t getBitWidth() const
   {
      return interval->getBitWidth();
   }

   /// Changes the status of the variable represented by this node.
   void setRange(const RangeConstRef& newInterval)
   {
      interval.reset(newInterval->clone());
   }

   RangeRef getMaxRange() const
   {
      return getRangeFor(V, Regular);
   }

   char getAbstractState()
   {
      return abstractState;
   }
   // The possible states are '0', '+', '-' and '?'.
   void storeAbstractState();

   int updateIR(const tree_managerRef& TM, const tree_manipulationRef& tree_man, const DesignFlowManagerConstRef& design_flow_manager
#ifndef NDEBUG
                ,
                int debug_level, application_managerRef AppM
#endif
   );

   /// Pretty print.
   void print(std::ostream& OS) const;
   std::string ToString() const;
};

/// The ctor.
VarNode::VarNode(const tree_nodeConstRef& _V, unsigned int _function_id) : V(_V), function_id(_function_id), abstractState(0)
{
   THROW_ASSERT(_V != nullptr, "Variable cannot be null");
   THROW_ASSERT(_V->get_kind() == tree_reindex_K, "Variable should be a tree_reindex node");
   interval = getRangeFor(_V, Unknown);
}

/// Initializes the value of the node.
void VarNode::init(bool outside)
{
   THROW_ASSERT(getGIMPLE_BW(V), "Bitwidth not valid");
   THROW_ASSERT(interval, "Interval should be initialized during VarNode construction");
   if(interval->isUnknown()) // Ranges already initialized come from user defined hints and shouldn't be overwritten
   {
      if(GetPointer<const cst_node>(GET_CONST_NODE(V)) != nullptr)
      {
         interval = getGIMPLE_range(V);
      }
      else
      {
         interval = getRangeFor(V, outside ? Regular : Unknown);
      }
   }
}

int VarNode::updateIR(const tree_managerRef& TM, const tree_manipulationRef& tree_man, const DesignFlowManagerConstRef& design_flow_manager
#ifndef NDEBUG
                      ,
                      int debug_level, application_managerRef AppM
#endif
)
{
   const auto ssa_node = TM->GetTreeReindex(GET_INDEX_CONST_NODE(V));
   auto* SSA = GetPointer<ssa_name>(GET_NODE(ssa_node));
   if(SSA == nullptr || interval->isUnknown())
   {
      return ut_None;
   }

#ifdef BITVALUE_UPDATE
   auto isBetter = [](const std::deque<bit_lattice>& a, const std::deque<bit_lattice>& b) {
      auto a_it = a.crbegin();
      auto b_it = b.crbegin();
      const auto a_end = a.crend();
      const auto b_end = b.crend();
      for(; a_it != a_end && b_it != b_end; a_it++, b_it++)
      {
         if((*b_it == bit_lattice::U && *a_it != bit_lattice::U) || (*b_it != bit_lattice::X && *a_it == bit_lattice::X))
         {
            return true;
         }
      }
      return false;
   };

   auto updateBitValue = [&](ssa_name* ssa, const std::deque<bit_lattice>& bv) -> int {
      const auto curr_bv = string_to_bitstring(ssa->bit_values);
      if(isBetter(bv, curr_bv))
      {
         ssa->bit_values = bitstring_to_string(bv);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "BitValue updated for " + ssa->ToString() + " " + GET_CONST_NODE(ssa->type)->get_kind_text() + ": " + SSA->bit_values + " <= " + bitstring_to_string(curr_bv));
         return ut_BitValue;
      }
      return ut_None;
   };
#endif

   const bool isSigned = isSignedType(SSA->type);
   if(SSA->range != nullptr)
   {
      if(SSA->range->isSameRange(interval))
      {
         return ut_None;
      }
#ifndef NDEBUG
      if(not AppM->ApplyNewTransformation())
      {
         return ut_None;
      }
#endif
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Modified range " + SSA->range->ToString() + " to " + interval->ToString() + " for " + SSA->ToString() + " " + GET_CONST_NODE(SSA->type)->get_kind_text());
   }
   else
   {
      bw_t newBW = interval->getBitWidth();
      if(interval->isFullSet())
      {
         return ut_None;
      }
      if(interval->isConstant())
      {
         newBW = 0U;
      }
      else if(!interval->isReal())
      {
         if(interval->isRegular())
         {
            newBW = isSigned ? Range::neededBits(interval->getSignedMin(), interval->getSignedMax(), true) : Range::neededBits(interval->getUnsignedMin(), interval->getUnsignedMax(), false);
            const auto currentBW = getGIMPLE_BW(V);
            if(newBW >= currentBW)
            {
               return ut_None;
            }
         }
         else if(interval->isAnti())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Anti range " + interval->ToString() + " not stored for " + SSA->ToString() + " " + GET_CONST_NODE(SSA->type)->get_kind_text());
            return ut_None;
         }
         else if(interval->isEmpty())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Empty range not stored for " + SSA->ToString() + " " + GET_CONST_NODE(SSA->type)->get_kind_text());
            return ut_None;
         }
      }
#ifndef NDEBUG
      if(not AppM->ApplyNewTransformation())
      {
         return ut_None;
      }
#endif
      //    const auto hasBetterSuper = [&]() {
      //       if(SSA->min && SSA->max)
      //       {
      //          RangeRef superRange(
      //              new Range(Regular, interval->getBitWidth(), tree_helper::get_integer_cst_value(GetPointer<const integer_cst>(GET_CONST_NODE(SSA->min))), tree_helper::get_integer_cst_value(GetPointer<const integer_cst>(GET_CONST_NODE(SSA->max)))));
      //          if(superRange->isRegular())
      //          {
      //             // Intersect with computed range, because range computed from LLVM range analysis may not be valid any more
      //             superRange = superRange->intersectWith(interval);
      //             if(superRange->isRegular() && superRange->getSpan() < interval->getSpan())
      //             {
      //                const auto superBW = isSigned ? Range::neededBits(superRange->getSignedMin(), superRange->getSignedMax(), true) : Range::neededBits(superRange->getUnsignedMin(), superRange->getUnsignedMax(), false);
      //                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
      //                               "Current range " + superRange->ToString() + "<" + STR(superBW) + ">" + " was better than computed range " + interval->ToString() + "<" + STR(newBW) + "> for " + SSA->ToString() + " " +
      //                                   GET_CONST_NODE(SSA->type)->get_kind_text() + "<" + SSA->bit_values + ">");
      //                interval = superRange;
      //                newBW = superBW;
      //                return true;
      //             }
      //          }
      //       }
      //       return false;
      //    }();

      //    if(!hasBetterSuper)
      //    {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Added range " + interval->ToString() + "<" + STR(newBW) + "> for " + SSA->ToString() + " " + GET_CONST_NODE(SSA->type)->get_kind_text());
      //    }
   }

   auto getConstNode = [&](const RangeConstRef& range) {
      const auto type_node = TM->GetTreeReindex(tree_helper::get_type_index(TM, SSA->index));
      if(range->isReal())
      {
         const auto rRange = RefcountCast<const RealRange>(range);
         if(rRange->getBitWidth() == 32)
         {
            union vcFloat vc;
#if __BYTE_ORDER == __BIG_ENDIAN
            vc.bits.sign = rRange->getSign()->getUnsignedMax().cast_to<bool>();
            vc.bits.exp = rRange->getExponent()->getUnsignedMax().cast_to<uint8_t>();
            vc.bits.frac = rRange->getSignificand()->getUnsignedMax().cast_to<uint32_t>();
#else
            vc.bits.coded = rRange->getSign()->getUnsignedMax().cast_to<uint32_t>() << 31;
            vc.bits.coded += rRange->getExponent()->getUnsignedMax().cast_to<uint32_t>() << 23;
            vc.bits.coded += rRange->getSignificand()->getUnsignedMax().cast_to<uint32_t>();
#endif
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Floating point constant from range is " + STR(vc.flt));
            return tree_man->CreateRealCst(type_node, static_cast<long double>(vc.flt), TM->new_tree_node_id());
         }
         else
         {
            union vcDouble vc;
#if __BYTE_ORDER == __BIG_ENDIAN
            vc.bits.sign = rRange->getSign()->getUnsignedMax().cast_to<bool>();
            vc.bits.exp = rRange->getExponent()->getUnsignedMax().cast_to<uint16_t>();
            vc.bits.frac = rRange->getSignificand()->getUnsignedMax().cast_to<uint64_t>();
#else
            vc.bits.coded = rRange->getSign()->getUnsignedMax().cast_to<uint64_t>() << 63;
            vc.bits.coded += rRange->getExponent()->getUnsignedMax().cast_to<uint64_t>() << 52;
            vc.bits.coded += rRange->getSignificand()->getUnsignedMax().cast_to<uint64_t>();
#endif
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Double precision constant from range is " + STR(vc.dub));
            return tree_man->CreateRealCst(type_node, static_cast<long double>(vc.dub), TM->new_tree_node_id());
         }
      }
      else
      {
         const auto cst_value = isSigned ? range->getSignedMax().cast_to<int64_t>() : static_cast<int64_t>(range->getUnsignedMax().cast_to<uint64_t>());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + (isSigned ? ("Signed int " + STR(cst_value)) : ("Unsigned int " + STR(static_cast<uint64_t>(cst_value)))));
         return tree_man->CreateIntegerCst(type_node, cst_value, TM->new_tree_node_id());
      }
   };

   int updateState = ut_None;
   auto bit_values = string_to_bitstring(SSA->bit_values);
   if(interval->isConstant())
   {
      const auto cst_node = getConstNode(interval);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Replacing variable with " + GET_CONST_NODE(cst_node)->ToString());
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      const auto useStmts = SSA->CGetUseStmts();
      for(const auto& use : useStmts)
      {
#ifndef NDEBUG
         if(not AppM->ApplyNewTransformation())
         {
            break;
         }
         auto dbg_conversion = GET_CONST_NODE(use.first)->ToString() + " -> ";
#endif
         tree_nodeRef lhs = nullptr;
         if(const auto* ga = GetPointer<const gimple_assign>(GET_CONST_NODE(use.first)))
         {
            lhs = ga->op0;
         }
         else if(const auto* gp = GetPointer<const gimple_phi>(GET_CONST_NODE(use.first)))
         {
            lhs = gp->res;
         }
         if(lhs != nullptr && static_cast<bool>(tree_helper::ComputeSsaUses(lhs).count(ssa_node)))
         {
            // TODO: maybe remove statement with constant variable on lhs
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, dbg_conversion + "Left hand side variable can't be replaced by a constant");
            continue;
         }

         TM->ReplaceTreeNode(use.first, ssa_node, cst_node);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, dbg_conversion + GET_CONST_NODE(use.first)->ToString());

#ifndef NDEBUG
         AppM->RegisterTransformation("RangeAnalysis", use.first);
#endif
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

#ifndef NDEBUG
      if(AppM->ApplyNewTransformation())
#endif
         if(SSA->CGetUseStmts().empty())
            if(const auto def = SSA->CGetDefStmt())
            {
               if(const auto* ga = GetPointer<const gimple_assign>(GET_CONST_NODE(def)))
               {
                  // Leave call_expr and aggr_init_expr removal to dead_code_elimination step
                  if(GET_NODE(ga->op1)->get_kind() != call_expr_K && GET_NODE(ga->op1)->get_kind() != aggr_init_expr_K)
                  {
                     const auto curr_tn = TM->get_tree_node_const(function_id);
                     const auto* fd = GetPointer<const function_decl>(curr_tn);
                     const auto* sl = GetPointer<const statement_list>(GET_CONST_NODE(fd->body));
                     THROW_ASSERT(sl->list_of_bloc.count(ga->bb_index), "BB" + STR(ga->bb_index) + " not found in funciton " + STR(function_id));
                     auto bb = sl->list_of_bloc.at(ga->bb_index);
                     bb->RemoveStmt(def);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Removed definition " + ga->ToString());
                     dead_code_elimination::fix_sdc_motion(design_flow_manager, function_id, def);
#ifndef NDEBUG
                     AppM->RegisterTransformation("RangeAnalysis", def);
#endif
                  }
               }
               else if(const auto* gp = GetPointer<const gimple_phi>(GET_CONST_NODE(def)))
               {
                  const auto curr_tn = TM->get_tree_node_const(function_id);
                  const auto* fd = GetPointer<const function_decl>(curr_tn);
                  const auto* sl = GetPointer<const statement_list>(GET_CONST_NODE(fd->body));
                  THROW_ASSERT(sl->list_of_bloc.count(gp->bb_index), "BB" + STR(gp->bb_index) + " not found in funciton " + STR(function_id));
                  auto bb = sl->list_of_bloc.at(gp->bb_index);
                  bb->RemovePhi(def);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Removed definition " + gp->ToString());
#ifndef NDEBUG
                  AppM->RegisterTransformation("RangeAnalysis", def);
#endif
               }
            }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

      updateState = ut_Constant;
#ifdef BITVALUE_UPDATE
      bit_values = interval->getBitValues(isSigned);
#endif
   }
   else if(interval->isReal())
   {
   }
   else if(interval->isAnti() || interval->isEmpty())
   {
      updateState = ut_Range;
      THROW_ASSERT(SSA->range, "");
      if(SSA->range->isRegular())
      {
         SSA->max.reset();
         SSA->min.reset();
      }
   }
   else if(interval->isFullSet())
   {
      updateState = ut_Range | ut_BitValue;
      SSA->max.reset();
      SSA->min.reset();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Min: reset   Max: reset");
#ifdef BITVALUE_UPDATE
      bit_values = interval->getBitValues(isSigned);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---BitValue updated for " + SSA->ToString() + " " + GET_CONST_NODE(SSA->type)->get_kind_text() + ": " + bitstring_to_string(bit_values) + " <= " + SSA->bit_values);
      SSA->bit_values = bitstring_to_string(bit_values);
#endif
   }
   else
   {
      updateState = ut_Range;
      const auto type_id = GET_INDEX_CONST_NODE(SSA->type);
      long long min, max;
      if(isSigned)
      {
         min = interval->getSignedMin().cast_to<long long>();
         max = interval->getSignedMax().cast_to<long long>();
      }
      else
      {
         min = interval->getUnsignedMin().cast_to<long long>();
         max = interval->getUnsignedMax().cast_to<long long>();
      }
      SSA->min = TM->CreateUniqueIntegerCst(min, type_id);
      SSA->max = TM->CreateUniqueIntegerCst(max, type_id);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Min: " + GET_CONST_NODE(SSA->min)->ToString() + "   Max: " + GET_CONST_NODE(SSA->max)->ToString());

#ifdef BITVALUE_UPDATE
      if(not bit_values.empty())
      {
         auto range_bv = interval->getBitValues(isSigned);
         while(range_bv.size() > bit_values.size())
         {
            range_bv.pop_front();
         }
         const auto sup_bv = BitLatticeManipulator::sup(bit_values, range_bv, interval->getBitWidth(), isSigned, interval->getBitWidth() == 1);
         //    THROW_ASSERT(std::count(bit_values.begin(), bit_values.end(), bit_lattice::X) >= std::count(sup_bv.begin(), sup_bv.end(), bit_lattice::X), "Don't care should not be generated here (" + bitstring_to_string(bit_values) + " sup " +
         //    bitstring_to_string(range_bv) +" = " + bitstring_to_string(sup_bv) + ")");
         if(bit_values != sup_bv)
         {
            bit_values = sup_bv;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Range bit_values: " + bitstring_to_string(range_bv));
         }
      }
      else
      {
         // TODO: could initialize bit_values here, but not sure
      }
#endif
   }

#ifndef NDEBUG
   if(not AppM->ApplyNewTransformation())
   {
      return ut_None;
   }
#endif

   SSA->range = RangeRef(interval->clone());
#ifdef BITVALUE_UPDATE
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   updateState |= updateBitValue(SSA, bit_values);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
#endif

#ifndef NDEBUG
   AppM->RegisterTransformation("RangeAnalysis", V);
#endif

   if(const auto* gp = GetPointer<const gimple_phi>(GET_NODE(SSA->CGetDefStmt())))
   {
      if(gp->CGetDefEdgesList().size() == 1)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Sigma defined variable not considered for invalidation loop...");
         return ut_None;
      }
   }
   return updateState;
}

/// Pretty print.
void VarNode::print(std::ostream& OS) const
{
   if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V)))
   {
      OS << C->value;
   }
   else
   {
      printVarName(V, OS);
   }
   OS << " ";
   this->getRange()->print(OS);
}

std::string VarNode::ToString() const
{
   std::stringstream ss;
   print(ss);
   return ss.str();
}

void VarNode::storeAbstractState()
{
   THROW_ASSERT(!this->interval->isUnknown(), "storeAbstractState doesn't handle empty set");

   if(this->interval->getLower() == Range::Min)
   {
      if(this->interval->getUpper() == Range::Max)
      {
         this->abstractState = '?';
      }
      else
      {
         this->abstractState = '-';
      }
   }
   else if(this->interval->getUpper() == Range::Max)
   {
      this->abstractState = '+';
   }
   else
   {
      this->abstractState = '0';
   }
}

std::ostream& operator<<(std::ostream& OS, const VarNode* VN)
{
   VN->print(OS);
   return OS;
}

// ========================================================================== //
// ValueRange
// ========================================================================== //
enum ValueRangeType
{
   ValueRangeId,
   SymbRangeId
};

REF_FORWARD_DECL(ValueRange);
CONSTREF_FORWARD_DECL(ValueRange);

template <class T>
inline T* GetVR(const ValueRange* t)
{
   return T::classof(t) ? static_cast<T*>(t) : nullptr;
}

class ValueRange
{
 private:
   RangeConstRef range;

 public:
   explicit ValueRange(const RangeConstRef& range);
   virtual ~ValueRange() = default;
   ValueRange(const ValueRange&) = delete;
   ValueRange(ValueRange&&) = delete;
   ValueRange& operator=(const ValueRange&) = delete;
   ValueRange& operator=(ValueRange&&) = delete;

   // Methods for RTTI
   virtual ValueRangeType getValueId() const
   {
      return ValueRangeId;
   }
   static bool classof(ValueRange const* /*unused*/)
   {
      return true;
   }

   /// Returns the range of this interval.
   RangeConstRef getRange() const
   {
      return this->range;
   }
   /// Sets the range of this interval to another range.
   void setRange(const RangeConstRef& newRange)
   {
      THROW_ASSERT(this->range->isReal() == newRange->isReal(), "New range must have same type of previous range");
      this->range.reset(newRange->clone());
   }

   /// Pretty print.
   virtual void print(std::ostream& OS) const;
   std::string ToString() const;
};

ValueRange::ValueRange(const RangeConstRef& _range) : range(_range->clone())
{
}

/// Pretty print.
void ValueRange::print(std::ostream& OS) const
{
   this->getRange()->print(OS);
}

std::string ValueRange::ToString() const
{
   std::stringstream ss;
   print(ss);
   return ss.str();
}

std::ostream& operator<<(std::ostream& OS, const ValueRange* BI)
{
   BI->print(OS);
   return OS;
}

// ========================================================================== //
// SymbRange
// ========================================================================== //

/// This is an interval that contains a symbolic limit, which is
/// given by the bounds of a program name, e.g.: [-inf, ub(b) + 1].
class SymbRange : public ValueRange
{
 private:
   /// The bound. It is a node which limits the interval of this range.
   const tree_nodeConstRef bound;
   /// The predicate of the operation in which this interval takes part.
   /// It is useful to know how we can constrain this interval
   /// after we fix the intersects.
   kind pred;

 public:
   SymbRange(const RangeConstRef& range, const tree_nodeConstRef& bound, kind pred);
   ~SymbRange() = default;
   SymbRange(const SymbRange&) = delete;
   SymbRange(SymbRange&&) = delete;
   SymbRange& operator=(const SymbRange&) = delete;
   SymbRange& operator=(SymbRange&&) = delete;

   // Methods for RTTI
   ValueRangeType getValueId() const override
   {
      return SymbRangeId;
   }
   static bool classof(SymbRange const* /*unused*/)
   {
      return true;
   }
   static bool classof(ValueRange const* BI)
   {
      return BI->getValueId() == SymbRangeId;
   }

   /// Returns the opcode of the operation that create this interval.
   kind getOperation() const
   {
      return this->pred;
   }
   /// Returns the node which is the bound of this interval.
   const tree_nodeConstRef& getBound() const
   {
      return this->bound;
   }
   /// Replace symbolic bound with hard-wired constants.
   RangeConstRef solveFuture(const VarNode* bound, const VarNode* sink) const;

   /// Prints the content of the interval.
   void print(std::ostream& OS) const override;
};

SymbRange::SymbRange(const RangeConstRef& _range, const tree_nodeConstRef& _bound, kind _pred) : ValueRange(_range), bound(_bound), pred(_pred)
{
}

RangeConstRef SymbRange::solveFuture(const VarNode* _bound, const VarNode* _sink) const
{
   // Get the lower and the upper bound of the
   // node which bounds this intersection.
   const auto boundRange = _bound->getRange();
   const auto sinkRange = _sink->getRange();
   THROW_ASSERT(!boundRange->isEmpty(), "Bound range should not be empty");
   THROW_ASSERT(!sinkRange->isEmpty(), "Sink range should not be empty");
   THROW_ASSERT(boundRange->isReal() == sinkRange->isReal(), "Range type mismatch");

   if(boundRange->isReal())
   {
      const auto* rr = static_cast<const RealRange*>(boundRange.get());
      if(this->getOperation() == eq_expr_K && !rr->getExponent()->isAnti() && !rr->getSignificand()->isAnti())
      {
         return RangeRef(boundRange->clone());
      }
      return getRangeFor(_sink->getValue(), Regular);
   }

   auto IsAnti = boundRange->isAnti() || sinkRange->isAnti();
   const auto l = IsAnti ? (boundRange->isUnknown() ? Range::Min : boundRange->getUnsignedMin()) : boundRange->getLower();
   const auto u = IsAnti ? (boundRange->isUnknown() ? Range::Max : boundRange->getUnsignedMax()) : boundRange->getUpper();

   // Get the lower and upper bound of the interval of this operation
   const auto lower = IsAnti ? (sinkRange->isUnknown() ? Range::Min : sinkRange->getUnsignedMin()) : sinkRange->getLower();
   const auto upper = IsAnti ? (sinkRange->isUnknown() ? Range::Max : sinkRange->getUnsignedMax()) : sinkRange->getUpper();

   const auto bw = getRange()->getBitWidth();
   switch(this->getOperation())
   {
      case uneq_expr_K:
      case eq_expr_K: // equal
         return RangeRef(new Range(Regular, bw, l, u));
      case le_expr_K: // signed less or equal
         if(lower > u)
         {
            return RangeRef(new Range(Empty, bw));
         }
         else
         {
            return RangeRef(new Range(Regular, bw, lower, u));
         }
      case lt_expr_K: // signed less than
         if(u != Range::Max && u != APInt::getSignedMaxValue(bw))
         {
            if(lower > (u - 1))
            {
               return RangeRef(new Range(Empty, bw));
            }

            return RangeRef(new Range(Regular, bw, lower, u - 1));
         }
         else
         {
            if(lower > u)
            {
               return RangeRef(new Range(Empty, bw));
            }

            return RangeRef(new Range(Regular, bw, lower, u));
         }
      case ge_expr_K: // signed greater or equal
         if(l > upper)
         {
            return RangeRef(new Range(Empty, bw));
         }
         else
         {
            return RangeRef(new Range(Regular, bw, l, upper));
         }
      case gt_expr_K: // signed greater than
         if(l != Range::Min && l != APInt::getSignedMinValue(bw))
         {
            if((l + 1) > upper)
            {
               return RangeRef(new Range(Empty, bw));
            }

            return RangeRef(new Range(Regular, bw, l + 1, upper));
         }
         else
         {
            if(l > upper)
            {
               return RangeRef(new Range(Empty, bw));
            }

            return RangeRef(new Range(Regular, bw, l, upper));
         }
      case ne_expr_K:
      case unge_expr_K:
      case ungt_expr_K:
      case unle_expr_K:
      case unlt_expr_K:
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
   return getRangeFor(_sink->getValue(), Regular);
}

/// Pretty print.
void SymbRange::print(std::ostream& OS) const
{
   const auto bnd = getBound();
   switch(this->getOperation())
   {
      case uneq_expr_K:
      case eq_expr_K: // equal
         OS << "[lb(";
         printVarName(bnd, OS);
         OS << "), ub(";
         printVarName(bnd, OS);
         OS << ")]";
         break;
      case unle_expr_K:
         OS << "[0, ub(";
         printVarName(bnd, OS);
         OS << ")]";
         break;
      case le_expr_K: // sign less or equal
         OS << "[-inf, ub(";
         printVarName(bnd, OS);
         OS << ")]";
         break;
      case unlt_expr_K:
         OS << "[0, ub(";
         printVarName(bnd, OS);
         OS << ") - 1]";
         break;
      case lt_expr_K: // sign less than
         OS << "[-inf, ub(";
         printVarName(bnd, OS);
         OS << ") - 1]";
         break;
      case unge_expr_K:
      case ge_expr_K: // sign greater or equal
         OS << "[lb(";
         printVarName(bnd, OS);
         OS << "), +inf]";
         break;
      case ungt_expr_K:
      case gt_expr_K: // sign greater than
         OS << "[lb(";
         printVarName(bnd, OS);
         OS << " - 1), +inf]";
         break;
      case ne_expr_K:
         OS << ")b(";
         printVarName(bnd, OS);
         OS << ")(";
         break;
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
         OS << "Unknown Instruction.";
   }
}

// ========================================================================== //
// ConditionalValueRange
// ========================================================================== //
class ConditionalValueRange
{
 private:
   const tree_nodeConstRef V;
   std::map<unsigned int, ValueRangeRef> bbVR;

 public:
   ConditionalValueRange(const tree_nodeConstRef& _V, const std::map<unsigned int, ValueRangeRef>& _bbVR) : V(_V), bbVR(_bbVR)
   {
   }
   ConditionalValueRange(const tree_nodeConstRef& _V, unsigned int TrueBBI, unsigned int FalseBBI, const ValueRangeRef& TrueVR, const ValueRangeRef& FalseVR) : V(_V), bbVR({{FalseBBI, FalseVR}, {TrueBBI, TrueVR}})
   {
   }
   ~ConditionalValueRange() = default;
   ConditionalValueRange(const ConditionalValueRange&) = default;
   ConditionalValueRange(ConditionalValueRange&&) = default;

   /// Get the interval associated to the switch case idx
   const std::map<unsigned int, ValueRangeRef>& getVR() const
   {
      return bbVR;
   }
   /// Get the value associated to the switch.
   const tree_nodeConstRef& getVar() const
   {
      return V;
   }
   /// Add an interval associated to a new basic block
   void addVR(unsigned int bbi, const ValueRangeRef& cvr)
   {
      if(!static_cast<bool>(bbVR.count(bbi)))
      {
         bbVR.insert(std::make_pair(bbi, cvr));
      }
      // TODO: maybe find some way to combine two ValueRange instances (difficult because of symbolic ranges)
   }
};

using ConditionalValueRanges = std::map<tree_nodeConstRef, ConditionalValueRange, tree_reindexCompare>;

// ========================================================================== //
// OpNode
// ========================================================================== //

class OpNode;
template <typename T>
inline T* GetOp(OpNode* t)
{
   return T::classof(t) ? static_cast<T*>(t) : nullptr;
}
template <typename T>
inline const T* GetOp(const OpNode* t)
{
   return T::classof(t) ? static_cast<const T*>(t) : nullptr;
}

/// This class represents a generic operation in our analysis.
class OpNode
{
 private:
   /// The range of the operation. Each operation has a range associated to it.
   /// This range is obtained by inspecting the branches in the source program
   /// and extracting its condition and intervals.
   ValueRangeRef intersect;
   // The target of the operation, that is, the node which
   // will store the result of the operation.
   VarNode* sink;
   // The instruction that originated this op node
   const tree_nodeConstRef inst;

 protected:
   /// We do not want people creating objects of this class,
   /// but we want to inherit from it.
   OpNode(const ValueRangeRef& intersect, VarNode* sink, const tree_nodeConstRef& inst);

 public:
   enum class OperationId
   {
      UnaryOpId,
      SigmaOpId,
      BinaryOpId,
      TernaryOpId,
      PhiOpId,
      ControlDepId,
      LoadOpId,
      StoreOpId
   };

#ifndef NDEBUG
   static int debug_level;
#endif

   /// The dtor. It's virtual because this is a base class.
   virtual ~OpNode() = default;
   // We do not want people creating objects of this class.
   OpNode(const OpNode&) = delete;
   OpNode(OpNode&&) = delete;
   OpNode& operator=(const OpNode&) = delete;
   OpNode& operator=(OpNode&&) = delete;

   // Methods for RTTI
   virtual OperationId getValueId() const = 0;
   static bool classof(OpNode const* /*unused*/)
   {
      return true;
   }

   /// Given the input of the operation and the operation that will be
   /// performed, evaluates the result of the operation.
   virtual RangeRef eval() const = 0;
   /// Return the instruction that originated this op node
   const tree_nodeConstRef& getInstruction() const
   {
      return inst;
   }
   /// Replace symbolic intervals with hard-wired constants.
   void solveFuture(VarNode* future);
   /// Returns the range of the operation.
   ValueRangeConstRef getIntersect() const
   {
      return intersect;
   }
   /// Changes the interval of the operation.
   void setIntersect(const RangeConstRef& newIntersect)
   {
      this->intersect->setRange(newIntersect);
   }
   /// Returns the target of the operation, that is,
   /// where the result will be stored.
   VarNode* getSink() const
   {
      return sink;
   }

   virtual std::vector<tree_nodeConstRef> getSources() const = 0;

   /// Prints the content of the operation.
   virtual void print(std::ostream& OS) const = 0;
   virtual void printDot(std::ostream& OS) const = 0;
   std::string ToString() const;
};

#ifndef NDEBUG
int OpNode::debug_level = DEBUG_LEVEL_NONE;
#endif

/// We can not want people creating objects of this class,
/// but we want to inherit of it.
OpNode::OpNode(const ValueRangeRef& _intersect, VarNode* _sink, const tree_nodeConstRef& _inst) : intersect(_intersect), sink(_sink), inst(_inst)
{
}

void OpNode::solveFuture(VarNode* future)
{
   if(const auto SI = RefcountCast<const SymbRange>(getIntersect()))
   {
      this->setIntersect(SI->solveFuture(future, getSink()));
   }
}

std::string OpNode::ToString() const
{
   std::stringstream ss;
   print(ss);
   return ss.str();
}

// ========================================================================== //
// NodeContainer
// ========================================================================== //

struct UnpackSelector
{
   enum PackPos
   {
      packPos_Undefined,
      packPos_Sign32,
      packPos_Exp32,
      packPos_Sigf32,
      packPos_Sign64,
      packPos_Exp64,
      packPos_Sigf64
   };

   uint64_t mask;
   uint8_t rshift;

   UnpackSelector(uint64_t m = 0ULL, uint8_t rs = 0U) : mask(m), rshift(rs)
   {
   }

   friend UnpackSelector operator&(const UnpackSelector& a, const uint64_t& m)
   {
      return UnpackSelector(a.mask & (m << a.rshift), a.rshift);
   }

   friend UnpackSelector operator>>(const UnpackSelector& a, const uint8_t& s)
   {
      const auto rs = static_cast<uint8_t>(a.rshift + s);
      return UnpackSelector((a.mask >> rs) << rs, rs);
   }

   PackPos getSelector() const
   {
      if(mask == 4294967296U && rshift == 31U)
      {
         return packPos_Sign32;
      }
      else if(mask == 2139095040U && rshift == 23U)
      {
         return packPos_Exp32;
      }
      else if(mask == 8388607U && !rshift)
      {
         return packPos_Sigf32;
      }
      else if(mask == 9223372036854775808ULL && rshift == 63U)
      {
         return packPos_Sign64;
      }
      else if(mask == 9218868437227405312ULL && rshift == 52U)
      {
         return packPos_Exp64;
      }
      else if(mask == 4503599627370495ULL && !rshift)
      {
         return packPos_Sigf64;
      }
      return packPos_Undefined;
   }
};

// The VarNodes type.
using VarNodes = std::map<tree_nodeConstRef, VarNode*, tree_reindexCompare>;
// The Operations type.
using OpNodes = CustomSet<OpNode*>;
// A map from varnodes to the operation in which this variable is defined
using DefMap = std::map<tree_nodeConstRef, OpNode*, tree_reindexCompare>;
// A map from variables to the operations where these variables are used.
using UseMap = std::map<tree_nodeConstRef, CustomSet<OpNode*>, tree_reindexCompare>;
// Map varnodes to their view_converted float ancestor if any; pair contains original float variable and view_convert mask
using VCMap = CustomMap<VarNode*, std::pair<VarNode*, UnpackSelector>>;

class NodeContainer
{
 private:
   static const std::vector<std::function<std::function<OpNode*(NodeContainer*)>(const tree_nodeConstRef&, unsigned int, const FunctionBehaviorConstRef&, const tree_managerConstRef&, const application_managerRef&)>> _opCtorGenerators;

   VarNodes _varNodes;

   OpNodes _opNodes;

   DefMap _defMap;

   UseMap _useMap;

   ConditionalValueRanges _cvrMap;

   VCMap _vcMap;

 protected:
   UseMap& getUses()
   {
      return _useMap;
   }

 public:
   virtual ~NodeContainer()
   {
      for(auto varNode : _varNodes)
      {
         delete varNode.second;
      }
      for(auto op : _opNodes)
      {
         delete op;
      }
   }

   const VarNodes& getVarNodes() const
   {
      return _varNodes;
   }

   VarNode* addVarNode(const tree_nodeConstRef& V, unsigned int function_id)
   {
      THROW_ASSERT(V, "Can't insert nullptr as variable");
      auto vit = _varNodes.find(V);
      if(vit != _varNodes.end())
      {
         return vit->second;
      }

      auto* node = new VarNode(V, function_id);
      _varNodes.insert(std::make_pair(V, node));

      // Inserts the node in the use map list.
      CustomSet<OpNode*> useList;
      _useMap.insert(std::make_pair(V, useList));
      return node;
   }

   const ConditionalValueRanges& getCVR() const
   {
      return _cvrMap;
   }

   void addConditionalValueRange(const ConditionalValueRange&& cvr)
   {
      auto cvrIt = _cvrMap.find(cvr.getVar());
      if(cvrIt != _cvrMap.end())
      {
         for(const auto& BBIvr : cvr.getVR())
         {
            cvrIt->second.addVR(BBIvr.first, BBIvr.second);
         }
      }
      else
      {
         _cvrMap.insert(std::make_pair(cvr.getVar(), cvr));
      }
   }

   const VCMap& getVCMap() const
   {
      return _vcMap;
   }

   void addViewConvertMask(VarNode* var, VarNode* realVar, const UnpackSelector& mask)
   {
      THROW_ASSERT(!static_cast<bool>(_vcMap.count(var)), "View-convert mask already present for " + GET_CONST_NODE(var->getValue())->ToString());
      _vcMap.insert({var, {realVar, mask}});
   }

   const OpNodes& getOpNodes() const
   {
      return _opNodes;
   }

   OpNode* pushOperation(OpNode* op)
   {
      if(op)
      {
         _opNodes.insert(op);
         _defMap.insert({op->getSink()->getValue(), op});
         for(const auto& tn : op->getSources())
         {
            _useMap[tn].insert(op);
         }
      }
      return op;
   }

   OpNode* addOperation(const tree_nodeConstRef& stmt, unsigned int function_id, const FunctionBehaviorConstRef& FB, const tree_managerConstRef& TM, const application_managerRef& AppM)
   {
      for(const auto& generateCtorFor : _opCtorGenerators)
      {
         if(auto generateOpFor = generateCtorFor(stmt, function_id, FB, TM, AppM))
         {
            return pushOperation(generateOpFor(this));
         }
      }
      return nullptr;
   }

   const DefMap& getDefs() const
   {
      return _defMap;
   }

   const UseMap& getUses() const
   {
      return _useMap;
   }

#ifndef NDEBUG
   static int debug_level;
#endif
};

#ifndef NDEBUG
int NodeContainer::debug_level = DEBUG_LEVEL_NONE;
#endif

#ifndef NDEBUG
static bool enable_add = true;
static bool enable_sub = true;
static bool enable_mul = true;
static bool enable_sdiv = true;
static bool enable_udiv = true;
static bool enable_srem = true;
static bool enable_urem = true;
static bool enable_shl = true;
static bool enable_shr = true;
static bool enable_abs = true;
static bool enable_negate = true;
static bool enable_not = true;
static bool enable_and = true;
static bool enable_or = true;
static bool enable_xor = true;
static bool enable_sext = true;
static bool enable_zext = true;
static bool enable_trunc = true;
static bool enable_min = true;
static bool enable_max = true;
static bool enable_load = true;
static bool enable_float_pack = true;
static bool enable_view_convert = true;
static bool enable_float_unpack = true;
static bool enable_ternary = false; // TODO: disable because of problem with reduced precision fdiv/f64div operator (fix before enabling back)
static bool enable_bit_phi = true;

#define OPERATION_OPTION(opts, X)                                                                          \
   if(opts.erase("no_" #X))                                                                                \
   {                                                                                                       \
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: " #X " operation disabled"); \
      enable_##X = false;                                                                                  \
   }
#define RETURN_DISABLED_OPTION(x, bw)          \
   if(!enable_##x)                             \
   {                                           \
      return RangeRef(new Range(Regular, bw)); \
   }
#define RESULT_DISABLED_OPTION(x, var, stdResult) enable_##x ? stdResult : getRangeFor(var, Regular)
#else

#define OPERATION_OPTION(opts, X) void(0)
#define RETURN_DISABLED_OPTION(x, bw) void(0)
#define RESULT_DISABLED_OPTION(x, var, stdResult) stdResult
#endif

// Tells if --soft-float flag is active inside bambu
static bool enable_softfloat = true;

// ========================================================================== //
// PhiOp
// ========================================================================== //

/// A constraint like sink = phi(src1, src2, ..., srcN)
class PhiOpNode : public OpNode
{
 private:
   // Vector of sources
   std::vector<const VarNode*> sources;
   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   RangeRef eval() const override;

 public:
   PhiOpNode(const ValueRangeRef& intersect, VarNode* sink, const tree_nodeConstRef& inst);
   ~PhiOpNode() override = default;
   PhiOpNode(const PhiOpNode&) = delete;
   PhiOpNode(PhiOpNode&&) = delete;
   PhiOpNode& operator=(const PhiOpNode&) = delete;
   PhiOpNode& operator=(PhiOpNode&&) = delete;

   /// Add source to the vector of sources
   void addSource(const VarNode* newsrc)
   {
      sources.push_back(newsrc);
   }
   /// Return source identified by index
   const VarNode* getSource(size_t index) const
   {
      return sources[index];
   }
   /// return the number of sources
   size_t getNumSources() const
   {
      return sources.size();
   }

   std::vector<tree_nodeConstRef> getSources() const override
   {
      std::vector<tree_nodeConstRef> tSources;
      std::transform(sources.begin(), sources.end(), std::back_inserter(tSources), [](const VarNode* vn) -> tree_nodeConstRef { return vn->getValue(); });
      return tSources;
   }

   // Methods for RTTI
   OperationId getValueId() const override
   {
      return OperationId::PhiOpId;
   }
   static bool classof(PhiOpNode const* /*unused*/)
   {
      return true;
   }
   static bool classof(OpNode const* BO)
   {
      return BO->getValueId() == OperationId::PhiOpId;
   }

   void print(std::ostream& OS) const override;
   void printDot(std::ostream& OS) const override;

   static std::function<OpNode*(NodeContainer*)> opCtorGenerator(const tree_nodeConstRef& stmt, unsigned int, const FunctionBehaviorConstRef& FB, const tree_managerConstRef& TM, const application_managerRef& AppM);
};

// The ctor.
PhiOpNode::PhiOpNode(const ValueRangeRef& _intersect, VarNode* _sink, const tree_nodeConstRef& _inst) : OpNode(_intersect, _sink, _inst)
{
}

/// Computes the interval of the sink based on the interval of the sources.
/// The result of evaluating a phi-function is the union of the ranges of
/// every variable used in the phi.
RangeRef PhiOpNode::eval() const
{
   THROW_ASSERT(sources.size() > 0, "Phi operation sources list empty");
   auto result = getRangeFor(getSink()->getValue(), Empty);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, GET_CONST_NODE(getSink()->getValue())->ToString() + " = PHI");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   // Iterate over the sources of the phiop
   for(const VarNode* varNode : sources)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "->" + varNode->ToString());
      result = result->unionWith(varNode->getRange());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--= " + result->ToString());

   bool test = this->getIntersect()->getRange()->isFullSet();
   if(!test)
   {
      const auto aux = this->getIntersect()->getRange();
      auto _intersect = result->intersectWith(aux);
      if(!_intersect->isEmpty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "aux = " + aux->ToString() + " from " + getIntersect()->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "result = " + _intersect->ToString());
         result = _intersect;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "res = " + result->ToString());
   return result;
}

std::function<OpNode*(NodeContainer*)> PhiOpNode::opCtorGenerator(const tree_nodeConstRef& stmt, unsigned int function_id, const FunctionBehaviorConstRef&, const tree_managerConstRef&, const application_managerRef&)
{
   const auto* phi = GetPointer<const gimple_phi>(GET_CONST_NODE(stmt));
   if(!phi || phi->CGetDefEdgesList().size() <= 1)
   {
      return nullptr;
   }
   return [stmt, phi, function_id](NodeContainer* NC) {
      if(phi->virtual_flag)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---This is a virtual phi, skipping...");
         return static_cast<PhiOpNode*>(nullptr);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "Analysing phi operation " + phi->ToString());

      // Create the sink.
      VarNode* sink = NC->addVarNode(phi->res, function_id);
      auto BI = ValueRangeRef(new ValueRange(getGIMPLE_range(stmt)));
      PhiOpNode* phiOp = new PhiOpNode(BI, sink, stmt);

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added PhiOp with range " + BI->ToString() + " and " + STR(phi->CGetDefEdgesList().size()) + " sources");

      // Create the sources.
      for(const auto& operandBBI : phi->CGetDefEdgesList())
      {
         VarNode* source = NC->addVarNode(operandBBI.first, function_id);
         phiOp->addSource(source);
      }
      return phiOp;
   };
}

void PhiOpNode::print(std::ostream& OS) const
{
   OS << GET_CONST_NODE(getSink()->getValue())->ToString() << " = PHI<";
   int i = 0;
   for(; i < static_cast<int>(sources.size() - 1); ++i)
   {
      OS << GET_CONST_NODE(sources.at(static_cast<decltype(sources.size())>(i))->getValue())->ToString() << ", ";
   }
   OS << GET_CONST_NODE(sources.at(static_cast<decltype(sources.size())>(i))->getValue())->ToString() << ">";
}

void PhiOpNode::printDot(std::ostream& OS) const
{
   const char* quot = R"(")";
   OS << " " << quot << this << quot << R"( [label=")";
   OS << "phi";
   OS << "\"]\n";
   for(const VarNode* varNode : sources)
   {
      const auto& V = varNode->getValue();
      if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V)))
      {
         OS << " " << C->value << " -> " << quot << this << quot << "\n";
      }
      else
      {
         OS << " " << quot;
         printVarName(V, OS);
         OS << quot << " -> " << quot << this << quot << "\n";
      }
   }
   const auto& VS = this->getSink()->getValue();
   OS << " " << quot << this << quot << " -> " << quot;
   printVarName(VS, OS);
   OS << quot << "\n";
}

// ========================================================================== //
// UnaryOp
// ========================================================================== //
/// A constraint like sink = operation(source) \intersec [l, u]
/// Examples: unary instructions such as truncation, sign extensions,
/// zero extensions.
class UnaryOpNode : public OpNode
{
 private:
   // The source node of the operation.
   VarNode* source;
   // The opcode of the operation.
   kind opcode;
   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   RangeRef eval() const override;

 public:
   UnaryOpNode(const ValueRangeRef& intersect, VarNode* sink, const tree_nodeConstRef& inst, VarNode* source, kind opcode);
   ~UnaryOpNode() override = default;
   UnaryOpNode(const UnaryOpNode&) = delete;
   UnaryOpNode(UnaryOpNode&&) = delete;
   UnaryOpNode& operator=(const UnaryOpNode&) = delete;
   UnaryOpNode& operator=(UnaryOpNode&&) = delete;

   // Methods for RTTI
   OperationId getValueId() const override
   {
      return OperationId::UnaryOpId;
   }
   static bool classof(UnaryOpNode const* /*unused*/)
   {
      return true;
   }
   static bool classof(OpNode const* BO)
   {
      return BO->getValueId() == OperationId::UnaryOpId || BO->getValueId() == OperationId::SigmaOpId;
   }

   /// Return the opcode of the operation.
   kind getOpcode() const
   {
      return opcode;
   }
   /// Returns the source of the operation.
   VarNode* getSource() const
   {
      return source;
   }
   virtual std::vector<tree_nodeConstRef> getSources() const override
   {
      return {source->getValue()};
   }

   void print(std::ostream& OS) const override;
   void printDot(std::ostream& OS) const override;

   static std::function<OpNode*(NodeContainer*)> opCtorGenerator(const tree_nodeConstRef& stmt, unsigned int, const FunctionBehaviorConstRef& FB, const tree_managerConstRef& TM, const application_managerRef& AppM);
};

UnaryOpNode::UnaryOpNode(const ValueRangeRef& _intersect, VarNode* _sink, const tree_nodeConstRef& _inst, VarNode* _source, kind _opcode) : OpNode(_intersect, _sink, _inst), source(_source), opcode(_opcode)
{
}

/// Computes the interval of the sink based on the interval of the sources,
/// the operation and the interval associated to the operation.
RangeRef UnaryOpNode::eval() const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, ToString());

   const auto bw = getSink()->getBitWidth();
   const auto oprnd = source->getRange();
   const auto resultType = getGIMPLE_Type(getSink()->getValue());
   const bool oprndSigned = isSignedType(source->getValue());
   auto result = getRangeFor(getSink()->getValue(), Unknown);
   if(oprnd->isEmpty())
   {
      if(resultType->get_kind() == real_type_K)
      {
         result = RangeRef(new RealRange(RangeRef(new Range(Empty, bw))));
      }
      else
      {
         result = RangeRef(new Range(Empty, bw));
      }
   }
   else if(oprnd->isReal())
   {
      auto rr = std::static_pointer_cast<const RealRange>(oprnd);
      switch(this->getOpcode())
      {
         case bit_and_expr_K:
         {
            result = rr->getSignificand()->zextOrTrunc(bw);
            break;
         }
         case rshift_expr_K:
         {
            result = rr->getExponent()->zextOrTrunc(bw);
            break;
         }
         case lt_expr_K:
         {
            result = rr->getSign()->zextOrTrunc(bw);
            break;
         }
         case le_expr_K:
         {
            result = rr->getSign()->sextOrTrunc(bw);
            break;
         }
         case view_convert_expr_K:
         {
            THROW_ASSERT(bw == 64 || bw == 32, "View_convert sink bitwidth should be 64 or 32 bits (" + STR(bw) + ")");
            result = RESULT_DISABLED_OPTION(view_convert, getSink()->getValue(), rr->getRange()->zextOrTrunc(bw));
            break;
         }
         case nop_expr_K:
         {
            THROW_ASSERT(bw == 32 || bw == 64, "Unhandled floating point bitwidth (" + STR(bw) + ")");
            result = bw == 32 ? rr->toFloat32() : rr->toFloat64();
            break;
         }
         case abs_expr_K:
         {
            result = RESULT_DISABLED_OPTION(abs, getSink()->getValue(), oprnd->abs());
            break;
         }
         case negate_expr_K:
         {
            result = RESULT_DISABLED_OPTION(negate, getSink()->getValue(), oprnd->negate());
            break;
         }
         case addr_expr_K:
         case paren_expr_K:
         case arrow_expr_K:
         case bit_not_expr_K:
         case buffer_ref_K:
         case card_expr_K:
         case cleanup_point_expr_K:
         case conj_expr_K:
         case convert_expr_K:
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
         case assert_expr_K:
         case bit_ior_expr_K:
         case bit_xor_expr_K:
         case catch_expr_K:
         case ceil_div_expr_K:
         case ceil_mod_expr_K:
         case complex_expr_K:
         case compound_expr_K:
         case eh_filter_expr_K:
         case eq_expr_K:
         case exact_div_expr_K:
         case fdesc_expr_K:
         case floor_div_expr_K:
         case floor_mod_expr_K:
         case ge_expr_K:
         case gt_expr_K:
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
         case ne_expr_K:
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
         case uneq_expr_K:
         case ltgt_expr_K:
         case unge_expr_K:
         case ungt_expr_K:
         case unle_expr_K:
         case unlt_expr_K:
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
            THROW_UNREACHABLE("Unhandled unary operation for real case");
      }
   }
   else if(oprnd->isRegular() || oprnd->isAnti())
   {
      switch(this->getOpcode())
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
            if(resultType->get_kind() == real_type_K)
            {
               result = RESULT_DISABLED_OPTION(float_pack, getSink()->getValue(), RangeRef(new RealRange(oprnd)));
            }
            else
            {
               //    std::cout << "Strange view_convert here " << getSink()->ToString() << " = view_convert(" << getSource()->ToString() << ")" << std::endl;
               THROW_ASSERT(!oprnd->isReal(), "Operand here should not be real");
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
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, result->ToString() + " = " + tree_node::GetString(this->getOpcode()) + "( " + oprnd->ToString() + " )");

   auto test = this->getIntersect()->getRange()->isFullSet();
   if(!test)
   {
      const auto aux = this->getIntersect()->getRange();
      auto _intersect = result->intersectWith(aux);
      if(!_intersect->isEmpty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "aux = " + aux->ToString() + " from " + getIntersect()->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "result = " + _intersect->ToString());
         result = _intersect;
      }
   }
   return result;
}

std::function<OpNode*(NodeContainer*)> UnaryOpNode::opCtorGenerator(const tree_nodeConstRef& stmt, unsigned int function_id, const FunctionBehaviorConstRef&, const tree_managerConstRef&, const application_managerRef&)
{
   const auto* assign = GetPointer<const gimple_assign>(GET_CONST_NODE(stmt));
   if(assign == nullptr)
   {
      return nullptr;
   }
   if(GetPointer<const ssa_name>(GET_CONST_NODE(assign->op1)) != nullptr || GetPointer<const cst_node>(GET_CONST_NODE(assign->op1)))
   {
      return [function_id, stmt, assign](NodeContainer* NC) {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "Analysing assign operation " + assign->ToString());

         VarNode* sink = NC->addVarNode(assign->op0, function_id);
         VarNode* _source = NC->addVarNode(assign->op1, function_id);

         auto BI = ValueRangeRef(new ValueRange(getGIMPLE_range(stmt)));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added assign operation with range " + BI->ToString());
         return new UnaryOpNode(BI, sink, stmt, _source, nop_expr_K);
      };
   }
   const auto* un_op = GetPointer<const unary_expr>(GET_CONST_NODE(assign->op1));
   if(un_op == nullptr)
   {
      return nullptr;
   }
   return [stmt, assign, un_op, function_id](NodeContainer* NC) {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "Analysing unary operation " + un_op->get_kind_text() + " " + assign->ToString());

      // Create the sink.
      auto* sink = NC->addVarNode(assign->op0, function_id);
      // Create the source.
      auto* _source = NC->addVarNode(un_op->op, function_id);
      const auto sourceType = getGIMPLE_Type(_source->getValue());
      auto BI = ValueRangeRef(new ValueRange(getGIMPLE_range(stmt)));
      const auto op_kind = un_op->get_kind();

#ifndef NDEBUG
      if(enable_float_unpack)
      {
#endif
         const auto fromVC = NC->getVCMap().find(_source);
         if(fromVC != NC->getVCMap().end())
         {
            const auto& f = fromVC->second.first;
            const auto& us = fromVC->second.second;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Operand " + GET_CONST_NODE(_source->getValue())->ToString() + " is view_convert from " + f->ToString() + "& (" + STR(us.mask) + " >> " + STR(+us.rshift) + ")");
            const auto curr_us = us & ((1ULL << sink->getBitWidth()) - 1ULL);
            // Detect exponent trucantion after right shift for 32 bit floating-point
            if((op_kind == nop_expr_K || op_kind == convert_expr_K) && f->getBitWidth() == 32 && curr_us.getSelector() == UnpackSelector::packPos_Exp32)
            {
               //    NC->addViewConvertMask(sink, f, curr_us);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added UnaryOp for exponent view_convert");
               return new UnaryOpNode(BI, sink, nullptr, f, rshift_expr_K);
            }
            NC->addViewConvertMask(sink, f, us);
         }
#ifndef NDEBUG
      }
#endif
      // Store active bitmask for variable initialized from float view_convert operation
      if(op_kind == view_convert_expr_K && sourceType->get_kind() == real_type_K)
      {
         NC->addViewConvertMask(sink, _source, _source->getBitWidth() == 32 ? UnpackSelector(UINT32_MAX, 0U) : UnpackSelector(UINT64_MAX, 0U));
      }

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added UnaryOp for " + un_op->get_kind_text() + " with range " + BI->ToString());
      return new UnaryOpNode(BI, sink, stmt, _source, op_kind);
   };
}

void UnaryOpNode::print(std::ostream& OS) const
{
   OS << GET_CONST_NODE(getSink()->getValue())->ToString() << " = " << tree_node::GetString(this->getOpcode()) << "( " << GET_CONST_NODE(getSource()->getValue())->ToString() << " )";
}

void UnaryOpNode::printDot(std::ostream& OS) const
{
   const char* quot = R"(")";
   OS << " " << quot << this << quot << R"( [label=")";

   // Instruction bitwidth
   const auto bw = getSink()->getBitWidth();
   const bool oprndSigned = isSignedType(source->getValue());

   if(opcode == nop_expr_K || opcode == convert_expr_K)
   {
      if(bw < getSource()->getBitWidth())
      {
         OS << "trunc i" << bw;
      }
      else
      {
         if(getGIMPLE_Type(getSource()->getValue())->get_kind() == pointer_type_K)
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
      const auto type = getGIMPLE_Type(getSink()->getValue());
      if(const auto* int_type = GetPointer<const integer_type>(type))
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
      this->getIntersect()->print(OS);
   }

   OS << "\"]\n";

   const auto& V = this->getSource()->getValue();
   if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V)))
   {
      OS << " " << C->value << " -> " << quot << this << quot << "\n";
   }
   else
   {
      OS << " " << quot;
      printVarName(V, OS);
      OS << quot << " -> " << quot << this << quot << "\n";
   }

   const auto& VS = this->getSink()->getValue();
   OS << " " << quot << this << quot << " -> " << quot;
   printVarName(VS, OS);
   OS << quot << "\n";
}

// ========================================================================== //
// SigmaOp
// ========================================================================== //
/// Specific type of UnaryOp used to represent sigma functions
class SigmaOpNode : public UnaryOpNode
{
 private:
   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   RangeRef eval() const override;

   // The symbolic source node of the operation.
   VarNode* SymbolicSource;

   bool unresolved;

 public:
   SigmaOpNode(const ValueRangeRef& intersect, VarNode* sink, const tree_nodeConstRef& inst, VarNode* source, VarNode* SymbolicSource, kind opcode);
   ~SigmaOpNode() override = default;
   SigmaOpNode(const SigmaOpNode&) = delete;
   SigmaOpNode(SigmaOpNode&&) = delete;
   SigmaOpNode& operator=(const SigmaOpNode&) = delete;
   SigmaOpNode& operator=(SigmaOpNode&&) = delete;

   // Methods for RTTI
   OperationId getValueId() const override
   {
      return OperationId::SigmaOpId;
   }
   static bool classof(SigmaOpNode const* /*unused*/)
   {
      return true;
   }
   static bool classof(UnaryOpNode const* UO)
   {
      return UO->getValueId() == OperationId::SigmaOpId;
   }
   static bool classof(OpNode const* BO)
   {
      return BO->getValueId() == OperationId::SigmaOpId;
   }
   std::vector<tree_nodeConstRef> getSources() const override
   {
      std::vector<tree_nodeConstRef> s = UnaryOpNode::getSources();
      if(SymbolicSource != nullptr)
      {
         s.push_back(SymbolicSource->getValue());
      }
      return s;
   }

   bool isUnresolved() const
   {
      return unresolved;
   }
   void markResolved()
   {
      unresolved = false;
   }
   void markUnresolved()
   {
      unresolved = true;
   }

   void print(std::ostream& OS) const override;
   void printDot(std::ostream& OS) const override;

   static std::function<OpNode*(NodeContainer*)> opCtorGenerator(const tree_nodeConstRef& stmt, unsigned int, const FunctionBehaviorConstRef& FB, const tree_managerConstRef& TM, const application_managerRef& AppM);
};

SigmaOpNode::SigmaOpNode(const ValueRangeRef& _intersect, VarNode* _sink, const tree_nodeConstRef& _inst, VarNode* _source, VarNode* _SymbolicSource, kind _opcode)
    : UnaryOpNode(_intersect, _sink, _inst, _source, _opcode), SymbolicSource(_SymbolicSource), unresolved(false)
{
}

/// Computes the interval of the sink based on the interval of the sources,
/// the operation and the interval associated to the operation.
RangeRef SigmaOpNode::eval() const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, ToString());

   RangeRef result(this->getSource()->getRange()->clone());
   const auto aux = this->getIntersect()->getRange();
   if(!aux->isUnknown())
   {
      auto _intersect = result->intersectWith(aux);
      if(!_intersect->isEmpty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "aux = " + aux->ToString() + " from " + getIntersect()->ToString());
         // Sigma operations are used to enhance live range split after conditional statements,
         // thus it is useful to intersect their range only if it actually produces tighter interval
         if(_intersect->isReal() || _intersect->getSpan() < result->getSpan())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "result = " + _intersect->ToString());
            result = _intersect;
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "result not changed because not improved");
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, result->ToString() + " = SIGMA< " + getSource()->getRange()->ToString() + " >");
   return result;
}

std::function<OpNode*(NodeContainer*)> SigmaOpNode::opCtorGenerator(const tree_nodeConstRef& stmt, unsigned int function_id, const FunctionBehaviorConstRef&, const tree_managerConstRef&, const application_managerRef&)
{
   const auto* phi = GetPointer<const gimple_phi>(GET_CONST_NODE(stmt));
   if(!phi || phi->CGetDefEdgesList().size() != 1)
   {
      return nullptr;
   }
   return [stmt, phi, function_id](NodeContainer* NC) {
      const auto BBI = phi->bb_index;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "Analysing sigma operation " + phi->ToString());
      const auto& sourceTN = phi->CGetDefEdgesList().front().first;

      // Create the sink.
      VarNode* sink = NC->addVarNode(phi->res, function_id);
      VarNode* source = NC->addVarNode(sourceTN, function_id);

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
            const auto& bound = symb->getBound();
            SymbSrc = NC->addVarNode(bound, function_id);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added SigmaOp with " + std::string(SymbSrc ? "symbolic " : "") + "range " + CondRange->ToString());
         return new SigmaOpNode(CondRange, sink, stmt, source, SymbSrc, phi->get_kind());
      }
      else
      {
         auto BI = ValueRangeRef(new ValueRange(getGIMPLE_range(stmt)));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added SigmaOp with range " + BI->ToString());
         return new SigmaOpNode(BI, sink, stmt, source, nullptr, phi->get_kind());
      }
   };
}

void SigmaOpNode::print(std::ostream& OS) const
{
   OS << GET_CONST_NODE(getSink()->getValue())->ToString() << " = SIGMA< " << GET_CONST_NODE(getSource()->getValue())->ToString() << " >";
}

void SigmaOpNode::printDot(std::ostream& OS) const
{
   const char* quot = R"(")";
   OS << " " << quot << this << quot << R"( [label=")"
      << "SigmaOp:";
   this->getIntersect()->print(OS);
   OS << "\"]\n";
   const auto& V = this->getSource()->getValue();
   if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V)))
   {
      OS << " " << C->value << " -> " << quot << this << quot << "\n";
   }
   else
   {
      OS << " " << quot;
      printVarName(V, OS);
      OS << quot << " -> " << quot << this << quot << "\n";
   }
   if(SymbolicSource)
   {
      const auto& _V = SymbolicSource->getValue();
      if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(_V)))
      {
         OS << " " << C->value << " -> " << quot << this << quot << "\n";
      }
      else
      {
         OS << " " << quot;
         printVarName(_V, OS);
         OS << quot << " -> " << quot << this << quot << "\n";
      }
   }

   const auto& VS = this->getSink()->getValue();
   OS << " " << quot << this << quot << " -> " << quot;
   printVarName(VS, OS);
   OS << quot << "\n";
}

// ========================================================================== //
// BinaryOp
// ========================================================================== //
/// A constraint like sink = source1 operation source2 intersect [l, u].
class BinaryOpNode : public OpNode
{
 private:
   // The first operand.
   VarNode* source1;
   // The second operand.
   VarNode* source2;
   // The opcode of the operation.
   kind opcode;
   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   RangeRef eval() const override;

 public:
   BinaryOpNode(const ValueRangeRef& intersect, VarNode* sink, const tree_nodeConstRef& inst, VarNode* source1, VarNode* source2, kind opcode);
   ~BinaryOpNode() override = default;
   BinaryOpNode(const BinaryOpNode&) = delete;
   BinaryOpNode(BinaryOpNode&&) = delete;
   BinaryOpNode& operator=(const BinaryOpNode&) = delete;
   BinaryOpNode& operator=(BinaryOpNode&&) = delete;

   // Methods for RTTI
   OperationId getValueId() const override
   {
      return OperationId::BinaryOpId;
   }
   static bool classof(BinaryOpNode const* /*unused*/)
   {
      return true;
   }
   static bool classof(OpNode const* BO)
   {
      return BO->getValueId() == OperationId::BinaryOpId;
   }

   static RangeRef evaluate(kind opcode, bw_t bw, const RangeConstRef& op1, const RangeConstRef& op2, bool isSigned);

   /// Return the opcode of the operation.
   kind getOpcode() const
   {
      return opcode;
   }
   /// Returns the first operand of this operation.
   VarNode* getSource1() const
   {
      return source1;
   }
   /// Returns the second operand of this operation.
   VarNode* getSource2() const
   {
      return source2;
   }
   std::vector<tree_nodeConstRef> getSources() const override
   {
      return {source1->getValue(), source2->getValue()};
   }

   void print(std::ostream& OS) const override;
   void printDot(std::ostream& OS) const override;

   static std::function<OpNode*(NodeContainer*)> opCtorGenerator(const tree_nodeConstRef& stmt, unsigned int, const FunctionBehaviorConstRef& FB, const tree_managerConstRef& TM, const application_managerRef& AppM);
};

// The ctor.
BinaryOpNode::BinaryOpNode(const ValueRangeRef& _intersect, VarNode* _sink, const tree_nodeConstRef& _inst, VarNode* _source1, VarNode* _source2, kind _opcode) : OpNode(_intersect, _sink, _inst), source1(_source1), source2(_source2), opcode(_opcode)
{
   THROW_ASSERT(isValidType(_sink->getValue()), "Binary operation sink should be of valid type (" + GET_CONST_NODE(_sink->getValue())->ToString() + ")");
}

RangeRef BinaryOpNode::evaluate(kind opcode, bw_t bw, const RangeConstRef& op1, const RangeConstRef& op2, bool opSigned)
{
   switch(opcode)
   {
#ifdef INTEGER_PTR
      case pointer_plus_expr_K:
#endif
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
         return opSigned ? op1->sextOrTrunc(bw)->mul(op2->sextOrTrunc(bw)) : op1->zextOrTrunc(bw)->mul(op2->sextOrTrunc(bw));
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
            if(res->getSignedMin() == 0)
            {
               const auto consRes = res->unionWith(res->negate());
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Being conservative on signed modulo operator: " + res->ToString() + " -> " + consRes->ToString());
               return consRes;
            }
            return op1->srem(op2);
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
      case uneq_expr_K:
      case eq_expr_K:
         if(op1->getBitWidth() < op2->getBitWidth())
         {
            return opSigned ? op1->sextOrTrunc(op2->getBitWidth())->Eq(op2, bw) : op1->zextOrTrunc(op2->getBitWidth())->Eq(op2, bw);
         }
         else if(op2->getBitWidth() < op1->getBitWidth())
         {
            return opSigned ? op2->sextOrTrunc(op1->getBitWidth())->Eq(op1, bw) : op2->zextOrTrunc(op1->getBitWidth())->Eq(op1, bw);
         }
         return op1->Eq(op2, bw);
      case ne_expr_K:
         return op1->Ne(op2, bw);
      case unge_expr_K:
         return op1->Uge(op2, bw);
      case ungt_expr_K:
         return op1->Ugt(op2, bw);
      case unlt_expr_K:
         return op1->Ult(op2, bw);
      case unle_expr_K:
         return op1->Ule(op2, bw);
      case gt_expr_K:
         return op1->Sgt(op2, bw);
      case ge_expr_K:
         return op1->Sge(op2, bw);
      case lt_expr_K:
         return op1->Slt(op2, bw);
      case le_expr_K:
         return op1->Sle(op2, bw);
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
   const auto op1 = this->getSource1()->getRange();
   const auto op2 = this->getSource2()->getRange();
   // Instruction bitwidth
   const auto sinkBW = getSink()->getBitWidth();
   auto result = getRangeFor(getSink()->getValue(), Unknown);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, ToString());

   // only evaluate if all operands are Regular
   if((op1->isRegular() || op1->isAnti()) && (op2->isRegular() || op2->isAnti()))
   {
      const auto opSigned = isSignedType(getSource1()->getValue());

      result = evaluate(this->getOpcode(), sinkBW, op1, op2, opSigned);

      // Bitvalue may consider only lower bits for some variables, thus it is necessary to perform evaluation on truncated opernds to obtain valid results
      if(const auto* ssa = GetPointer<const ssa_name>(GET_CONST_NODE(getSink()->getValue())))
      {
         const auto sinkSigned = isSignedType(getSink()->getValue());
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
            const auto r = RangeRef(new Range(Regular, static_cast<bw_t>(ssa->bit_values.size()), bits, bits));
            THROW_ASSERT(r->isConstant(), "Range derived from <" + ssa->bit_values + "> should be constant");
            return r;
         }();
         const auto op_code = this->getOpcode();
         if(bvRange->isConstant() && (bvRange->getSignedMax() != -1 || bvRange->getBitWidth() < result->getBitWidth()) &&
            (op_code == mult_expr_K || op_code == widen_mult_expr_K || op_code == plus_expr_K /* || op_code == minus_expr_K || op_code == pointer_plus_expr_K */))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Result range " + result->ToString() + " filtered with mask " + bitstring_to_string(bvRange->getBitValues(sinkSigned)) + "<" + STR(bvRange->getBitWidth()) + "> from " + ssa->bit_values + "<" +
                               (sinkSigned ? "signed" : "unsigned") + "> " + bvRange->ToString());
            // #if HAVE_ASSERTS
            // const auto resEmpty = result->isEmpty();
            // #endif
            const auto truncRes = sinkSigned ? result->truncate(bvRange->getBitWidth())->sextOrTrunc(result->getBitWidth()) : result->truncate(bvRange->getBitWidth())->zextOrTrunc(result->getBitWidth());
            const auto maskRes = sinkSigned ? result->And(bvRange->zextOrTrunc(result->getBitWidth()))->truncate(bvRange->getBitWidth())->sextOrTrunc(result->getBitWidth()) : result->And(bvRange->zextOrTrunc(result->getBitWidth()));
            result = truncRes->getSpan() < maskRes->getSpan() ? truncRes : maskRes;
            // THROW_ASSERT(result->isEmpty() == resEmpty, "");
         }
      }

      if(result->getBitWidth() != sinkBW)
      {
         result = result->zextOrTrunc(sinkBW);
      }
   }
   else if(op1->isReal() && op2->isReal())
   {
      // When softfloat is disabled, floating-point operators are not replaced by function calls
      if(!enable_softfloat && this->getOpcode() != eq_expr_K && this->getOpcode() != ne_expr_K)
      {
         result = getRangeFor(getSink()->getValue(), Regular);
      }
      else
      {
         THROW_ASSERT(this->getOpcode() == eq_expr_K || this->getOpcode() == ne_expr_K, tree_node::GetString(this->getOpcode()) + " with real operands not supported");
         result = evaluate(this->getOpcode(), sinkBW, op1, op2, false);
      }
   }
   else if(op1->isEmpty() || op2->isEmpty())
   {
      result = getRangeFor(getSink()->getValue(), Empty);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, result->ToString() + " = " + op1->ToString() + " " + tree_node::GetString(this->getOpcode()) + " " + op2->ToString());

   bool test = this->getIntersect()->getRange()->isFullSet();
   if(!test)
   {
      const auto aux = this->getIntersect()->getRange();
      auto _intersect = result->intersectWith(aux);
      if(!_intersect->isEmpty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "aux = " + aux->ToString() + " from " + getIntersect()->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "result = " + _intersect->ToString());
         result = _intersect;
      }
   }
   return result;
}

std::function<OpNode*(NodeContainer*)> BinaryOpNode::opCtorGenerator(const tree_nodeConstRef& stmt, unsigned int function_id, const FunctionBehaviorConstRef&, const tree_managerConstRef&, const application_managerRef&)
{
   const auto* assign = GetPointer<const gimple_assign>(GET_CONST_NODE(stmt));
   if(assign == nullptr)
   {
      return nullptr;
   }
   const auto* bin_op = GetPointer<const binary_expr>(GET_CONST_NODE(assign->op1));
   if(bin_op == nullptr)
   {
      return nullptr;
   }
   return [stmt, assign, bin_op, function_id](NodeContainer* NC) {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "Analysing binary operation " + bin_op->get_kind_text() + " " + assign->ToString());

      // Create the sink.
      auto* sink = NC->addVarNode(assign->op0, function_id);
      auto op_kind = bin_op->get_kind();

      // Create the sources.
      auto* _source1 = NC->addVarNode(bin_op->op0, function_id);
      auto* _source2 = NC->addVarNode(bin_op->op1, function_id);

      auto BI = ValueRangeRef(new ValueRange(getGIMPLE_range(stmt)));

#ifndef NDEBUG
      if(enable_float_unpack)
      {
#endif
         if((op_kind == rshift_expr_K || op_kind == bit_and_expr_K) && GET_CONST_NODE(_source2->getValue())->get_kind() == integer_cst_K)
         {
            const auto fromVC = NC->getVCMap().find(_source1);
            if(fromVC != NC->getVCMap().end())
            {
               const auto& f = fromVC->second.first;
               const auto& us = fromVC->second.second;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Operand " + GET_CONST_NODE(_source1->getValue())->ToString() + " is view_convert from " + f->ToString() + "& (" + STR(us.mask) + " >> " + STR(+us.rshift) + ")");
               const auto cst_int = tree_helper::get_integer_cst_value(GetPointer<const integer_cst>(GET_CONST_NODE(_source2->getValue())));
               UnpackSelector new_us;

               if(op_kind == rshift_expr_K)
               {
                  new_us = us >> static_cast<uint8_t>(cst_int);
               }
               else if(op_kind == bit_and_expr_K)
               {
                  new_us = us & static_cast<uint64_t>(cst_int);
               }
               else
               {
                  THROW_UNREACHABLE("");
               }

               const auto addSignViewConvert = [&](VarNode* fpVar) {
                  const auto oprndSigned = isSignedType(bin_op->op0);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added UnaryOp for sign view_convert (" + std::string(oprndSigned ? "signed" : "unsigned") + ")");
                  return static_cast<OpNode*>(new UnaryOpNode(BI, sink, nullptr, fpVar, oprndSigned ? le_expr_K : lt_expr_K));
               };
               const auto addExponentViewConvert = [&](VarNode* fpVar) {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added UnaryOp for exponent view_convert");
                  return static_cast<OpNode*>(new UnaryOpNode(BI, sink, nullptr, fpVar, rshift_expr_K));
               };
               const auto addFractionalViewConvert = [&](VarNode* fpVar) {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added UnaryOp for significand view_convert");
                  return static_cast<OpNode*>(new UnaryOpNode(BI, sink, nullptr, fpVar, bit_and_expr_K));
               };

               if(f->getBitWidth() == 32)
               {
                  switch(new_us.getSelector())
                  {
                     case UnpackSelector::packPos_Sign32:
                        return addSignViewConvert(f);
                     case UnpackSelector::packPos_Exp32:
                        return addExponentViewConvert(f);
                     case UnpackSelector::packPos_Sigf32:
                        return addFractionalViewConvert(f);
                     case UnpackSelector::packPos_Sign64:
                     case UnpackSelector::packPos_Exp64:
                     case UnpackSelector::packPos_Sigf64:
                     case UnpackSelector::packPos_Undefined:
                     default:
                        break;
                  }
               }
               else
               {
                  switch(new_us.getSelector())
                  {
                     case UnpackSelector::packPos_Sign64:
                        return addSignViewConvert(f);
                     case UnpackSelector::packPos_Exp64:
                        return addExponentViewConvert(f);
                     case UnpackSelector::packPos_Sigf64:
                        return addFractionalViewConvert(f);
                     case UnpackSelector::packPos_Sign32:
                     case UnpackSelector::packPos_Exp32:
                     case UnpackSelector::packPos_Sigf32:
                     case UnpackSelector::packPos_Undefined:
                     default:
                        break;
                  }
               }
               NC->addViewConvertMask(sink, f, std::move(new_us));
            }
         }
         else if(op_kind == lt_expr_K && GET_CONST_NODE(_source2->getValue())->get_kind() == integer_cst_K && tree_helper::get_integer_cst_value(GetPointer<const integer_cst>(GET_CONST_NODE(_source2->getValue()))) == 0)
         {
            const auto fromVC = NC->getVCMap().find(_source1);
            if(fromVC != NC->getVCMap().end())
            {
               const auto& f = fromVC->second.first;
#ifndef NDEBUG
               const auto& us = fromVC->second.second;
#endif
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Operand " + GET_CONST_NODE(_source1->getValue())->ToString() + " is view_convert from " + f->ToString() + "& (" + STR(us.mask) + " >> " + STR(+us.rshift) + ")");
               //    NC->addViewConvertMask(sink, f, f->getBitWidth() == 32 ? UnpackSelector(4294967296U,31U) : UnpackSelector(9223372036854775808ULL, 63U));
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added UnaryOp for sign view_convert");
               return static_cast<OpNode*>(new UnaryOpNode(BI, sink, nullptr, f, lt_expr_K));
            }
         }
#ifndef NDEBUG
      }
#endif

      if(isCompare(op_kind))
      {
         const auto opSigned = isSignedType(bin_op->op0);
         if(!opSigned)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Compare operation flagged as unsigned");
            op_kind = op_unsigned(op_kind);
         }
      }

      // Create the operation using the intersect to constrain sink's interval.
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added BinaryOp for " + tree_node::GetString(op_kind) + " with range " + BI->ToString());
      return static_cast<OpNode*>(new BinaryOpNode(BI, sink, stmt, _source1, _source2, op_kind));
   };
}

void BinaryOpNode::print(std::ostream& OS) const
{
   OS << GET_CONST_NODE(getSink()->getValue())->ToString() << " = (" << GET_CONST_NODE(getSource1()->getValue())->ToString() << ")" << tree_node::GetString(this->getOpcode()) + "(" << GET_CONST_NODE(getSource2()->getValue())->ToString() << ")";
}

void BinaryOpNode::printDot(std::ostream& OS) const
{
   const char* quot = R"(")";
   std::string opcodeName = tree_node::GetString(opcode);
   OS << " " << quot << this << quot << R"( [label=")" << opcodeName << "\"]\n";
   const auto& V1 = this->getSource1()->getValue();
   if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V1)))
   {
      OS << " " << C->value << " -> " << quot << this << quot << "\n";
   }
   else
   {
      OS << " " << quot;
      printVarName(V1, OS);
      OS << quot << " -> " << quot << this << quot << "\n";
   }
   const auto& V2 = this->getSource2()->getValue();
   if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V2)))
   {
      OS << " " << C->value << " -> " << quot << this << quot << "\n";
   }
   else
   {
      OS << " " << quot;
      printVarName(V2, OS);
      OS << quot << " -> " << quot << this << quot << "\n";
   }
   const auto& VS = this->getSink()->getValue();
   OS << " " << quot << this << quot << " -> " << quot;
   printVarName(VS, OS);
   OS << quot << "\n";
}

unsigned int evaluateBranch(const tree_nodeRef br_op, const blocRef branchBB
#ifndef NDEBUG
                            ,
                            int debug_level
#endif
)
{
   // Evaluate condition variable if possible
   if(const auto* ic = GetPointer<const integer_cst>(GET_CONST_NODE(br_op)))
   {
      const auto branchValue = tree_helper::get_integer_cst_value(ic);
      if(branchValue)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch variable value is " + STR(branchValue) + ", false edge BB" + STR(branchBB->false_edge) + " to be removed");
         return branchBB->false_edge;
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch variable value is " + STR(branchValue) + ", true edge BB" + STR(branchBB->true_edge) + " to be removed");
         return branchBB->true_edge;
      }
   }
   else if(const auto* bin_op = GetPointer<const binary_expr>(GET_CONST_NODE(br_op)))
   {
      const auto* l = GetPointer<const integer_cst>(GET_CONST_NODE(bin_op->op0));
      const auto* r = GetPointer<const integer_cst>(GET_CONST_NODE(bin_op->op1));
      if(l != nullptr && r != nullptr)
      {
         const auto lc = tree_helper::get_integer_cst_value(l);
         const auto rc = tree_helper::get_integer_cst_value(r);
         RangeRef lhs(new Range(Regular, Range::MAX_BIT_INT, lc, lc));
         RangeRef rhs(new Range(Regular, Range::MAX_BIT_INT, rc, rc));
         const auto branchValue = BinaryOpNode::evaluate(bin_op->get_kind(), 1, lhs, rhs, isSignedType(bin_op->op0));
         THROW_ASSERT(branchValue->isConstant(), "Constant binary operation should resolve to either true or false");
         if(branchValue->getUnsignedMax())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch condition " + STR(lc) + " " + bin_op->get_kind_text() + " " + STR(rc) + " == " + STR(branchValue) + ", false edge BB" + STR(branchBB->false_edge) + " to be removed");
            return branchBB->false_edge;
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch condition " + STR(lc) + " " + bin_op->get_kind_text() + " " + STR(rc) + " == " + STR(branchValue) + ", false edge BB" + STR(branchBB->false_edge) + " to be removed");
            return branchBB->true_edge;
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch condition has non-integer cst_node operands, skipping...");
      return bloc::EXIT_BLOCK_ID;
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch variable is a non-integer cst_node, skipping...");
   return bloc::EXIT_BLOCK_ID;
}

// ========================================================================== //
// TernaryOp
// ========================================================================== //
class TernaryOpNode : public OpNode
{
 private:
   // The first operand.
   VarNode* source1;
   // The second operand.
   VarNode* source2;
   // The third operand.
   VarNode* source3;
   // The opcode of the operation.
   kind opcode;
   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   RangeRef eval() const override;

 public:
   TernaryOpNode(const ValueRangeRef& intersect, VarNode* sink, const tree_nodeConstRef& inst, VarNode* source1, VarNode* source2, VarNode* source3, kind opcode);
   ~TernaryOpNode() override = default;
   TernaryOpNode(const TernaryOpNode&) = delete;
   TernaryOpNode(TernaryOpNode&&) = delete;
   TernaryOpNode& operator=(const TernaryOpNode&) = delete;
   TernaryOpNode& operator=(TernaryOpNode&&) = delete;

   // Methods for RTTI
   OperationId getValueId() const override
   {
      return OperationId::TernaryOpId;
   }
   static bool classof(TernaryOpNode const* /*unused*/)
   {
      return true;
   }
   static bool classof(OpNode const* BO)
   {
      return BO->getValueId() == OperationId::TernaryOpId;
   }

   /// Return the opcode of the operation.
   kind getOpcode() const
   {
      return opcode;
   }
   /// Returns the first operand of this operation.
   VarNode* getSource1() const
   {
      return source1;
   }
   /// Returns the second operand of this operation.
   VarNode* getSource2() const
   {
      return source2;
   }
   /// Returns the third operand of this operation.
   VarNode* getSource3() const
   {
      return source3;
   }
   std::vector<tree_nodeConstRef> getSources() const override
   {
      return {source1->getValue(), source2->getValue(), source3->getValue()};
   }

   void print(std::ostream& OS) const override;
   void printDot(std::ostream& OS) const override;

   static std::function<OpNode*(NodeContainer*)> opCtorGenerator(const tree_nodeConstRef&, unsigned int, const FunctionBehaviorConstRef&, const tree_managerConstRef&, const application_managerRef&);
};

// The ctor.
TernaryOpNode::TernaryOpNode(const ValueRangeRef& _intersect, VarNode* _sink, const tree_nodeConstRef& _inst, VarNode* _source1, VarNode* _source2, VarNode* _source3, kind _opcode)
    : OpNode(_intersect, _sink, _inst), source1(_source1), source2(_source2), source3(_source3), opcode(_opcode)
{
#if HAVE_ASSERTS
   const auto* ga = GetPointer<const gimple_assign>(GET_CONST_NODE(_inst));
   THROW_ASSERT(ga, "TernaryOp associated statement should be a gimple_assign " + GET_CONST_NODE(_inst)->ToString());
   const auto* I = GetPointer<const ternary_expr>(GET_CONST_NODE(ga->op1));
   THROW_ASSERT(I, "TernaryOp operator should be a ternary_expr");
   THROW_ASSERT(_sink->getBitWidth() >= _source2->getBitWidth(), "Operator bitwidth overflow (sink= " + STR(_sink->getBitWidth()) + ", op2= " + STR(_source2->getBitWidth()) + ")");
   THROW_ASSERT(_sink->getBitWidth() >= _source3->getBitWidth(), "Operator bitwidth overflow (sink= " + STR(_sink->getBitWidth()) + ", op3= " + STR(_source3->getBitWidth()) + ")");
#endif
}

RangeRef TernaryOpNode::eval() const
{
   const auto op1 = this->getSource1()->getRange();
   auto op2 = this->getSource2()->getRange();
   auto op3 = this->getSource3()->getRange();

   auto result = getRangeFor(getSink()->getValue(), Regular);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, ToString());

#ifndef NDEBUG
   if(enable_ternary)
   {
      //    #endif
      // only evaluate if all operands are Regular
      if((op1->isRegular() || op1->isAnti()) && (op2->isRegular() || op2->isAnti()) && (op3->isRegular() || op3->isAnti()))
      {
         if(this->getOpcode() == cond_expr_K)
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
               const auto BranchVar = branchOpRecurse(I->op0);
               std::vector<const struct binary_expr*> BranchConds;
               // Check if branch variable is correlated with op1 or op2
               if(GetPointer<const gimple_phi>(BranchVar) != nullptr)
               {
                  // TODO: find a way to propagate range from all phi edges when phi->res is one of the two result of the cond_expr
               }
               else if(const auto* BranchExpr = GetPointer<const binary_expr>(BranchVar))
               {
                  BranchConds.push_back(BranchExpr);
               }

               for(const auto* be : BranchConds)
               {
                  if(isCompare(be))
                  {
                     const auto& CondOp0 = be->op0;
                     const auto& CondOp1 = be->op1;
                     if(GET_CONST_NODE(CondOp0)->get_kind() == integer_cst_K || GET_CONST_NODE(CondOp1)->get_kind() == integer_cst_K)
                     {
                        const auto& variable = GET_CONST_NODE(CondOp0)->get_kind() == integer_cst_K ? CondOp1 : CondOp0;
                        const auto& constant = GET_CONST_NODE(CondOp0)->get_kind() == integer_cst_K ? CondOp0 : CondOp1;
                        const auto& opV1 = I->op1;
                        const auto& opV2 = I->op2;
                        if(GET_INDEX_CONST_NODE(variable) == GET_INDEX_CONST_NODE(opV1) || GET_INDEX_CONST_NODE(variable) == GET_INDEX_CONST_NODE(opV2))
                        {
                           const auto CR = getGIMPLE_range(constant);
                           THROW_ASSERT(CR->isConstant(), "Range from constant should be constant (" + GET_CONST_NODE(constant)->ToString() + " => " + CR->ToString() + ")");
                           kind pred = isSignedType(CondOp0) ? be->get_kind() : op_unsigned(be->get_kind());
                           kind swappred = op_swap(pred);

                           auto tmpT = (variable == CondOp0) ? Range::makeSatisfyingCmpRegion(pred, CR) : Range::makeSatisfyingCmpRegion(swappred, CR);
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
            result = getRangeFor(getSink()->getValue(), Empty);
         }
      }
      //    #ifndef NDEBUG
   }
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, result->ToString() + " = " + op1->ToString() + " ? " + op2->ToString() + " : " + op3->ToString());

   bool test = this->getIntersect()->getRange()->isFullSet();
   if(!test)
   {
      const auto aux = this->getIntersect()->getRange();
      auto _intersect = result->intersectWith(aux);
      if(!_intersect->isEmpty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "aux = " + aux->ToString() + " from " + getIntersect()->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "result = " + _intersect->ToString());
         result = _intersect;
      }
   }
   return result;
}

std::function<OpNode*(NodeContainer*)> TernaryOpNode::opCtorGenerator(const tree_nodeConstRef& stmt, unsigned int function_id, const FunctionBehaviorConstRef&, const tree_managerConstRef&, const application_managerRef&)
{
   const auto* assign = GetPointer<const gimple_assign>(GET_CONST_NODE(stmt));
   if(assign == nullptr)
   {
      return nullptr;
   }
   const auto* ter_op = GetPointer<const ternary_expr>(GET_CONST_NODE(assign->op1));
   if(ter_op == nullptr)
   {
      return nullptr;
   }
   return [stmt, assign, ter_op, function_id](NodeContainer* NC) {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "Analysing ternary operation " + ter_op->get_kind_text() + " " + assign->ToString());
      // Create the sink.
      VarNode* sink = NC->addVarNode(assign->op0, function_id);

      // Create the sources.
      VarNode* _source1 = NC->addVarNode(ter_op->op0, function_id);
      VarNode* _source2 = NC->addVarNode(ter_op->op1, function_id);
      VarNode* _source3 = NC->addVarNode(ter_op->op2, function_id);

      // Create the operation using the intersect to constrain sink's interval.
      auto BI = ValueRangeRef(new ValueRange(getGIMPLE_range(stmt)));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Added TernaryOp for " + ter_op->get_kind_text() + " with range " + BI->ToString());
      return new TernaryOpNode(BI, sink, stmt, _source1, _source2, _source3, ter_op->get_kind());
   };
}

void TernaryOpNode::print(std::ostream& OS) const
{
   OS << GET_CONST_NODE(getSink()->getValue())->ToString() << " = " << GET_CONST_NODE(getSource1()->getValue())->ToString() << " ? " << GET_CONST_NODE(getSource2()->getValue())->ToString() << " : " << GET_CONST_NODE(getSource3()->getValue())->ToString();
}

void TernaryOpNode::printDot(std::ostream& OS) const
{
   const char* quot = R"(")";
   std::string opcodeName = tree_node::GetString(this->getOpcode());
   OS << " " << quot << this << quot << R"( [label=")" << opcodeName << "\"]\n";

   const auto& V1 = this->getSource1()->getValue();
   if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V1)))
   {
      OS << " " << C->value << " -> " << quot << this << quot << "\n";
   }
   else
   {
      OS << " " << quot;
      printVarName(V1, OS);
      OS << quot << " -> " << quot << this << quot << "\n";
   }
   const auto& V2 = this->getSource2()->getValue();
   if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V2)))
   {
      OS << " " << C->value << " -> " << quot << this << quot << "\n";
   }
   else
   {
      OS << " " << quot;
      printVarName(V2, OS);
      OS << quot << " -> " << quot << this << quot << "\n";
   }

   const auto& V3 = this->getSource3()->getValue();
   if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V3)))
   {
      OS << " " << C->value << " -> " << quot << this << quot << "\n";
   }
   else
   {
      OS << " " << quot;
      printVarName(V3, OS);
      OS << quot << " -> " << quot << this << quot << "\n";
   }
   const auto& VS = this->getSink()->getValue();
   OS << " " << quot << this << quot << " -> " << quot;
   printVarName(VS, OS);
   OS << quot << "\n";
}

// ========================================================================== //
// LoadOp
// ========================================================================== //
class LoadOpNode : public OpNode
{
 private:
   /// reference to the memory access operand
   std::vector<const VarNode*> sources;
   RangeRef eval() const override;

 public:
   LoadOpNode(const ValueRangeRef& intersect, VarNode* sink, const tree_nodeConstRef& inst);
   ~LoadOpNode() override = default;
   LoadOpNode(const LoadOpNode&) = delete;
   LoadOpNode(LoadOpNode&&) = delete;
   LoadOpNode& operator=(const LoadOpNode&) = delete;
   LoadOpNode& operator=(LoadOpNode&&) = delete;

   // Methods for RTTI
   OperationId getValueId() const override
   {
      return OperationId::LoadOpId;
   }
   static bool classof(LoadOpNode const* /*unused*/)
   {
      return true;
   }
   static bool classof(OpNode const* BO)
   {
      return BO->getValueId() == OperationId::LoadOpId;
   }

   /// Add source to the vector of sources
   void addSource(const VarNode* newsrc)
   {
      sources.push_back(newsrc);
   }
   /// Return source identified by index
   const VarNode* getSource(size_t index) const
   {
      return sources[index];
   }
   /// return the number of sources
   size_t getNumSources() const
   {
      return sources.size();
   }
   std::vector<tree_nodeConstRef> getSources() const override
   {
      std::vector<tree_nodeConstRef> sourceTNs;
      for(const auto& s : sources)
      {
         sourceTNs.push_back(s->getValue());
      }
      return sourceTNs;
   }

   void print(std::ostream& OS) const override;
   void printDot(std::ostream& OS) const override;

   static std::function<OpNode*(NodeContainer*)> opCtorGenerator(const tree_nodeConstRef& stmt, unsigned int function_id, const FunctionBehaviorConstRef& FB, const tree_managerConstRef& TM, const application_managerRef& AppM);
};

LoadOpNode::LoadOpNode(const ValueRangeRef& _intersect, VarNode* _sink, const tree_nodeConstRef& _inst) : OpNode(_intersect, _sink, _inst)
{
}

RangeRef LoadOpNode::eval() const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, ToString());

#ifndef NDEBUG
   if(getNumSources() == 0 || !enable_load)
#else
   if(getNumSources() == 0)
#endif
   {
      THROW_ASSERT(getSink()->getBitWidth() == getIntersect()->getRange()->getBitWidth(),
                   "Sink (" + GET_CONST_NODE(getSink()->getValue())->ToString() + ") has bitwidth " + STR(getSink()->getBitWidth()) + " while intersect has bitwidth " + STR(getIntersect()->getRange()->getBitWidth()));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "= " + getIntersect()->getRange()->ToString());
      return RangeRef(getIntersect()->getRange()->clone());
   }

   // Iterate over the sources of the load
   auto result = getRangeFor(getSink()->getValue(), Empty);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   for(const VarNode* varNode : sources)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "->" + varNode->getRange()->ToString() + " " + varNode->ToString());
      result = result->unionWith(varNode->getRange());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--= " + result->ToString());

   bool test = this->getIntersect()->getRange()->isFullSet();
   if(!test)
   {
      const auto aux = this->getIntersect()->getRange();
      auto _intersect = result->intersectWith(aux);
      if(!_intersect->isEmpty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "aux = " + aux->ToString() + " from " + getIntersect()->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "result = " + _intersect->ToString());
         result = _intersect;
      }
   }
   return result;
}

static RangeRef constructor_range(const tree_managerConstRef TM, const tree_nodeConstRef tn, const RangeConstRef init)
{
   THROW_ASSERT(tn->get_kind() == constructor_K, "tn is not constructor node");
   const auto* c = GetPointer<const constructor>(tn);
   std::vector<unsigned int> array_dims;
   unsigned int elements_bitsize;
   tree_helper::get_array_dim_and_bitsize(TM, GET_INDEX_CONST_NODE(c->type), array_dims, elements_bitsize);
   unsigned int initialized_elements = 0;
   auto ctor_range = RangeRef(init->clone());
   for(const auto& i : c->list_of_idx_valu)
   {
      const auto el = GET_CONST_NODE(i.second);
      THROW_ASSERT(el, "unexpected condition");

      if(el->get_kind() == constructor_K && GetPointer<array_type>(GET_CONST_NODE(GetPointer<constructor>(el)->type)))
      {
         THROW_ASSERT(array_dims.size() > 1 || GET_CONST_NODE(c->type)->get_kind() == record_type_K, "invalid nested constructors:" + tn->ToString() + " " + STR(array_dims.size()));
         ctor_range = ctor_range->unionWith(constructor_range(TM, el, ctor_range));
      }
      else
      {
         const auto init_range = getGIMPLE_range(el);
         if(init_range->getBitWidth() > static_cast<Range::bw_t>(elements_bitsize) || init_range->isReal() != ctor_range->isReal())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Initializer value not compliant " + el->ToString());
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Initializer value is " + el->ToString());
            ctor_range = ctor_range->unionWith(init_range);
         }
      }
      initialized_elements++;
   }
   if(initialized_elements < array_dims.front())
   {
      if(ctor_range->isReal())
      {
         ctor_range = ctor_range->unionWith(RangeRef(new RealRange(RangeRef(new Range(Regular, static_cast<Range::bw_t>(elements_bitsize), 0, 0)))));
      }
      else
      {
         ctor_range = ctor_range->unionWith(RangeRef(new Range(Regular, static_cast<Range::bw_t>(elements_bitsize), 0, 0)));
      }
   }
   return ctor_range;
}

std::function<OpNode*(NodeContainer*)> LoadOpNode::opCtorGenerator(const tree_nodeConstRef& stmt, unsigned int function_id, const FunctionBehaviorConstRef& FB, const tree_managerConstRef& TM, const application_managerRef& AppM)
{
   const auto* ga = GetPointer<const gimple_assign>(GET_CONST_NODE(stmt));
   if(ga == nullptr)
   {
      return nullptr;
   }
   if(!tree_helper::IsLoad(TM, stmt, FB->get_function_mem()))
   {
      return nullptr;
   }
   return [stmt, ga, function_id, FB, TM, AppM](NodeContainer* NC) {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "Analysing load operation " + ga->ToString());
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "-->");
      const auto bw = getGIMPLE_BW(ga->op0);
      VarNode* sink = NC->addVarNode(ga->op0, function_id);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "Sink variable is " + GET_CONST_NODE(ga->op0)->get_kind_text() + " (size = " + STR(bw) + ")");

      auto intersection = getRangeFor(sink->getValue(), Empty);
      if(GET_NODE(ga->op1)->get_kind() == array_ref_K || GET_NODE(ga->op1)->get_kind() == mem_ref_K || GET_NODE(ga->op1)->get_kind() == target_mem_ref_K || GET_NODE(ga->op1)->get_kind() == target_mem_ref461_K || GET_NODE(ga->op1)->get_kind() == var_decl_K)
      {
         unsigned int base_index = tree_helper::get_base_index(TM, GET_INDEX_NODE(ga->op1));
         const auto* hm = GetPointer<HLS_manager>(AppM);
         if(base_index && AppM->get_written_objects().find(base_index) == AppM->get_written_objects().end() && hm && hm->Rmem && FB->is_variable_mem(base_index) && hm->Rmem->is_sds_var(base_index))
         {
            const auto* vd = GetPointer<const var_decl>(TM->get_tree_node_const(base_index));
            if(vd && vd->init)
            {
               if(GET_NODE(vd->init)->get_kind() == constructor_K)
               {
                  intersection = constructor_range(TM, GET_CONST_NODE(vd->init), intersection);
               }
               else if(GetPointer<const cst_node>(GET_CONST_NODE(vd->init)))
               {
                  auto init_range = getGIMPLE_range(GET_NODE(vd->init));
                  if(intersection->isReal() && !init_range->isReal())
                  {
                     init_range.reset(new RealRange(init_range));
                  }
                  else if(!intersection->isReal() && init_range->isReal())
                  {
                     init_range = std::static_pointer_cast<RealRange>(init_range)->getRange();
                  }
                  if(init_range->getBitWidth() != bw)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Initializer value not compliant " + GET_NODE(vd->init)->ToString());
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Initializer value is " + GET_NODE(vd->init)->ToString());
                     intersection = init_range;
                  }
               }
            }
         }
         if(base_index && AppM->get_written_objects().find(base_index) != AppM->get_written_objects().end() && hm && hm->Rmem && FB->is_variable_mem(base_index) && hm->Rmem->is_private_memory(base_index) && hm->Rmem->is_sds_var(base_index))
         {
            const auto* vd = GetPointer<const var_decl>(TM->get_tree_node_const(base_index));
            if(vd && vd->init)
            {
               if(GET_NODE(vd->init)->get_kind() == constructor_K)
               {
                  intersection = constructor_range(TM, GET_CONST_NODE(vd->init), intersection);
               }
               else if(GetPointer<const cst_node>(GET_CONST_NODE(vd->init)))
               {
                  auto init_range = getGIMPLE_range(GET_CONST_NODE(vd->init));
                  if(intersection->isReal() && !init_range->isReal())
                  {
                     init_range.reset(new RealRange(init_range));
                  }
                  else if(!intersection->isReal() && init_range->isReal())
                  {
                     init_range = std::static_pointer_cast<RealRange>(init_range)->getRange();
                  }
                  if(init_range->getBitWidth() != bw)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Initializer value not compliant " + GET_NODE(vd->init)->ToString());
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Initializer value is " + GET_NODE(vd->init)->ToString());
                     intersection = init_range;
                  }
               }
            }
            else
            {
               if(intersection->isReal())
               {
                  intersection = RangeRef(new RealRange(RangeRef(new Range(Regular, bw, 0, 0))));
               }
               else
               {
                  intersection = RangeRef(new Range(Regular, bw, 0, 0));
               }
            }
            for(const auto& cur_var : hm->Rmem->get_source_values(base_index))
            {
               const auto cur_node = TM->get_tree_node_const(cur_var);
               THROW_ASSERT(cur_node, "");
               auto init_range = getGIMPLE_range(cur_node);
               if(intersection->isReal() && !init_range->isReal())
               {
                  init_range.reset(new RealRange(init_range));
               }
               else if(!intersection->isReal() && init_range->isReal())
               {
                  init_range = std::static_pointer_cast<RealRange>(init_range)->getRange();
               }
               if(init_range->getBitWidth() != bw)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Initializer value not compliant " + cur_node->ToString());
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "---Initializer value is " + cur_node->ToString());
                  intersection = intersection->unionWith(init_range);
               }
            }
         }
      }
      if(intersection->isEmpty())
      {
         intersection = getGIMPLE_range(stmt);
      }
      THROW_ASSERT(intersection->getBitWidth() <= bw, "Pointed variables range should have bitwidth contained in sink bitwidth");
      THROW_ASSERT(!intersection->isEmpty(), "Variable range should not be empty");
      if(intersection->getBitWidth() < bw)
      {
         intersection = intersection->zextOrTrunc(bw);
      }
      auto BI = ValueRangeRef(new ValueRange(intersection));
#ifndef NDEBUG
      if(!enable_load)
      {
         BI = ValueRangeRef(new ValueRange(getGIMPLE_range(stmt)));
      }
#endif
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, NodeContainer::debug_level, "<--Added LoadOp with range " + BI->ToString());
      return new LoadOpNode(BI, sink, stmt);
   };
}

void LoadOpNode::print(std::ostream& OS) const
{
   OS << GET_CONST_NODE(getSink()->getValue())->ToString() << " = LOAD()";
}

void LoadOpNode::printDot(std::ostream& OS) const
{
   const char* quot = R"(")";
   OS << " " << quot << this << quot << R"( [label=")";
   OS << "LoadOp\"]\n";

   for(auto src : sources)
   {
      const auto& V = src->getValue();
      if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V)))
      {
         OS << " " << C->value << " -> " << quot << this << quot << "\n";
      }
      else
      {
         OS << " " << quot;
         printVarName(V, OS);
         OS << quot << " -> " << quot << this << quot << "\n";
      }
   }

   const auto& VS = this->getSink()->getValue();
   OS << " " << quot << this << quot << " -> " << quot;
   printVarName(VS, OS);
   OS << quot << "\n";
}

const std::vector<std::function<std::function<OpNode*(NodeContainer*)>(const tree_nodeConstRef&, unsigned int, const FunctionBehaviorConstRef&, const tree_managerConstRef&, const application_managerRef&)>> NodeContainer::_opCtorGenerators = {
    LoadOpNode::opCtorGenerator, UnaryOpNode::opCtorGenerator, BinaryOpNode::opCtorGenerator, PhiOpNode::opCtorGenerator, SigmaOpNode::opCtorGenerator, TernaryOpNode::opCtorGenerator};

// ========================================================================== //
// ControlDep
// ========================================================================== //
/// Specific type of OpNode used in Nuutila's strongly connected
/// components algorithm.
class ControlDepNode : public OpNode
{
 private:
   VarNode* source;
   RangeRef eval() const override;

 public:
   ControlDepNode(VarNode* sink, VarNode* source);
   ~ControlDepNode() override = default;
   ControlDepNode(const ControlDepNode&) = delete;
   ControlDepNode(ControlDepNode&&) = delete;
   ControlDepNode& operator=(const ControlDepNode&) = delete;
   ControlDepNode& operator=(ControlDepNode&&) = delete;

   // Methods for RTTI
   OperationId getValueId() const override
   {
      return OperationId::ControlDepId;
   }
   static bool classof(ControlDepNode const* /*unused*/)
   {
      return true;
   }
   static bool classof(OpNode const* BO)
   {
      return BO->getValueId() == OperationId::ControlDepId;
   }

   /// Returns the source of the operation.
   VarNode* getSource() const
   {
      return source;
   }
   std::vector<tree_nodeConstRef> getSources() const override
   {
      return {source->getValue()};
   }

   void print(std::ostream& OS) const override;
   void printDot(std::ostream& OS) const override;
};

ControlDepNode::ControlDepNode(VarNode* _sink, VarNode* _source) : OpNode(ValueRangeRef(new ValueRange(_sink->getMaxRange())), _sink, nullptr), source(_source)
{
}

RangeRef ControlDepNode::eval() const
{
   return RangeRef(new Range(Regular, Range::MAX_BIT_INT));
}

void ControlDepNode::print(std::ostream& /*OS*/) const
{
}

void ControlDepNode::printDot(std::ostream& /*OS*/) const
{
}

// ========================================================================== //
// Nuutila
// ========================================================================== //

// A map from variables to the operations where these
// variables are present as bounds
using SymbMap = std::map<tree_nodeConstRef, CustomSet<OpNode*>, tree_reindexCompare>;

class Nuutila
{
#ifndef NDEBUG
   int debug_level;
#ifdef SCC_DEBUG
   bool checkWorklist() const;
   bool checkComponents() const;
   bool checkTopologicalSort(const UseMap& useMap) const;
   bool hasEdge(const CustomSet<VarNode*>& componentFrom, const CustomSet<VarNode*>& componentTo, const UseMap& useMap) const;
#endif
#endif

   const VarNodes& variables;
   int index;
   std::map<tree_nodeConstRef, int, tree_reindexCompare> dfs;
   std::map<tree_nodeConstRef, tree_nodeConstRef, tree_reindexCompare> root;
   std::set<tree_nodeConstRef, tree_reindexCompare> inComponent;
   std::map<tree_nodeConstRef, CustomSet<VarNode*>, tree_reindexCompare> components;
   std::deque<tree_nodeConstRef> worklist;

 public:
   Nuutila(const VarNodes& varNodes, UseMap& useMap, const SymbMap& symbMap
#ifndef NDEBUG
           ,
           int _debug_level
#endif
   );
   ~Nuutila() = default;
   Nuutila(const Nuutila&) = delete;
   Nuutila(Nuutila&&) = delete;
   Nuutila& operator=(const Nuutila&) = delete;
   Nuutila& operator=(Nuutila&&) = delete;

   void addControlDependenceEdges(UseMap& useMap, const SymbMap& symbMap, const VarNodes& vars);
   void delControlDependenceEdges(UseMap& useMap);
   void visit(const tree_nodeConstRef& V, std::stack<tree_nodeConstRef>& stack, const UseMap& useMap);

   const CustomSet<VarNode*>& getComponent(const tree_nodeConstRef& n) const
   {
      THROW_ASSERT(static_cast<bool>(components.count(n)), "Required component not found (" + GET_CONST_NODE(n)->ToString() + ")");
      return components.at(n);
   }

   using iterator = std::deque<tree_nodeConstRef>::reverse_iterator;
   using const_iterator = std::deque<tree_nodeConstRef>::const_reverse_iterator;
   iterator begin()
   {
      return worklist.rbegin();
   }
   const_iterator cbegin() const
   {
      return worklist.crbegin();
   }
   iterator end()
   {
      return worklist.rend();
   }
   const_iterator cend() const
   {
      return worklist.crend();
   }
};

/*
 *  Finds the strongly connected components in the constraint graph formed
 * by Variables and UseMap. The class receives the map of futures to insert
 * the control dependence edges in the constraint graph. These edges are removed
 * after the class is done computing the SCCs.
 */
Nuutila::Nuutila(const VarNodes& varNodes, UseMap& useMap, const SymbMap& symbMap
#ifndef NDEBUG
                 ,
                 int _debug_level)
    : debug_level(_debug_level),
#else
                 )
    :
#endif
      variables(varNodes)
{
   // Copy structures
   this->index = 0;

   // Iterate over all varnodes of the constraint graph
   for(const auto& vNode : varNodes)
   {
      // Initialize DFS control variable for each Value in the graph
      dfs[vNode.first] = -1;
   }
   addControlDependenceEdges(useMap, symbMap, varNodes);
   // Iterate again over all varnodes of the constraint graph
   for(const auto& vNode : varNodes)
   {
      // If the Value has not been visited yet, call visit for him
      if(dfs[vNode.first] < 0)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Start visit from " + GET_CONST_NODE(vNode.first)->ToString());
         std::stack<tree_nodeConstRef> pilha;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
         visit(vNode.first, pilha, useMap);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
   }
   delControlDependenceEdges(useMap);

#ifdef SCC_DEBUG
   THROW_ASSERT(checkWorklist(), "An inconsistency in SCC worklist have been found");
   THROW_ASSERT(checkComponents(), "A component has been used more than once");
   THROW_ASSERT(checkTopologicalSort(useMap), "Topological sort is incorrect");
#endif
}

/*
 *  Adds the edges that ensure that we solve a future before fixing its
 *  interval. I have created a new class: ControlDep edges, to represent
 *  the control dependencies. In this way, in order to delete these edges,
 *  one just need to go over the map of uses removing every instance of the
 *  ControlDep class.
 */
void Nuutila::addControlDependenceEdges(UseMap& useMap, const SymbMap& symbMap, const VarNodes& vars)
{
   for(const auto& varOps : symbMap)
   {
      for(const auto& op : varOps.second)
      {
         THROW_ASSERT(static_cast<bool>(vars.count(varOps.first)), "Variable should be stored in VarNodes map");
         auto* source = vars.at(varOps.first);
         auto* cdedge = new ControlDepNode(op->getSink(), source);
         useMap[varOps.first].insert(cdedge);
      }
   }
}

/*
 *  Removes the control dependence edges from the constraint graph.
 */
void Nuutila::delControlDependenceEdges(UseMap& useMap)
{
   for(auto& varOps : useMap)
   {
      std::deque<ControlDepNode*> cds;
      for(auto sit : varOps.second)
      {
         if(auto* cd = GetOp<ControlDepNode>(sit))
         {
            cds.push_back(cd);
         }
      }

      for(auto* cd : cds)
      {
#ifndef NDEBUG
         // Add pseudo edge to the string
         const auto& V = cd->getSource()->getValue();
         if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(V)))
         {
            pseudoEdgesString << " " << C->value << " -> ";
         }
         else
         {
            pseudoEdgesString << " " << '"';
            printVarName(V, pseudoEdgesString);
            pseudoEdgesString << '"' << " -> ";
         }
         const auto& VS = cd->getSink()->getValue();
         pseudoEdgesString << '"';
         printVarName(VS, pseudoEdgesString);
         pseudoEdgesString << '"';
         pseudoEdgesString << " [style=dashed]\n";
#endif
         // Remove pseudo edge from the map
         varOps.second.erase(cd);
         delete cd;
      }
   }
}

/*
 *  Finds SCCs using Nuutila's algorithm. This algorithm is divided in
 *  two parts. The first calls the recursive visit procedure on every node
 *  in the constraint graph. The second phase revisits these nodes,
 *  grouping them in components.
 */
void Nuutila::visit(const tree_nodeConstRef& V, std::stack<tree_nodeConstRef>& stack, const UseMap& useMap)
{
   dfs[V] = index;
   ++index;
   root[V] = V;

   // Visit every node defined in an instruction that uses V
   for(const auto& op : useMap.at(V))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, op->ToString());
      const auto& sink = op->getSink()->getValue();
      if(dfs[sink] < 0)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->" + GET_CONST_NODE(sink)->ToString());
         visit(sink, stack, useMap);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      if((!static_cast<bool>(inComponent.count(sink))) && (dfs[root[V]] >= dfs[root[sink]]))
      {
         root[V] = root[sink];
      }
   }

   // The second phase of the algorithm assigns components to stacked nodes
   if(GET_INDEX_CONST_NODE(root[V]) == GET_INDEX_CONST_NODE(V))
   {
      // Neither the worklist nor the map of components is part of Nuutila's
      // original algorithm. We are using these data structures to get a
      // topological ordering of the SCCs without having to go over the root
      // list once more.
      worklist.push_back(V);
      components[V].insert(variables.at(V));
      inComponent.insert(V);
      while(!stack.empty() && (dfs[stack.top()] > dfs[V]))
      {
         auto node = stack.top();
         stack.pop();
         inComponent.insert(node);
         components[V].insert(variables.at(node));
      }
   }
   else
   {
      stack.push(V);
   }
}

#ifdef SCC_DEBUG
bool Nuutila::checkWorklist() const
{
   bool consistent = true;
   for(auto nit = cbegin(), nend = cend(); nit != nend;)
   {
      auto v1 = *nit;
      for(const auto& v2 : boost::make_iterator_range(++nit, cend()))
      {
         if(GET_INDEX_CONST_NODE(v1) == GET_INDEX_CONST_NODE(v2))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "[Nuutila::checkWorklist] Duplicated entry in worklist " + GET_CONST_NODE(v1)->ToString());
            consistent = false;
         }
      }
   }
   return consistent;
}

bool Nuutila::checkComponents() const
{
   bool isConsistent = true;
   for(auto n1it = cbegin(), n1end = cend(); n1it != n1end;)
   {
      const auto& component1 = components.at(*n1it);
      for(const auto& n2 : boost::make_iterator_range(++n1it, cend()))
      {
         const auto& component2 = components.at(n2);
         if(&component1 == &component2)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "[Nuutila::checkComponent] Component [" + STR(&component1) + ", " + STR(component1.size()) + "]");
            isConsistent = false;
         }
      }
   }
   return isConsistent;
}

/**
 * Check if a component has an edge to another component
 */
bool Nuutila::hasEdge(const CustomSet<VarNode*>& componentFrom, const CustomSet<VarNode*>& componentTo, const UseMap& useMap) const
{
   for(const auto& v : componentFrom)
   {
      const auto& source = v->getValue();
      THROW_ASSERT(static_cast<bool>(useMap.count(source)), "Variable should be in use map");
      for(const auto& op : useMap.at(source))
      {
         if(static_cast<bool>(componentTo.count(op->getSink())))
         {
            return true;
         }
      }
   }
   return false;
}

bool Nuutila::checkTopologicalSort(const UseMap& useMap) const
{
   bool isConsistent = true;
   for(auto n1it = cbegin(), nend = cend(); n1it != nend; ++n1it)
   {
      const auto& curr_component = components.at(*n1it);
      // check if this component points to another component that has already
      // been visited
      for(const auto& n2 : boost::make_iterator_range(cbegin(), n1it))
      {
         const auto& prev_component = components.at(n2);
         if(hasEdge(curr_component, prev_component, useMap))
         {
            isConsistent = false;
         }
      }
   }
   return isConsistent;
}

#endif

// ========================================================================== //
// Meet
// ========================================================================== //
class Meet
{
 private:
   static const APInt& getFirstGreaterFromVector(const std::vector<APInt>& constantvector, const APInt& val);
   static const APInt& getFirstLessFromVector(const std::vector<APInt>& constantvector, const APInt& val);

 public:
   static bool widen(OpNode* op, const std::vector<APInt>& constantvector);
   static bool narrow(OpNode* op, const std::vector<APInt>& constantvector);
   static bool crop(OpNode* op);
   static bool growth(OpNode* op);
   static bool fixed(OpNode* op);

#ifndef NDEBUG
   static int debug_level;
#endif
};

#ifndef NDEBUG
int Meet::debug_level = DEBUG_LEVEL_NONE;
#endif

/*
 * Get the first constant from vector greater than val
 */
const APInt& Meet::getFirstGreaterFromVector(const std::vector<APInt>& constantvector, const APInt& val)
{
   for(const auto& vapint : constantvector)
   {
      if(vapint >= val)
      {
         return vapint;
      }
   }
   return Range::Max;
}

/*
 * Get the first constant from vector less than val
 */
const APInt& Meet::getFirstLessFromVector(const std::vector<APInt>& constantvector, const APInt& val)
{
   for(auto vit = constantvector.rbegin(), vend = constantvector.rend(); vit != vend; ++vit)
   {
      const auto& vapint = *vit;
      if(vapint <= val)
      {
         return vapint;
      }
   }
   return Range::Min;
}

bool Meet::fixed(OpNode* op)
{
   const auto oldInterval = op->getSink()->getRange();
   const auto newInterval = op->eval();

   op->getSink()->setRange(newInterval);
   if(op->getInstruction())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "FIXED::@" + STR(GET_INDEX_CONST_NODE(op->getInstruction())) + ": " + oldInterval->ToString() + " -> " + newInterval->ToString());
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "FIXED::%artificial phi : " + oldInterval->ToString() + " -> " + newInterval->ToString());
   }
   return !oldInterval->isSameRange(newInterval);
}

/// This is the meet operator of the growth analysis. The growth analysis
/// will change the bounds of each variable, if necessary. Initially, each
/// variable is bound to either the undefined interval, e.g. [., .], or to
/// a constant interval, e.g., [3, 15]. After this analysis runs, there will
/// be no undefined interval. Each variable will be either bound to a
/// constant interval, or to [-, c], or to [c, +], or to [-, +].
bool Meet::widen(OpNode* op, const std::vector<APInt>& constantvector)
{
   const auto oldRange = op->getSink()->getRange();
   const auto newRange = op->eval();

   auto intervalWiden = [&](RangeConstRef oldInterval, RangeConstRef newInterval) {
      const auto bw = oldInterval->getBitWidth();
      if(oldInterval->isUnknown() || oldInterval->isEmpty() || oldInterval->isAnti() || newInterval->isEmpty() || newInterval->isAnti())
      {
         if(oldInterval->isAnti() && newInterval->isAnti() && !newInterval->isSameRange(oldInterval))
         {
            const auto oldAnti = oldInterval->getAnti();
            const auto newAnti = newInterval->getAnti();
            const auto& oldLower = oldAnti->getLower();
            const auto& oldUpper = oldAnti->getUpper();
            const auto& newLower = newAnti->getLower();
            const auto& newUpper = newAnti->getUpper();
            const auto& nlconstant = getFirstGreaterFromVector(constantvector, newLower);
            const auto& nuconstant = getFirstLessFromVector(constantvector, newUpper);

            if(newLower > oldLower || newUpper < oldUpper)
            {
               const auto& l = newLower > oldLower ? nlconstant : oldLower;
               const auto& u = newUpper < oldUpper ? nuconstant : oldUpper;
               if(l > u)
               {
                  return RangeRef(new Range(Regular, bw));
               }
               return RangeRef(new Range(Anti, bw, l, u));
            }
         }
         else
         {
            // Sometimes sigma operation could cause confusion after maximum widening has been reached and generate loops
            if(!oldInterval->isUnknown() && oldInterval->isFullSet() && newInterval->isAnti())
            {
               return RangeRef(oldInterval->clone());
            }
            if(oldInterval->isRegular() && newInterval->isAnti())
            {
               return oldInterval->unionWith(newInterval);
            }
            return RangeRef(newInterval->clone());
         }
      }
      else
      {
         const auto& oldLower = oldInterval->getLower();
         const auto& oldUpper = oldInterval->getUpper();
         const auto& newLower = newInterval->getLower();
         const auto& newUpper = newInterval->getUpper();

         // Jump-set
         const auto& nlconstant = getFirstLessFromVector(constantvector, newLower);
         const auto& nuconstant = getFirstGreaterFromVector(constantvector, newUpper);

         if(newLower < oldLower || newUpper > oldUpper)
         {
            return RangeRef(new Range(Regular, bw, newLower < oldLower ? nlconstant : oldLower, newUpper > oldUpper ? nuconstant : oldUpper));
         }
      }
      //    THROW_UNREACHABLE("Meet::widen unreachable state");
      return RangeRef(oldInterval->clone());
   };

   if(oldRange->isReal())
   {
      THROW_ASSERT(newRange->isReal(), "Real range should not change type");
      const auto oldRR = RefcountCast<const RealRange>(oldRange);
      const auto newRR = RefcountCast<const RealRange>(newRange);
      RangeConstRef oldIntervals[] = {oldRR->getSign(), oldRR->getExponent(), oldRR->getSignificand()};
      RangeConstRef newIntervals[] = {newRR->getSign(), newRR->getExponent(), newRR->getSignificand()};
      for(auto i = 0; i < 3; ++i)
      {
         newIntervals[i] = intervalWiden(oldIntervals[i], newIntervals[i]);
      }
      op->getSink()->setRange(RangeRef(new RealRange(newIntervals[0], newIntervals[1], newIntervals[2])));
   }
   else
   {
      const auto widen = intervalWiden(oldRange, newRange);
      //    THROW_ASSERT(oldRange->getSpan() <= widen->getSpan(), "Widening should produce bigger range: " + oldRange->ToString() + " > " + widen->ToString());
      op->getSink()->setRange(widen);
   }

   const auto sinkRange = op->getSink()->getRange();

   if(op->getInstruction())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "WIDEN::@" + STR(GET_INDEX_CONST_NODE(op->getInstruction())) + ": " + oldRange->ToString() + " -> " + newRange->ToString() + " -> " + sinkRange->ToString());
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "WIDEN::%artificial phi : " + oldRange->ToString() + " -> " + newRange->ToString() + " -> " + sinkRange->ToString());
   }
   return !oldRange->isSameRange(sinkRange);
}

bool Meet::growth(OpNode* op)
{
   const auto oldRange = op->getSink()->getRange();
   const auto newRange = op->eval();

   auto intervalGrowth = [](RangeConstRef oldInterval, RangeConstRef newInterval) {
      if(oldInterval->isUnknown() || oldInterval->isEmpty() || oldInterval->isAnti() || newInterval->isEmpty() || newInterval->isAnti())
      {
         return RangeRef(newInterval->clone());
      }
      else
      {
         auto bw = oldInterval->getBitWidth();
         const auto& oldLower = oldInterval->getLower();
         const auto& oldUpper = oldInterval->getUpper();
         const auto& newLower = newInterval->getLower();
         const auto& newUpper = newInterval->getUpper();

         if(newLower < oldLower || newUpper > oldUpper)
         {
            return RangeRef(new Range(Regular, bw, newLower < oldLower ? Range::Min : oldLower, newUpper > oldUpper ? Range::Max : oldUpper));
         }
      }
      //    THROW_UNREACHABLE("Meet::growth unreachable state");
      return RangeRef(oldInterval->clone());
   };

   if(oldRange->isReal())
   {
      THROW_ASSERT(newRange->isReal(), "Real range should not change type");
      const auto oldRR = std::static_pointer_cast<const RealRange>(oldRange);
      const auto newRR = std::static_pointer_cast<const RealRange>(newRange);
      RangeConstRef oldIntervals[] = {oldRR->getSign(), oldRR->getExponent(), oldRR->getSignificand()};
      RangeConstRef newIntervals[] = {newRR->getSign(), newRR->getExponent(), newRR->getSignificand()};
      for(auto i = 0; i < 3; ++i)
      {
         newIntervals[i] = intervalGrowth(oldIntervals[i], newIntervals[i]);
      }
      op->getSink()->setRange(RangeRef(new RealRange(newIntervals[0], newIntervals[1], newIntervals[2])));
   }
   else
   {
      op->getSink()->setRange(intervalGrowth(oldRange, newRange));
   }

   const auto sinkRange = op->getSink()->getRange();
   if(op->getInstruction())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "GROWTH::@" + STR(GET_INDEX_CONST_NODE(op->getInstruction())) + ": " + oldRange->ToString() + " -> " + sinkRange->ToString());
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "GROWTH::%artificial phi : " + oldRange->ToString() + " -> " + sinkRange->ToString());
   }
   return !oldRange->isSameRange(sinkRange);
}

/// This is the meet operator of the cropping analysis. Whereas the growth
/// analysis expands the bounds of each variable, regardless of intersections
/// in the constraint graph, the cropping analysis shrinks these bounds back
/// to ranges that respect the intersections.
bool Meet::narrow(OpNode* op, const std::vector<APInt>& constantvector)
{
   const auto oldRange = op->getSink()->getRange();
   const auto newRange = op->eval();

   auto intervalNarrow = [&](RangeConstRef oldInterval, RangeConstRef newInterval) {
      if(newInterval->isConstant())
      {
         return RangeRef(newInterval->clone());
      }
      const auto bw = oldInterval->getBitWidth();
      if(oldInterval->isAnti() || newInterval->isAnti() || oldInterval->isEmpty() || newInterval->isEmpty())
      {
         if(oldInterval->isAnti() && newInterval->isAnti() && !newInterval->isSameRange(oldInterval))
         {
            const auto oldAnti = oldInterval->getAnti();
            const auto newAnti = newInterval->getAnti();
            const auto& oldLower = oldAnti->getLower();
            const auto& oldUpper = oldAnti->getUpper();
            const auto& newLower = newAnti->getLower();
            const auto& newUpper = newAnti->getUpper();
            const auto& nlconstant = getFirstGreaterFromVector(constantvector, newLower);
            const auto& nuconstant = getFirstLessFromVector(constantvector, newUpper);

            if(oldLower < nlconstant && oldUpper > nuconstant)
            {
               if(nlconstant <= nuconstant)
               {
                  return RangeRef(new Range(Anti, bw, nlconstant, nuconstant));
               }
               return RangeRef(new Range(Regular, bw));
            }
            if(oldLower < nlconstant)
            {
               return RangeRef(new Range(Anti, bw, nlconstant, oldUpper));
            }
            if(oldUpper > nuconstant)
            {
               return RangeRef(new Range(Anti, bw, oldLower, nuconstant));
            }
         }
         else if(newInterval->isUnknown() || !newInterval->isFullSet())
         {
            return RangeRef(newInterval->clone());
         }
      }
      else
      {
         const auto& oLower = oldInterval->isFullSet() ? Range::Min : oldInterval->getLower();
         const auto& oUpper = oldInterval->isFullSet() ? Range::Max : oldInterval->getUpper();
         const auto& nLower = newInterval->isFullSet() ? Range::Min : newInterval->getLower();
         const auto& nUpper = newInterval->isFullSet() ? Range::Max : newInterval->getUpper();
         auto sinkInterval = RangeRef(oldInterval->clone());
         if((oLower == Range::Min) && (nLower != Range::Min))
         {
            sinkInterval = RangeRef(new Range(Regular, bw, nLower, oUpper));
         }
         else if(nLower < oLower)
         {
            sinkInterval = RangeRef(new Range(Regular, bw, nLower, oUpper));
         }
         if(!sinkInterval->isAnti())
         {
            if((oUpper == Range::Max) && (nUpper != Range::Max))
            {
               sinkInterval = RangeRef(new Range(Regular, bw, sinkInterval->getLower(), nUpper));
            }
            else if(oUpper < nUpper)
            {
               sinkInterval = RangeRef(new Range(Regular, bw, sinkInterval->getLower(), nUpper));
            }
         }
         return sinkInterval;
      }
      return RangeRef(oldInterval->clone());
   };

   if(oldRange->isReal())
   {
      THROW_ASSERT(newRange->isReal(), "Real range should not change type");
      const auto oldRR = std::static_pointer_cast<const RealRange>(oldRange);
      const auto newRR = std::static_pointer_cast<const RealRange>(newRange);
      RangeConstRef oldIntervals[] = {oldRR->getSign(), oldRR->getExponent(), oldRR->getSignificand()};
      RangeConstRef newIntervals[] = {newRR->getSign(), newRR->getExponent(), newRR->getSignificand()};
      for(auto i = 0; i < 3; ++i)
      {
         const auto narrow = intervalNarrow(oldIntervals[i], newIntervals[i]);
         //    THROW_ASSERT(oldIntervals[i]->getSpan() >= narrow->getSpan(), "Narrowing should produce smaller range: " + oldIntervals[i]->ToString() + " < " + narrow->ToString());
         newIntervals[i] = narrow;
      }
      op->getSink()->setRange(RangeRef(new RealRange(newIntervals[0], newIntervals[1], newIntervals[2])));
   }
   else
   {
      const auto narrow = intervalNarrow(oldRange, newRange);
      //    THROW_ASSERT(oldRange->getSpan() >= narrow->getSpan(), "Narrowing should produce smaller range: " + oldRange->ToString() + " < " + narrow->ToString());
      op->getSink()->setRange(narrow);
   }

   const auto sinkRange = op->getSink()->getRange();
   if(op->getInstruction())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "NARROW::@" + STR(GET_INDEX_CONST_NODE(op->getInstruction())) + ": " + oldRange->ToString() + " -> " + sinkRange->ToString());
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "NARROW::%artificial phi : " + oldRange->ToString() + " -> " + sinkRange->ToString());
   }
   return !oldRange->isSameRange(sinkRange);
}

bool Meet::crop(OpNode* op)
{
   const auto oldRange = op->getSink()->getRange();
   const auto newRange = op->eval();
   const char _abstractState = op->getSink()->getAbstractState();

   auto intervalCrop = [](RangeConstRef oldInterval, RangeConstRef newInterval, char abstractState) {
      if(oldInterval->isAnti() || newInterval->isAnti() || oldInterval->isEmpty() || newInterval->isEmpty())
      {
         return RangeRef(newInterval->clone());
      }
      else
      {
         const auto bw = oldInterval->getBitWidth();
         if((abstractState == '-' || abstractState == '?') && (newInterval->getLower() > oldInterval->getLower()))
         {
            return RangeRef(new Range(Regular, bw, newInterval->getLower(), oldInterval->getUpper()));
         }

         if((abstractState == '+' || abstractState == '?') && (newInterval->getUpper() < oldInterval->getUpper()))
         {
            return RangeRef(new Range(Regular, bw, oldInterval->getLower(), newInterval->getUpper()));
         }
         return RangeRef(oldInterval->clone());
      }
   };

   if(oldRange->isReal())
   {
      THROW_ASSERT(newRange->isReal(), "Real range should not change type");
      const auto oldRR = std::static_pointer_cast<const RealRange>(oldRange);
      const auto newRR = std::static_pointer_cast<const RealRange>(newRange);
      RangeConstRef oldIntervals[] = {oldRR->getSign(), oldRR->getExponent(), oldRR->getSignificand()};
      RangeConstRef newIntervals[] = {newRR->getSign(), newRR->getExponent(), newRR->getSignificand()};
      for(auto i = 0; i < 3; ++i)
      {
         newIntervals[i] = intervalCrop(oldIntervals[i], newIntervals[i], _abstractState);
      }
      op->getSink()->setRange(RangeRef(new RealRange(newIntervals[0], newIntervals[1], newIntervals[2])));
   }
   else
   {
      op->getSink()->setRange(intervalCrop(oldRange, newRange, _abstractState));
   }

   const auto sinkRange = op->getSink()->getRange();
   if(op->getInstruction())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "CROP::@" + STR(GET_INDEX_CONST_NODE(op->getInstruction())) + ": " + oldRange->ToString() + " -> " + sinkRange->ToString());
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "CROP::%artificial phi : " + oldRange->ToString() + " -> " + sinkRange->ToString());
   }
   return !oldRange->isSameRange(sinkRange);
}

// ========================================================================== //
// ConstraintGraph
// ========================================================================== //
using CallMap = CustomMap<unsigned int, std::list<tree_nodeConstRef>>;

using ParmMap = CustomMap<unsigned int, std::pair<bool, std::vector<tree_nodeConstRef>>>;

class ConstraintGraph : public NodeContainer
{
 protected:
   // Perform the widening and narrowing operations
   void update(const UseMap& compUseMap, std::set<tree_nodeConstRef, tree_reindexCompare>& actv, std::function<bool(OpNode*, const std::vector<APInt>&)> meet)
   {
      while(!actv.empty())
      {
         const auto V = *actv.begin();
         actv.erase(V);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "-> update: " + GET_CONST_NODE(V)->ToString());

         // The use list.
         const auto& L = compUseMap.at(V);

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "-->");
         for(auto* op : L)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "> " + op->getSink()->ToString());
            if(meet(op, constantvector))
            {
               // I want to use it as a set, but I also want
               // keep an order of insertions and removals.
               const auto& val = op->getSink()->getValue();
               actv.insert(val);
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "<--");
      }
   }

   void update(size_t nIterations, const UseMap& compUseMap, std::set<tree_nodeConstRef, tree_reindexCompare>& actv)
   {
      std::deque<tree_nodeConstRef> queue(actv.begin(), actv.end());
      actv.clear();
      while(!queue.empty())
      {
         const auto V = queue.front();
         queue.pop_front();
         // The use list.
         const auto& L = compUseMap.at(V);
         for(auto op : L)
         {
            if(nIterations == 0)
            {
               return;
            }
            --nIterations;
            if(Meet::fixed(op))
            {
               const auto& next = op->getSink()->getValue();
               if(std::find(queue.begin(), queue.end(), next) == queue.end())
               {
                  queue.push_back(next);
               }
            }
         }
      }
   }

   virtual void preUpdate(const UseMap& compUseMap, std::set<tree_nodeConstRef, tree_reindexCompare>& entryPoints) = 0;
   virtual void posUpdate(const UseMap& compUseMap, std::set<tree_nodeConstRef, tree_reindexCompare>& activeVars, const CustomSet<VarNode*>& component) = 0;

 private:
#ifndef NDEBUG
   int debug_level;
   int graph_debug;
#endif

   const application_managerRef AppM;

   // A map from variables to the operations where these
   // variables are present as bounds
   SymbMap symbMap;
   // A map from functions to the operations where they are called
   CallMap callMap;
   // A map from functions to the ssa_name associated with parm_decl (bool value is true when all parameters are associated with a variable)
   ParmMap parmMap;

   // Vector containing the constants from a SCC
   // It is cleared at the beginning of every SCC resolution
   std::vector<APInt> constantvector;

   /**
    * @brief Analyze branch instruction and build conditional value range
    *
    * @param br Branch instruction
    * @param branchBB Branch basic block
    * @param function_id Function id
    * @return unsigned int Return dead basic block to be removed when necessary and possible (bloc::ENTRY_BLOCK_ID indicates no dead block found, bloc::EXIT_BLOCK_ID indicates constant codition was found but could not be evaluated)
    */
   unsigned int buildCVR(const gimple_cond* br, const blocRef branchBB, unsigned int function_id)
   {
      if(GetPointer<const cst_node>(GET_CONST_NODE(br->op0)) != nullptr)
      {
         return evaluateBranch(br->op0, branchBB
#ifndef NDEBUG
                               ,
                               debug_level
#endif
         );
      }
      THROW_ASSERT(GET_CONST_NODE(br->op0)->get_kind() == ssa_name_K, "Non SSA variable found in branch (" + GET_CONST_NODE(br->op0)->get_kind_text() + " " + GET_CONST_NODE(br->op0)->ToString() + ")");
      const auto Cond = branchOpRecurse(br->op0);

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch condition is " + Cond->get_kind_text() + " " + Cond->ToString());
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      if(const auto* bin_op = GetPointer<const binary_expr>(Cond))
      {
         if(!isCompare(bin_op))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not a compare codition, skipping...");
            return bloc::ENTRY_BLOCK_ID;
         }

         if(!isValidType(bin_op->op0) || !isValidType(bin_op->op1))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Non-integer operands, skipping...");
            return bloc::ENTRY_BLOCK_ID;
         }

         // Create VarNodes for comparison operands explicitly
         addVarNode(bin_op->op0, function_id);
         addVarNode(bin_op->op1, function_id);

         // Gets the successors of the current basic block.
         const auto TrueBBI = branchBB->true_edge;
         const auto FalseBBI = branchBB->false_edge;

         // We have a Variable-Constant comparison.
         const auto Op0 = GET_CONST_NODE(bin_op->op0);
         const auto Op1 = GET_CONST_NODE(bin_op->op1);
         tree_nodeConstRef constant = nullptr;
         tree_nodeConstRef variable = nullptr;

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Op0 is " + Op0->get_kind_text() + " and Op1 is " + Op1->get_kind_text());

         // If both operands are constants, nothing to do here
         if(GetPointer<const cst_node>(Op0) != nullptr && GetPointer<const cst_node>(Op1) != nullptr)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
            return evaluateBranch(br->op0, branchBB
#ifndef NDEBUG
                                  ,
                                  debug_level
#endif
            );
         }

         // Then there are two cases: variable being compared to a constant,
         // or variable being compared to another variable

         // Op0 is constant, Op1 is variable
         if(GetPointer<const cst_node>(Op0) != nullptr)
         {
            constant = Op0;
            variable = bin_op->op1;
            // Op0 is variable, Op1 is constant
         }
         else if(GetPointer<const cst_node>(Op1) != nullptr)
         {
            constant = Op1;
            variable = bin_op->op0;
         }
         // Both are variables
         // which means constant == 0 and variable == 0

         if(constant != nullptr)
         {
            const kind pred = isSignedType(variable) ? bin_op->get_kind() : op_unsigned(bin_op->get_kind());
            const kind swappred = op_swap(pred);
            RangeRef CR = getGIMPLE_range(constant);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variable bitwidth is " + STR(getGIMPLE_BW(variable)) + " and constant value is " + constant->ToString());

            const auto TValues = (GET_INDEX_CONST_NODE(variable) == GET_INDEX_CONST_NODE(bin_op->op0)) ? Range::makeSatisfyingCmpRegion(pred, CR) : Range::makeSatisfyingCmpRegion(swappred, CR);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Condition is true on " + TValues->ToString());

            const auto FValues = TValues->isFullSet() ? getRangeFor(variable, Empty) : TValues->getAnti();

            // Create the interval using the intersection in the branch.
            const auto BT = ValueRangeRef(new ValueRange(TValues));
            const auto BF = ValueRangeRef(new ValueRange(FValues));

            addConditionalValueRange(ConditionalValueRange(variable, TrueBBI, FalseBBI, BT, BF));

            // Do the same for the operand of variable (if variable is a cast
            // instruction)
            if(const auto* Var = GetPointer<const ssa_name>(GET_CONST_NODE(variable)))
            {
               const auto* VDef = GetPointer<const gimple_assign>(GET_CONST_NODE(Var->CGetDefStmt()));
               if(VDef && (GET_CONST_NODE(VDef->op1)->get_kind() == nop_expr_K || GET_CONST_NODE(VDef->op1)->get_kind() == convert_expr_K))
               {
                  const auto* cast_inst = GetPointer<const unary_expr>(GET_CONST_NODE(VDef->op1));
#ifndef NDEBUG
                  if(GET_INDEX_CONST_NODE(variable) == GET_INDEX_CONST_NODE(bin_op->op0))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Op0 comes from a cast expression " + cast_inst->ToString());
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Op1 comes from a cast expression" + cast_inst->ToString());
                  }
#endif

                  const auto _BT = ValueRangeRef(new ValueRange(TValues));
                  const auto _BF = ValueRangeRef(new ValueRange(FValues));

                  addConditionalValueRange(ConditionalValueRange(cast_inst->op, TrueBBI, FalseBBI, _BT, _BF));
               }
            }
         }
         else
         {
            const kind pred = isSignedType(bin_op->op0) ? bin_op->get_kind() : op_unsigned(bin_op->get_kind());
            const kind invPred = op_inv(pred);
            const kind swappred = op_swap(pred);
            const kind invSwappred = op_inv(swappred);

#if !defined(NDEBUG) or HAVE_ASSERTS
            const auto bw0 = getGIMPLE_BW(bin_op->op0);
#endif
#if HAVE_ASSERTS
            const auto bw1 = getGIMPLE_BW(bin_op->op1);
            THROW_ASSERT(bw0 == bw1, "Operands of same operation have different bitwidth (Op0 = " + STR(bw0) + ", Op1 = " + STR(bw1) + ").");
#endif

            const auto CR = getRangeFor(bin_op->op0, Unknown);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variables bitwidth is " + STR(bw0));

            // Symbolic intervals for op0
            const auto STOp0 = ValueRangeRef(new SymbRange(CR, bin_op->op1, pred));
            const auto SFOp0 = ValueRangeRef(new SymbRange(CR, bin_op->op1, invPred));

            addConditionalValueRange(ConditionalValueRange(bin_op->op0, TrueBBI, FalseBBI, STOp0, SFOp0));

            // Symbolic intervals for operand of op0 (if op0 is a cast instruction)
            if(const auto* Var = GetPointer<const ssa_name>(Op0))
            {
               const auto* VDef = GetPointer<const gimple_assign>(GET_CONST_NODE(Var->CGetDefStmt()));
               if(VDef && (GET_CONST_NODE(VDef->op1)->get_kind() == nop_expr_K || GET_CONST_NODE(VDef->op1)->get_kind() == convert_expr_K))
               {
                  const auto* cast_inst = GetPointer<const unary_expr>(GET_CONST_NODE(VDef->op1));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Op0 comes from a cast expression " + cast_inst->ToString());

                  const auto STOp0_0 = ValueRangeRef(new SymbRange(CR, bin_op->op1, pred));
                  const auto SFOp0_0 = ValueRangeRef(new SymbRange(CR, bin_op->op1, invPred));

                  addConditionalValueRange(ConditionalValueRange(cast_inst->op, TrueBBI, FalseBBI, STOp0_0, SFOp0_0));
               }
            }

            // Symbolic intervals for op1
            const auto STOp1 = ValueRangeRef(new SymbRange(CR, bin_op->op0, swappred));
            const auto SFOp1 = ValueRangeRef(new SymbRange(CR, bin_op->op0, invSwappred));
            addConditionalValueRange(ConditionalValueRange(bin_op->op1, TrueBBI, FalseBBI, STOp1, SFOp1));

            // Symbolic intervals for operand of op1 (if op1 is a cast instruction)
            if(const auto* Var = GetPointer<const ssa_name>(Op1))
            {
               const auto* VDef = GetPointer<const gimple_assign>(GET_CONST_NODE(Var->CGetDefStmt()));
               if(VDef && (GET_CONST_NODE(VDef->op1)->get_kind() == nop_expr_K || GET_CONST_NODE(VDef->op1)->get_kind() == convert_expr_K))
               {
                  const auto* cast_inst = GetPointer<const unary_expr>(GET_CONST_NODE(VDef->op1));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Op1 comes from a cast expression" + cast_inst->ToString());

                  const auto STOp1_1 = ValueRangeRef(new SymbRange(CR, bin_op->op0, swappred));
                  const auto SFOp1_1 = ValueRangeRef(new SymbRange(CR, bin_op->op0, invSwappred));

                  addConditionalValueRange(ConditionalValueRange(cast_inst->op, TrueBBI, FalseBBI, STOp1_1, SFOp1_1));
               }
            }
         }
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Not a compare codition, skipping...");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      return bloc::ENTRY_BLOCK_ID;
   }

   bool buildCVR(const gimple_multi_way_if* mwi, const blocRef /*mwifBB*/, unsigned int function_id)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Multi-way if with " + STR(mwi->list_of_cond.size()) + " conditions");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");

      // Find else branch BBI if any
      unsigned int DefaultBBI = 0;
      for(const auto& condBBI : mwi->list_of_cond)
      {
         if(!condBBI.first)
         {
            DefaultBBI = condBBI.second;
            break;
         }
      }

      // Analyze each if branch condition
      CustomMap<tree_nodeConstRef, std::map<unsigned int, ValueRangeRef>> switchSSAMap;
      for(const auto& condBBI : mwi->list_of_cond)
      {
         if(!condBBI.first)
         {
            // Default branch is handled at the end
            continue;
         }

         if(GetPointer<const cst_node>(GET_CONST_NODE(condBBI.first)) != nullptr)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch variable is a cst_node, dead code elimination necessary!");
            // TODO: abort and call dead code elimination to evaluate constant condition
            //    return true;
            continue;
         }
         THROW_ASSERT(GET_CONST_NODE(condBBI.first)->get_kind() == ssa_name_K, "Case conditional variable should be an ssa_name (" + GET_CONST_NODE(condBBI.first)->get_kind_text() + " " + GET_CONST_NODE(condBBI.first)->ToString() + ")");
         const auto case_compare = branchOpRecurse(condBBI.first);
         if(const auto* cmp_op = GetPointer<const binary_expr>(case_compare))
         {
            if(!isCompare(cmp_op))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Not a compare codition, skipping...");
               continue;
            }

            if(!isValidType(cmp_op->op0) || !isValidType(cmp_op->op1))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Non-integer operands, skipping...");
               continue;
            }

            // Create VarNodes for comparison operands explicitly
            addVarNode(cmp_op->op0, function_id);
            addVarNode(cmp_op->op1, function_id);

            // We have a Variable-Constant comparison.
            const auto Op0 = GET_CONST_NODE(cmp_op->op0);
            const auto Op1 = GET_CONST_NODE(cmp_op->op1);
            const struct integer_cst* constant = nullptr;
            tree_nodeConstRef variable = nullptr;

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Op0 is " + Op0->get_kind_text() + " and Op1 is " + Op1->get_kind_text());

            // If both operands are constants, nothing to do here
            if(GetPointer<const cst_node>(Op0) != nullptr && GetPointer<const cst_node>(Op1) != nullptr)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Both operands are constants, dead code elimination necessary!");
               // TODO: abort and call dead code elimination to evaluate constant condition
               //    return true;
               continue;
            }

            // Then there are two cases: variable being compared to a constant,
            // or variable being compared to another variable

            // Op0 is constant, Op1 is variable
            if((constant = GetPointer<const integer_cst>(Op0)) != nullptr)
            {
               variable = cmp_op->op1;
            }
            else if((constant = GetPointer<const integer_cst>(Op1)) != nullptr)
            {
               // Op0 is variable, Op1 is constant
               variable = cmp_op->op0;
            }
            // Both are variables
            // which means constant == 0 and variable == 0

            if(constant != nullptr)
            {
               const kind pred = isSignedType(variable) ? cmp_op->get_kind() : op_unsigned(cmp_op->get_kind());
               const kind swappred = op_swap(pred);
               const auto bw = getGIMPLE_BW(variable);
               RangeConstRef CR(new Range(Regular, bw, constant->value, constant->value));
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variable bitwidth is " + STR(bw) + " and constant value is " + STR(constant->value));

               const auto tmpT = (GET_INDEX_CONST_NODE(variable) == GET_INDEX_CONST_NODE(cmp_op->op0)) ? Range::makeSatisfyingCmpRegion(pred, CR) : Range::makeSatisfyingCmpRegion(swappred, CR);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Condition is true on " + tmpT->ToString());

               RangeRef TValues = tmpT->isFullSet() ? RangeRef(new Range(Regular, bw)) : tmpT;

               // Create the interval using the intersection in the branch.
               auto BT = ValueRangeRef(new ValueRange(TValues));
               switchSSAMap[variable].insert(std::make_pair(condBBI.second, BT));

               // Do the same for the operand of variable (if variable is a cast
               // instruction)
               if(const auto* Var = GetPointer<const ssa_name>(GET_CONST_NODE(variable)))
               {
                  const auto* VDef = GetPointer<const gimple_assign>(GET_CONST_NODE(Var->CGetDefStmt()));
                  if(VDef && (GET_CONST_NODE(VDef->op1)->get_kind() == nop_expr_K || GET_CONST_NODE(VDef->op1)->get_kind() == convert_expr_K))
                  {
                     const auto* cast_inst = GetPointer<const unary_expr>(GET_CONST_NODE(VDef->op1));
#ifndef NDEBUG
                     if(GET_INDEX_CONST_NODE(variable) == GET_INDEX_CONST_NODE(cmp_op->op0))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Op0 comes from a cast expression " + cast_inst->ToString());
                     }
                     else
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Op1 comes from a cast expression" + cast_inst->ToString());
                     }
#endif

                     auto _BT = ValueRangeRef(new ValueRange(TValues));
                     switchSSAMap[cast_inst->op].insert(std::make_pair(condBBI.second, _BT));
                  }
               }
            }
            else
            {
               const kind pred = isSignedType(cmp_op->op0) ? cmp_op->get_kind() : op_unsigned(cmp_op->get_kind());
               const kind swappred = op_swap(pred);

#if !defined(NDEBUG) or HAVE_ASSERTS
               const auto bw0 = getGIMPLE_BW(cmp_op->op0);
#endif
#if HAVE_ASSERTS
               const auto bw1 = getGIMPLE_BW(cmp_op->op1);
               THROW_ASSERT(bw0 == bw1, "Operands of same operation have different bitwidth (Op0 = " + STR(bw0) + ", Op1 = " + STR(bw1) + ").");
#endif

               const auto CR = getRangeFor(cmp_op->op0, Unknown);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variables bitwidth is " + STR(bw0));

               // Symbolic intervals for op0
               const auto STOp0 = ValueRangeRef(new SymbRange(CR, cmp_op->op1, pred));
               switchSSAMap[cmp_op->op0].insert(std::make_pair(condBBI.second, STOp0));

               // Symbolic intervals for operand of op0 (if op0 is a cast instruction)
               if(const auto* Var = GetPointer<const ssa_name>(Op0))
               {
                  const auto* VDef = GetPointer<const gimple_assign>(GET_CONST_NODE(Var->CGetDefStmt()));
                  if(VDef && (GET_CONST_NODE(VDef->op1)->get_kind() == nop_expr_K || GET_CONST_NODE(VDef->op1)->get_kind() == convert_expr_K))
                  {
                     const auto* cast_inst = GetPointer<const unary_expr>(GET_CONST_NODE(VDef->op1));
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Op0 comes from a cast expression" + cast_inst->ToString());

                     const auto STOp0_0 = ValueRangeRef(new SymbRange(CR, cmp_op->op1, pred));
                     switchSSAMap[cast_inst->op].insert(std::make_pair(condBBI.second, STOp0_0));
                  }
               }

               // Symbolic intervals for op1
               const auto STOp1 = ValueRangeRef(new SymbRange(CR, cmp_op->op0, swappred));
               switchSSAMap[cmp_op->op1].insert(std::make_pair(condBBI.second, STOp1));

               // Symbolic intervals for operand of op1 (if op1 is a cast instruction)
               if(const auto* Var = GetPointer<const ssa_name>(Op1))
               {
                  const auto* VDef = GetPointer<const gimple_assign>(GET_CONST_NODE(Var->CGetDefStmt()));
                  if(VDef && (GET_CONST_NODE(VDef->op1)->get_kind() == nop_expr_K || GET_CONST_NODE(VDef->op1)->get_kind() == convert_expr_K))
                  {
                     const auto* cast_inst = GetPointer<const unary_expr>(GET_CONST_NODE(VDef->op1));
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Op1 comes from a cast expression" + cast_inst->ToString());

                     const auto STOp1_1 = ValueRangeRef(new SymbRange(CR, cmp_op->op0, swappred));
                     switchSSAMap[cast_inst->op].insert(std::make_pair(condBBI.second, STOp1_1));
                  }
               }
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Multi-way condition different from binary_expr not handled, skipping... (" + case_compare->get_kind_text() + " " + case_compare->ToString() + ")");
         }
      }

      // Handle else branch, if there is any
      // TODO: maybe it should be better to leave fullset as interval for default edge
      //       because usign getAnti implies internal values to be excluded while they
      //       could still be valid values
      if(static_cast<bool>(DefaultBBI))
      {
         for(auto& varVSM : switchSSAMap)
         {
            auto elseRange = getRangeFor(varVSM.first, Empty);
            for(const auto& BBIinterval : varVSM.second)
            {
               elseRange = elseRange->unionWith(BBIinterval.second->getRange());
            }
            elseRange = elseRange->getAnti();
            varVSM.second.insert(std::make_pair(DefaultBBI, ValueRangeRef(new ValueRange(elseRange))));
         }
      }

      for(const auto& varVSM : switchSSAMap)
      {
         addConditionalValueRange(ConditionalValueRange(varVSM.first, varVSM.second));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      return false;
   }

   /*
    *	This method builds a map that binds each variable label to the
    * operations
    * where this variable is used.
    */
   UseMap buildUseMap(const CustomSet<VarNode*>& component)
   {
      UseMap compUseMap;
      for(auto vit = component.begin(), vend = component.end(); vit != vend; ++vit)
      {
         const VarNode* var = *vit;
         const auto& V = var->getValue();
         // Get the component's use list for V (it does not exist until we try to get it)
         auto& list = compUseMap[V];
         // Get the use list of the variable in component
         auto p = getUses().find(V);
         // For each operation in the list, verify if its sink is in the component
         for(auto* opit : p->second)
         {
            VarNode* sink = opit->getSink();
            // If it is, add op to the component's use map
            if(static_cast<bool>(component.count(sink)))
            {
               list.insert(opit);
            }
         }
      }
      return compUseMap;
   }

   /*
    * Used to insert constant in the right position
    */
   void insertConstantIntoVector(const APInt& constantval, bw_t bw)
   {
      constantvector.push_back(constantval.extOrTrunc(bw, true));
   }

   /*
    * Create a vector containing all constants related to the component
    * They include:
    *   - Constants inside component
    *   - Constants that are source of an edge to an entry point
    *   - Constants from intersections generated by sigmas
    */
   void buildConstantVector(const CustomSet<VarNode*>& component, const UseMap& compusemap)
   {
      // Remove all elements from the vector
      constantvector.clear();

      // Get constants inside component (TODO: may not be necessary, since
      // components with more than 1 node may
      // never have a constant inside them)
      for(const auto* varNode : component)
      {
         const auto& V = varNode->getValue();
         if(const auto* ic = GetPointer<const integer_cst>(GET_CONST_NODE(V)))
         {
            insertConstantIntoVector(tree_helper::get_integer_cst_value(ic), varNode->getBitWidth());
         }
      }

      // Get constants that are sources of operations whose sink belong to the
      // component
      for(const auto* varNode : component)
      {
         const auto& V = varNode->getValue();
         auto dfit = getDefs().find(V);
         if(dfit == getDefs().end())
         {
            continue;
         }

         auto pushConstFor = [this](const APInt& cst, bw_t bw, kind pred) {
            if(isCompare(pred))
            {
               if(pred == eq_expr_K || pred == ne_expr_K)
               {
                  insertConstantIntoVector(cst, bw);
                  insertConstantIntoVector(cst - 1, bw);
                  insertConstantIntoVector(cst + 1, bw);
               }
               else if(pred == uneq_expr_K)
               {
                  const auto ucst = cst.extOrTrunc(bw, false);
                  insertConstantIntoVector(ucst, bw);
                  insertConstantIntoVector(ucst - 1, bw);
                  insertConstantIntoVector(ucst + 1, bw);
               }
               else if(pred == gt_expr_K || pred == le_expr_K)
               {
                  insertConstantIntoVector(cst, bw);
                  insertConstantIntoVector(cst + 1, bw);
               }
               else if(pred == ge_expr_K || pred == lt_expr_K)
               {
                  insertConstantIntoVector(cst, bw);
                  insertConstantIntoVector(cst - 1, bw);
               }
               else if(pred == ungt_expr_K || pred == unle_expr_K)
               {
                  const auto ucst = cst.extOrTrunc(bw, false);
                  insertConstantIntoVector(ucst, bw);
                  insertConstantIntoVector(ucst + 1, bw);
               }
               else if(pred == unge_expr_K || pred == unlt_expr_K)
               {
                  const auto ucst = cst.extOrTrunc(bw, false);
                  insertConstantIntoVector(ucst, bw);
                  insertConstantIntoVector(ucst - 1, bw);
               }
               else
               {
                  THROW_UNREACHABLE("unexpected condition (" + tree_node::GetString(pred) + ")");
               }
            }
            else
            {
               insertConstantIntoVector(cst, bw);
            }
         };

         // Handle BinaryOp case
         if(const auto* bop = GetOp<BinaryOpNode>(dfit->second))
         {
            const auto* source1 = bop->getSource1();
            const auto& sourceval1 = source1->getValue();
            const auto* source2 = bop->getSource2();
            const auto& sourceval2 = source2->getValue();

            const auto pred = bop->getOpcode();

            if(const auto* const1 = GetPointer<const integer_cst>(GET_CONST_NODE(sourceval1)))
            {
               const auto bw = source1->getBitWidth();
               const auto cst_val = tree_helper::get_integer_cst_value(const1);
               pushConstFor(cst_val, bw, pred); // TODO: maybe should swap predicate for lhs constant?
            }
            if(const auto* const2 = GetPointer<const integer_cst>(GET_CONST_NODE(sourceval2)))
            {
               const auto bw = source2->getBitWidth();
               const auto cst_val = tree_helper::get_integer_cst_value(const2);
               pushConstFor(cst_val, bw, pred);
            }
         }
         // Handle PhiOp case
         else if(const auto* pop = GetOp<PhiOpNode>(dfit->second))
         {
            for(size_t i = 0, e = pop->getNumSources(); i < e; ++i)
            {
               const auto* source = pop->getSource(i);
               const auto& sourceval = source->getValue();
               if(const auto* ic = GetPointer<const integer_cst>(GET_CONST_NODE(sourceval)))
               {
                  insertConstantIntoVector(tree_helper::get_integer_cst_value(ic), source->getBitWidth());
               }
            }
         }
      }

      // Get constants used in intersections
      for(const auto& varOps : compusemap)
      {
         for(const auto* op : varOps.second)
         {
            const auto* sigma = GetOp<SigmaOpNode>(op);
            // Symbolic intervals are discarded, as they don't have fixed values yet
            if(sigma == nullptr || SymbRange::classof(sigma->getIntersect().get()) || sigma->getIntersect()->getRange()->isReal())
            {
               continue;
            }
            const auto rintersect = op->getIntersect()->getRange();
            const auto bw = rintersect->getBitWidth();
            if(rintersect->isAnti())
            {
               const auto anti = rintersect->getAnti();
               const auto& lb = anti->getLower();
               const auto& ub = anti->getUpper();
               if((lb != Range::Min) && (lb != Range::Max))
               {
                  insertConstantIntoVector(lb - 1, bw);
                  insertConstantIntoVector(lb, bw);
               }
               if((ub != Range::Min) && (ub != Range::Max))
               {
                  insertConstantIntoVector(ub, bw);
                  insertConstantIntoVector(ub + 1, bw);
               }
            }
            else
            {
               const auto& lb = rintersect->getLower();
               const auto& ub = rintersect->getUpper();
               if((lb != Range::Min) && (lb != Range::Max))
               {
                  insertConstantIntoVector(lb - 1, bw);
                  insertConstantIntoVector(lb, bw);
               }
               if((ub != Range::Min) && (ub != Range::Max))
               {
                  insertConstantIntoVector(ub, bw);
                  insertConstantIntoVector(ub + 1, bw);
               }
            }
         }
      }

      // Sort vector in ascending order and remove duplicates
      std::sort(constantvector.begin(), constantvector.end(), [](const APInt& i1, const APInt& i2) { return i1 < i2; });

      // std::unique doesn't remove duplicate elements, only
      // move them to the end
      // This is why erase is necessary. To remove these duplicates
      // that will be now at the end.
      auto last = std::unique(constantvector.begin(), constantvector.end());
      constantvector.erase(last, constantvector.end());
   }

   /*
    * This method builds a map of variables to the lists of operations where
    * these variables are used as futures. Its C++ type should be something like
    * map<VarNode, List<Operation>>.
    */
   void buildSymbolicIntersectMap()
   {
      // Creates the symbolic intervals map
      symbMap = SymbMap();

      // Iterate over the operations set
      for(auto* op : getOpNodes())
      {
         // If the operation is unary and its interval is symbolic
         auto* uop = GetOp<UnaryOpNode>(op);
         if((uop != nullptr) && SymbRange::classof(uop->getIntersect().get()))
         {
            const auto symbi = std::static_pointer_cast<const SymbRange>(uop->getIntersect());
            const auto V = symbi->getBound();
            auto p = symbMap.find(V);
            if(p != symbMap.end())
            {
               p->second.insert(uop);
            }
            else
            {
               CustomSet<OpNode*> l;
               l.insert(uop);
               symbMap.insert(std::make_pair(V, l));
            }
         }
      }
   }

   /*
    * This method evaluates once each operation that uses a variable in
    * component, so that the next SCCs after component will have entry
    * points to kick start the range analysis algorithm.
    */
   void propagateToNextSCC(const CustomSet<VarNode*>& component)
   {
      const auto& uses = getUses();
      for(const auto& var : component)
      {
         const auto& V = var->getValue();
         const auto& p = uses.at(V);
         for(auto* op : p)
         {
            /// VarNodes belonging to the current SCC must not be evaluated otherwise we break the fixed point previously computed
            if(component.contains(op->getSink()))
            {
               continue;
            }
            auto* sigmaop = GetOp<SigmaOpNode>(op);
            op->getSink()->setRange(op->eval());
            if((sigmaop != nullptr) && sigmaop->getIntersect()->getRange()->isUnknown())
            {
               sigmaop->markUnresolved();
            }
         }
      }
   }

   void generateEntryPoints(const CustomSet<VarNode*>& component, std::set<tree_nodeConstRef, tree_reindexCompare>& entryPoints)
   {
      // Iterate over the varnodes in the component
      for(VarNode* varNode : component)
      {
         const auto& V = varNode->getValue();
         if(const auto* ssa = GetPointer<const ssa_name>(GET_CONST_NODE(V)))
            if(const auto* phi_def = GetPointer<const gimple_phi>(GET_CONST_NODE(ssa->CGetDefStmt())))
               if(phi_def->CGetDefEdgesList().size() == 1)
               {
                  auto dit = getDefs().find(V);
                  if(dit != getDefs().end())
                  {
                     auto* bop = dit->second;
                     auto* defop = GetOp<SigmaOpNode>(bop);

                     if((defop != nullptr) && defop->isUnresolved())
                     {
                        defop->getSink()->setRange(bop->eval());
                        defop->markResolved();
                     }
                  }
               }
         if(!varNode->getRange()->isUnknown())
         {
            entryPoints.insert(V);
         }
      }
   }

   void solveFutures(const CustomSet<VarNode*>& component)
   {
      // Iterate again over the varnodes in the component
      for(auto* varNode : component)
      {
         solveFuturesSC(varNode);
      }
   }

   void solveFuturesSC(VarNode* varNode)
   {
      const auto& V = varNode->getValue();
      auto sit = symbMap.find(V);
      if(sit != symbMap.end())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Fix intersects: " + varNode->ToString());
         for(auto* op : sit->second)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Op intersects: " + op->ToString());
            op->solveFuture(varNode);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Sink: " + op->ToString());
         }
      }
   }

   void generateActivesVars(const CustomSet<VarNode*>& component, std::set<tree_nodeConstRef, tree_reindexCompare>& activeVars)
   {
      for(auto* varNode : component)
      {
         const auto& V = varNode->getValue();
         const auto* CI = GetPointer<const integer_cst>(GET_CONST_NODE(V));
         if(CI != nullptr)
         {
            continue;
         }
         activeVars.insert(V);
      }
   }

   void parametersBinding(const tree_nodeRef stmt, const struct function_decl* FD)
   {
      const auto& args = FD->list_of_args;
      auto parmMapIt = parmMap.find(FD->index);
      if(parmMapIt == parmMap.end())
      {
         parmMapIt = parmMap.insert(std::make_pair(FD->index, std::make_pair(false, std::vector<tree_nodeConstRef>(args.size(), nullptr)))).first;
      }
      auto& foundAll = parmMapIt->second.first;
      // Skip ssa uses computation when all parameters have already been associated with a variable
      if(foundAll)
      {
         return;
      }

      auto& parmBind = parmMapIt->second.second;
      const auto ssa_uses = tree_helper::ComputeSsaUses(stmt);
      for(const auto& ssa_use_counter : ssa_uses)
      {
         auto ssa = ssa_use_counter.first;
         const auto* SSA = GetPointer<const ssa_name>(GET_CONST_NODE(ssa));
         // If ssa_name references a parm_decl and is defined by a gimple_nop, it represents the formal function parameter inside the function body
         if(SSA->var != nullptr && GET_CONST_NODE(SSA->var)->get_kind() == parm_decl_K && GET_CONST_NODE(SSA->CGetDefStmt())->get_kind() == gimple_nop_K)
         {
            auto argIt = std::find_if(args.begin(), args.end(), [&](const tree_nodeRef& arg) { return GET_INDEX_CONST_NODE(arg) == GET_INDEX_CONST_NODE(SSA->var); });
            THROW_ASSERT(argIt != args.end(), "parm_decl associated with ssa_name not found in function parameters");
            size_t arg_pos = static_cast<size_t>(argIt - args.begin());
            THROW_ASSERT(arg_pos < args.size(), "Computed parameter position outside actual parameters number");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variable " + SSA->ToString() + " is defined from parameter " + STR(arg_pos) + " of function " + GetPointer<const identifier_node>(GET_CONST_NODE(FD->name))->strg);
            parmBind[arg_pos] = ssa;
            foundAll = std::find(parmBind.begin(), parmBind.end(), nullptr) == parmBind.end();
         }
      }
   }

   bool storeFunctionCall(const tree_nodeConstRef tn)
   {
      tree_nodeRef fun_node = nullptr;

      if(const auto* ga = GetPointer<const gimple_assign>(GET_CONST_NODE(tn)))
         if(const auto* ce = GetPointer<const call_expr>(GET_CONST_NODE(ga->op1)))
         {
            fun_node = ce->fn;
         }
      if(const auto* ce = GetPointer<const gimple_call>(GET_CONST_NODE(tn)))
      {
         fun_node = ce->fn;
      }

      if(fun_node)
      {
         if(GET_NODE(fun_node)->get_kind() == addr_expr_K)
         {
            const auto* ue = GetPointer<const unary_expr>(GET_NODE(fun_node));
            fun_node = ue->op;
         }
         else if(GET_NODE(fun_node)->get_kind() == obj_type_ref_K)
         {
            fun_node = tree_helper::find_obj_type_ref_function(fun_node);
         }

         const auto* FD = GetPointer<const function_decl>(GET_CONST_NODE(fun_node));
         THROW_ASSERT(FD, "Function call should reference a function_decl node");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Analysing function call to " + tree_helper::print_type(AppM->get_tree_manager(), FD->index, false, true, false, 0U, var_pp_functorConstRef(new std_var_pp_functor(AppM->CGetFunctionBehavior(FD->index)->CGetBehavioralHelper()))));

         auto it = callMap.find(FD->index);
         if(it == callMap.end())
         {
            it = callMap.insert(std::make_pair(FD->index, std::list<tree_nodeConstRef>())).first;
         }
         it->second.emplace_back(tn);
         return true;
      }
      return false;
   }

 public:
   ConstraintGraph(application_managerRef _AppM
#ifndef NDEBUG
                   ,
                   int _debug_level, int _graph_debug
#else
                   ,
                   int, int
#endif
                   )
       :
#ifndef NDEBUG
         debug_level(_debug_level),
         graph_debug(_graph_debug),
#endif
         AppM(_AppM)
   {
#ifndef NDEBUG
      NodeContainer::debug_level = debug_level;
#endif
   }

   virtual ~ConstraintGraph() = default;

   CallMap* getCallMap()
   {
      return &callMap;
   }
   const ParmMap& getParmMap()
   {
      return parmMap;
   }

   /// Iterates through all instructions in the function and builds the graph.
   bool buildGraph(unsigned int function_id)
   {
      const auto TM = AppM->get_tree_manager();
      const auto FB = AppM->CGetFunctionBehavior(function_id);
      const auto* FD = GetPointer<const function_decl>(TM->get_tree_node_const(function_id));
      const auto* SL = GetPointer<const statement_list>(GET_CONST_NODE(FD->body));
#ifndef NDEBUG
      std::string fn_name = tree_helper::print_type(TM, function_id, false, true, false, 0U, var_pp_functorConstRef(new std_var_pp_functor(FB->CGetBehavioralHelper())));
#endif
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing function " + fn_name + " with " + STR(SL->list_of_bloc.size()) + " blocks");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch variables analysis...");
      for(const auto& idxBB : SL->list_of_bloc)
      {
         const auto& stmt_list = idxBB.second->CGetStmtList();
         if(stmt_list.empty())
         {
            continue;
         }

         const auto terminator = GET_CONST_NODE(stmt_list.back());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "BB" + STR(idxBB.first) + " has terminator type " + terminator->get_kind_text() + " " + terminator->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
         if(const auto* br = GetPointer<const gimple_cond>(terminator))
         {
#ifdef EARLY_DEAD_CODE_RESTART
            if(buildCVR(br, idxBB.second, function_id))
            {
               // Dead code elimination necessary
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               return true;
            }
#else
            buildCVR(br, idxBB.second, function_id);
#endif
         }
         else if(const auto* mwi = GetPointer<const gimple_multi_way_if>(terminator))
         {
#ifdef EARLY_DEAD_CODE_RESTART
            if(buildCVR(mwi, idxBB.second, function_id))
            {
               // Dead code elimination necessary
               return true;
            }
#else
            buildCVR(mwi, idxBB.second, function_id);
#endif
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Branch variables analysis completed");

      for(const auto& idxBB : SL->list_of_bloc)
      {
         const auto& phi_list = idxBB.second->CGetPhiList();
         if(phi_list.size())
         {
            for(const auto& stmt : phi_list)
            {
               parametersBinding(stmt, FD);
               if(isValidInstruction(stmt, FB, TM))
               {
                  addOperation(stmt, function_id, FB, TM, AppM);
               }
            }
         }

         const auto& stmt_list = idxBB.second->CGetStmtList();
         if(stmt_list.size())
         {
            for(const auto& stmt : stmt_list)
            {
               if(!isValidInstruction(stmt, FB, TM))
               {
                  parametersBinding(stmt, FD);
                  if(!storeFunctionCall(stmt))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Skipping " + GET_NODE(stmt)->get_kind_text() + " " + GET_NODE(stmt)->ToString());
                  }
                  continue;
               }
               addOperation(stmt, function_id, FB, TM, AppM);
               parametersBinding(stmt, FD);
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Graph built for function " + fn_name);
      return false;
   }

   void buildVarNodes()
   {
      // Initializes the nodes and the use map structure.
      for(auto& pair : getVarNodes())
      {
         pair.second->init(!static_cast<bool>(getDefs().count(pair.first)));
      }
   }

   void findIntervals(
#ifndef NDEBUG
       const ParameterConstRef parameters, const std::string& step_name
#endif
   )
   {
      buildSymbolicIntersectMap();
// List of SCCs
#ifndef NDEBUG
      Nuutila sccList(getVarNodes(), getUses(), symbMap, graph_debug);
#else
      Nuutila sccList(getVarNodes(), getUses(), symbMap);
#endif

      for(const auto& n : sccList)
      {
         const auto& component = sccList.getComponent(n);

#ifndef NDEBUG
         if(DEBUG_LEVEL_VERY_PEDANTIC <= graph_debug)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Components:");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "-->");
            for(const auto* var : component)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, var->ToString());
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "-------------");
         }
#endif
         if(component.size() == 1)
         {
            VarNode* var = *component.begin();
            solveFuturesSC(var);
            auto varDef = getDefs().find(var->getValue());
            if(varDef != getDefs().end())
            {
               auto* op = varDef->second;
               var->setRange(op->eval());
            }
            if(var->getRange()->isUnknown())
            {
               var->setRange(var->getMaxRange());
            }
         }
         else
         {
            const auto compUseMap = buildUseMap(component);

#ifdef RA_JUMPSET
            // Create vector of constants inside component
            // Comment this line below to deactivate jump-set
            buildConstantVector(component, compUseMap);
#ifndef NDEBUG
            if(DEBUG_LEVEL_VERY_PEDANTIC <= graph_debug)
            {
               std::stringstream ss;
               for(auto cnst : constantvector)
               {
                  ss << cnst << ", ";
               }
               if(!constantvector.empty())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Constant lattice: {-inf, " + ss.str() + "+inf}");
               }
            }
#endif
#endif

            // Get the entry points of the SCC
            std::set<tree_nodeConstRef, tree_reindexCompare> entryPoints;
#ifndef NDEBUG
            auto printEntryFor = [&](const std::string& mType) {
               if(DEBUG_LEVEL_VERY_PEDANTIC <= graph_debug)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, mType + " step entry points:");
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "-->");
                  for(const auto& el : entryPoints)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, GET_CONST_NODE(el)->ToString());
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "<--");
               }
            };
#endif

            generateEntryPoints(component, entryPoints);
#ifndef NDEBUG
            printEntryFor("Fixed");
#endif
            // iterate a fixed number of time before widening
            update(static_cast<size_t>(component.size() * 16L), compUseMap, entryPoints);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Printed constraint graph to " + printToFile("after_" + step_name + ".fixed." + STR(GET_INDEX_CONST_NODE(n)) + ".dot", parameters));

            generateEntryPoints(component, entryPoints);
#ifndef NDEBUG
            printEntryFor("Widen");
#endif
            // First iterate till fix point
            preUpdate(compUseMap, entryPoints);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "fixIntersects");
            solveFutures(component);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, " --");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Printed constraint graph to " + printToFile("after_" + step_name + ".futures." + STR(GET_INDEX_CONST_NODE(n)) + ".dot", parameters));

            for(VarNode* varNode : component)
            {
               if(varNode->getRange()->isUnknown())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "initialize unknown: " + varNode->ToString());
                  //    THROW_UNREACHABLE("unexpected condition");
                  varNode->setRange(varNode->getMaxRange());
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Printed constraint graph to " + printToFile("after_" + step_name + ".int." + STR(GET_INDEX_CONST_NODE(n)) + ".dot", parameters));

            // Second iterate till fix point
            std::set<tree_nodeConstRef, tree_reindexCompare> activeVars;
            generateActivesVars(component, activeVars);
#ifndef NDEBUG
            printEntryFor("Narrow");
#endif
            posUpdate(compUseMap, activeVars, component);
         }
         propagateToNextSCC(component);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, graph_debug, "Printed final constraint graph to " + printToFile(step_name + ".constraints.dot", parameters));
   }

#ifndef NDEBUG
   std::string printToFile(const std::string& file_name, const ParameterConstRef parameters) const
   {
      std::string output_directory = parameters->getOption<std::string>(OPT_dot_directory) + "RangeAnalysis/";
      if(!boost::filesystem::exists(output_directory))
      {
         boost::filesystem::create_directories(output_directory);
      }
      const std::string full_name = output_directory + file_name;
      std::ofstream file(full_name);
      printDot(file);
      return full_name;
   }

   /// Prints the content of the graph in dot format. For more information
   /// about the dot format, see: http://www.graphviz.org/pdf/dotguide.pdf
   void printDot(std::ostream& OS) const
   {
      const char* quot = R"(")";
      // Print the header of the .dot file.
      OS << "digraph dotgraph {\n";
      OS << R"(label="Constraint Graph for ')";
      OS << "all\' functions\";\n";
      OS << "node [shape=record,fontname=\"Times-Roman\",fontsize=14];\n";

      // Print the body of the .dot file.
      for(const auto& varNode : getVarNodes())
      {
         if(const auto* C = GetPointer<const integer_cst>(GET_CONST_NODE(varNode.first)))
         {
            OS << " " << C->value;
         }
         else
         {
            OS << quot;
            printVarName(varNode.first, OS);
            OS << quot;
         }
         OS << R"( [label=")" << varNode.second << "\"]\n";
      }

      for(auto* op : getOpNodes())
      {
         op->printDot(OS);
         OS << '\n';
      }
      OS << pseudoEdgesString.str();
      // Print the footer of the .dot file.
      OS << "}\n";
   }
#endif
};

// ========================================================================== //
// Cousot
// ========================================================================== //
class Cousot : public ConstraintGraph
{
 private:
   void preUpdate(const UseMap& compUseMap, std::set<tree_nodeConstRef, tree_reindexCompare>& entryPoints) override
   {
      update(compUseMap, entryPoints, Meet::widen);
   }

   void posUpdate(const UseMap& compUseMap, std::set<tree_nodeConstRef, tree_reindexCompare>& entryPoints, const CustomSet<VarNode*>& /*component*/) override
   {
      update(compUseMap, entryPoints, Meet::narrow);
   }

 public:
   Cousot(application_managerRef _AppM, int _debug_level, int _graph_debug) : ConstraintGraph(_AppM, _debug_level, _graph_debug)
   {
   }
};

// ========================================================================== //
// CropDFS
// ========================================================================== //
class CropDFS : public ConstraintGraph
{
 private:
   void preUpdate(const UseMap& compUseMap, std::set<tree_nodeConstRef, tree_reindexCompare>& entryPoints) override
   {
      update(compUseMap, entryPoints, [](OpNode* b, const std::vector<APInt>&) { return Meet::growth(b); });
   }

   void posUpdate(const UseMap& compUseMap, std::set<tree_nodeConstRef, tree_reindexCompare>& /*activeVars*/, const CustomSet<VarNode*>& component) override
   {
      storeAbstractStates(component);
      for(const auto& op : getOpNodes())
      {
         if(static_cast<bool>(component.count(op->getSink())))
         {
            crop(compUseMap, op);
         }
      }
   }

   void storeAbstractStates(const CustomSet<VarNode*>& component)
   {
      for(const auto& varNode : component)
      {
         varNode->storeAbstractState();
      }
   }

   void crop(const UseMap& compUseMap, OpNode* op)
   {
      CustomSet<OpNode*> activeOps;
      CustomSet<const VarNode*> visitedOps;

      // init the activeOps only with the op received
      activeOps.insert(op);

      while(!activeOps.empty())
      {
         auto* V = *activeOps.begin();
         activeOps.erase(V);
         const VarNode* sink = V->getSink();

         // if the sink has been visited go to the next activeOps
         if(static_cast<bool>(visitedOps.count(sink)))
         {
            continue;
         }

         Meet::crop(V);
         visitedOps.insert(sink);

         // The use list.of sink
         const auto& L = compUseMap.at(sink->getValue());
         for(auto* opr : L)
         {
            activeOps.insert(opr);
         }
      }
   }

 public:
   CropDFS(application_managerRef _AppM, int _debug_level, int _graph_debug) : ConstraintGraph(_AppM, _debug_level, _graph_debug)
   {
   }
};

static void TopFunctionUserHits(unsigned int function_id, const application_managerRef AppM, const ConstraintGraphRef CG,
                                int
#ifndef NDEBUG
                                    debug_level
#endif
)
{
   const auto TM = AppM->get_tree_manager();
   const auto fd = TM->get_tree_node_const(function_id);
   const auto* FD = GetPointer<const function_decl>(fd);

   const auto& parmMap = CG->getParmMap();
   const auto funParm = parmMap.find(function_id);
   THROW_ASSERT(funParm != parmMap.end(), "Function parameters binding unavailable");
   const auto& parmBind = funParm->second.second;
   THROW_ASSERT(parmBind.size() == FD->list_of_args.size(), "Parameters count mismatch");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   for(size_t i = 0; i < FD->list_of_args.size(); ++i)
   {
      if(const auto& p = parmBind.at(i))
      {
         const auto pType = getGIMPLE_Type(p);
         if(!isValidType(pType))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameter " + STR(i) + " is of non-valid type (" + pType->get_kind_text() + ")");
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameter " + STR(i) + " defined as " + GET_CONST_NODE(p)->ToString());

         VarNode* sink = CG->addVarNode(p, function_id);

         // Check for pragma mask directives user defined range
         const auto parm = GetPointer<const parm_decl>(GET_CONST_NODE(FD->list_of_args.at(i)));
         if(parm->range != nullptr)
         {
            sink->setRange(parm->range);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---User-defined hints found " + parm->range->ToString());
         }
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameter " + STR(i) + " missing from function body");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
}

static void ParmAndRetValPropagation(unsigned int function_id, const application_managerRef AppM, const ConstraintGraphRef CG,
                                     int
#ifndef NDEBUG
                                         debug_level
#endif
)
{
   const auto TM = AppM->get_tree_manager();
   const auto fd = TM->get_tree_node_const(function_id);
   const auto* FD = GetPointer<const function_decl>(fd);
#if !defined(NDEBUG) or HAVE_ASSERTS
   std::string fn_name = tree_helper::print_type(TM, function_id, false, true, false, 0U, var_pp_functorConstRef(new std_var_pp_functor(AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper())));
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameters and return value propagation on function " + fn_name);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");

   if(!static_cast<bool>(CG->getCallMap()->count(function_id)))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "No call statements for this function, skipping...");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      return;
   }
   const auto& functionCalls = CG->getCallMap()->at(function_id);

   // Data structure which contains the matches between formal and real parameters
   // First: formal parameter
   // Second: real parameter
   std::vector<std::pair<tree_nodeConstRef, tree_nodeConstRef>> parameters(FD->list_of_args.size());

   // Fetch the function arguments (formal parameters) into the data structure and generate PhiOp nodes for parameters call values
   const auto& parmMap = CG->getParmMap();
   const auto funParm = parmMap.find(function_id);
   THROW_ASSERT(funParm != parmMap.end(), "Function parameters binding unavailable");
   const auto& parmBind = funParm->second.second;
   THROW_ASSERT(parmBind.size() == parameters.size(), "Parameters count mismatch");
   std::vector<PhiOpNode*> matchers(parameters.size(), nullptr);
   for(size_t i = 0; i < parameters.size(); ++i)
   {
      if(const auto& p = parmBind.at(i))
      {
         const auto pType = getGIMPLE_Type(p);
         if(!isValidType(pType))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameter " + STR(i) + " is of non-valid type (" + pType->get_kind_text() + ")");
            continue;
         }
         parameters[i].first = p;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameter " + STR(i) + " defined as " + GET_CONST_NODE(p)->ToString());

         VarNode* sink = CG->addVarNode(p, function_id);

         // Check for pragma mask directives user defined range
         const auto parm = GetPointer<const parm_decl>(GET_CONST_NODE(FD->list_of_args.at(i)));
         if(parm->range != nullptr)
         {
            sink->setRange(parm->range);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---User-defined hints found " + parm->range->ToString());
         }
         else
         {
            sink->setRange(sink->getMaxRange());
         }
         matchers[i] = new PhiOpNode(ValueRangeRef(new ValueRange(sink->getRange())), sink, nullptr);
      }
      else
      {
         parameters[i].first = nullptr;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameter " + STR(i) + " missing from function body");
      }
   }

   // Check if the function returns a supported value type. If not, no return
   // value matching is done
   const auto ret_type = tree_helper::GetFunctionReturnType(fd);
   bool noReturn = ret_type == nullptr || !isValidType(ret_type);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Function has " + (noReturn ? "no return type" : ("return type " + ret_type->get_kind_text())));

   // Creates the data structure which receives the return values of the
   // function, if there is any
   std::vector<VarNode*> returnVars;
   if(!noReturn)
   {
      const auto* SL = GetPointer<const statement_list>(GET_CONST_NODE(FD->body));
      for(const auto& idxBB : SL->list_of_bloc)
      {
         const auto& stmt_list = idxBB.second->CGetStmtList();

         if(stmt_list.size())
            if(const auto* gr = GetPointer<const gimple_return>(GET_CONST_NODE(stmt_list.back())))
               if(gr->op != nullptr) // Compiler defined return statements may be without argument
               {
                  returnVars.push_back(CG->addVarNode(gr->op, function_id));
               }
      }
   }
   if(returnVars.empty() && !noReturn)
   {
#ifndef NDEBUG
      if(isValidType(ret_type))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Function should return, but no return statement was found");
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Function return type not supported");
      }
#endif
      noReturn = true;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, std::string("Function ") + (noReturn ? "has no" : "has explicit") + " return statement" + (returnVars.size() > 1 ? "s" : ""));

   for(const auto& call : functionCalls)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing call " + GET_CONST_NODE(call)->ToString());
      const std::vector<tree_nodeRef>* args = nullptr;
      tree_nodeConstRef ret_var = nullptr;
      if(const auto* ga = GetPointer<const gimple_assign>(GET_CONST_NODE(call)))
      {
         const auto* ce = GetPointer<const call_expr>(GET_CONST_NODE(ga->op1));
         args = &ce->args;
         ret_var = ga->op0;
      }
      else if(const auto* gc = GetPointer<const gimple_call>(GET_CONST_NODE(call)))
      {
         args = &gc->args;
      }
      else
      {
         THROW_UNREACHABLE("Call statement should be a gimple_assign or a gimple_call");
      }

      THROW_ASSERT(args->size() == parameters.size(), "Function parameters and call arguments size mismatch");
      for(size_t i = 0; i < parameters.size(); ++i)
      {
         parameters[i].second = args->at(i);
      }

      // Do the inter-procedural construction of CG
      VarNode* to = nullptr;
      VarNode* from = nullptr;

      // Match formal and real parameters
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(size_t i = 0; i < parameters.size(); ++i)
      {
         if(parameters[i].first == nullptr)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameter " + STR(i) + " was constant, matching not necessary");
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, GET_CONST_NODE(parameters[i].second)->ToString() + " bound to argument " + GET_CONST_NODE(parameters[i].first)->ToString());
         // Add real parameter to the CG
         from = CG->addVarNode(parameters[i].second, function_id);

         // Connect nodes
         matchers[i]->addSource(from);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

      // Match return values when return type is stored from caller
      if(!noReturn && GET_CONST_NODE(call)->get_kind() != gimple_call_K)
      {
         // Add caller instruction to the CG (it receives the return value)
         to = CG->addVarNode(ret_var, function_id);
         to->setRange(to->getMaxRange());

         auto* phiOp = new PhiOpNode(ValueRangeRef(new ValueRange(to->getRange())), to, nullptr);
         for(VarNode* var : returnVars)
         {
            phiOp->addSource(var);
         }
         CG->pushOperation(phiOp);

#ifndef NDEBUG
         if(DEBUG_LEVEL_VERY_PEDANTIC <= debug_level)
         {
            std::string phiString = "Return variable " + GET_CONST_NODE(phiOp->getSink()->getValue())->ToString() + " = PHI<";
            for(size_t i = 0; i < phiOp->getNumSources(); ++i)
            {
               phiString += GET_CONST_NODE(phiOp->getSource(i)->getValue())->ToString() + ", ";
            }
            phiString[phiString.size() - 2] = '>';
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, phiString);
         }
#endif
      }

      // Real parameters are cleaned before moving to the next use (for safety's
      // sake)
      for(auto& pair : parameters)
      {
         pair.second = nullptr;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   for(auto* m : matchers)
   {
      if(m == nullptr)
      {
         continue;
      }
      CG->pushOperation(m);
#ifndef NDEBUG
      if(DEBUG_LEVEL_VERY_PEDANTIC <= debug_level)
      {
         std::string phiString = GET_CONST_NODE(m->getSink()->getValue())->ToString() + " = PHI<";
         for(size_t i = 0; i < m->getNumSources(); ++i)
         {
            phiString += GET_CONST_NODE(m->getSource(i)->getValue())->ToString() + ", ";
         }
         phiString[phiString.size() - 2] = '>';
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, phiString);
      }
#endif
   }
}

// ========================================================================== //
// RangeAnalysis
// ========================================================================== //
RangeAnalysis::RangeAnalysis(const application_managerRef AM, const DesignFlowManagerConstRef dfm, const ParameterConstRef par)
    : ApplicationFrontendFlowStep(AM, RANGE_ANALYSIS, dfm, par)
#ifndef NDEBUG
      ,
      graph_debug(DEBUG_LEVEL_NONE),
      iteration(0),
      stop_iteration(std::numeric_limits<decltype(stop_iteration)>::max()),
      stop_transformation(std::numeric_limits<decltype(stop_transformation)>::max())
#endif
      ,
      solverType(st_Cousot),
      requireESSA(true),
      execution_mode(RA_EXEC_NORMAL)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
   if(!parameters->isOption(OPT_soft_float) || !parameters->getOption<bool>(OPT_soft_float))
   {
      enable_softfloat = false;
   }
   const auto opts = SplitString(parameters->getOption<std::string>(OPT_range_analysis_mode), ",");
   CustomSet<std::string> ra_mode;
   for(const auto& opt : opts)
   {
      if(opt.size())
      {
         ra_mode.insert(opt);
      }
   }
   if(ra_mode.erase("crop"))
   {
      solverType = st_Crop;
   }
   if(ra_mode.erase("noESSA"))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: no Extended SSA required");
      requireESSA = false;
   }
#ifndef NDEBUG
   if(ra_mode.erase("ro"))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: read-only mode enabled");
      execution_mode = RA_EXEC_READONLY;
   }
#endif
   if(ra_mode.erase("skip"))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: skip mode enabled");
      execution_mode = RA_EXEC_SKIP;
   }
#ifndef NDEBUG
   if(ra_mode.erase("debug_op"))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: range operations debug");
      OpNode::debug_level = debug_level;
   }
   if(ra_mode.erase("debug_graph"))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: graph debug");
      graph_debug = debug_level;
      Meet::debug_level = debug_level;
   }
   OPERATION_OPTION(ra_mode, add);
   OPERATION_OPTION(ra_mode, sub);
   OPERATION_OPTION(ra_mode, mul);
   OPERATION_OPTION(ra_mode, sdiv);
   OPERATION_OPTION(ra_mode, udiv);
   OPERATION_OPTION(ra_mode, srem);
   OPERATION_OPTION(ra_mode, urem);
   OPERATION_OPTION(ra_mode, shl);
   OPERATION_OPTION(ra_mode, shr);
   OPERATION_OPTION(ra_mode, abs);
   OPERATION_OPTION(ra_mode, negate);
   OPERATION_OPTION(ra_mode, not );
   OPERATION_OPTION(ra_mode, and);
   OPERATION_OPTION(ra_mode, or);
   OPERATION_OPTION(ra_mode, xor);
   OPERATION_OPTION(ra_mode, sext);
   OPERATION_OPTION(ra_mode, zext);
   OPERATION_OPTION(ra_mode, trunc);
   OPERATION_OPTION(ra_mode, min);
   OPERATION_OPTION(ra_mode, max);
   OPERATION_OPTION(ra_mode, float_pack);
   OPERATION_OPTION(ra_mode, view_convert);
   OPERATION_OPTION(ra_mode, float_unpack);
   OPERATION_OPTION(ra_mode, load);
   OPERATION_OPTION(ra_mode, ternary);
   OPERATION_OPTION(ra_mode, bit_phi);
   if(ra_mode.size() && ra_mode.begin()->size())
   {
      THROW_ASSERT(ra_mode.size() <= 2, "Too many range analysis options left to parse");
      auto it = ra_mode.begin();
      if(ra_mode.size() == 2)
      {
         auto tr = ++ra_mode.begin();
         if(it->front() == 't')
         {
            it = ++ra_mode.begin();
            tr = ra_mode.begin();
         }
         THROW_ASSERT(tr->front() == 't', "Invalid range analysis option: " + *tr);
         stop_transformation = std::strtoull(tr->data() + sizeof(char), nullptr, 10);
         if(stop_transformation == 0)
         {
            THROW_ERROR("Invalid range analysis option: " + *tr);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: only " + STR(stop_transformation) + " transformation" + (stop_transformation > 1 ? "s" : "") + " will run on last iteration");
      }
      if(it->front() == 'i')
      {
         stop_iteration = std::strtoull(it->data() + sizeof(char), nullptr, 10);
      }
      else
      {
         stop_iteration = std::strtoull(it->data(), nullptr, 10);
      }
      if(stop_iteration == 0)
      {
         THROW_ERROR("Invalid range analysis option: " + *it);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range analysis: only " + STR(stop_iteration) + " iteration" + (stop_iteration > 1 ? "s" : "") + " will run");
   }
#else
   THROW_ASSERT(ra_mode.empty(), "Invalid range analysis mode falgs. (" + *ra_mode.begin() + ")");
#endif
}

RangeAnalysis::~RangeAnalysis() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> RangeAnalysis::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   if(execution_mode == RA_EXEC_SKIP)
   {
      return relationships;
   }
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         if(requireESSA)
         {
            relationships.insert(std::make_pair(ESSA, ALL_FUNCTIONS));
         }
         relationships.insert(std::make_pair(FUNCTION_PARM_MASK, WHOLE_APPLICATION));
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
         relationships.insert(std::make_pair(BIT_VALUE, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(IR_LOWERING, ALL_FUNCTIONS));
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

void RangeAnalysis::ComputeRelationships(DesignFlowStepSet& relationships, const DesignFlowStep::RelationshipType relationship_type)
{
   if(relationship_type == INVALIDATION_RELATIONSHIP)
   {
      for(const auto f_id : fun_id_to_restart)
      {
         const std::string dce_signature = FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::DEAD_CODE_ELIMINATION, f_id);
         vertex frontend_dce = design_flow_manager.lock()->GetDesignFlowStep(dce_signature);
         THROW_ASSERT(frontend_dce != NULL_VERTEX, "step " + dce_signature + " is not present");

         const std::string bv_signature = FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::BIT_VALUE, f_id);
         vertex frontend_bv = design_flow_manager.lock()->GetDesignFlowStep(bv_signature);
         THROW_ASSERT(frontend_bv != NULL_VERTEX, "step " + bv_signature + " is not present");

         const auto design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
         const auto bv = design_flow_graph->CGetDesignFlowStepInfo(frontend_bv)->design_flow_step;
         const auto dce = design_flow_graph->CGetDesignFlowStepInfo(frontend_dce)->design_flow_step;
         relationships.insert(dce);
         relationships.insert(bv);
      }
      fun_id_to_restart.clear();
   }
   ApplicationFrontendFlowStep::ComputeRelationships(relationships, relationship_type);
}

bool RangeAnalysis::HasToBeExecuted() const
{
#ifndef NDEBUG
   if(iteration >= stop_iteration || execution_mode == RA_EXEC_SKIP)
#else
   if(execution_mode == RA_EXEC_SKIP)
#endif
   {
      return false;
   }
   std::map<unsigned int, unsigned int> cur_bitvalue_ver;
   std::map<unsigned int, unsigned int> cur_bb_ver;
   const auto CGMan = AppM->CGetCallGraphManager();
   for(const auto i : CGMan->GetReachedBodyFunctions())
   {
      const auto FB = AppM->CGetFunctionBehavior(i);
      cur_bitvalue_ver[i] = FB->GetBitValueVersion();
      cur_bb_ver[i] = FB->GetBBVersion();
   }
   return cur_bb_ver != last_bb_ver || cur_bitvalue_ver != last_bitvalue_ver;
}

void RangeAnalysis::Initialize()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range Analysis step");
   fun_id_to_restart.clear();
}

DesignFlowStep_Status RangeAnalysis::Exec()
{
#ifndef NDEBUG
   if(iteration >= stop_iteration || execution_mode == RA_EXEC_SKIP)
#else
   if(execution_mode == RA_EXEC_SKIP)
#endif
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Range analysis no execution mode enabled");
      return DesignFlowStep_Status::SKIPPED;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");

   // Initialize constraint graph
   ConstraintGraphRef CG;
   switch(solverType)
   {
      case st_Cousot:
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Using jump-set abstract operators");
         CG.reset(new Cousot(AppM,
#ifndef NDEBUG
                             debug_level, graph_debug));
#else
                             DEBUG_LEVEL_NONE, DEBUG_LEVEL_NONE));
#endif
         break;
      case st_Crop:
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Using standard abstract operators");
         CG.reset(new CropDFS(AppM,
#ifndef NDEBUG
                              debug_level, graph_debug));
#else
                              DEBUG_LEVEL_NONE, DEBUG_LEVEL_NONE));
#endif
         break;
      default:
         THROW_UNREACHABLE("Unknown solver type " + STR(solverType));
         break;
   }

      // Analyse only reached functions
#if defined(EARLY_DEAD_CODE_RESTART) || !defined(NDEBUG)
   const auto TM = AppM->get_tree_manager();
#endif
   auto rb_funcs = AppM->CGetCallGraphManager()->GetReachedBodyFunctions();

#ifdef EARLY_DEAD_CODE_RESTART
   for(const auto f : rb_funcs)
   {
      bool dead_code_necessary = CG->buildGraph(f);
      if(dead_code_necessary)
      {
         fun_id_to_restart.insert(f);
      }
   }
   if(fun_id_to_restart.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Following functions have unpropagated constants:");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(const auto f_id : fun_id_to_restart)
      {
         const auto FB = AppM->GetFunctionBehavior(f_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, tree_helper::print_type(TM, f_id, false, true, false, 0U, var_pp_functorConstRef(new std_var_pp_functor(FB->CGetBehavioralHelper()))));
         FB->UpdateBBVersion();
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Unpropagated constants detected, aborting...");
      return DesignFlowStep_Status::ABORTED;
   }
#else
   for(const auto& f : rb_funcs)
   {
      CG->buildGraph(f);
   }
#endif

   // Top functions are not called by any other functions, so they do not have any call statement to analyse
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameters and return value propagation...");
   for(const auto& top_fn : AppM->CGetCallGraphManager()->GetRootFunctions())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, tree_helper::print_type(TM, top_fn, false, true, false, 0U, var_pp_functorConstRef(new std_var_pp_functor(AppM->CGetFunctionBehavior(top_fn)->CGetBehavioralHelper()))) + " is top function");
      TopFunctionUserHits(top_fn, AppM, CG, debug_level);
      rb_funcs.erase(top_fn);
   }
   // The two operations are split because the CallMap is built for all functions in buildGraph
   // then it is used from ParmAndRetValPropagation
   for(const auto f : rb_funcs)
   {
      ParmAndRetValPropagation(f, AppM, CG, debug_level);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameters and return value propagation completed");
   CG->buildVarNodes();

#ifndef NDEBUG
   CG->findIntervals(parameters, GetName() + "(" + STR(iteration) + ")");
   ++iteration;
#else
   CG->findIntervals();
#endif

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
#ifndef NDEBUG
   const auto modified = finalize(CG);
   if(stop_iteration != std::numeric_limits<decltype(stop_iteration)>::max())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Iteration " + STR(iteration) + "/" + STR(stop_iteration) + "completed (" + STR(stop_iteration - iteration) + " to go)");
   }
   if(modified)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variable ranges updated");
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variable ranges reached fixed point");
   }
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
#else
   return finalize(CG) ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
#endif
}

bool RangeAnalysis::finalize(ConstraintGraphRef CG)
{
   THROW_ASSERT(CG, "");
   const auto& vars = std::static_pointer_cast<const ConstraintGraph>(CG)->getVarNodes();
   CustomMap<unsigned int, int> modifiedFunctions;
#ifndef NDEBUG
   if(execution_mode >= RA_EXEC_READONLY)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Bounds for " + STR(vars.size()) + " variables");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      for(const auto& varNode : vars)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Range " + varNode.second->getRange()->ToString() + " for " + GET_CONST_NODE(varNode.first)->ToString());
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "IR update not applied in read-only mode");
   }
   else
   {
#endif
      const auto TM = AppM->get_tree_manager();
      const auto tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));

#ifndef NDEBUG
      unsigned long long updated = 0;
#endif
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Bounds for " + STR(vars.size()) + " variables");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      const auto dfm = design_flow_manager.lock();
      for(const auto& varNode : vars)
      {
#ifndef NDEBUG
         if(iteration == stop_iteration && updated >= stop_transformation)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Max required transformations performed. IR update aborted.");
            break;
         }
#endif
         if(const auto ut = varNode.second->updateIR(TM, tree_man, dfm
#ifndef NDEBUG
                                                     ,
                                                     debug_level, AppM
#endif
                                                     ))
         {
            const auto funID = varNode.second->getFunctionId();
            modifiedFunctions[funID] |= ut;
#ifndef NDEBUG
            ++updated;
#endif
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Bounds updated for " + STR(updated) + "/" + STR(vars.size()) + " variables");
#ifndef NDEBUG
   }
#endif

   const auto rbf = AppM->CGetCallGraphManager()->GetReachedBodyFunctions();
   const auto cgm = AppM->CGetCallGraphManager();
   const auto cg = cgm->CGetCallGraph();
#ifndef NDEBUG
   const auto TM = AppM->get_tree_manager();
#endif

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Modified " + STR(modifiedFunctions.size()) + " functions:");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   // Previous steps must be invalidated for caller functions too
   for(const auto fUT : modifiedFunctions)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, tree_helper::print_type(TM, fUT.first, false, true, false, 0U, var_pp_functorConstRef(new std_var_pp_functor(AppM->CGetFunctionBehavior(fUT.first)->CGetBehavioralHelper()))));
      fun_id_to_restart.insert(fUT.first);
      // Restart necessary steps on caller functions only for constant propagation
      if(fUT.second & ut_Constant)
      {
         const auto f_v = cgm->GetVertex(fUT.first);
         for(const auto& caller : boost::make_iterator_range(boost::in_edges(f_v, *cg)))
         {
            const auto caller_id = cgm->get_function(boost::source(caller, *cg));
            if(rbf.count(caller_id) && cg->CGetFunctionEdgeInfo(caller)->direct_call_points.size())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + tree_helper::print_type(TM, caller_id, false, true, false, 0U, var_pp_functorConstRef(new std_var_pp_functor(AppM->CGetFunctionBehavior(caller_id)->CGetBehavioralHelper()))));
               fun_id_to_restart.insert(caller_id);
            }
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

   for(const auto f : rbf)
   {
      const auto FB = AppM->GetFunctionBehavior(f);
      if(fun_id_to_restart.count(f))
      {
         last_bitvalue_ver[f] = FB->UpdateBitValueVersion();
         last_bb_ver[f] = FB->UpdateBBVersion();
      }
      else
      {
         last_bitvalue_ver[f] = FB->GetBitValueVersion();
         last_bb_ver[f] = FB->GetBBVersion();
      }
   }
   return !fun_id_to_restart.empty();
}
