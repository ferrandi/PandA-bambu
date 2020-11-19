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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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
 * @file tree_node_factory.cpp
 * @brief tree node factory. This class, exploiting the visitor design pattern, add a tree node to the tree_manager.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_HAVE_CODE_ESTIMATION_BUILT.hpp"

/// parser/treegcc include
#include "token_interface.hpp"

/// tree includes
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_node_factory.hpp"
#include "tree_reindex.hpp"
#if HAVE_CODE_ESTIMATION_BUILT
#include "weight_information.hpp"
#endif
#include "utility.hpp"

#define CREATE_TREE_NODE_CASE_BODY(tree_node_name, node_id) \
   {                                                        \
      auto tnn = new tree_node_name(node_id);               \
      tree_nodeRef cur = tree_nodeRef(tnn);                 \
      TM.AddTreeNode(node_id, cur);                         \
      curr_tree_node_ptr = tnn;                             \
      tnn->visit(this);                                     \
      curr_tree_node_ptr = nullptr;                         \
      break;                                                \
   }

void tree_node_factory::create_tree_node(unsigned int node_id, enum kind tree_node_type)
{
   switch(tree_node_type)
   {
      case abs_expr_K:
         CREATE_TREE_NODE_CASE_BODY(abs_expr, node_id)
      case addr_expr_K:
         CREATE_TREE_NODE_CASE_BODY(addr_expr, node_id)
      case array_range_ref_K:
         CREATE_TREE_NODE_CASE_BODY(array_range_ref, node_id)
      case array_ref_K:
         CREATE_TREE_NODE_CASE_BODY(array_ref, node_id)
      case array_type_K:
         CREATE_TREE_NODE_CASE_BODY(array_type, node_id)
      case arrow_expr_K:
         CREATE_TREE_NODE_CASE_BODY(arrow_expr, node_id)
      case gimple_asm_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_asm, node_id)
      case baselink_K:
         CREATE_TREE_NODE_CASE_BODY(baselink, node_id)
      case gimple_bind_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_bind, node_id)
      case binfo_K:
         CREATE_TREE_NODE_CASE_BODY(binfo, node_id)
      case bit_and_expr_K:
         CREATE_TREE_NODE_CASE_BODY(bit_and_expr, node_id)
      case bit_field_ref_K:
         CREATE_TREE_NODE_CASE_BODY(bit_field_ref, node_id)
      case bit_ior_expr_K:
         CREATE_TREE_NODE_CASE_BODY(bit_ior_expr, node_id)
      case bit_ior_concat_expr_K:
         CREATE_TREE_NODE_CASE_BODY(bit_ior_concat_expr, node_id)
      case bit_not_expr_K:
         CREATE_TREE_NODE_CASE_BODY(bit_not_expr, node_id)
      case bit_xor_expr_K:
         CREATE_TREE_NODE_CASE_BODY(bit_xor_expr, node_id)
      case block_K:
         CREATE_TREE_NODE_CASE_BODY(block, node_id)
      case boolean_type_K:
         CREATE_TREE_NODE_CASE_BODY(boolean_type, node_id)
      case buffer_ref_K:
         CREATE_TREE_NODE_CASE_BODY(buffer_ref, node_id)
      case call_expr_K:
         CREATE_TREE_NODE_CASE_BODY(call_expr, node_id)
      case aggr_init_expr_K:
         CREATE_TREE_NODE_CASE_BODY(aggr_init_expr, node_id)
      case gimple_call_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_call, node_id)
      case card_expr_K:
         CREATE_TREE_NODE_CASE_BODY(card_expr, node_id)
      case case_label_expr_K:
         CREATE_TREE_NODE_CASE_BODY(case_label_expr, node_id)
      case cast_expr_K:
         CREATE_TREE_NODE_CASE_BODY(cast_expr, node_id)
      case catch_expr_K:
         CREATE_TREE_NODE_CASE_BODY(catch_expr, node_id)
      case ceil_div_expr_K:
         CREATE_TREE_NODE_CASE_BODY(ceil_div_expr, node_id)
      case ceil_mod_expr_K:
         CREATE_TREE_NODE_CASE_BODY(ceil_mod_expr, node_id)
      case CharType_K:
         CREATE_TREE_NODE_CASE_BODY(CharType, node_id)
      case nullptr_type_K:
         CREATE_TREE_NODE_CASE_BODY(nullptr_type, node_id)
      case type_pack_expansion_K:
         CREATE_TREE_NODE_CASE_BODY(type_pack_expansion, node_id)
      case cleanup_point_expr_K:
         CREATE_TREE_NODE_CASE_BODY(cleanup_point_expr, node_id)
      case complex_cst_K:
         CREATE_TREE_NODE_CASE_BODY(complex_cst, node_id)
      case complex_expr_K:
         CREATE_TREE_NODE_CASE_BODY(complex_expr, node_id)
      case complex_type_K:
         CREATE_TREE_NODE_CASE_BODY(complex_type, node_id)
      case component_ref_K:
         CREATE_TREE_NODE_CASE_BODY(component_ref, node_id)
      case compound_expr_K:
         CREATE_TREE_NODE_CASE_BODY(compound_expr, node_id)
      case cond_expr_K:
         CREATE_TREE_NODE_CASE_BODY(cond_expr, node_id)
      case gimple_cond_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_cond, node_id)
      case conj_expr_K:
         CREATE_TREE_NODE_CASE_BODY(conj_expr, node_id)
      case const_decl_K:
         CREATE_TREE_NODE_CASE_BODY(const_decl, node_id)
      case constructor_K:
         CREATE_TREE_NODE_CASE_BODY(constructor, node_id)
      case convert_expr_K:
         CREATE_TREE_NODE_CASE_BODY(convert_expr, node_id)
      case ctor_initializer_K:
         CREATE_TREE_NODE_CASE_BODY(ctor_initializer, node_id)
      case eh_filter_expr_K:
         CREATE_TREE_NODE_CASE_BODY(eh_filter_expr, node_id)
      case enumeral_type_K:
         CREATE_TREE_NODE_CASE_BODY(enumeral_type, node_id)
      case eq_expr_K:
         CREATE_TREE_NODE_CASE_BODY(eq_expr, node_id)
      case error_mark_K:
         CREATE_TREE_NODE_CASE_BODY(error_mark, node_id)
      case exact_div_expr_K:
         CREATE_TREE_NODE_CASE_BODY(exact_div_expr, node_id)
      case exit_expr_K:
         CREATE_TREE_NODE_CASE_BODY(exit_expr, node_id)
      case expr_stmt_K:
         CREATE_TREE_NODE_CASE_BODY(expr_stmt, node_id)
      case fdesc_expr_K:
         CREATE_TREE_NODE_CASE_BODY(fdesc_expr, node_id)
      case field_decl_K:
         CREATE_TREE_NODE_CASE_BODY(field_decl, node_id)
      case fix_ceil_expr_K:
         CREATE_TREE_NODE_CASE_BODY(fix_ceil_expr, node_id)
      case fix_floor_expr_K:
         CREATE_TREE_NODE_CASE_BODY(fix_floor_expr, node_id)
      case fix_round_expr_K:
         CREATE_TREE_NODE_CASE_BODY(fix_round_expr, node_id)
      case fix_trunc_expr_K:
         CREATE_TREE_NODE_CASE_BODY(fix_trunc_expr, node_id)
      case float_expr_K:
         CREATE_TREE_NODE_CASE_BODY(float_expr, node_id)
      case floor_div_expr_K:
         CREATE_TREE_NODE_CASE_BODY(floor_div_expr, node_id)
      case floor_mod_expr_K:
         CREATE_TREE_NODE_CASE_BODY(floor_mod_expr, node_id)
      case function_decl_K:
         CREATE_TREE_NODE_CASE_BODY(function_decl, node_id)
      case function_type_K:
         CREATE_TREE_NODE_CASE_BODY(function_type, node_id)
      case ge_expr_K:
         CREATE_TREE_NODE_CASE_BODY(ge_expr, node_id)
      case gimple_assign_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_assign, node_id)
      case gimple_goto_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_goto, node_id)
      case goto_subroutine_K:
         CREATE_TREE_NODE_CASE_BODY(goto_subroutine, node_id)
      case gt_expr_K:
         CREATE_TREE_NODE_CASE_BODY(gt_expr, node_id)
      case handler_K:
         CREATE_TREE_NODE_CASE_BODY(handler, node_id)
      case imagpart_expr_K:
         CREATE_TREE_NODE_CASE_BODY(imagpart_expr, node_id)
      case indirect_ref_K:
         CREATE_TREE_NODE_CASE_BODY(indirect_ref, node_id)
      case misaligned_indirect_ref_K:
         CREATE_TREE_NODE_CASE_BODY(misaligned_indirect_ref, node_id)
      case in_expr_K:
         CREATE_TREE_NODE_CASE_BODY(in_expr, node_id)
      case init_expr_K:
         CREATE_TREE_NODE_CASE_BODY(init_expr, node_id)
      case integer_cst_K:
         CREATE_TREE_NODE_CASE_BODY(integer_cst, node_id)
      case integer_type_K:
         CREATE_TREE_NODE_CASE_BODY(integer_type, node_id)
      case label_decl_K:
         CREATE_TREE_NODE_CASE_BODY(label_decl, node_id)
      case gimple_label_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_label, node_id)
      case lang_type_K:
         CREATE_TREE_NODE_CASE_BODY(lang_type, node_id)
      case le_expr_K:
         CREATE_TREE_NODE_CASE_BODY(le_expr, node_id)
      case loop_expr_K:
         CREATE_TREE_NODE_CASE_BODY(loop_expr, node_id)
      case lut_expr_K:
         CREATE_TREE_NODE_CASE_BODY(lut_expr, node_id)
      case lrotate_expr_K:
         CREATE_TREE_NODE_CASE_BODY(lrotate_expr, node_id)
      case lshift_expr_K:
         CREATE_TREE_NODE_CASE_BODY(lshift_expr, node_id)
      case lt_expr_K:
         CREATE_TREE_NODE_CASE_BODY(lt_expr, node_id)
      case ltgt_expr_K:
         CREATE_TREE_NODE_CASE_BODY(ltgt_expr, node_id)
      case max_expr_K:
         CREATE_TREE_NODE_CASE_BODY(max_expr, node_id)
      case method_type_K:
         CREATE_TREE_NODE_CASE_BODY(method_type, node_id)
      case min_expr_K:
         CREATE_TREE_NODE_CASE_BODY(min_expr, node_id)
      case minus_expr_K:
         CREATE_TREE_NODE_CASE_BODY(minus_expr, node_id)
      case modify_expr_K:
         CREATE_TREE_NODE_CASE_BODY(modify_expr, node_id)
      case modop_expr_K:
         CREATE_TREE_NODE_CASE_BODY(modop_expr, node_id)
      case mult_expr_K:
         CREATE_TREE_NODE_CASE_BODY(mult_expr, node_id)
      case mult_highpart_expr_K:
         CREATE_TREE_NODE_CASE_BODY(mult_highpart_expr, node_id)
      case gimple_multi_way_if_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_multi_way_if, node_id)
      case namespace_decl_K:
         CREATE_TREE_NODE_CASE_BODY(namespace_decl, node_id)
      case ne_expr_K:
         CREATE_TREE_NODE_CASE_BODY(ne_expr, node_id)
      case negate_expr_K:
         CREATE_TREE_NODE_CASE_BODY(negate_expr, node_id)
      case new_expr_K:
         CREATE_TREE_NODE_CASE_BODY(new_expr, node_id)
      case non_lvalue_expr_K:
         CREATE_TREE_NODE_CASE_BODY(non_lvalue_expr, node_id)
      case nop_expr_K:
         CREATE_TREE_NODE_CASE_BODY(nop_expr, node_id)
      case obj_type_ref_K:
         CREATE_TREE_NODE_CASE_BODY(obj_type_ref, node_id)
      case offset_type_K:
         CREATE_TREE_NODE_CASE_BODY(offset_type, node_id)
      case ordered_expr_K:
         CREATE_TREE_NODE_CASE_BODY(ordered_expr, node_id)
      case overload_K:
         CREATE_TREE_NODE_CASE_BODY(overload, node_id)
      case parm_decl_K:
         CREATE_TREE_NODE_CASE_BODY(parm_decl, node_id)
      case gimple_phi_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_phi, node_id)
      case placeholder_expr_K:
         CREATE_TREE_NODE_CASE_BODY(placeholder_expr, node_id)
      case plus_expr_K:
         CREATE_TREE_NODE_CASE_BODY(plus_expr, node_id)
      case pointer_plus_expr_K:
         CREATE_TREE_NODE_CASE_BODY(pointer_plus_expr, node_id)
      case pointer_type_K:
         CREATE_TREE_NODE_CASE_BODY(pointer_type, node_id)
      case postdecrement_expr_K:
         CREATE_TREE_NODE_CASE_BODY(postdecrement_expr, node_id)
      case postincrement_expr_K:
         CREATE_TREE_NODE_CASE_BODY(postincrement_expr, node_id)
      case predecrement_expr_K:
         CREATE_TREE_NODE_CASE_BODY(predecrement_expr, node_id)
      case preincrement_expr_K:
         CREATE_TREE_NODE_CASE_BODY(preincrement_expr, node_id)
      case qual_union_type_K:
         CREATE_TREE_NODE_CASE_BODY(qual_union_type, node_id)
      case range_expr_K:
         CREATE_TREE_NODE_CASE_BODY(range_expr, node_id)
      case paren_expr_K:
         CREATE_TREE_NODE_CASE_BODY(paren_expr, node_id)
      case rdiv_expr_K:
         CREATE_TREE_NODE_CASE_BODY(rdiv_expr, node_id)
      case real_cst_K:
         CREATE_TREE_NODE_CASE_BODY(real_cst, node_id)
      case realpart_expr_K:
         CREATE_TREE_NODE_CASE_BODY(realpart_expr, node_id)
      case real_type_K:
         CREATE_TREE_NODE_CASE_BODY(real_type, node_id)
      case record_type_K:
         CREATE_TREE_NODE_CASE_BODY(record_type, node_id)
      case reduc_max_expr_K:
         CREATE_TREE_NODE_CASE_BODY(reduc_max_expr, node_id)
      case reduc_min_expr_K:
         CREATE_TREE_NODE_CASE_BODY(reduc_min_expr, node_id)
      case reduc_plus_expr_K:
         CREATE_TREE_NODE_CASE_BODY(reduc_plus_expr, node_id)
      case reference_expr_K:
         CREATE_TREE_NODE_CASE_BODY(reference_expr, node_id)
      case reference_type_K:
         CREATE_TREE_NODE_CASE_BODY(reference_type, node_id)
      case reinterpret_cast_expr_K:
         CREATE_TREE_NODE_CASE_BODY(reinterpret_cast_expr, node_id)
      case result_decl_K:
         CREATE_TREE_NODE_CASE_BODY(result_decl, node_id)
      case gimple_resx_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_resx, node_id)
      case gimple_return_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_return, node_id)
      case return_stmt_K:
         CREATE_TREE_NODE_CASE_BODY(return_stmt, node_id)
      case round_div_expr_K:
         CREATE_TREE_NODE_CASE_BODY(round_div_expr, node_id)
      case round_mod_expr_K:
         CREATE_TREE_NODE_CASE_BODY(round_mod_expr, node_id)
      case rrotate_expr_K:
         CREATE_TREE_NODE_CASE_BODY(rrotate_expr, node_id)
      case rshift_expr_K:
         CREATE_TREE_NODE_CASE_BODY(rshift_expr, node_id)
      case save_expr_K:
         CREATE_TREE_NODE_CASE_BODY(save_expr, node_id)
      case scope_ref_K:
         CREATE_TREE_NODE_CASE_BODY(scope_ref, node_id)
      case set_le_expr_K:
         CREATE_TREE_NODE_CASE_BODY(set_le_expr, node_id)
      case set_type_K:
         CREATE_TREE_NODE_CASE_BODY(set_type, node_id)
      case sizeof_expr_K:
         CREATE_TREE_NODE_CASE_BODY(sizeof_expr, node_id)
      case ssa_name_K:
         CREATE_TREE_NODE_CASE_BODY(ssa_name, node_id)
      case statement_list_K:
         CREATE_TREE_NODE_CASE_BODY(statement_list, node_id)
      case static_cast_expr_K:
         CREATE_TREE_NODE_CASE_BODY(static_cast_expr, node_id)
      case string_cst_K:
         CREATE_TREE_NODE_CASE_BODY(string_cst, node_id)
      case gimple_switch_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_switch, node_id)
      case target_expr_K:
         CREATE_TREE_NODE_CASE_BODY(target_expr, node_id)
      case target_mem_ref_K:
         CREATE_TREE_NODE_CASE_BODY(target_mem_ref, node_id)
      case target_mem_ref461_K:
         CREATE_TREE_NODE_CASE_BODY(target_mem_ref461, node_id)
      case mem_ref_K:
         CREATE_TREE_NODE_CASE_BODY(mem_ref, node_id)
      case template_decl_K:
         CREATE_TREE_NODE_CASE_BODY(template_decl, node_id)
      case template_id_expr_K:
         CREATE_TREE_NODE_CASE_BODY(template_id_expr, node_id)
      case template_parm_index_K:
         CREATE_TREE_NODE_CASE_BODY(template_parm_index, node_id)
      case template_type_parm_K:
         CREATE_TREE_NODE_CASE_BODY(template_type_parm, node_id)
      case ternary_plus_expr_K:
         CREATE_TREE_NODE_CASE_BODY(ternary_plus_expr, node_id)
      case ternary_pm_expr_K:
         CREATE_TREE_NODE_CASE_BODY(ternary_pm_expr, node_id)
      case ternary_mp_expr_K:
         CREATE_TREE_NODE_CASE_BODY(ternary_mp_expr, node_id)
      case ternary_mm_expr_K:
         CREATE_TREE_NODE_CASE_BODY(ternary_mm_expr, node_id)
      case throw_expr_K:
         CREATE_TREE_NODE_CASE_BODY(throw_expr, node_id)
      case translation_unit_decl_K:
         CREATE_TREE_NODE_CASE_BODY(translation_unit_decl, node_id)
      case tree_list_K:
         CREATE_TREE_NODE_CASE_BODY(tree_list, node_id)
      case tree_vec_K:
         CREATE_TREE_NODE_CASE_BODY(tree_vec, node_id)
      case trunc_div_expr_K:
         CREATE_TREE_NODE_CASE_BODY(trunc_div_expr, node_id)
      case trunc_mod_expr_K:
         CREATE_TREE_NODE_CASE_BODY(trunc_mod_expr, node_id)
      case truth_and_expr_K:
         CREATE_TREE_NODE_CASE_BODY(truth_and_expr, node_id)
      case truth_andif_expr_K:
         CREATE_TREE_NODE_CASE_BODY(truth_andif_expr, node_id)
      case truth_not_expr_K:
         CREATE_TREE_NODE_CASE_BODY(truth_not_expr, node_id)
      case truth_or_expr_K:
         CREATE_TREE_NODE_CASE_BODY(truth_or_expr, node_id)
      case truth_orif_expr_K:
         CREATE_TREE_NODE_CASE_BODY(truth_orif_expr, node_id)
      case truth_xor_expr_K:
         CREATE_TREE_NODE_CASE_BODY(truth_xor_expr, node_id)
      case try_block_K:
         CREATE_TREE_NODE_CASE_BODY(try_block, node_id)
      case try_catch_expr_K:
         CREATE_TREE_NODE_CASE_BODY(try_catch_expr, node_id)
      case try_finally_K:
         CREATE_TREE_NODE_CASE_BODY(try_finally, node_id)
      case type_decl_K:
         CREATE_TREE_NODE_CASE_BODY(type_decl, node_id)
      case typename_type_K:
         CREATE_TREE_NODE_CASE_BODY(typename_type, node_id)
      case uneq_expr_K:
         CREATE_TREE_NODE_CASE_BODY(uneq_expr, node_id)
      case unge_expr_K:
         CREATE_TREE_NODE_CASE_BODY(unge_expr, node_id)
      case ungt_expr_K:
         CREATE_TREE_NODE_CASE_BODY(ungt_expr, node_id)
      case union_type_K:
         CREATE_TREE_NODE_CASE_BODY(union_type, node_id)
      case unle_expr_K:
         CREATE_TREE_NODE_CASE_BODY(unle_expr, node_id)
      case unlt_expr_K:
         CREATE_TREE_NODE_CASE_BODY(unlt_expr, node_id)
      case unordered_expr_K:
         CREATE_TREE_NODE_CASE_BODY(unordered_expr, node_id)
      case unsave_expr_K:
         CREATE_TREE_NODE_CASE_BODY(unsave_expr, node_id)
      case using_decl_K:
         CREATE_TREE_NODE_CASE_BODY(using_decl, node_id)
      case va_arg_expr_K:
         CREATE_TREE_NODE_CASE_BODY(va_arg_expr, node_id)
      case var_decl_K:
         CREATE_TREE_NODE_CASE_BODY(var_decl, node_id)
      case vec_cond_expr_K:
         CREATE_TREE_NODE_CASE_BODY(vec_cond_expr, node_id)
      case vec_perm_expr_K:
         CREATE_TREE_NODE_CASE_BODY(vec_perm_expr, node_id)
      case dot_prod_expr_K:
         CREATE_TREE_NODE_CASE_BODY(dot_prod_expr, node_id)
      case vec_lshift_expr_K:
         CREATE_TREE_NODE_CASE_BODY(vec_lshift_expr, node_id)
      case vec_rshift_expr_K:
         CREATE_TREE_NODE_CASE_BODY(vec_rshift_expr, node_id)
      case widen_mult_hi_expr_K:
         CREATE_TREE_NODE_CASE_BODY(widen_mult_hi_expr, node_id)
      case widen_mult_lo_expr_K:
         CREATE_TREE_NODE_CASE_BODY(widen_mult_lo_expr, node_id)
      case vec_unpack_hi_expr_K:
         CREATE_TREE_NODE_CASE_BODY(vec_unpack_hi_expr, node_id)
      case vec_unpack_lo_expr_K:
         CREATE_TREE_NODE_CASE_BODY(vec_unpack_lo_expr, node_id)
      case vec_unpack_float_hi_expr_K:
         CREATE_TREE_NODE_CASE_BODY(vec_unpack_float_hi_expr, node_id)
      case vec_unpack_float_lo_expr_K:
         CREATE_TREE_NODE_CASE_BODY(vec_unpack_float_lo_expr, node_id)
      case vec_pack_trunc_expr_K:
         CREATE_TREE_NODE_CASE_BODY(vec_pack_trunc_expr, node_id)
      case vec_pack_sat_expr_K:
         CREATE_TREE_NODE_CASE_BODY(vec_pack_sat_expr, node_id)
      case vec_pack_fix_trunc_expr_K:
         CREATE_TREE_NODE_CASE_BODY(vec_pack_fix_trunc_expr, node_id)
      case vec_extracteven_expr_K:
         CREATE_TREE_NODE_CASE_BODY(vec_extracteven_expr, node_id)
      case vec_extractodd_expr_K:
         CREATE_TREE_NODE_CASE_BODY(vec_extractodd_expr, node_id)
      case vec_interleavehigh_expr_K:
         CREATE_TREE_NODE_CASE_BODY(vec_interleavehigh_expr, node_id);
      case vec_interleavelow_expr_K:
         CREATE_TREE_NODE_CASE_BODY(vec_interleavelow_expr, node_id);
      case vec_new_expr_K:
         CREATE_TREE_NODE_CASE_BODY(vec_new_expr, node_id)
      case vector_cst_K:
         CREATE_TREE_NODE_CASE_BODY(vector_cst, node_id)
      case void_cst_K:
         CREATE_TREE_NODE_CASE_BODY(void_cst, node_id)
      case type_argument_pack_K:
         CREATE_TREE_NODE_CASE_BODY(type_argument_pack, node_id)
      case nontype_argument_pack_K:
         CREATE_TREE_NODE_CASE_BODY(nontype_argument_pack, node_id)
      case expr_pack_expansion_K:
         CREATE_TREE_NODE_CASE_BODY(expr_pack_expansion, node_id)
      case vector_type_K:
         CREATE_TREE_NODE_CASE_BODY(vector_type, node_id)
      case view_convert_expr_K:
         CREATE_TREE_NODE_CASE_BODY(view_convert_expr, node_id)
      case void_type_K:
         CREATE_TREE_NODE_CASE_BODY(void_type, node_id)
      case vtable_ref_K:
         CREATE_TREE_NODE_CASE_BODY(vtable_ref, node_id)
      case with_cleanup_expr_K:
         CREATE_TREE_NODE_CASE_BODY(with_cleanup_expr, node_id)
      case with_size_expr_K:
         CREATE_TREE_NODE_CASE_BODY(with_size_expr, node_id)
      case gimple_while_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_while, node_id)
      case gimple_for_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_for, node_id)
      case gimple_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_pragma, node_id)
      case omp_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(omp_pragma, node_id)
      case omp_atomic_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(omp_atomic_pragma, node_id)
      case omp_for_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(omp_for_pragma, node_id)
      case omp_critical_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(omp_critical_pragma, node_id)
      case omp_declare_simd_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(omp_declare_simd_pragma, node_id)
      case omp_simd_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(omp_simd_pragma, node_id)
      case omp_parallel_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(omp_parallel_pragma, node_id)
      case omp_sections_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(omp_sections_pragma, node_id)
      case omp_parallel_sections_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(omp_parallel_sections_pragma, node_id)
      case omp_section_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(omp_section_pragma, node_id)
      case omp_target_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(omp_target_pragma, node_id)
      case omp_task_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(omp_task_pragma, node_id)
      case map_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(map_pragma, node_id)
      case call_hw_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(call_hw_pragma, node_id)
      case call_point_hw_pragma_K:
      {
         CREATE_TREE_NODE_CASE_BODY(call_point_hw_pragma, node_id)
      }
      case issue_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(issue_pragma, node_id)
      case blackbox_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(blackbox_pragma, node_id)
      case profiling_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(profiling_pragma, node_id)
      case statistical_profiling_K:
         CREATE_TREE_NODE_CASE_BODY(statistical_profiling, node_id)
      case null_node_K:
         CREATE_TREE_NODE_CASE_BODY(null_node, node_id)
      case identifier_node_K: /// special care is reserved for identifier_nodes
      {
         tree_nodeRef cur;
         if(tree_node_schema.find(TOK(TOK_STRG)) != tree_node_schema.end())
         {
            cur = tree_nodeRef(new identifier_node(node_id, tree_node_schema.find(TOK(TOK_STRG))->second, &TM));
         }
         else if(tree_node_schema.find(TOK(TOK_OPERATOR)) != tree_node_schema.end())
         {
            cur = tree_nodeRef(new identifier_node(node_id, boost::lexical_cast<bool>(tree_node_schema.find(TOK(TOK_OPERATOR))->second), &TM));
         }
         else
         {
            THROW_ERROR("Incorrect schema for identifier_node: no TOK_STRG nor TOK_OPERATOR");
         }
         TM.AddTreeNode(node_id, cur);
         break;
      }
      case widen_sum_expr_K:
         CREATE_TREE_NODE_CASE_BODY(widen_sum_expr, node_id)
      case widen_mult_expr_K:
         CREATE_TREE_NODE_CASE_BODY(widen_mult_expr, node_id)
      case gimple_nop_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_nop, node_id)
      case extract_bit_expr_K:
         CREATE_TREE_NODE_CASE_BODY(extract_bit_expr, node_id)
      case sat_plus_expr_K:
         CREATE_TREE_NODE_CASE_BODY(sat_plus_expr, node_id)
      case sat_minus_expr_K:
         CREATE_TREE_NODE_CASE_BODY(sat_minus_expr, node_id)
      case assert_expr_K:
      case do_stmt_K:
      case for_stmt_K:
      case gimple_predict_K:
      case if_stmt_K:
      case last_tree_K:
      case none_K:
      case trait_expr_K:
      case tree_reindex_K:
      case while_stmt_K:
      {
         THROW_UNREACHABLE("Creation of tree node of type " + boost::lexical_cast<std::string>(tree_node_type) + " not implemented");
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
}

