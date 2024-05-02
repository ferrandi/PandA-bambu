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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file tree_node_dup.cpp
 * @brief tree node duplication class.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "tree_node_dup.hpp"

#include "application_manager.hpp"
#include "exceptions.hpp"
#include "ext_tree_node.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_common.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#include <string>
#include <utility>
#include <vector>

#define DECLARATION (2) // All nodes including declarations are duplicated (not function_decl)

#define CREATE_TREE_NODE_CASE_BODY(tree_node_name, node_id) \
   {                                                        \
      (node_id) = TM->new_tree_node_id();                   \
      remap.insert({tn->index, (node_id)});                 \
      auto tnn = new tree_node_name(node_id);               \
      tree_nodeRef cur = tree_nodeRef(tnn);                 \
      if(tnn->get_kind() == function_decl_K)                \
      {                                                     \
         TM->add_function(node_id, cur);                    \
      }                                                     \
      TM->AddTreeNode(cur);                                 \
      curr_tree_node_ptr = tnn;                             \
      source_tn = tn;                                       \
      tnn->visit(this);                                     \
      curr_tree_node_ptr = nullptr;                         \
      source_tn = tree_nodeRef();                           \
      break;                                                \
   }

#define RET_NODE_ID_CASE_BODY(tree_node_name, node_id) \
   {                                                   \
      if(remap.find(tn->index) != remap.end())         \
         (node_id) = remap.find(tn->index)->second;    \
      else                                             \
         (node_id) = tn->index;                        \
      break;                                           \
   }

tree_node_dup::tree_node_dup(CustomUnorderedMapStable<unsigned int, unsigned int>& _remap,
                             const application_managerRef _AppM, unsigned int _remap_bbi, unsigned int _remap_loop_id,
                             bool _use_counting)
    : remap(_remap),
      use_counting(_use_counting),
      remap_bbi(_remap_bbi),
      remap_bb(),
      remap_loop_id(_remap_loop_id),
      remap_lid(),
      AppM(_AppM),
      TM(_AppM->get_tree_manager()),
      curr_tree_node_ptr(nullptr),
      curr_bloc(nullptr)
{
   if(remap_bbi)
   {
      remap_bb.insert(std::make_pair(bloc::ENTRY_BLOCK_ID, bloc::ENTRY_BLOCK_ID));
      remap_bb.insert(std::make_pair(bloc::EXIT_BLOCK_ID, bloc::EXIT_BLOCK_ID));
   }
}

