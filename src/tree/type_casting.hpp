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
 * @file type_casting.hpp
 * @brief tree node visitor collecting the types used in type casting
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef TYPE_CASTING_HPP
#define TYPE_CASTING_HPP

/// Tree include
#include "tree_node.hpp"

/// Utility include
#include "refcount.hpp"
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(type_casting);
//@}

#define LOCAL_OBJ_NOT_SPECIALIZED_SEQ                                                                                                                                                                                                                         \
   (translation_unit_decl)(label_decl)(using_decl)(void_cst)(void_type)(template_type_parm)(set_type)(qual_union_type)(offset_type)(lang_type)(CharType)(nullptr_type)(boolean_type)(typename_type)(none)(vec_new_expr)                                       \
       TERNARY_EXPRESSION_TREE_NODES(ctor_initializer)(trait_expr)(template_id_expr)(placeholder_expr)(new_expr)(gimple_resx)(gimple_predict)(gimple_nop)QUATERNARY_EXPRESSION_TREE_NODES(modop_expr)(PointToSolution)(omp_atomic_pragma)(abs_expr)(          \
           addr_expr)(arrow_expr)(bit_not_expr)(buffer_ref)(card_expr)(cleanup_point_expr)(conj_expr)(convert_expr)(exit_expr)(fix_ceil_expr)(fix_floor_expr)(fix_round_expr)(fix_trunc_expr)(float_expr)(imagpart_expr)(misaligned_indirect_ref)(loop_expr)( \
           negate_expr)(non_lvalue_expr)(nop_expr)(realpart_expr)(reference_expr)(reinterpret_cast_expr)(sizeof_expr)(static_cast_expr)(throw_expr)(truth_not_expr)(unsave_expr)(va_arg_expr)(view_convert_expr)(reduc_max_expr)(reduc_min_expr)(             \
           reduc_plus_expr)(vec_unpack_hi_expr)(vec_unpack_lo_expr)(vec_unpack_float_hi_expr)(vec_unpack_float_lo_expr)(assert_expr)(bit_and_expr)(bit_ior_expr)(bit_xor_expr)(catch_expr)(ceil_div_expr)(ceil_mod_expr)(complex_expr)(compound_expr)(        \
           eh_filter_expr)(eq_expr)(exact_div_expr)(fdesc_expr)(floor_div_expr)(floor_mod_expr)(ge_expr)(gt_expr)(goto_subroutine)(in_expr)(init_expr)(le_expr)(lrotate_expr)(lshift_expr)(lt_expr)(max_expr)(min_expr)(minus_expr)(modify_expr)(mult_expr)(  \
           mult_highpart_expr)(ne_expr)(ordered_expr)(plus_expr)(pointer_plus_expr)(postdecrement_expr)(postincrement_expr)(predecrement_expr)(preincrement_expr)(range_expr)(paren_expr)(rdiv_expr)(round_div_expr)(round_mod_expr)(rrotate_expr)(           \
           rshift_expr)(set_le_expr)(trunc_div_expr)(trunc_mod_expr)(truth_and_expr)(truth_andif_expr)(truth_or_expr)(truth_orif_expr)(truth_xor_expr)(try_catch_expr)(try_finally)(uneq_expr)(ltgt_expr)(unge_expr)(ungt_expr)(unle_expr)(unlt_expr)(        \
           unordered_expr)(widen_sum_expr)(widen_mult_expr)(with_size_expr)(vec_lshift_expr)(vec_rshift_expr)(widen_mult_hi_expr)(widen_mult_lo_expr)(vec_pack_trunc_expr)(vec_pack_sat_expr)(vec_pack_fix_trunc_expr)(vec_extracteven_expr)(                 \
           vec_extractodd_expr)(vec_interleavehigh_expr)(vec_interleavelow_expr)(extract_bit_expr)(sat_plus_expr)(sat_minus_expr)

struct type_casting : public tree_node_visitor
{
   /// default constructor
   explicit type_casting(CustomUnorderedSet<unsigned int>& _types) : types(_types)
   {
   }

   void operator()(const mem_ref* obj, unsigned int& mask) override;

   void operator()(const indirect_ref* obj, unsigned int& mask) override;
   /// tree_node visitors
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO_DECLO, BOOST_PP_EMPTY, OBJ_SPECIALIZED_SEQ)
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACROO, BOOST_PP_EMPTY, LOCAL_OBJ_NOT_SPECIALIZED_SEQ)

 private:
   /// set of types used in type casting
   CustomUnorderedSet<unsigned int>& types;

   /// already visited
   CustomUnorderedSet<unsigned int> visited;
};

#endif