void tree_node_factory::operator()(const tree_node* obj, unsigned int&)
{
   THROW_ERROR("tree_node not supported: " + std::string(obj->get_kind_text()));
}

void tree_node_factory::operator()(const tree_reindex* obj, unsigned int&)
{
   THROW_ERROR("tree_node not supported: " + std::string(obj->get_kind_text()));
}

void tree_node_factory::operator()(const attr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == dynamic_cast<attr*>(curr_tree_node_ptr), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   // cppcheck-suppress unusedVariable
   bool attr_p;

#define ATTR_SEQ                                                                                                                                                                                                                                              \
   (TOK_NEW)(TOK_DELETE)(TOK_ASSIGN)(TOK_MEMBER)(TOK_PUBLIC)(TOK_PROTECTED)(TOK_PRIVATE)(TOK_NORETURN)(TOK_VOLATILE)(TOK_NOINLINE)(TOK_ALWAYS_INLINE)(TOK_USED)(TOK_UNUSED)(TOK_CONST)(TOK_TRANSPARENT_UNION)(TOK_CONSTRUCTOR)(TOK_DESTRUCTOR)(TOK_MODE)(     \
       TOK_SECTION)(TOK_ALIGNED)(TOK_WEAK)(TOK_ALIAS)(TOK_NO_INSTRUMENT_FUNCTION)(TOK_MALLOC)(TOK_NO_STACK_LIMIT)(TOK_PURE)(TOK_DEPRECATED)(TOK_VECTOR_SIZE)(TOK_VISIBILITY)(TOK_TLS_MODEL)(TOK_NONNULL)(TOK_NOTHROW)(TOK_MAY_ALIAS)(TOK_WARN_UNUSED_RESULT)( \
       TOK_FORMAT)(TOK_FORMAT_ARG)(TOK_NULL)(TOK_GLOBAL_INIT)(TOK_GLOBAL_FINI)(TOK_CONVERSION)(TOK_VIRTUAL)(TOK_LSHIFT)(TOK_MUTABLE)(TOK_PSEUDO_TMPL)(TOK_VECNEW)(TOK_VECDELETE)(TOK_POS)(TOK_NEG)(TOK_ADDR)(TOK_DEREF)(TOK_LNOT)(TOK_NOT)(TOK_PREINC)(       \
       TOK_PREDEC)(TOK_PLUSASSIGN)(TOK_PLUS)(TOK_MINUSASSIGN)(TOK_MINUS)(TOK_MULTASSIGN)(TOK_MULT)(TOK_DIVASSIGN)(TOK_DIV)(TOK_MODASSIGN)(TOK_MOD)(TOK_ANDASSIGN)(TOK_AND)(TOK_ORASSIGN)(TOK_OR)(TOK_XORASSIGN)(TOK_XOR)(TOK_LSHIFTASSIGN)(TOK_RSHIFTASSIGN)( \
       TOK_RSHIFT)(TOK_EQ)(TOK_NE)(TOK_LT)(TOK_GT)(TOK_LE)(TOK_GE)(TOK_LAND)(TOK_LOR)(TOK_COMPOUND)(TOK_MEMREF)(TOK_REF)(TOK_SUBS)(TOK_POSTINC)(TOK_POSTDEC)(TOK_CALL)(TOK_THUNK)(TOK_THIS_ADJUSTING)(TOK_RESULT_ADJUSTING)(TOK_BITFIELD)
#define ATTR_MACRO(r, data, elem)                                       \
   attr_p = tree_node_schema.find(TOK(elem)) != tree_node_schema.end(); \
   if(attr_p)                                                           \
      dynamic_cast<attr*>(curr_tree_node_ptr)->list_attr.insert(TOK(elem));

   BOOST_PP_SEQ_FOR_EACH(ATTR_MACRO, BOOST_PP_EMPTY, ATTR_SEQ);
#undef ATTR_MACRO
#undef ATTR_SEQ
}