unsigned int tree_node_dup::create_tree_node(const tree_nodeRef& tn, int _mode)
{
   unsigned int node_id = 0U;
   mode = _mode;
   switch(tn->get_kind())
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
         RET_NODE_ID_CASE_BODY(array_type, node_id)
      case alignof_expr_K:
         CREATE_TREE_NODE_CASE_BODY(alignof_expr, node_id)
      case arrow_expr_K:
         CREATE_TREE_NODE_CASE_BODY(arrow_expr, node_id)
      case gimple_asm_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_asm, node_id)
      case assert_expr_K:
         CREATE_TREE_NODE_CASE_BODY(assert_expr, node_id)
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
         RET_NODE_ID_CASE_BODY(boolean_type, node_id)
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
         RET_NODE_ID_CASE_BODY(complex_cst, node_id)
      case complex_expr_K:
         CREATE_TREE_NODE_CASE_BODY(complex_expr, node_id)
      case complex_type_K:
         RET_NODE_ID_CASE_BODY(complex_type, node_id)
      case component_ref_K:
         CREATE_TREE_NODE_CASE_BODY(component_ref, node_id)
      case compound_expr_K:
         CREATE_TREE_NODE_CASE_BODY(compound_expr, node_id)
      case cond_expr_K:
         CREATE_TREE_NODE_CASE_BODY(cond_expr, node_id)
      case conj_expr_K:
         CREATE_TREE_NODE_CASE_BODY(conj_expr, node_id)
      case const_decl_K:
         RET_NODE_ID_CASE_BODY(const_decl, node_id)
      case constructor_K:
         if(mode)
            CREATE_TREE_NODE_CASE_BODY(constructor, node_id)
         else
            RET_NODE_ID_CASE_BODY(constructor, node_id)
      case convert_expr_K:
         CREATE_TREE_NODE_CASE_BODY(convert_expr, node_id)
      case ctor_initializer_K:
         CREATE_TREE_NODE_CASE_BODY(ctor_initializer, node_id)
      case eh_filter_expr_K:
         CREATE_TREE_NODE_CASE_BODY(eh_filter_expr, node_id)
      case enumeral_type_K:
         RET_NODE_ID_CASE_BODY(enumeral_type, node_id)
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
         if(mode >= tree_node_dup_mode::FUNCTION)
            CREATE_TREE_NODE_CASE_BODY(field_decl, node_id)
         else
            RET_NODE_ID_CASE_BODY(field_decl, node_id)
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
         if(mode >= tree_node_dup_mode::FUNCTION)
            CREATE_TREE_NODE_CASE_BODY(function_decl, node_id)
         else
            RET_NODE_ID_CASE_BODY(function_decl, node_id)
      case function_type_K:
         if(mode >= tree_node_dup_mode::FUNCTION)
            CREATE_TREE_NODE_CASE_BODY(function_type, node_id)
         else
            RET_NODE_ID_CASE_BODY(function_type, node_id)
      case ge_expr_K:
         CREATE_TREE_NODE_CASE_BODY(ge_expr, node_id)
      case gimple_assign_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_assign, node_id)
      case gimple_cond_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_cond, node_id)
      case gimple_goto_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_goto, node_id)
      case gimple_label_K:
         if(mode >= DECLARATION)
            CREATE_TREE_NODE_CASE_BODY(gimple_label, node_id)
         else
            RET_NODE_ID_CASE_BODY(gimple_label, node_id)
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
         RET_NODE_ID_CASE_BODY(integer_cst, node_id)
      case integer_type_K:
         RET_NODE_ID_CASE_BODY(integer_type, node_id)
      case label_decl_K:
         if(mode >= DECLARATION)
            CREATE_TREE_NODE_CASE_BODY(label_decl, node_id)
         else
            RET_NODE_ID_CASE_BODY(label_decl, node_id)
      case lang_type_K:
         RET_NODE_ID_CASE_BODY(lang_type, node_id)
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
         if(mode >= tree_node_dup_mode::FUNCTION)
            CREATE_TREE_NODE_CASE_BODY(method_type, node_id)
         else
            RET_NODE_ID_CASE_BODY(method_type, node_id)
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
      case namespace_decl_K:
         RET_NODE_ID_CASE_BODY(namespace_decl, node_id)
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
         RET_NODE_ID_CASE_BODY(offset_type, node_id)
      case ordered_expr_K:
         CREATE_TREE_NODE_CASE_BODY(ordered_expr, node_id)
      case overload_K:
         CREATE_TREE_NODE_CASE_BODY(overload, node_id)
      case parm_decl_K:
         if(mode >= DECLARATION)
            CREATE_TREE_NODE_CASE_BODY(parm_decl, node_id)
         else
            RET_NODE_ID_CASE_BODY(parm_decl, node_id)
      case gimple_phi_K:
         if(mode)
            CREATE_TREE_NODE_CASE_BODY(gimple_phi, node_id)
         else
            RET_NODE_ID_CASE_BODY(gimple_phi, node_id)
      case plus_expr_K:
         CREATE_TREE_NODE_CASE_BODY(plus_expr, node_id)
      case pointer_plus_expr_K:
         CREATE_TREE_NODE_CASE_BODY(pointer_plus_expr, node_id)
      case pointer_type_K:
         RET_NODE_ID_CASE_BODY(pointer_type, node_id)
      case postdecrement_expr_K:
         CREATE_TREE_NODE_CASE_BODY(postdecrement_expr, node_id)
      case postincrement_expr_K:
         CREATE_TREE_NODE_CASE_BODY(postincrement_expr, node_id)
      case predecrement_expr_K:
         CREATE_TREE_NODE_CASE_BODY(predecrement_expr, node_id)
      case preincrement_expr_K:
         CREATE_TREE_NODE_CASE_BODY(preincrement_expr, node_id)
      case qual_union_type_K:
         RET_NODE_ID_CASE_BODY(qual_union_type, node_id)
      case range_expr_K:
         CREATE_TREE_NODE_CASE_BODY(range_expr, node_id)
      case paren_expr_K:
         CREATE_TREE_NODE_CASE_BODY(paren_expr, node_id)
      case rdiv_expr_K:
         CREATE_TREE_NODE_CASE_BODY(rdiv_expr, node_id)
      case real_cst_K:
         RET_NODE_ID_CASE_BODY(real_cst, node_id)
      case realpart_expr_K:
         CREATE_TREE_NODE_CASE_BODY(realpart_expr, node_id)
      case real_type_K:
         RET_NODE_ID_CASE_BODY(real_type, node_id)
      case record_type_K:
         RET_NODE_ID_CASE_BODY(record_type, node_id)
      case reduc_max_expr_K:
         CREATE_TREE_NODE_CASE_BODY(reduc_max_expr, node_id)
      case reduc_min_expr_K:
         CREATE_TREE_NODE_CASE_BODY(reduc_min_expr, node_id)
      case reduc_plus_expr_K:
         CREATE_TREE_NODE_CASE_BODY(reduc_plus_expr, node_id)
      case reference_expr_K:
         CREATE_TREE_NODE_CASE_BODY(reference_expr, node_id)
      case reference_type_K:
         RET_NODE_ID_CASE_BODY(reference_type, node_id)
      case reinterpret_cast_expr_K:
         CREATE_TREE_NODE_CASE_BODY(reinterpret_cast_expr, node_id)
      case result_decl_K:
         if(mode >= DECLARATION)
            CREATE_TREE_NODE_CASE_BODY(result_decl, node_id)
         else
            RET_NODE_ID_CASE_BODY(result_decl, node_id)
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
         if(mode)
            CREATE_TREE_NODE_CASE_BODY(scope_ref, node_id)
         else
            RET_NODE_ID_CASE_BODY(scope_ref, node_id)
      case set_le_expr_K:
         CREATE_TREE_NODE_CASE_BODY(set_le_expr, node_id)
      case set_type_K:
         RET_NODE_ID_CASE_BODY(set_type, node_id)
      case sizeof_expr_K:
         CREATE_TREE_NODE_CASE_BODY(sizeof_expr, node_id)
      case ssa_name_K:
         if(mode)
            CREATE_TREE_NODE_CASE_BODY(ssa_name, node_id)
         else
            RET_NODE_ID_CASE_BODY(ssa_name, node_id)
      case statement_list_K:
         if(mode)
            CREATE_TREE_NODE_CASE_BODY(statement_list, node_id)
         else
            RET_NODE_ID_CASE_BODY(statement_list, node_id)
      case static_cast_expr_K:
         CREATE_TREE_NODE_CASE_BODY(static_cast_expr, node_id)
      case string_cst_K:
         RET_NODE_ID_CASE_BODY(string_cst, node_id)
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
         if(mode >= tree_node_dup_mode::FUNCTION)
            CREATE_TREE_NODE_CASE_BODY(template_decl, node_id)
         else
            RET_NODE_ID_CASE_BODY(template_decl, node_id)
      case template_id_expr_K:
         CREATE_TREE_NODE_CASE_BODY(template_id_expr, node_id)
      case template_parm_index_K:
         CREATE_TREE_NODE_CASE_BODY(template_parm_index, node_id)
      case template_type_parm_K:
         RET_NODE_ID_CASE_BODY(template_type_parm, node_id)
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
         RET_NODE_ID_CASE_BODY(translation_unit_decl, node_id)
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
         RET_NODE_ID_CASE_BODY(type_decl, node_id)
      case typename_type_K:
         RET_NODE_ID_CASE_BODY(typename_type, node_id)
      case uneq_expr_K:
         CREATE_TREE_NODE_CASE_BODY(uneq_expr, node_id)
      case unge_expr_K:
         CREATE_TREE_NODE_CASE_BODY(unge_expr, node_id)
      case ungt_expr_K:
         CREATE_TREE_NODE_CASE_BODY(ungt_expr, node_id)
      case union_type_K:
         RET_NODE_ID_CASE_BODY(union_type, node_id)
      case unle_expr_K:
         CREATE_TREE_NODE_CASE_BODY(unle_expr, node_id)
      case unlt_expr_K:
         CREATE_TREE_NODE_CASE_BODY(unlt_expr, node_id)
      case unordered_expr_K:
         CREATE_TREE_NODE_CASE_BODY(unordered_expr, node_id)
      case unsave_expr_K:
         CREATE_TREE_NODE_CASE_BODY(unsave_expr, node_id)
      case using_decl_K:
         RET_NODE_ID_CASE_BODY(using_decl, node_id)
      case va_arg_expr_K:
         CREATE_TREE_NODE_CASE_BODY(va_arg_expr, node_id)
      case var_decl_K:
      {
         const auto vd = GetPointerS<const var_decl>(tn);
         if(mode >= DECLARATION && (!vd->scpe || (vd->scpe->get_kind() != translation_unit_decl_K &&
                                                  vd->scpe->get_kind() != namespace_decl_K)))
         {
            if(vd->static_flag)
            {
               if(remap.find(tn->index) != remap.end())
               {
                  node_id = remap.find(tn->index)->second;
               }
               else
               {
                  node_id = tn->index;
               }
               const auto new_tn = GetPointerS<var_decl>(TM->GetTreeNode(node_id));
               new_tn->scpe = GetPointerS<const decl_node>(new_tn->scpe)->scpe;
               break;
            }
            else
               CREATE_TREE_NODE_CASE_BODY(var_decl, node_id)
         }
         else
            RET_NODE_ID_CASE_BODY(var_decl, node_id)
      }
      case vec_new_expr_K:
         CREATE_TREE_NODE_CASE_BODY(vec_new_expr, node_id)
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
         CREATE_TREE_NODE_CASE_BODY(vec_interleavehigh_expr, node_id)
      case vec_interleavelow_expr_K:
         CREATE_TREE_NODE_CASE_BODY(vec_interleavelow_expr, node_id)
      case vector_cst_K:
         RET_NODE_ID_CASE_BODY(vector_cst, node_id)
      case void_cst_K:
         RET_NODE_ID_CASE_BODY(void_cst, node_id)
      case type_argument_pack_K:
         RET_NODE_ID_CASE_BODY(type_argument_pack, node_id)
      case nontype_argument_pack_K:
         RET_NODE_ID_CASE_BODY(nontype_argument_pack, node_id)
      case expr_pack_expansion_K:
         RET_NODE_ID_CASE_BODY(expr_pack_expansion, node_id)
      case vector_type_K:
         RET_NODE_ID_CASE_BODY(vector_type, node_id)
      case view_convert_expr_K:
         CREATE_TREE_NODE_CASE_BODY(view_convert_expr, node_id)
      case gimple_predict_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_predict, node_id)
      case void_type_K:
         RET_NODE_ID_CASE_BODY(void_type, node_id)
      case vtable_ref_K:
         RET_NODE_ID_CASE_BODY(vtable_ref, node_id)
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
      case omp_simd_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(omp_simd_pragma, node_id)
      case omp_declare_simd_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(omp_declare_simd_pragma, node_id)
      case omp_parallel_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(omp_parallel_pragma, node_id)
      case omp_sections_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(omp_sections_pragma, node_id)
      case omp_parallel_sections_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(omp_parallel_sections_pragma, node_id)
      case omp_section_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(omp_section_pragma, node_id)
      case map_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(map_pragma, node_id)
      case call_hw_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(call_hw_pragma, node_id)
      case call_point_hw_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(call_point_hw_pragma, node_id)
      case issue_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(issue_pragma, node_id)
      case profiling_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(profiling_pragma, node_id)
      case blackbox_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(blackbox_pragma, node_id)
      case statistical_profiling_K:
         CREATE_TREE_NODE_CASE_BODY(statistical_profiling, node_id)
      case null_node_K:
         CREATE_TREE_NODE_CASE_BODY(null_node, node_id)
      case gimple_nop_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_nop, node_id)
      case identifier_node_K:
         RET_NODE_ID_CASE_BODY(identifier_node, node_id)
      case widen_sum_expr_K:
         CREATE_TREE_NODE_CASE_BODY(widen_sum_expr, node_id)
      case widen_mult_expr_K:
         CREATE_TREE_NODE_CASE_BODY(widen_mult_expr, node_id)
      case trait_expr_K:
         CREATE_TREE_NODE_CASE_BODY(trait_expr, node_id)
      case omp_target_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(omp_target_pragma, node_id)
      case omp_task_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(omp_task_pragma, node_id)
      case omp_critical_pragma_K:
         CREATE_TREE_NODE_CASE_BODY(omp_critical_pragma, node_id)
      case gimple_multi_way_if_K:
         CREATE_TREE_NODE_CASE_BODY(gimple_multi_way_if, node_id)
      case extract_bit_expr_K:
         CREATE_TREE_NODE_CASE_BODY(extract_bit_expr, node_id)
      case sat_plus_expr_K:
         CREATE_TREE_NODE_CASE_BODY(sat_plus_expr, node_id)
      case sat_minus_expr_K:
         CREATE_TREE_NODE_CASE_BODY(sat_minus_expr, node_id)
      case fshl_expr_K:
         CREATE_TREE_NODE_CASE_BODY(fshl_expr, node_id)
      case fshr_expr_K:
         CREATE_TREE_NODE_CASE_BODY(fshr_expr, node_id)
      case extractvalue_expr_K:
         CREATE_TREE_NODE_CASE_BODY(extractvalue_expr, node_id)
      case insertvalue_expr_K:
         CREATE_TREE_NODE_CASE_BODY(insertvalue_expr, node_id)
      case extractelement_expr_K:
         CREATE_TREE_NODE_CASE_BODY(extractelement_expr, node_id)
      case insertelement_expr_K:
         CREATE_TREE_NODE_CASE_BODY(insertelement_expr, node_id)
      case frem_expr_K:
         CREATE_TREE_NODE_CASE_BODY(frem_expr, node_id)
      case do_stmt_K:
      case if_stmt_K:
      case for_stmt_K:
      case while_stmt_K:
      case CASE_FAKE_NODES:
      {
         THROW_UNREACHABLE(tn->get_kind_text());
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return node_id;
}

