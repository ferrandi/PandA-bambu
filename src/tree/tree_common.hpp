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
 * @file tree_common.hpp
 * @brief This C++ header file contains common macros for the tree structure
 *
 * This C++ header file define some macros useful during the tree structure hierarchy definition and manipulation.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 *
 */
#ifndef TREE_COMMON_HPP
#define TREE_COMMON_HPP

/// STD include
#include <string>

/// Utility include
#include "refcount.hpp"
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#define TREE_NODE_NAME(r, data, elem) #elem,

#define TREE_NODE_KIND(r, data, elem) BOOST_PP_CAT(elem, _K),

#define UNARY_EXPRESSION_TREE_NODES                                                                                                                                                                                                               \
   (abs_expr)(addr_expr)(arrow_expr)(bit_not_expr)(buffer_ref)(card_expr)(cleanup_point_expr)(conj_expr)(convert_expr)(exit_expr)(fix_ceil_expr)(fix_floor_expr)(fix_round_expr)(fix_trunc_expr)(float_expr)(imagpart_expr)(indirect_ref)(        \
       misaligned_indirect_ref)(loop_expr)(negate_expr)(non_lvalue_expr)(nop_expr)(realpart_expr)(reference_expr)(reinterpret_cast_expr)(sizeof_expr)(static_cast_expr)(throw_expr)(truth_not_expr)(unsave_expr)(va_arg_expr)(view_convert_expr)( \
       reduc_max_expr)(reduc_min_expr)(reduc_plus_expr)(vec_unpack_hi_expr)(vec_unpack_lo_expr)(vec_unpack_float_hi_expr)(vec_unpack_float_lo_expr)(paren_expr)

#define BINARY_EXPRESSION_TREE_NODES                                                                                                                                                                                                                           \
   (assert_expr)(bit_and_expr)(bit_ior_expr)(bit_xor_expr)(catch_expr)(ceil_div_expr)(ceil_mod_expr)(complex_expr)(compound_expr)(eh_filter_expr)(eq_expr)(exact_div_expr)(fdesc_expr)(floor_div_expr)(floor_mod_expr)(ge_expr)(gt_expr)(goto_subroutine)(     \
       in_expr)(init_expr)(le_expr)(lrotate_expr)(lshift_expr)(lt_expr)(max_expr)(mem_ref)(min_expr)(minus_expr)(modify_expr)(mult_expr)(mult_highpart_expr)(ne_expr)(ordered_expr)(plus_expr)(pointer_plus_expr)(postdecrement_expr)(postincrement_expr)(     \
       predecrement_expr)(preincrement_expr)(range_expr)(rdiv_expr)(round_div_expr)(round_mod_expr)(rrotate_expr)(rshift_expr)(set_le_expr)(trunc_div_expr)(trunc_mod_expr)(truth_and_expr)(truth_andif_expr)(truth_or_expr)(truth_orif_expr)(truth_xor_expr)( \
       try_catch_expr)(try_finally)(uneq_expr)(ltgt_expr)(unge_expr)(ungt_expr)(unle_expr)(unlt_expr)(unordered_expr)(widen_sum_expr)(widen_mult_expr)(with_size_expr)(vec_lshift_expr)(vec_rshift_expr)(widen_mult_hi_expr)(widen_mult_lo_expr)(              \
       vec_pack_trunc_expr)(vec_pack_sat_expr)(vec_pack_fix_trunc_expr)(vec_extracteven_expr)(vec_extractodd_expr)(vec_interleavehigh_expr)(vec_interleavelow_expr)(extract_bit_expr)(sat_plus_expr)(sat_minus_expr)

#define TERNARY_EXPRESSION_TREE_NODES \
   (bit_field_ref)(bit_ior_concat_expr)(component_ref)(cond_expr)(vec_cond_expr)(vec_perm_expr)(dot_prod_expr)(obj_type_ref)(save_expr)(ternary_plus_expr)(ternary_pm_expr)(ternary_mp_expr)(ternary_mm_expr)(vtable_ref)(with_cleanup_expr)

#define QUATERNARY_EXPRESSION_TREE_NODES (array_range_ref)(array_ref)

#define MISCELLANEOUS_EXPR_TREE_NODES \
   (call_expr)(aggr_init_expr)(case_label_expr)(lut_expr)(modop_expr)(new_expr)(placeholder_expr)(scope_ref)(target_expr)(target_mem_ref)(target_mem_ref461)(template_id_expr)(trait_expr)(vec_new_expr)(nontype_argument_pack)(expr_pack_expansion)(cast_expr)

#define PANDA_GIMPLE_NODES (gimple_for)(gimple_multi_way_if)(gimple_pragma)(gimple_while)

#define GIMPLE_NODES (gimple_asm)(gimple_assign)(gimple_bind)(gimple_call)(gimple_cond)(gimple_goto)(gimple_label)(gimple_nop)(gimple_phi)(gimple_predict)(gimple_resx)(gimple_return)(gimple_switch)