#define SET_NODE_ID_OPT(token, field, type)                                                        \
   if(tree_node_schema.find(TOK(token)) != tree_node_schema.end())                                 \
   {                                                                                               \
      auto node_id = boost::lexical_cast<unsigned int>(tree_node_schema.find(TOK(token))->second); \
      dynamic_cast<type*>(curr_tree_node_ptr)->field = TM.GetTreeReindex(node_id);                 \
   }

#define SET_NODE_ID(token, field, type)                                                                                                               \
   {                                                                                                                                                  \
      THROW_ASSERT(tree_node_schema.find(TOK(token)) != tree_node_schema.end(), std::string("tree_node_schema must have ") + STOK(token) + " value"); \
      auto node_id = boost::lexical_cast<unsigned int>(tree_node_schema.find(TOK(token))->second);                                                    \
      dynamic_cast<type*>(curr_tree_node_ptr)->field = TM.GetTreeReindex(node_id);                                                                    \
   }

#define SET_VALUE_OPT(token, field, type, value_type)                                                                              \
   if(tree_node_schema.find(TOK(token)) != tree_node_schema.end())                                                                 \
   {                                                                                                                               \
      dynamic_cast<type*>(curr_tree_node_ptr)->field = boost::lexical_cast<value_type>(tree_node_schema.find(TOK(token))->second); \
   }