unsigned int tree_node_dup::get_bbi(unsigned int old_bbi)
{
   if(remap_bbi)
   {
      const auto t = remap_bb.insert(std::make_pair(old_bbi, remap_bbi));
      return t.second ? remap_bbi++ : t.first->second;
   }
   return old_bbi;
}

unsigned int tree_node_dup::get_loop_id(unsigned int old_loop_id)
{
   if(remap_loop_id)
   {
      const auto t = remap_lid.insert(std::make_pair(old_loop_id, remap_loop_id));
      return t.second ? remap_loop_id++ : t.first->second;
   }
   return old_loop_id;
}

void tree_node_dup::operator()(const tree_node* obj, unsigned int&)
{
   THROW_ERROR("tree_node not supported: " + std::string(obj->get_kind_text()));
}

void tree_node_dup::operator()(const tree_reindex* obj, unsigned int&)
{
   THROW_ERROR("tree_node not supported: " + std::string(obj->get_kind_text()));
}

void tree_node_dup::operator()(const attr* obj, unsigned int& mask)
{
   // NOTE: const_cast below is "safe" since the following assert must be true
   THROW_ASSERT(obj == dynamic_cast<attr*>(curr_tree_node_ptr), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   // cppcheck-suppress unusedVariable
   bool attr_p;

#define ATTR_SEQ                                                                                                      \
   (TOK_NEW)(TOK_DELETE)(TOK_ASSIGN)(TOK_MEMBER)(TOK_PUBLIC)(TOK_PROTECTED)(TOK_PRIVATE)(TOK_NORETURN)(TOK_VOLATILE)( \
       TOK_NOINLINE)(TOK_ALWAYS_INLINE)(TOK_USED)(TOK_UNUSED)(TOK_CONST)(TOK_TRANSPARENT_UNION)(TOK_CONSTRUCTOR)(     \
       TOK_DESTRUCTOR)(TOK_MODE)(TOK_SECTION)(TOK_ALIGNED)(TOK_WEAK)(TOK_ALIAS)(TOK_NO_INSTRUMENT_FUNCTION)(          \
       TOK_MALLOC)(TOK_NO_STACK_LIMIT)(TOK_PURE)(TOK_DEPRECATED)(TOK_VECTOR_SIZE)(TOK_VISIBILITY)(TOK_TLS_MODEL)(     \
       TOK_NONNULL)(TOK_NOTHROW)(TOK_MAY_ALIAS)(TOK_WARN_UNUSED_RESULT)(TOK_FORMAT)(TOK_FORMAT_ARG)(TOK_NULL)(        \
       TOK_GLOBAL_INIT)(TOK_GLOBAL_FINI)(TOK_CONVERSION)(TOK_VIRTUAL)(TOK_LSHIFT)(TOK_MUTABLE)(TOK_PSEUDO_TMPL)(      \
       TOK_VECNEW)(TOK_VECDELETE)(TOK_POS)(TOK_NEG)(TOK_ADDR)(TOK_DEREF)(TOK_LNOT)(TOK_NOT)(TOK_PREINC)(TOK_PREDEC)(  \
       TOK_PLUSASSIGN)(TOK_PLUS)(TOK_MINUSASSIGN)(TOK_MINUS)(TOK_MULTASSIGN)(TOK_MULT)(TOK_DIVASSIGN)(TOK_DIV)(       \
       TOK_MODASSIGN)(TOK_MOD)(TOK_ANDASSIGN)(TOK_AND)(TOK_ORASSIGN)(TOK_OR)(TOK_XORASSIGN)(TOK_XOR)(                 \
       TOK_LSHIFTASSIGN)(TOK_RSHIFTASSIGN)(TOK_RSHIFT)(TOK_EQ)(TOK_NE)(TOK_LT)(TOK_GT)(TOK_LE)(TOK_GE)(TOK_LAND)(     \
       TOK_LOR)(TOK_COMPOUND)(TOK_MEMREF)(TOK_REF)(TOK_SUBS)(TOK_POSTINC)(TOK_POSTDEC)(TOK_CALL)(TOK_THUNK)(          \
       TOK_THIS_ADJUSTING)(TOK_RESULT_ADJUSTING)(TOK_BITFIELD)
#define ATTR_MACRO(r, data, elem)                                                                                   \
   attr_p = GetPointer<attr>(source_tn)->list_attr.find(TOK(elem)) != GetPointer<attr>(source_tn)->list_attr.end(); \
   if(attr_p)                                                                                                       \
      const_cast<attr*>(obj)->list_attr.insert(TOK(elem));

   BOOST_PP_SEQ_FOR_EACH(ATTR_MACRO, BOOST_PP_EMPTY, ATTR_SEQ);
#undef ATTR_MACRO
#undef ATTR_SEQ
}

#define SET_NODE_ID(field, type)                                                \
   if(GetPointerS<type>(source_tn)->field)                                      \
   {                                                                            \
      unsigned int node_id = GetPointerS<type>(source_tn)->field->index;        \
      if(remap.find(node_id) != remap.end())                                    \
      {                                                                         \
         node_id = remap.find(node_id)->second;                                 \
      }                                                                         \
      else                                                                      \
      {                                                                         \
         tree_node* saved_curr_tree_node_ptr = curr_tree_node_ptr;              \
         tree_nodeRef saved_source_tn = source_tn;                              \
         node_id = create_tree_node(GetPointerS<type>(source_tn)->field, mode); \
         curr_tree_node_ptr = saved_curr_tree_node_ptr;                         \
         source_tn = saved_source_tn;                                           \
      }                                                                         \
      static_cast<type*>(curr_tree_node_ptr)->field = TM->GetTreeNode(node_id); \
   }

#define SEQ_SET_NODE_ID(list_field, type)                                                        \
   if(!GetPointerS<type>(source_tn)->list_field.empty())                                         \
   {                                                                                             \
      for(auto const& field : GetPointerS<type>(source_tn)->list_field)                          \
      {                                                                                          \
         unsigned int node_id = field->index;                                                    \
         if(remap.find(node_id) != remap.end())                                                  \
            node_id = remap.find(node_id)->second;                                               \
         else                                                                                    \
         {                                                                                       \
            tree_node* saved_curr_tree_node_ptr = curr_tree_node_ptr;                            \
            tree_nodeRef saved_source_tn = source_tn;                                            \
            node_id = create_tree_node(field, mode);                                             \
            curr_tree_node_ptr = saved_curr_tree_node_ptr;                                       \
            source_tn = saved_source_tn;                                                         \
         }                                                                                       \
         static_cast<type*>(curr_tree_node_ptr)->list_field.push_back(TM->GetTreeNode(node_id)); \
      }                                                                                          \
   }

#define SET_SET_NODE_ID(list_field, type)                                                     \
   if(!GetPointerS<type>(source_tn)->list_field.empty())                                      \
   {                                                                                          \
      for(const auto& i : GetPointerS<type>(source_tn)->list_field)                           \
      {                                                                                       \
         unsigned int node_id = i->index;                                                     \
         if(remap.find(node_id) != remap.end())                                               \
            node_id = remap.find(node_id)->second;                                            \
         else                                                                                 \
         {                                                                                    \
            tree_node* saved_curr_tree_node_ptr = curr_tree_node_ptr;                         \
            tree_nodeRef saved_source_tn = source_tn;                                         \
            node_id = create_tree_node(i, mode);                                              \
            curr_tree_node_ptr = saved_curr_tree_node_ptr;                                    \
            source_tn = saved_source_tn;                                                      \
         }                                                                                    \
         static_cast<type*>(curr_tree_node_ptr)->list_field.insert(TM->GetTreeNode(node_id)); \
      }                                                                                       \
   }

#define LSEQ_SET_NODE_ID(list_field, type)                                                                         \
   if(!GetPointerS<type>(source_tn)->list_field.empty())                                                           \
   {                                                                                                               \
      std::list<tree_nodeRef>::const_iterator vend = GetPointerS<type>(source_tn)->list_field.end();               \
      for(std::list<tree_nodeRef>::const_iterator i = GetPointerS<type>(source_tn)->list_field.begin(); i != vend; \
          ++i)                                                                                                     \
      {                                                                                                            \
         unsigned int node_id = (*i)->index;                                                                       \
         if(remap.find(node_id) != remap.end())                                                                    \
            node_id = remap.find(node_id)->second;                                                                 \
         else                                                                                                      \
         {                                                                                                         \
            tree_node* saved_curr_tree_node_ptr = curr_tree_node_ptr;                                              \
            tree_nodeRef saved_source_tn = source_tn;                                                              \
            node_id = create_tree_node(*i, mode);                                                                  \
            curr_tree_node_ptr = saved_curr_tree_node_ptr;                                                         \
            source_tn = saved_source_tn;                                                                           \
         }                                                                                                         \
         static_cast<type*>(curr_tree_node_ptr)->list_field.push_back(TM->GetTreeNode(node_id));                   \
      }                                                                                                            \
   }

#define SET_VALUE(field, type) (static_cast<type*>(curr_tree_node_ptr)->field = GetPointerS<type>(source_tn)->field)

void tree_node_dup::operator()(const srcp* obj, unsigned int& mask)
{
   // NOTE: const_cast below are "safe" since the following assert must be true
   THROW_ASSERT(obj == dynamic_cast<srcp*>(curr_tree_node_ptr), "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   const_cast<srcp*>(obj)->include_name = GetPointer<srcp>(source_tn)->include_name;
   const_cast<srcp*>(obj)->line_number = GetPointer<srcp>(source_tn)->line_number;
   const_cast<srcp*>(obj)->column_number = GetPointer<srcp>(source_tn)->column_number;
}

void tree_node_dup::operator()(const WeightedNode* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_dup::operator()(const decl_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_NODE_ID(name, decl_node);
   SET_NODE_ID(mngl, decl_node);
   SET_NODE_ID(orig, decl_node);
   SET_NODE_ID(type, decl_node);
   SET_NODE_ID(scpe, decl_node);
   SET_NODE_ID(attributes, decl_node);
   SET_NODE_ID(chan, decl_node);
   SET_VALUE(artificial_flag, decl_node);
   SET_VALUE(packed_flag, decl_node);
   SET_VALUE(operating_system_flag, decl_node);
   SET_VALUE(library_system_flag, decl_node);
   SET_VALUE(C_flag, decl_node);
   SET_VALUE(uid, decl_node);
}

void tree_node_dup::operator()(const expr_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(type, expr_node);
}

void tree_node_dup::operator()(const gimple_node* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(memuse, gimple_node);
   SET_NODE_ID(memdef, gimple_node);
   SET_SET_NODE_ID(vuses, gimple_node);
   SET_NODE_ID(vdef, gimple_node);
   SET_SET_NODE_ID(vovers, gimple_node);
   SEQ_SET_NODE_ID(pragmas, gimple_node);
   SET_NODE_ID(scpe, gimple_node);
   SET_VALUE(bb_index, gimple_node);
   SET_VALUE(use_set->anything, gimple_node);
   SET_VALUE(use_set->escaped, gimple_node);
   SET_VALUE(use_set->ipa_escaped, gimple_node);
   SET_VALUE(use_set->nonlocal, gimple_node);
   SET_VALUE(use_set->null, gimple_node);
   SET_VALUE(clobbered_set->anything, gimple_node);
   SET_VALUE(clobbered_set->escaped, gimple_node);
   SET_VALUE(clobbered_set->ipa_escaped, gimple_node);
   SET_VALUE(clobbered_set->nonlocal, gimple_node);
   SET_VALUE(clobbered_set->null, gimple_node);
   SEQ_SET_NODE_ID(use_set->variables, gimple_node);
   SEQ_SET_NODE_ID(clobbered_set->variables, gimple_node);
}

void tree_node_dup::operator()(const unary_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(op, unary_expr);
}

void tree_node_dup::operator()(const binary_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(op0, binary_expr);
   SET_NODE_ID(op1, binary_expr);
}

void tree_node_dup::operator()(const ternary_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(op0, ternary_expr);
   SET_NODE_ID(op1, ternary_expr);
   SET_NODE_ID(op2, ternary_expr);
}

void tree_node_dup::operator()(const quaternary_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(op0, quaternary_expr);
   SET_NODE_ID(op1, quaternary_expr);
   SET_NODE_ID(op2, quaternary_expr);
   SET_NODE_ID(op3, quaternary_expr);
}

void tree_node_dup::operator()(const type_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE(qual, type_node);
   SET_NODE_ID(name, type_node);
   SET_NODE_ID(unql, type_node);
   SET_NODE_ID(size, type_node);
   SET_NODE_ID(scpe, type_node);
   SET_VALUE(system_flag, type_node);
   SET_VALUE(packed_flag, type_node);
   SET_VALUE(algn, type_node);
}

void tree_node_dup::operator()(const cst_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(type, cst_node);
}

void tree_node_dup::operator()(const error_mark* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_dup::operator()(const array_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(elts, array_type);
   SET_NODE_ID(domn, array_type);
}

void tree_node_dup::operator()(const gimple_asm* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE(volatile_flag, gimple_asm);
   SET_VALUE(str, gimple_asm);
   SET_NODE_ID(out, gimple_asm);
   SET_NODE_ID(in, gimple_asm);
   SET_NODE_ID(clob, gimple_asm);
}

void tree_node_dup::operator()(const baselink* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(type, baselink);
}

void tree_node_dup::operator()(const gimple_bind* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SEQ_SET_NODE_ID(list_of_vars, gimple_bind);
   SET_NODE_ID(body, gimple_bind);
}

void tree_node_dup::operator()(const binfo* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(type, binfo);
   SET_VALUE(virt_flag, binfo);
   SET_VALUE(bases, binfo);
   if(!GetPointerS<binfo>(source_tn)->list_of_access_binf.empty())
   {
      auto vend = GetPointerS<binfo>(source_tn)->list_of_access_binf.end();
      for(auto i = GetPointerS<binfo>(source_tn)->list_of_access_binf.begin(); i != vend; ++i)
      {
         unsigned int node_id = i->second->index;
         THROW_ASSERT(remap.find(node_id) != remap.end(), "missing an index");
         node_id = remap.find(node_id)->second;
         static_cast<binfo*>(curr_tree_node_ptr)->add_access_binf(TM->GetTreeNode(node_id), i->first);
      }
   }
}

void tree_node_dup::operator()(const block* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE(bl_flag, block);
   SET_VALUE(bl, block);
}

void tree_node_dup::operator()(const call_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(fn, call_expr);
   SEQ_SET_NODE_ID(args, call_expr);
}

void tree_node_dup::operator()(const aggr_init_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(slot, aggr_init_expr);
}

void tree_node_dup::operator()(const gimple_call* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(fn, gimple_call);
   SEQ_SET_NODE_ID(args, gimple_call);
   SET_VALUE(use_set->anything, gimple_call);
   SET_VALUE(use_set->escaped, gimple_call);
   SET_VALUE(use_set->ipa_escaped, gimple_call);
   SET_VALUE(use_set->nonlocal, gimple_call);
   SET_VALUE(use_set->null, gimple_call);
   SET_VALUE(clobbered_set->anything, gimple_call);
   SET_VALUE(clobbered_set->escaped, gimple_call);
   SET_VALUE(clobbered_set->ipa_escaped, gimple_call);
   SET_VALUE(clobbered_set->nonlocal, gimple_call);
   SET_VALUE(clobbered_set->null, gimple_call);
   SEQ_SET_NODE_ID(use_set->variables, gimple_call);
   SEQ_SET_NODE_ID(clobbered_set->variables, gimple_call);
}

void tree_node_dup::operator()(const case_label_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(op0, case_label_expr);
   SET_NODE_ID(op1, case_label_expr);
   SET_VALUE(default_flag, case_label_expr);
   SET_NODE_ID(got, case_label_expr);
}

void tree_node_dup::operator()(const cast_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(op, cast_expr);
}

void tree_node_dup::operator()(const complex_cst* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(real, complex_cst);
   SET_NODE_ID(imag, complex_cst);
}

void tree_node_dup::operator()(const complex_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE(unsigned_flag, complex_type);
   SET_VALUE(real_flag, complex_type);
}

void tree_node_dup::operator()(const gimple_cond* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(op0, gimple_cond);
}

void tree_node_dup::operator()(const const_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(cnst, const_decl);
}

void tree_node_dup::operator()(const constructor* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(type, constructor);
   if(!GetPointerS<constructor>(source_tn)->list_of_idx_valu.empty())
   {
      auto vend = GetPointerS<constructor>(source_tn)->list_of_idx_valu.end();
      for(auto i = GetPointerS<constructor>(source_tn)->list_of_idx_valu.begin(); i != vend; ++i)
      {
         unsigned int node_id1 = i->first ? i->first->index : 0;
         unsigned int node_id2 = i->second->index;
         if(mode && node_id1 && remap.find(node_id1) == remap.end())
         {
            tree_node* saved_curr_tree_node_ptr = curr_tree_node_ptr;
            tree_nodeRef saved_source_tn = source_tn;
            node_id1 = create_tree_node(i->first, mode);
            curr_tree_node_ptr = saved_curr_tree_node_ptr;
            source_tn = saved_source_tn;
         }
         else
         {
            THROW_ASSERT(!node_id1 || remap.find(node_id1) != remap.end(), "missing an index");
            node_id1 = node_id1 ? remap.find(node_id1)->second : 0;
         }
         if(mode && remap.find(node_id2) == remap.end())
         {
            tree_node* saved_curr_tree_node_ptr = curr_tree_node_ptr;
            tree_nodeRef saved_source_tn = source_tn;
            node_id2 = create_tree_node(i->second, mode);
            curr_tree_node_ptr = saved_curr_tree_node_ptr;
            source_tn = saved_source_tn;
         }
         else
         {
            THROW_ASSERT(remap.find(node_id2) != remap.end(), "missing an index");
            node_id2 = remap.find(node_id2)->second;
         }
         if(node_id1)
         {
            static_cast<constructor*>(curr_tree_node_ptr)
                ->add_idx_valu(TM->GetTreeNode(node_id1), TM->GetTreeNode(node_id2));
         }
         else
         {
            static_cast<constructor*>(curr_tree_node_ptr)->add_valu(TM->GetTreeNode(node_id2));
         }
      }
   }
}

void tree_node_dup::operator()(const enumeral_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE(prec, enumeral_type);
   SET_VALUE(unsigned_flag, enumeral_type);
   SET_NODE_ID(min, enumeral_type);
   SET_NODE_ID(max, enumeral_type);
   SET_NODE_ID(csts, enumeral_type);
}

void tree_node_dup::operator()(const expr_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_VALUE(line, expr_stmt);
   SET_NODE_ID(expr, expr_stmt);
   SET_NODE_ID(next, expr_stmt);
}

void tree_node_dup::operator()(const field_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE(algn, field_decl);
   SET_NODE_ID(init, field_decl);
   SET_NODE_ID(size, field_decl);
   SET_NODE_ID(bpos, field_decl);
   SET_NODE_ID(smt_ann, field_decl);
}

void tree_node_dup::operator()(const function_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE(operator_flag, function_decl);
   if(!GetPointerS<function_decl>(source_tn)->list_of_op_names.empty())
   {
      auto vend = GetPointerS<function_decl>(source_tn)->list_of_op_names.end();
      for(auto i = GetPointerS<function_decl>(source_tn)->list_of_op_names.begin(); i != vend; ++i)
      {
         static_cast<function_decl*>(curr_tree_node_ptr)->add(*i);
      }
   }

   SET_NODE_ID(tmpl_parms, function_decl);
   SET_NODE_ID(tmpl_args, function_decl);

   SET_VALUE(fixd, function_decl);
   SET_VALUE(fixd_flag, function_decl);
   SET_VALUE(virt_flag, function_decl);
   SET_VALUE(virt, function_decl);
   SET_NODE_ID(fn, function_decl);
   SEQ_SET_NODE_ID(list_of_args, function_decl);
   SET_VALUE(undefined_flag, function_decl);
   SET_VALUE(builtin_flag, function_decl);
   SET_VALUE(static_flag, function_decl);
   SET_VALUE(hwcall_flag, function_decl);
   SET_VALUE(reverse_restrict_flag, function_decl);
   SET_VALUE(writing_memory, function_decl);
   SET_VALUE(reading_memory, function_decl);
   SET_VALUE(pipeline_enabled, function_decl);
   SET_VALUE(simple_pipeline, function_decl);
   SET_VALUE(initiation_time, function_decl);
#if HAVE_FROM_PRAGMA_BUILT
   SET_VALUE(omp_atomic, function_decl);
   SET_VALUE(omp_body_loop, function_decl);
   SET_VALUE(omp_critical, function_decl);
   SET_VALUE(omp_for_wrapper, function_decl);
#endif
   const auto prev_mode = mode;
   if(mode == tree_node_dup_mode::FUNCTION)
   {
      // Avoid recursvie function_decl duplication
      mode = DECLARATION;
   }
   SET_NODE_ID(body, function_decl);
   SET_NODE_ID(inline_body, function_decl);
   mode = prev_mode;
}

void tree_node_dup::operator()(const function_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(retn, function_type);
   SET_NODE_ID(prms, function_type);
   SET_VALUE(varargs_flag, function_type);
}

void tree_node_dup::operator()(const gimple_assign* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(op0, gimple_assign);
   SET_NODE_ID(op1, gimple_assign);
   SET_NODE_ID(predicate, gimple_assign);
   SET_VALUE(init_assignment, gimple_assign);
   SET_VALUE(clobber, gimple_assign);
   SET_VALUE(temporary_address, gimple_assign);
   SET_VALUE(use_set->anything, gimple_assign);
   SET_VALUE(use_set->escaped, gimple_assign);
   SET_VALUE(use_set->ipa_escaped, gimple_assign);
   SET_VALUE(use_set->nonlocal, gimple_assign);
   SET_VALUE(use_set->null, gimple_assign);
   SET_VALUE(clobbered_set->anything, gimple_assign);
   SET_VALUE(clobbered_set->escaped, gimple_assign);
   SET_VALUE(clobbered_set->ipa_escaped, gimple_assign);
   SET_VALUE(clobbered_set->nonlocal, gimple_assign);
   SET_VALUE(clobbered_set->null, gimple_assign);
   SEQ_SET_NODE_ID(use_set->variables, gimple_assign);
   SEQ_SET_NODE_ID(clobbered_set->variables, gimple_assign);
}

void tree_node_dup::operator()(const gimple_goto* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(op, gimple_goto);
}

void tree_node_dup::operator()(const handler* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_VALUE(line, handler);
   SET_NODE_ID(body, handler);
}

void tree_node_dup::operator()(const identifier_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   THROW_ERROR("Use find_identifier_nodeID to find identifier_node objects");
}

void tree_node_dup::operator()(const integer_cst* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE(value, integer_cst);
}

void tree_node_dup::operator()(const integer_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE(prec, integer_type);
   SET_VALUE(str, integer_type);
   SET_VALUE(unsigned_flag, integer_type);
   SET_NODE_ID(min, integer_type);
   SET_NODE_ID(max, integer_type);
}

void tree_node_dup::operator()(const gimple_label* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(op, gimple_label);
}

void tree_node_dup::operator()(const method_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(clas, method_type);
}

void tree_node_dup::operator()(const namespace_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(dcls, namespace_decl);
}

void tree_node_dup::operator()(const overload* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_NODE_ID(crnt, overload);
   SET_NODE_ID(chan, overload);
}

void tree_node_dup::operator()(const parm_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(argt, parm_decl);
   SET_NODE_ID(size, parm_decl);
   SET_NODE_ID(scpe, parm_decl);
   SET_VALUE(algn, parm_decl);
   SET_VALUE(used, parm_decl);
   SET_VALUE(register_flag, parm_decl);
   SET_VALUE(readonly_flag, parm_decl);
   SET_NODE_ID(smt_ann, parm_decl);
}

void tree_node_dup::operator()(const gimple_phi* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_NODE_ID(res, gimple_phi);
   for(const auto& def_edge : GetPointerS<gimple_phi>(source_tn)->CGetDefEdgesList())
   {
      unsigned int node_id = def_edge.first->index;
      if(mode)
      {
         const auto rnode = remap.find(node_id);
         if(rnode != remap.end())
         {
            node_id = rnode->second;
         }
         else
         {
            tree_node* saved_curr_tree_node_ptr = curr_tree_node_ptr;
            tree_nodeRef saved_source_tn = source_tn;
            node_id = create_tree_node(def_edge.first, mode);
            curr_tree_node_ptr = saved_curr_tree_node_ptr;
            source_tn = saved_source_tn;
         }
      }
      else
      {
         const auto rnode = remap.find(node_id);
         if(rnode != remap.end())
         {
            node_id = rnode->second;
         }
      }

      static_cast<gimple_phi*>(curr_tree_node_ptr)
          ->AddDefEdge(TM, gimple_phi::DefEdge(TM->GetTreeNode(node_id), get_bbi(def_edge.second)));
   }
   SET_VALUE(virtual_flag, gimple_phi);
   if(use_counting)
   {
      static_cast<gimple_phi*>(curr_tree_node_ptr)->SetSSAUsesComputed();
   }
}

void tree_node_dup::operator()(const pointer_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(ptd, pointer_type);
}

void tree_node_dup::operator()(const real_cst* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE(overflow_flag, real_cst);
   SET_VALUE(valr, real_cst);
   SET_VALUE(valx, real_cst);
}

void tree_node_dup::operator()(const real_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE(prec, real_type);
}

void tree_node_dup::operator()(const record_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(tmpl_parms, record_type);
   SET_NODE_ID(tmpl_args, record_type);

   SET_VALUE(ptrmem_flag, record_type);
   SET_NODE_ID(ptd, record_type);
   SET_NODE_ID(cls, record_type);
   SET_NODE_ID(bfld, record_type);
   SET_NODE_ID(vfld, record_type);
   SET_VALUE(spec_flag, record_type);
   SET_VALUE(struct_flag, record_type);
   SEQ_SET_NODE_ID(list_of_flds, record_type);
   SEQ_SET_NODE_ID(list_of_fncs, record_type);
   SET_NODE_ID(binf, record_type);
}

void tree_node_dup::operator()(const reference_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(refd, reference_type);
}

void tree_node_dup::operator()(const result_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(init, result_decl);
   SET_NODE_ID(size, result_decl);
   SET_VALUE(algn, result_decl);
   SET_NODE_ID(smt_ann, result_decl);
}

void tree_node_dup::operator()(const gimple_return* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(op, gimple_return);
}

void tree_node_dup::operator()(const return_stmt* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_VALUE(line, return_stmt);
   SET_NODE_ID(expr, return_stmt);
}

void tree_node_dup::operator()(const scope_ref* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(op0, scope_ref);
   SET_NODE_ID(op1, scope_ref);
}

void tree_node_dup::operator()(const ssa_name* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_NODE_ID(type, ssa_name);
   SET_NODE_ID(var, ssa_name);
   if(mode)
   {
      static_cast<ssa_name*>(curr_tree_node_ptr)->vers = TM->get_next_vers();
   }
   else
   {
      SET_VALUE(vers, ssa_name);
   }
   SET_VALUE(orig_vers, ssa_name);
   SET_VALUE(use_set->anything, ssa_name);
   SET_VALUE(use_set->escaped, ssa_name);
   SET_VALUE(use_set->ipa_escaped, ssa_name);
   SET_VALUE(use_set->nonlocal, ssa_name);
   SET_VALUE(use_set->null, ssa_name);
   SEQ_SET_NODE_ID(use_set->variables, ssa_name);

   SET_VALUE(volatile_flag, ssa_name);
   SET_VALUE(virtual_flag, ssa_name);
   SET_VALUE(default_flag, ssa_name);
   for(const auto& def_stmt : GetPointerS<ssa_name>(source_tn)->CGetDefStmts())
   {
      if(mode)
      {
         unsigned int node_id = def_stmt->index;
         const auto rnode = remap.find(node_id);
         if(rnode != remap.end())
         {
            node_id = rnode->second;
         }
         else
         {
            const auto saved_curr_tree_node_ptr = curr_tree_node_ptr;
            const auto saved_source_tn = source_tn;
            node_id = create_tree_node(def_stmt, mode);
            curr_tree_node_ptr = saved_curr_tree_node_ptr;
            source_tn = saved_source_tn;
         }
         static_cast<ssa_name*>(curr_tree_node_ptr)->AddDefStmt(TM->GetTreeNode(node_id));
      }
      else
      {
         static_cast<ssa_name*>(curr_tree_node_ptr)->AddDefStmt(def_stmt);
      }
   }
   if(!mode)
   {
      SET_NODE_ID(min, ssa_name);
      SET_NODE_ID(max, ssa_name);
      SET_VALUE(bit_values, ssa_name);
      SET_VALUE(range, ssa_name);
   }
}

void tree_node_dup::operator()(const statement_list* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   LSEQ_SET_NODE_ID(list_of_stmt, statement_list);
   const auto mend = GetPointerS<const statement_list>(source_tn)->list_of_bloc.end();
   for(auto i = GetPointerS<const statement_list>(source_tn)->list_of_bloc.begin(); i != mend; ++i)
   {
      curr_bloc = new bloc(get_bbi(i->first));
      source_bloc = i->second;
      curr_bloc->visit(this);
      THROW_ASSERT(!static_cast<statement_list*>(curr_tree_node_ptr)->list_of_bloc.count(get_bbi(i->first)),
                   "Block already present " + STR(get_bbi(i->first)));
      static_cast<statement_list*>(curr_tree_node_ptr)->add_bloc(blocRef(curr_bloc));
      curr_bloc = nullptr;
      source_bloc = blocRef();
   }
   THROW_ASSERT(static_cast<statement_list*>(curr_tree_node_ptr)->list_of_bloc.size() ==
                    GetPointerS<const statement_list>(source_tn)->list_of_bloc.size(),
                "");
}

void tree_node_dup::operator()(const string_cst* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE(strg, string_cst);
}

void tree_node_dup::operator()(const gimple_switch* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(op0, gimple_switch);
   SET_NODE_ID(op1, gimple_switch);
}

void tree_node_dup::operator()(const target_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(decl, target_expr);
   SET_NODE_ID(init, target_expr);
   SET_NODE_ID(clnp, target_expr);
}

void tree_node_dup::operator()(const lut_expr* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(op0, lut_expr);
   SET_NODE_ID(op1, lut_expr);
   SET_NODE_ID(op2, lut_expr);
   SET_NODE_ID(op3, lut_expr);
   SET_NODE_ID(op4, lut_expr);
   SET_NODE_ID(op5, lut_expr);
   SET_NODE_ID(op6, lut_expr);
   SET_NODE_ID(op7, lut_expr);
   SET_NODE_ID(op8, lut_expr);
}
void tree_node_dup::operator()(const template_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(rslt, template_decl);
   SET_NODE_ID(inst, template_decl);
   SET_NODE_ID(spcs, template_decl);
   SET_NODE_ID(prms, template_decl);
}

void tree_node_dup::operator()(const template_parm_index* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(type, template_parm_index);
   SET_NODE_ID(decl, template_parm_index);
   SET_VALUE(constant_flag, template_parm_index);
   SET_VALUE(readonly_flag, template_parm_index);
   SET_VALUE(idx, template_parm_index);
   SET_VALUE(level, template_parm_index);
   SET_VALUE(orig_level, template_parm_index);
}

void tree_node_dup::operator()(const tree_list* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_NODE_ID(purp, tree_list);
   SET_NODE_ID(valu, tree_list);
   SET_NODE_ID(chan, tree_list);
}

void tree_node_dup::operator()(const tree_vec* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_VALUE(lngt, tree_vec);
   SEQ_SET_NODE_ID(list_of_op, tree_vec);
}

void tree_node_dup::operator()(const try_block* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_VALUE(line, try_block);
   SET_NODE_ID(body, try_block);
   SET_NODE_ID(hdlr, try_block);
   SET_NODE_ID(next, try_block);
}

void tree_node_dup::operator()(const type_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(tmpl_parms, type_decl);
   SET_NODE_ID(tmpl_args, type_decl);
}

void tree_node_dup::operator()(const union_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SEQ_SET_NODE_ID(list_of_flds, union_type);
   SEQ_SET_NODE_ID(list_of_fncs, union_type);
   SET_NODE_ID(binf, union_type);
}

void tree_node_dup::operator()(const var_decl* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_VALUE(use_tmpl, var_decl);
   SET_VALUE(static_static_flag, var_decl);
   SET_VALUE(static_flag, var_decl);
   SET_VALUE(extern_flag, var_decl);
   SET_VALUE(addr_taken, var_decl);
   SET_VALUE(addr_not_taken, var_decl);
   SET_NODE_ID(init, var_decl);
   SET_NODE_ID(size, var_decl);
   SET_VALUE(algn, var_decl);
   SET_VALUE(used, var_decl);
   SET_VALUE(register_flag, var_decl);
   SET_VALUE(readonly_flag, var_decl);
   if(!mode)
   {
      SET_VALUE(bit_values, var_decl);
   }
   SET_NODE_ID(smt_ann, var_decl);
   SET_SET_NODE_ID(defs, var_decl);
   SET_SET_NODE_ID(uses, var_decl);
   SET_SET_NODE_ID(addressings, var_decl);
}

void tree_node_dup::operator()(const vector_cst* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SEQ_SET_NODE_ID(list_of_valu, vector_cst);
}

void tree_node_dup::operator()(const type_argument_pack* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(arg, type_argument_pack);
}

void tree_node_dup::operator()(const nontype_argument_pack* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(arg, nontype_argument_pack);
}

void tree_node_dup::operator()(const type_pack_expansion* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(op, type_pack_expansion);
   SET_NODE_ID(param_packs, type_pack_expansion);
   SET_NODE_ID(arg, type_pack_expansion);
}

void tree_node_dup::operator()(const expr_pack_expansion* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(op, expr_pack_expansion);
   SET_NODE_ID(param_packs, expr_pack_expansion);
   SET_NODE_ID(arg, expr_pack_expansion);
}

void tree_node_dup::operator()(const vector_type* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(elts, vector_type);
}

void tree_node_dup::operator()(const target_mem_ref* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_NODE_ID(type, target_mem_ref);
   SET_NODE_ID(symbol, target_mem_ref);
   SET_NODE_ID(base, target_mem_ref);
   SET_NODE_ID(idx, target_mem_ref);
   SET_NODE_ID(step, target_mem_ref);
   SET_NODE_ID(offset, target_mem_ref);
   SET_NODE_ID(orig, target_mem_ref);
   SET_NODE_ID(tag, target_mem_ref);
}

void tree_node_dup::operator()(const target_mem_ref461* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);

   SET_NODE_ID(type, target_mem_ref461);
   SET_NODE_ID(base, target_mem_ref461);
   SET_NODE_ID(idx, target_mem_ref461);
   SET_NODE_ID(step, target_mem_ref461);
   SET_NODE_ID(idx2, target_mem_ref461);
   SET_NODE_ID(offset, target_mem_ref461);
}

void tree_node_dup::operator()(const bloc* obj, unsigned int& mask)
{
   tree_node_mask::operator()(obj, mask);

   curr_bloc->hpl = source_bloc->hpl;
   curr_bloc->loop_id = get_loop_id(source_bloc->loop_id);
   std::transform(source_bloc->list_of_pred.cbegin(), source_bloc->list_of_pred.cend(),
                  std::back_inserter(curr_bloc->list_of_pred), [&](const unsigned int& bbi) { return get_bbi(bbi); });
   std::transform(source_bloc->list_of_succ.cbegin(), source_bloc->list_of_succ.cend(),
                  std::back_inserter(curr_bloc->list_of_succ), [&](const unsigned int& bbi) { return get_bbi(bbi); });
   curr_bloc->true_edge = get_bbi(source_bloc->true_edge);
   curr_bloc->false_edge = get_bbi(source_bloc->false_edge);

   if(use_counting)
   {
      curr_bloc->SetSSAUsesComputed();
   }
   for(const auto& phi : source_bloc->CGetPhiList())
   {
      unsigned int node_id = phi->index;
      if(mode)
      {
         const auto rnode = remap.find(node_id);
         if(rnode != remap.end())
         {
            node_id = rnode->second;
         }
         else
         {
            tree_node* saved_curr_tree_node_ptr = curr_tree_node_ptr;
            tree_nodeRef saved_source_tn = source_tn;
            bloc* saved_curr_bloc = curr_bloc;
            node_id = create_tree_node(phi, mode);
            curr_tree_node_ptr = saved_curr_tree_node_ptr;
            source_tn = saved_source_tn;
            curr_bloc = saved_curr_bloc;
         }
      }
      else
      {
         THROW_ASSERT(remap.find(node_id) != remap.end(), "missing an index");
         node_id = remap.find(node_id)->second;
      }
      curr_bloc->AddPhi(TM->GetTreeNode(node_id));
   }
   for(const auto& stmt : source_bloc->CGetStmtList())
   {
      unsigned int node_id = stmt->index;
      if(mode)
      {
         const auto rnode = remap.find(node_id);
         if(rnode != remap.end())
         {
            node_id = rnode->second;
         }
         else
         {
            const auto saved_curr_tree_node_ptr = curr_tree_node_ptr;
            const auto saved_source_tn = source_tn;
            const auto saved_curr_bloc = curr_bloc;
            node_id = create_tree_node(stmt, mode);
            curr_tree_node_ptr = saved_curr_tree_node_ptr;
            source_tn = saved_source_tn;
            curr_bloc = saved_curr_bloc;
         }
      }
      else
      {
         THROW_ASSERT(remap.find(node_id) != remap.end(), "missing an index");
         node_id = remap.find(node_id)->second;
      }
      curr_bloc->PushBack(TM->GetTreeNode(node_id), AppM);
   }
}

void tree_node_dup::operator()(const gimple_while* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(op0, gimple_while);
}

void tree_node_dup::operator()(const gimple_for* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(op1, gimple_for);
   SET_NODE_ID(op2, gimple_for);
}

void tree_node_dup::operator()(const gimple_multi_way_if* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   THROW_ASSERT(source_tn, "");
   if(!GetPointerS<gimple_multi_way_if>(source_tn)->list_of_cond.empty())
   {
      for(const auto& cond : GetPointerS<gimple_multi_way_if>(source_tn)->list_of_cond)
      {
         if(cond.first)
         {
            unsigned int node_id = cond.first->index;
            if(mode)
            {
               if(remap.find(node_id) != remap.end())
               {
                  node_id = remap.find(node_id)->second;
                  THROW_ASSERT(node_id, "");
               }
               else
               {
                  tree_node* saved_curr_tree_node_ptr = curr_tree_node_ptr;
                  tree_nodeRef saved_source_tn = source_tn;
                  node_id = create_tree_node(cond.first, mode);
                  curr_tree_node_ptr = saved_curr_tree_node_ptr;
                  source_tn = saved_source_tn;
               }
            }
            else
            {
               THROW_ASSERT(remap.find(node_id) != remap.end(), "missing " + STR(TM->GetTreeNode(node_id)));
               node_id = remap.find(node_id)->second;
               THROW_ASSERT(node_id, "");
            }
            static_cast<gimple_multi_way_if*>(curr_tree_node_ptr)
                ->add_cond(TM->GetTreeNode(node_id), get_bbi(cond.second));
         }
         else
         {
            static_cast<gimple_multi_way_if*>(curr_tree_node_ptr)->add_cond(tree_nodeRef(), get_bbi(cond.second));
         }
      }
   }
}

void tree_node_dup::operator()(const null_node* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_dup::operator()(const gimple_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE(is_block, gimple_pragma);
   SET_VALUE(is_opening, gimple_pragma);
   SET_VALUE(line, gimple_pragma);
   SET_NODE_ID(scope, gimple_pragma);
   SET_NODE_ID(directive, gimple_pragma);
}

void tree_node_dup::operator()(const omp_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_dup::operator()(const omp_parallel_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE(is_shortcut, omp_parallel_pragma);
   if(!GetPointerS<omp_parallel_pragma>(source_tn)->clauses.empty())
   {
      auto vend = GetPointerS<omp_parallel_pragma>(source_tn)->clauses.end();
      for(auto i = GetPointerS<omp_parallel_pragma>(source_tn)->clauses.begin(); i != vend; ++i)
      {
         static_cast<omp_parallel_pragma*>(curr_tree_node_ptr)->clauses[i->first] = i->second;
      }
   }
}

void tree_node_dup::operator()(const omp_for_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   if(!GetPointerS<omp_for_pragma>(source_tn)->clauses.empty())
   {
      auto vend = GetPointerS<omp_for_pragma>(source_tn)->clauses.end();
      for(auto i = GetPointerS<omp_for_pragma>(source_tn)->clauses.begin(); i != vend; ++i)
      {
         static_cast<omp_for_pragma*>(curr_tree_node_ptr)->clauses[i->first] = i->second;
      }
   }
}

void tree_node_dup::operator()(const omp_simd_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   if(!GetPointerS<omp_simd_pragma>(source_tn)->clauses.empty())
   {
      auto vend = GetPointerS<omp_simd_pragma>(source_tn)->clauses.end();
      for(auto i = GetPointerS<omp_simd_pragma>(source_tn)->clauses.begin(); i != vend; ++i)
      {
         static_cast<omp_simd_pragma*>(curr_tree_node_ptr)->clauses[i->first] = i->second;
      }
   }
}

void tree_node_dup::operator()(const omp_declare_simd_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   if(!GetPointerS<omp_declare_simd_pragma>(source_tn)->clauses.empty())
   {
      auto vend = GetPointerS<omp_declare_simd_pragma>(source_tn)->clauses.end();
      for(auto i = GetPointerS<omp_declare_simd_pragma>(source_tn)->clauses.begin(); i != vend; ++i)
      {
         static_cast<omp_declare_simd_pragma*>(curr_tree_node_ptr)->clauses[i->first] = i->second;
      }
   }
}

void tree_node_dup::operator()(const omp_target_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   if(!GetPointerS<omp_target_pragma>(source_tn)->clauses.empty())
   {
      auto vend = GetPointerS<omp_target_pragma>(source_tn)->clauses.end();
      for(auto i = GetPointerS<omp_target_pragma>(source_tn)->clauses.begin(); i != vend; ++i)
      {
         static_cast<omp_target_pragma*>(curr_tree_node_ptr)->clauses[i->first] = i->second;
      }
   }
}

void tree_node_dup::operator()(const omp_critical_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   if(!GetPointerS<omp_critical_pragma>(source_tn)->clauses.empty())
   {
      auto vend = GetPointerS<omp_critical_pragma>(source_tn)->clauses.end();
      for(auto i = GetPointerS<omp_critical_pragma>(source_tn)->clauses.begin(); i != vend; ++i)
      {
         static_cast<omp_critical_pragma*>(curr_tree_node_ptr)->clauses[i->first] = i->second;
      }
   }
}

void tree_node_dup::operator()(const omp_task_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   if(!GetPointerS<omp_task_pragma>(source_tn)->clauses.empty())
   {
      auto vend = GetPointerS<omp_task_pragma>(source_tn)->clauses.end();
      for(auto i = GetPointerS<omp_task_pragma>(source_tn)->clauses.begin(); i != vend; ++i)
      {
         static_cast<omp_task_pragma*>(curr_tree_node_ptr)->clauses[i->first] = i->second;
      }
   }
}

void tree_node_dup::operator()(const omp_sections_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE(is_shortcut, omp_sections_pragma);
}

void tree_node_dup::operator()(const omp_parallel_sections_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_NODE_ID(op0, omp_parallel_sections_pragma);
   SET_NODE_ID(op1, omp_parallel_sections_pragma);
}

void tree_node_dup::operator()(const omp_section_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_dup::operator()(const map_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_dup::operator()(const call_hw_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE(HW_component, call_hw_pragma);
}

void tree_node_dup::operator()(const call_point_hw_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
   SET_VALUE(HW_component, call_point_hw_pragma);
   SET_VALUE(ID_implementation, call_point_hw_pragma);
   SET_VALUE(recursive, call_point_hw_pragma);
}

void tree_node_dup::operator()(const issue_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_dup::operator()(const profiling_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_dup::operator()(const blackbox_pragma* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}

void tree_node_dup::operator()(const statistical_profiling* obj, unsigned int& mask)
{
   THROW_ASSERT(obj == curr_tree_node_ptr, "wrong factory setup");
   tree_node_mask::operator()(obj, mask);
}