#define MISCELLANEOUS_OBJ_TREE_NODES \
   (baselink)(binfo)(block)(constructor)(ctor_initializer)(error_mark)(expr_stmt)(handler)(identifier_node)(none)(null_node)(overload)(return_stmt)(ssa_name)(statement_list)(template_parm_index)(tree_list)(tree_reindex)(tree_vec)(try_block)

#define TYPE_NODE_TREE_NODES                                                                                                                                                                                                                                \
   (array_type)(boolean_type)(CharType)(nullptr_type)(type_pack_expansion)(complex_type)(enumeral_type)(function_type)(integer_type)(lang_type)(method_type)(offset_type)(pointer_type)(qual_union_type)(real_type)(record_type)(reference_type)(set_type)( \
       template_type_parm)(typename_type)(union_type)(vector_type)(type_argument_pack)(void_type)

#define CONST_OBJ_TREE_NODES (complex_cst)(integer_cst)(real_cst)(string_cst)(vector_cst)(void_cst)

#define DECL_NODE_TREE_NODES (const_decl)(field_decl)(function_decl)(namespace_decl)(parm_decl)(result_decl)(template_decl)(label_decl)(translation_unit_decl)(type_decl)(using_decl)(var_decl)

#define CPP_NODES                                                                                                                                                                                                                                          \
   (aggr_init_expr)(ctor_initializer)(expr_stmt)(if_stmt)(for_stmt)(handler)(modop_expr)(new_expr)(nullptr_type)(overload)(return_stmt)(scope_ref)(template_decl)(template_id_expr)(template_parm_index)(trait_expr)(try_block)(vec_new_expr)(while_stmt)( \
       nontype_argument_pack)(expr_pack_expansion)

#define CPP_STMT_NODES (do_stmt)(if_stmt)(for_stmt)(while_stmt)

/// basic block tree_node
#define BASIC_BLOCK_TREE_NODES (bloc)

/// pragma tree_node
#define PANDA_PRAGMA_TREE_NODES                                                                                                                                                                                                                           \
   (blackbox_pragma)(call_hw_pragma)(call_point_hw_pragma)(issue_pragma)(map_pragma)(omp_atomic_pragma)(omp_critical_pragma)(omp_declare_simd_pragma)(omp_for_pragma)(omp_parallel_pragma)(omp_parallel_sections_pragma)(omp_pragma)(omp_section_pragma)( \
       omp_sections_pragma)(omp_simd_pragma)(omp_target_pragma)(omp_task_pragma)(profiling_pragma)(statistical_profiling)

#define PANDA_EXTENSION_TREE_NODES \
   PANDA_PRAGMA_TREE_NODES         \
   PANDA_GIMPLE_NODES

enum kind : int
{
   BOOST_PP_SEQ_FOR_EACH(TREE_NODE_KIND, BOOST_PP_EMPTY, BINARY_EXPRESSION_TREE_NODES) BOOST_PP_SEQ_FOR_EACH(TREE_NODE_KIND, BOOST_PP_EMPTY, CONST_OBJ_TREE_NODES) BOOST_PP_SEQ_FOR_EACH(TREE_NODE_KIND, BOOST_PP_EMPTY, CPP_STMT_NODES)
       BOOST_PP_SEQ_FOR_EACH(TREE_NODE_KIND, BOOST_PP_EMPTY, DECL_NODE_TREE_NODES) BOOST_PP_SEQ_FOR_EACH(TREE_NODE_KIND, BOOST_PP_EMPTY, GIMPLE_NODES) BOOST_PP_SEQ_FOR_EACH(TREE_NODE_KIND, BOOST_PP_EMPTY, MISCELLANEOUS_EXPR_TREE_NODES)
           BOOST_PP_SEQ_FOR_EACH(TREE_NODE_KIND, BOOST_PP_EMPTY, MISCELLANEOUS_OBJ_TREE_NODES) BOOST_PP_SEQ_FOR_EACH(TREE_NODE_KIND, BOOST_PP_EMPTY, PANDA_EXTENSION_TREE_NODES)
               BOOST_PP_SEQ_FOR_EACH(TREE_NODE_KIND, BOOST_PP_EMPTY, QUATERNARY_EXPRESSION_TREE_NODES) BOOST_PP_SEQ_FOR_EACH(TREE_NODE_KIND, BOOST_PP_EMPTY, TERNARY_EXPRESSION_TREE_NODES)
                   BOOST_PP_SEQ_FOR_EACH(TREE_NODE_KIND, BOOST_PP_EMPTY, TYPE_NODE_TREE_NODES) BOOST_PP_SEQ_FOR_EACH(TREE_NODE_KIND, BOOST_PP_EMPTY, UNARY_EXPRESSION_TREE_NODES(last_tree))
};

/**
 * Macro which define a function that return the parameter as a enum kind.
 */
#define GET_KIND(meth)                 \
   enum kind get_kind() const override \
   {                                   \
      return (meth##_K);               \
   }

#endif