#define SET_VALUE(token, field, type, value_type)                                                                                                  \
   THROW_ASSERT(tree_node_schema.find(TOK(token)) != tree_node_schema.end(), std::string("tree node schema must have ") + STOK(token) + " value"); \
   dynamic_cast<type*>(curr_tree_node_ptr)->field = boost::lexical_cast<value_type>(tree_node_schema.find(TOK(token))->second);

#define TREE_NOT_YET_IMPLEMENTED(token) THROW_ASSERT(tree_node_schema.find(TOK(token)) == tree_node_schema.end(), std::string("field not yet supported ") + STOK(token))

void tree_node_factory::operator()(const WeightedNode*, unsigned int&)
{
#if HAVE_CODE_ESTIMATION_BUILT
   /// FIXME: TOK_TIME_WEIGHT not supported
   SET_VALUE_OPT(TOK_SIZE_WEIGHT, weight_information->instruction_size, WeightedNode, unsigned int);
   THROW_ASSERT(tree_node_schema.find(TOK(TOK_TIME_WEIGHT)) == tree_node_schema.end(), "Field time weight not supported");
   SET_VALUE_OPT(TOK_SIZE_WEIGHT, weight_information->instruction_size, WeightedNode, unsigned int);
#if HAVE_RTL_BUILT
   SET_VALUE_OPT(TOK_RTL_SIZE_WEIGHT, weight_information->rtl_instruction_size, WeightedNode, unsigned int);
#endif
#endif
}

void tree_node_factory::operator()(const srcp* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == dynamic_cast<srcp*>(curr_tree_node_ptr), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   THROW_ASSERT(tree_node_schema.find(TOK(TOK_SRCP)) != tree_node_schema.end(), "tree_node_schema must have TOK_SRCP value");
   const std::string& srcp_str = tree_node_schema.find(TOK(TOK_SRCP))->second;
   std::string::size_type colon_pos2 = srcp_str.rfind(':');
   std::string::size_type colon_pos = srcp_str.rfind(':', colon_pos2 - 1);
   dynamic_cast<srcp*>(curr_tree_node_ptr)->include_name = srcp_str.substr(0, colon_pos);
   dynamic_cast<srcp*>(curr_tree_node_ptr)->line_number = boost::lexical_cast<unsigned int>(srcp_str.substr(colon_pos + 1, colon_pos2 - colon_pos - 1));
   dynamic_cast<srcp*>(curr_tree_node_ptr)->column_number = boost::lexical_cast<unsigned int>(srcp_str.substr(colon_pos2 + 1));
}

void tree_node_factory::operator()(const decl_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_NODE_ID_OPT(TOK_NAME, name, decl_node);
   SET_NODE_ID_OPT(TOK_MNGL, mngl, decl_node);
   SET_NODE_ID_OPT(TOK_ORIG, orig, decl_node);
   SET_NODE_ID_OPT(TOK_TYPE, type, decl_node);
   SET_NODE_ID_OPT(TOK_SCPE, scpe, decl_node);
   SET_NODE_ID_OPT(TOK_ATTRIBUTES, attributes, decl_node);
   SET_NODE_ID_OPT(TOK_CHAN, chan, decl_node);
   SET_VALUE_OPT(TOK_ARTIFICIAL, artificial_flag, decl_node, bool);
   SET_VALUE_OPT(TOK_PACKED, packed_flag, decl_node, bool);
   SET_VALUE_OPT(TOK_OPERATING_SYSTEM, operating_system_flag, decl_node, bool);
   SET_VALUE_OPT(TOK_LIBRARY_SYSTEM, library_system_flag, decl_node, bool);
#if HAVE_BAMBU_BUILT
   SET_VALUE_OPT(TOK_LIBBAMBU, libbambu_flag, decl_node, bool);
#endif
   SET_VALUE_OPT(TOK_C, C_flag, decl_node, bool);
}

void tree_node_factory::operator()(const expr_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(TOK_TYPE, type, expr_node);
}

void tree_node_factory::operator()(const gimple_node* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_MEMUSE, memuse, gimple_node);
   SET_NODE_ID_OPT(TOK_MEMDEF, memdef, gimple_node);
   SET_NODE_ID_OPT(TOK_SCPE, scpe, gimple_node);
   SET_VALUE_OPT(TOK_BB_INDEX, bb_index, gimple_node, unsigned int);
   TREE_NOT_YET_IMPLEMENTED(TOK_VUSE);
   TREE_NOT_YET_IMPLEMENTED(TOK_VDEF);
}

void tree_node_factory::operator()(const unary_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_OP, op, unary_expr);
}

void tree_node_factory::operator()(const binary_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(TOK_OP0, op0, binary_expr);
   SET_NODE_ID(TOK_OP1, op1, binary_expr);
}

void tree_node_factory::operator()(const ternary_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(TOK_OP0, op0, ternary_expr);
   SET_NODE_ID(TOK_OP1, op1, ternary_expr);
   SET_NODE_ID_OPT(TOK_OP2, op2, ternary_expr);
}

void tree_node_factory::operator()(const quaternary_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(TOK_OP0, op0, quaternary_expr);
   SET_NODE_ID(TOK_OP1, op1, quaternary_expr);
   SET_NODE_ID_OPT(TOK_OP2, op2, quaternary_expr);
   SET_NODE_ID_OPT(TOK_OP3, op3, quaternary_expr);
}

void tree_node_factory::operator()(const type_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   if(tree_node_schema.find(TOK(TOK_QUAL)) != tree_node_schema.end())
   {
      dynamic_cast<type_node*>(curr_tree_node_ptr)->qual = static_cast<TreeVocabularyTokenTypes_TokenEnum>(boost::lexical_cast<unsigned int>(tree_node_schema.find(TOK(TOK_QUAL))->second));
   }
   SET_NODE_ID_OPT(TOK_NAME, name, type_node);
   SET_NODE_ID_OPT(TOK_UNQL, unql, type_node);
   SET_NODE_ID_OPT(TOK_SIZE, size, type_node);
   SET_NODE_ID_OPT(TOK_SCPE, scpe, type_node);
   SET_VALUE_OPT(TOK_SYSTEM, packed_flag, type_node, bool);
   SET_VALUE_OPT(TOK_SYSTEM, system_flag, type_node, bool);
   SET_VALUE_OPT(TOK_ALGN, algn, type_node, unsigned int);
}

void tree_node_factory::operator()(const memory_tag* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   // TREE_NOT_YET_IMPLEMENTED(TOK_ALIAS);
   // std::vector<tree_nodeRef>::const_iterator vend = obj->list_of_aliases.end();
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_aliases.begin(); i != vend; i++)
   //   write_when_not_null(STOK(TOK_ALIAS), *i);
}

void tree_node_factory::operator()(const cst_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_TYPE, type, cst_node);
}

void tree_node_factory::operator()(const error_mark* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_factory::operator()(const array_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_ELTS, elts, array_type);
   SET_NODE_ID_OPT(TOK_DOMN, domn, array_type);
}

void tree_node_factory::operator()(const gimple_asm* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE_OPT(TOK_CTOR, volatile_flag, gimple_asm, bool);
   SET_VALUE_OPT(TOK_STR, str, gimple_asm, std::string);
   SET_NODE_ID_OPT(TOK_OUT, out, gimple_asm);
   SET_NODE_ID_OPT(TOK_IN, in, gimple_asm);
   SET_NODE_ID_OPT(TOK_CLOB, clob, gimple_asm);
}

void tree_node_factory::operator()(const baselink* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_TYPE, type, baselink);
}

void tree_node_factory::operator()(const gimple_bind* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   TREE_NOT_YET_IMPLEMENTED(TOK_VARS);
   // std::vector<tree_nodeRef>::const_iterator vend = obj->list_of_vars.end();
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_vars.begin(); i != vend; i++)
   //   write_when_not_null(STOK(TOK_VARS), *i);
   SET_NODE_ID_OPT(TOK_BODY, body, gimple_bind);
}

void tree_node_factory::operator()(const binfo* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_TYPE, type, binfo);
   SET_VALUE_OPT(TOK_VIRT, virt_flag, binfo, bool);
   SET_VALUE(TOK_BASES, bases, binfo, int);
   TREE_NOT_YET_IMPLEMENTED(TOK_BINF);
   // std::vector<std::pair< unsigned int, tree_nodeRef> >::const_iterator vend = obj->list_of_access_binf.end();
   // for (std::vector<std::pair< unsigned int, tree_nodeRef> >::const_iterator i = obj->list_of_access_binf.begin(); i != vend; i++)
   //{
   //   WRITE_TOKEN2(os, i->first);
   //   write_when_not_null(STOK(TOK_BINF), i->second);
   //}
}

void tree_node_factory::operator()(const block* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   // if (obj->bl_flag)
   //   WRITE_UFIELD_STRING(os, obj->bl);
}

void tree_node_factory::operator()(const call_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_FN, fn, call_expr);
   if(tree_node_schema.find(TOK(TOK_ARG)) != tree_node_schema.end())
   {
      std::vector<unsigned int> args = convert_string_to_vector<unsigned int>(tree_node_schema.find(TOK(TOK_ARG))->second, "_");
      for(const auto arg : args)
      {
         dynamic_cast<call_expr*>(curr_tree_node_ptr)->args.push_back(TM.GetTreeReindex(arg));
      }
   }
}

void tree_node_factory::operator()(const aggr_init_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE(TOK_CTOR, ctor, aggr_init_expr, int);
   SET_NODE_ID_OPT(TOK_SLOT, slot, aggr_init_expr);
}

void tree_node_factory::operator()(const gimple_call* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(TOK_FN, fn, gimple_call);
   if(tree_node_schema.find(TOK(TOK_ARG)) != tree_node_schema.end())
   {
      std::vector<unsigned int> args = convert_string_to_vector<unsigned int>(tree_node_schema.find(TOK(TOK_ARG))->second, "_");
      for(const auto arg : args)
      {
         dynamic_cast<gimple_call*>(curr_tree_node_ptr)->args.push_back(TM.GetTreeReindex(arg));
      }
   }
}

void tree_node_factory::operator()(const case_label_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_OP0, op0, case_label_expr);
   SET_NODE_ID_OPT(TOK_OP1, op1, case_label_expr);
   SET_VALUE_OPT(TOK_DEFAULT, default_flag, case_label_expr, bool);
   SET_NODE_ID_OPT(TOK_GOTO, got, case_label_expr);
}

void tree_node_factory::operator()(const cast_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_OP, op, cast_expr);
}

void tree_node_factory::operator()(const complex_cst* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_REAL, real, complex_cst);
   SET_NODE_ID_OPT(TOK_IMAG, imag, complex_cst);
}

void tree_node_factory::operator()(const complex_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE_OPT(TOK_UNSIGNED, unsigned_flag, complex_type, bool);
   SET_VALUE_OPT(TOK_UNSIGNED, real_flag, complex_type, bool);
}

void tree_node_factory::operator()(const gimple_cond* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_OP0, op0, gimple_cond);
}

void tree_node_factory::operator()(const const_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_CNST, cnst, const_decl);
}

void tree_node_factory::operator()(const constructor* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_TYPE, type, constructor);
   TREE_NOT_YET_IMPLEMENTED(TOK_IDX);
   TREE_NOT_YET_IMPLEMENTED(TOK_VALU);
   // std::vector<std::pair< tree_nodeRef, tree_nodeRef> >::const_iterator vend = obj->list_of_idx_valu.end();
   // for (std::vector<std::pair< tree_nodeRef, tree_nodeRef> >::const_iterator i = obj->list_of_idx_valu.begin(); i != vend; i++)
   //{
   //   write_when_not_null(STOK(TOK_IDX), i->first);
   //   write_when_not_null(STOK(TOK_VALU), i->second);
   //}
}

void tree_node_factory::operator()(const enumeral_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE_OPT(TOK_PREC, prec, enumeral_type, unsigned int);
   SET_VALUE_OPT(TOK_UNSIGNED, unsigned_flag, enumeral_type, bool);
   SET_NODE_ID_OPT(TOK_MIN, min, enumeral_type);
   SET_NODE_ID_OPT(TOK_MAX, max, enumeral_type);
   SET_NODE_ID_OPT(TOK_CSTS, csts, enumeral_type);
}

void tree_node_factory::operator()(const expr_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_VALUE_OPT(TOK_LINE, line, expr_stmt, int);
   SET_NODE_ID_OPT(TOK_EXPR, expr, expr_stmt);
   SET_NODE_ID_OPT(TOK_NEXT, next, expr_stmt);
}

void tree_node_factory::operator()(const field_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE_OPT(TOK_ALGN, algn, field_decl, unsigned int);
   SET_NODE_ID_OPT(TOK_INIT, init, field_decl);
   SET_NODE_ID_OPT(TOK_SIZE, size, field_decl);
   SET_NODE_ID_OPT(TOK_BPOS, bpos, field_decl);
   SET_NODE_ID_OPT(TOK_SMT_ANN, smt_ann, field_decl);
}

void tree_node_factory::operator()(const function_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE_OPT(TOK_OPERATOR, operator_flag, function_decl, bool);
   // std::vector<std::string>::const_iterator vend = obj->list_of_op_names.end();
   // for (std::vector<std::string>::const_iterator i = obj->list_of_op_names.begin(); i != vend; i++)
   //   WRITE_UFIELD_STRING(os, *i);
   SET_NODE_ID_OPT(TOK_TMPL_PARMS, tmpl_parms, function_decl);
   SET_NODE_ID_OPT(TOK_TMPL_ARGS, tmpl_args, function_decl);

   SET_VALUE_OPT(TOK_FIXD, fixd, function_decl, int);
   SET_VALUE_OPT(TOK_FIXD, fixd_flag, function_decl, bool);
   SET_VALUE_OPT(TOK_VIRT, virt_flag, function_decl, bool);
   SET_VALUE_OPT(TOK_VIRT, virt, function_decl, int);
   SET_NODE_ID_OPT(TOK_FN, fn, function_decl);
   TREE_NOT_YET_IMPLEMENTED(TOK_ARG);
   // std::vector<tree_nodeRef>::const_iterator vend2 = obj->list_of_args.end();
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_args.begin(); i != vend2; i++)
   //   write_when_not_null(STOK(TOK_ARGS), *i);

   SET_VALUE_OPT(TOK_UNDEFINED, undefined_flag, function_decl, bool);
   SET_VALUE_OPT(TOK_UNDEFINED, builtin_flag, function_decl, bool);
   SET_VALUE_OPT(TOK_STATIC, static_flag, function_decl, bool);
   SET_VALUE_OPT(TOK_HWCALL, hwcall_flag, function_decl, bool);
   SET_VALUE_OPT(TOK_REVERSE_RESTRICT, reverse_restrict_flag, function_decl, bool);
   SET_VALUE_OPT(TOK_WRITING_MEMORY, writing_memory, function_decl, bool);
   SET_VALUE_OPT(TOK_READING_MEMORY, reading_memory, function_decl, bool);
   SET_VALUE_OPT(TOK_PIPELINE_ENABLED, pipeline_enabled, function_decl, bool);
   SET_VALUE_OPT(TOK_SIMPLE_PIPELINE, simple_pipeline, function_decl, bool);
   SET_VALUE_OPT(TOK_INITIATION_TIME, initiation_time, function_decl, int);
#if HAVE_FROM_PRAGMA_BUILT
   SET_VALUE_OPT(TOK_OMP_ATOMIC, omp_atomic, function_decl, bool);
   SET_VALUE_OPT(TOK_OMP_BODY_LOOP, omp_body_loop, function_decl, bool);
   SET_VALUE_OPT(TOK_OMP_CRITICAL_SESSION, omp_critical, function_decl, std::string);
   SET_VALUE_OPT(TOK_OMP_FOR_WRAPPER, omp_for_wrapper, function_decl, size_t);
#endif
   SET_NODE_ID_OPT(TOK_BODY, body, function_decl);
   SET_NODE_ID_OPT(TOK_INLINE_BODY, inline_body, function_decl);
}

void tree_node_factory::operator()(const function_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_RETN, retn, function_type);
   SET_NODE_ID_OPT(TOK_PRMS, prms, function_type);
   SET_VALUE_OPT(TOK_VARARGS, varargs_flag, function_type, bool);
}

void tree_node_factory::operator()(const gimple_assign* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(TOK_OP0, op0, gimple_assign);
   SET_NODE_ID(TOK_OP1, op1, gimple_assign);
   SET_NODE_ID_OPT(TOK_PREDICATE, predicate, gimple_assign);
   SET_NODE_ID_OPT(TOK_ORIG, orig, gimple_assign);
   SET_VALUE_OPT(TOK_INIT, init_assignment, gimple_assign, bool);
   SET_VALUE_OPT(TOK_CLOBBER, clobber, gimple_assign, bool);
   SET_VALUE_OPT(TOK_ADDR, temporary_address, gimple_assign, bool);
}

void tree_node_factory::operator()(const gimple_goto* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_OP, op, gimple_goto);
}

void tree_node_factory::operator()(const handler* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_VALUE_OPT(TOK_LINE, line, handler, int);
   SET_NODE_ID_OPT(TOK_BODY, body, handler);
}

void tree_node_factory::operator()(const identifier_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   THROW_ERROR("Use find_identifier_nodeID to find identifier_node objects");
}

void tree_node_factory::operator()(const integer_cst* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE_OPT(TOK_VALUE, value, integer_cst, long long int);
}

void tree_node_factory::operator()(const integer_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE_OPT(TOK_PREC, prec, integer_type, unsigned int);
   // if (obj->str != "")
   //   WRITE_UFIELD(os, obj->str);
   SET_VALUE_OPT(TOK_UNSIGNED, unsigned_flag, integer_type, bool);
   SET_NODE_ID_OPT(TOK_MIN, min, integer_type);
   SET_NODE_ID_OPT(TOK_MAX, max, integer_type);
}

void tree_node_factory::operator()(const gimple_label* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_OP, op, gimple_label);
}

void tree_node_factory::operator()(const method_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_CLAS, clas, method_type);
}

void tree_node_factory::operator()(const namespace_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_DCLS, dcls, namespace_decl);
}

void tree_node_factory::operator()(const overload* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_NODE_ID_OPT(TOK_CRNT, crnt, overload);
   SET_NODE_ID_OPT(TOK_CHAN, chan, overload);
}

void tree_node_factory::operator()(const parm_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_ARGT, argt, parm_decl);
   SET_NODE_ID_OPT(TOK_SIZE, size, parm_decl);
   SET_VALUE_OPT(TOK_ALGN, algn, parm_decl, unsigned int);
   SET_VALUE_OPT(TOK_USED, used, parm_decl, int);
   SET_VALUE_OPT(TOK_REGISTER, register_flag, parm_decl, bool);
   SET_VALUE_OPT(TOK_READONLY, readonly_flag, parm_decl, bool);
   SET_NODE_ID_OPT(TOK_SMT_ANN, smt_ann, parm_decl);
}

void tree_node_factory::operator()(const gimple_phi* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_NODE_ID_OPT(TOK_RES, res, gimple_phi);
   SET_VALUE_OPT(TOK_VIRTUAL, virtual_flag, gimple_phi, bool);
   // TREE_NOT_YET_IMPLEMENTED(TOK_DEF);
   // TREE_NOT_YET_IMPLEMENTED(TOK_EDGE);
   // std::vector<std::pair< tree_nodeRef, int> >::const_iterator vend = obj->list_of_def_edge.end();
   // for (std::vector<std::pair< tree_nodeRef, int> >::const_iterator i = obj->list_of_def_edge.begin(); i != vend; i++)
   //{
   //   write_when_not_null(STOK(TOK_DEF), i->first);
   //   WRITE_NFIELD(os, STOK(TOK_EDGE), i->second);
   //}
}

void tree_node_factory::operator()(const pointer_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_PTD, ptd, pointer_type);
}

void tree_node_factory::operator()(const real_cst* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE_OPT(TOK_OVERFLOW, overflow_flag, real_cst, bool);
   SET_VALUE_OPT(TOK_VALR, valr, real_cst, std::string);
   SET_VALUE_OPT(TOK_VALX, valx, real_cst, std::string);
}

void tree_node_factory::operator()(const real_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE_OPT(TOK_PREC, prec, real_type, unsigned int);
}

void tree_node_factory::operator()(const record_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_TMPL_PARMS, tmpl_parms, record_type);
   SET_NODE_ID_OPT(TOK_TMPL_ARGS, tmpl_args, record_type);

   SET_VALUE_OPT(TOK_PTRMEM, ptrmem_flag, record_type, bool);
   SET_NODE_ID_OPT(TOK_PTD, ptd, record_type);
   SET_NODE_ID_OPT(TOK_CLS, cls, record_type);
   SET_NODE_ID_OPT(TOK_BFLD, bfld, record_type);
   SET_NODE_ID_OPT(TOK_VFLD, vfld, record_type);
   SET_VALUE_OPT(TOK_SPEC, spec_flag, record_type, bool);
   SET_VALUE_OPT(TOK_STRUCT, struct_flag, record_type, bool);
   TREE_NOT_YET_IMPLEMENTED(TOK_FLDS);
   // std::vector<tree_nodeRef>::const_iterator vend1 = obj->list_of_flds.end();
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_flds.begin(); i != vend1; i++)
   //   write_when_not_null(STOK(TOK_FLDS), *i);
   TREE_NOT_YET_IMPLEMENTED(TOK_FNCS);
   // std::vector<tree_nodeRef>::const_iterator vend2 = obj->list_of_fncs.end();
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_fncs.begin(); i != vend2; i++)
   //   write_when_not_null(STOK(TOK_FNCS), *i);
   SET_NODE_ID_OPT(TOK_BINF, binf, record_type);
}

void tree_node_factory::operator()(const reference_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_REFD, refd, reference_type);
}

void tree_node_factory::operator()(const result_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_INIT, init, result_decl);
   SET_NODE_ID_OPT(TOK_SIZE, size, result_decl);
   SET_VALUE_OPT(TOK_ALGN, algn, result_decl, unsigned int);
   SET_NODE_ID_OPT(TOK_SMT_ANN, smt_ann, result_decl);
}

void tree_node_factory::operator()(const gimple_return* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_OP, op, gimple_return);
}

void tree_node_factory::operator()(const return_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_VALUE_OPT(TOK_LINE, line, return_stmt, int);
   SET_NODE_ID_OPT(TOK_EXPR, expr, return_stmt);
}

void tree_node_factory::operator()(const scope_ref* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_OP, op0, scope_ref);
   SET_NODE_ID_OPT(TOK_OP, op1, scope_ref);
}

void tree_node_factory::operator()(const ssa_name* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_NODE_ID_OPT(TOK_TYPE, type, ssa_name);
   SET_NODE_ID_OPT(TOK_VAR, var, ssa_name);
   SET_VALUE_OPT(TOK_VERS, vers, ssa_name, unsigned int);
   SET_VALUE_OPT(TOK_ORIG_VERS, orig_vers, ssa_name, unsigned int);
   // SET_NODE_ID_OPT(TOK_PTR_INFO,ptr_info,ssa_name);

   SET_VALUE_OPT(TOK_VOLATILE, volatile_flag, ssa_name, bool);
   SET_VALUE_OPT(TOK_VIRTUAL, virtual_flag, ssa_name, bool);
   SET_NODE_ID_OPT(TOK_MIN, min, ssa_name);
   SET_NODE_ID_OPT(TOK_MAX, max, ssa_name);
   SET_VALUE_OPT(TOK_BIT_VALUES, bit_values, ssa_name, std::string);
}

void tree_node_factory::operator()(const statement_list* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   TREE_NOT_YET_IMPLEMENTED(TOK_STMT);
   // std::vector<tree_nodeRef>::const_iterator vend = obj->list_of_stmt.end();
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_stmt.begin(); i != vend; i++)
   //   write_when_not_null(STOK(TOK_STMT), *i);
   TREE_NOT_YET_IMPLEMENTED(TOK_BLOC);
   // std::map<int, blocRef>::const_iterator mend = obj->list_of_bloc.end();
   // for (std::map<int, blocRef>::const_iterator i = obj->list_of_bloc.begin(); i != mend; i++)
   //   write_when_not_null_bloc(STOK(TOK_BLOC), i->second);
}

void tree_node_factory::operator()(const string_cst* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE_OPT(TOK_STRG, strg, string_cst, std::string);
}

void tree_node_factory::operator()(const gimple_switch* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_OP0, op0, gimple_switch);
   SET_NODE_ID_OPT(TOK_OP1, op1, gimple_switch);
}

void tree_node_factory::operator()(const target_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_DECL, decl, target_expr);
   SET_NODE_ID_OPT(TOK_INIT, init, target_expr);
   SET_NODE_ID_OPT(TOK_CLNP, clnp, target_expr);
}

void tree_node_factory::operator()(const lut_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_OP0, op0, lut_expr);
   SET_NODE_ID_OPT(TOK_OP1, op1, lut_expr);
   SET_NODE_ID_OPT(TOK_OP2, op2, lut_expr);
   SET_NODE_ID_OPT(TOK_OP3, op3, lut_expr);
   SET_NODE_ID_OPT(TOK_OP4, op4, lut_expr);
   SET_NODE_ID_OPT(TOK_OP5, op5, lut_expr);
   SET_NODE_ID_OPT(TOK_OP6, op6, lut_expr);
   SET_NODE_ID_OPT(TOK_OP7, op7, lut_expr);
   SET_NODE_ID_OPT(TOK_OP8, op8, lut_expr);
}

void tree_node_factory::operator()(const template_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_RSLT, rslt, template_decl);
   SET_NODE_ID_OPT(TOK_INST, inst, template_decl);
   SET_NODE_ID_OPT(TOK_SPCS, spcs, template_decl);
   SET_NODE_ID_OPT(TOK_PRMS, prms, template_decl);
}

void tree_node_factory::operator()(const template_parm_index* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_TYPE, type, template_parm_index);
   SET_NODE_ID_OPT(TOK_DECL, decl, template_parm_index);
   SET_VALUE_OPT(TOK_CONSTANT, constant_flag, template_parm_index, bool);
   SET_VALUE_OPT(TOK_READONLY, readonly_flag, template_parm_index, bool);
   SET_VALUE_OPT(TOK_IDX, idx, template_parm_index, int);
   SET_VALUE_OPT(TOK_LEVEL, level, template_parm_index, int);
   SET_VALUE_OPT(TOK_ORIG_LEVEL, orig_level, template_parm_index, int);
}

void tree_node_factory::operator()(const tree_list* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_NODE_ID_OPT(TOK_PURP, purp, tree_list);
   SET_NODE_ID_OPT(TOK_VALU, valu, tree_list);
   SET_NODE_ID_OPT(TOK_CHAN, chan, tree_list);
}

void tree_node_factory::operator()(const tree_vec* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_VALUE_OPT(TOK_LNGT, lngt, tree_vec, unsigned int);
   TREE_NOT_YET_IMPLEMENTED(TOK_OP);
   // std::vector<tree_nodeRef>::const_iterator vend = obj->list_of_op.end();
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_op.begin(); i != vend; i++)
   //   write_when_not_null(STOK(TOK_OP), *i);
}

void tree_node_factory::operator()(const try_block* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_VALUE_OPT(TOK_LINE, line, try_block, int);
   SET_NODE_ID_OPT(TOK_BODY, body, try_block);
   SET_NODE_ID_OPT(TOK_HDLR, hdlr, try_block);
   SET_NODE_ID_OPT(TOK_NEXT, next, try_block);
}

void tree_node_factory::operator()(const type_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_TMPL_PARMS, tmpl_parms, type_decl);
   SET_NODE_ID_OPT(TOK_TMPL_ARGS, tmpl_args, type_decl);
}

void tree_node_factory::operator()(const union_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   TREE_NOT_YET_IMPLEMENTED(TOK_FLDS);
   // std::vector<tree_nodeRef>::const_iterator vend1 = obj->list_of_flds.end();
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_flds.begin(); i != vend1; i++)
   //   write_when_not_null(STOK(TOK_FLDS), *i);
   TREE_NOT_YET_IMPLEMENTED(TOK_FNCS);
   // std::vector<tree_nodeRef>::const_iterator vend2 = obj->list_of_fncs.end();
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_fncs.begin(); i != vend2; i++)
   //   write_when_not_null(STOK(TOK_FNCS), *i);
   SET_NODE_ID_OPT(TOK_BINF, binf, union_type);
}

void tree_node_factory::operator()(const var_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_VALUE_OPT(TOK_USE_TMPL, use_tmpl, var_decl, int);
   SET_VALUE_OPT(TOK_STATIC_STATIC, static_static_flag, var_decl, bool);
   SET_VALUE_OPT(TOK_EXTERN, extern_flag, var_decl, bool);
   SET_VALUE_OPT(TOK_ADDR_TAKEN, addr_taken, var_decl, bool);
   SET_VALUE_OPT(TOK_ADDR_NOT_TAKEN, addr_not_taken, var_decl, bool);
   SET_VALUE_OPT(TOK_STATIC, static_flag, var_decl, bool);
   SET_NODE_ID_OPT(TOK_INIT, init, var_decl);
   SET_NODE_ID_OPT(TOK_SIZE, size, var_decl);
   SET_VALUE_OPT(TOK_ALGN, algn, var_decl, unsigned int);
   SET_VALUE_OPT(TOK_USED, used, var_decl, int);
   SET_VALUE_OPT(TOK_REGISTER, register_flag, var_decl, bool);
   SET_VALUE_OPT(TOK_READONLY, readonly_flag, var_decl, bool);
   SET_VALUE_OPT(TOK_BIT_VALUES, bit_values, var_decl, std::string);
   SET_NODE_ID_OPT(TOK_SMT_ANN, smt_ann, var_decl);
   TREE_NOT_YET_IMPLEMENTED(TOK_ADDR_STMT);
   TREE_NOT_YET_IMPLEMENTED(TOK_DEF_STMT);
   TREE_NOT_YET_IMPLEMENTED(TOK_USE_STMT);
}

void tree_node_factory::operator()(const vector_cst* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   TREE_NOT_YET_IMPLEMENTED(TOK_VALU);
   // std::vector<tree_nodeRef>::const_iterator vend = obj->list_of_valu.end();
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_valu.begin(); i != vend; i++)
   //   write_when_not_null(STOK(TOK_VALU), *i);
}

void tree_node_factory::operator()(const type_argument_pack* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_ARG, arg, type_argument_pack);
}

void tree_node_factory::operator()(const nontype_argument_pack* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_ARG, arg, nontype_argument_pack);
}

void tree_node_factory::operator()(const type_pack_expansion* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_OP, op, type_pack_expansion);
   SET_NODE_ID_OPT(TOK_PARAM_PACKS, param_packs, type_pack_expansion);
   SET_NODE_ID_OPT(TOK_ARG, arg, type_pack_expansion);
}

void tree_node_factory::operator()(const expr_pack_expansion* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_OP, op, expr_pack_expansion);
   SET_NODE_ID_OPT(TOK_PARAM_PACKS, param_packs, expr_pack_expansion);
   SET_NODE_ID_OPT(TOK_ARG, arg, expr_pack_expansion);
}

void tree_node_factory::operator()(const vector_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID_OPT(TOK_ELTS, elts, vector_type);
}

void tree_node_factory::operator()(const target_mem_ref* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_NODE_ID_OPT(TOK_TYPE, type, target_mem_ref);
   SET_NODE_ID_OPT(TOK_SYMBOL, symbol, target_mem_ref);
   SET_NODE_ID_OPT(TOK_BASE, base, target_mem_ref);
   SET_NODE_ID_OPT(TOK_IDX, idx, target_mem_ref);
   SET_NODE_ID_OPT(TOK_STEP, step, target_mem_ref);
   SET_NODE_ID_OPT(TOK_OFFSET, offset, target_mem_ref);
   SET_NODE_ID_OPT(TOK_ORIG, orig, target_mem_ref);
   SET_NODE_ID_OPT(TOK_TAG, tag, target_mem_ref);
}

void tree_node_factory::operator()(const target_mem_ref461* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_NODE_ID_OPT(TOK_TYPE, type, target_mem_ref461);
   SET_NODE_ID_OPT(TOK_BASE, base, target_mem_ref461);
   SET_NODE_ID_OPT(TOK_IDX, idx, target_mem_ref461);
   SET_NODE_ID_OPT(TOK_STEP, step, target_mem_ref461);
   SET_NODE_ID_OPT(TOK_IDX2, idx2, target_mem_ref461);
   SET_NODE_ID_OPT(TOK_OFFSET, offset, target_mem_ref461);
}

void tree_node_factory::operator()(const bloc* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   // WRITE_UFIELD(os, obj->number);
   SET_VALUE_OPT(TOK_HPL, hpl, bloc, bool);
   SET_VALUE_OPT(TOK_LOOP_ID, loop_id, bloc, unsigned int);
   TREE_NOT_YET_IMPLEMENTED(TOK_PRED);
   // std::vector<int>::const_iterator vend1 = obj->list_of_pred.end();
   // for (std::vector<int>::const_iterator i = obj->list_of_pred.begin(); i != vend1; i++)
   //   if (*i == bloc::ENTRY_BLOCK_ID)
   //      WRITE_NFIELD(os, STOK(TOK_PRED), STOK(TOK_ENTRY));
   // else
   //   WRITE_NFIELD(os, STOK(TOK_PRED), *i);
   TREE_NOT_YET_IMPLEMENTED(TOK_SUCC);
   // std::vector<int>::const_iterator vend2 = obj->list_of_succ.end();
   // for (std::vector<int>::const_iterator i = obj->list_of_succ.begin(); i != vend2; i++)
   //   if (*i == bloc::EXIT_BLOCK_ID)
   //      WRITE_NFIELD(os, STOK(TOK_SUCC), STOK(TOK_EXIT));
   // else
   //   WRITE_NFIELD(os, STOK(TOK_SUCC), *i);
   SET_VALUE_OPT(TOK_TRUE_EDGE, true_edge, bloc, unsigned int);
   SET_VALUE_OPT(TOK_FALSE_EDGE, false_edge, bloc, unsigned int);
   TREE_NOT_YET_IMPLEMENTED(TOK_PHI);
   // std::vector<tree_nodeRef>::const_iterator vend3 = obj->list_of_phi.end();
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_phi.begin(); i != vend3; i++)
   //   write_when_not_null(STOK(TOK_PHI), *i);
   // std::vector<tree_nodeRef>::const_iterator vend4 = obj->list_of_stmt.end();
   TREE_NOT_YET_IMPLEMENTED(TOK_STMT);
   // for (std::vector<tree_nodeRef>::const_iterator i = obj->list_of_stmt.begin(); i != vend4; i++)
   //   write_when_not_null(STOK(TOK_STMT), *i);
}

void tree_node_factory::operator()(const gimple_while* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(TOK_OP0, op0, gimple_while);
}

void tree_node_factory::operator()(const gimple_for* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(TOK_OP1, op1, gimple_for);
   SET_NODE_ID(TOK_OP2, op2, gimple_for);
}

void tree_node_factory::operator()(const gimple_multi_way_if* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   /// not yet implemented
}

void tree_node_factory::operator()(const null_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_factory::operator()(const gimple_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE_OPT(TOK_IS_BLOCK, is_block, gimple_pragma, bool);
   SET_VALUE_OPT(TOK_OPEN, is_opening, gimple_pragma, bool);
   SET_VALUE_OPT(TOK_LINE, line, gimple_pragma, std::string);
   SET_NODE_ID_OPT(TOK_PRAGMA_SCOPE, scope, gimple_pragma);
   SET_NODE_ID_OPT(TOK_PRAGMA_DIRECTIVE, directive, gimple_pragma);
   /// FIXME: after creation of pragma node, the control has not to be optional
#if 0
   SET_VALUE(TOK_BB_INDEX, bb_index, gimple_pragma, unsigned int);
#else
   SET_VALUE_OPT(TOK_BB_INDEX, bb_index, gimple_pragma, unsigned int);
#endif
}

void tree_node_factory::operator()(const omp_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_factory::operator()(const omp_for_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_factory::operator()(const omp_simd_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_factory::operator()(const omp_declare_simd_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_factory::operator()(const omp_target_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_factory::operator()(const omp_critical_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_factory::operator()(const omp_task_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_factory::operator()(const omp_parallel_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE_OPT(TOK_PRAGMA_OMP_SHORTCUT, is_shortcut, omp_parallel_pragma, bool);
}

void tree_node_factory::operator()(const omp_sections_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE_OPT(TOK_PRAGMA_OMP_SHORTCUT, is_shortcut, omp_sections_pragma, bool);
}

void tree_node_factory::operator()(const omp_parallel_sections_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(TOK_OP0, op0, omp_parallel_sections_pragma);
   SET_NODE_ID(TOK_OP1, op1, omp_parallel_sections_pragma);
}

void tree_node_factory::operator()(const omp_section_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_factory::operator()(const map_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_factory::operator()(const call_hw_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   THROW_ASSERT(tree_node_schema.find(TOK(TOK_HW_COMPONENT)) != tree_node_schema.end(), "error");
   SET_VALUE_OPT(TOK_HW_COMPONENT, HW_component, call_hw_pragma, std::string);
   SET_VALUE_OPT(TOK_ID_IMPLEMENTATION, ID_implementation, call_hw_pragma, std::string);
}

void tree_node_factory::operator()(const call_point_hw_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE_OPT(TOK_HW_COMPONENT, HW_component, call_point_hw_pragma, std::string);
   SET_VALUE_OPT(TOK_ID_IMPLEMENTATION, ID_implementation, call_hw_pragma, std::string);
   SET_VALUE_OPT(TOK_RECURSIVE, recursive, call_point_hw_pragma, bool);
}

void tree_node_factory::operator()(const issue_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_factory::operator()(const blackbox_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_factory::operator()(const profiling_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_factory::operator()(const statistical_profiling* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}
