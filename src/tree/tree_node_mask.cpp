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
 * @file tree_node_mask.cpp
 * @brief tree node mask. This class factorize the mask initialization common to all visitor classes.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "tree_node_mask.hpp"
#include "exceptions.hpp" // for THROW_ERROR
#include "ext_tree_node.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"
#include <string> // for operator+, string

#define CREATE_TREE_NODE_CASE_BODY(tree_node_name, node_id)

void tree_node_mask::operator()(const tree_node* obj, unsigned int&)
{
   THROW_ERROR("tree_node yet supported: " + std::string(obj->get_kind_text()));
}

void tree_node_mask::operator()(const WeightedNode*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, WeightedNode::tree_node);
}

void tree_node_mask::operator()(const tree_reindex* obj, unsigned int&)
{
   THROW_ERROR("tree_node yet supported: " + std::string(obj->get_kind_text()));
}

void tree_node_mask::operator()(const attr*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const srcp*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const decl_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, decl_node::srcp);
}

void tree_node_mask::operator()(const expr_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, expr_node::WeightedNode);
   SET_VISIT_INDEX(mask, expr_node::srcp);
}

void tree_node_mask::operator()(const gimple_node*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, expr_node::WeightedNode);
   SET_VISIT_INDEX(mask, expr_node::srcp);
}

void tree_node_mask::operator()(const unary_expr*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, unary_expr::expr_node);
}

void tree_node_mask::operator()(const binary_expr*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, binary_expr::expr_node);
}

void tree_node_mask::operator()(const ternary_expr*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, ternary_expr::expr_node);
}

void tree_node_mask::operator()(const quaternary_expr*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, quaternary_expr::expr_node);
}

void tree_node_mask::operator()(const type_node*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const memory_tag*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, memory_tag::decl_node);
}

void tree_node_mask::operator()(const cst_node*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const error_mark*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const array_type*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, array_type::type_node);
}

void tree_node_mask::operator()(const gimple_asm*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, gimple_asm::gimple_node);
}

void tree_node_mask::operator()(const baselink*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const gimple_bind*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, gimple_bind::expr_node);
}

void tree_node_mask::operator()(const binfo*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const block*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const call_expr*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, call_expr::expr_node);
}

void tree_node_mask::operator()(const aggr_init_expr*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, aggr_init_expr::call_expr);
}

void tree_node_mask::operator()(const gimple_call*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, gimple_call::gimple_node);
}

void tree_node_mask::operator()(const case_label_expr*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, case_label_expr::expr_node);
}

void tree_node_mask::operator()(const cast_expr*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, cast_expr::expr_node);
}

void tree_node_mask::operator()(const complex_cst*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, complex_cst::cst_node);
}

void tree_node_mask::operator()(const complex_type*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, complex_type::type_node);
}

void tree_node_mask::operator()(const gimple_cond*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, gimple_cond::gimple_node);
}

void tree_node_mask::operator()(const const_decl*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, const_decl::decl_node);
}

void tree_node_mask::operator()(const constructor*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const enumeral_type*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, enumeral_type::type_node);
}

void tree_node_mask::operator()(const expr_stmt*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const field_decl*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, field_decl::decl_node);
   SET_VISIT_INDEX(mask, field_decl::attr);
}

void tree_node_mask::operator()(const function_decl*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, function_decl::decl_node);
   SET_VISIT_INDEX(mask, function_decl::attr);
}

void tree_node_mask::operator()(const function_type*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, function_type::type_node);
}

void tree_node_mask::operator()(const gimple_assign*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, gimple_assign::gimple_node);
}

void tree_node_mask::operator()(const gimple_goto*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, gimple_goto::gimple_node);
}

void tree_node_mask::operator()(const handler*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const identifier_node*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const integer_cst*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, integer_cst::cst_node);
}

void tree_node_mask::operator()(const integer_type*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, integer_type::type_node);
}

void tree_node_mask::operator()(const gimple_label*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, gimple_label::gimple_node);
}

void tree_node_mask::operator()(const method_type*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, method_type::function_type);
}

void tree_node_mask::operator()(const namespace_decl*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, namespace_decl::decl_node);
}

void tree_node_mask::operator()(const overload*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const parm_decl*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, parm_decl::decl_node);
}

void tree_node_mask::operator()(const gimple_phi*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, gimple_phi::gimple_node);
}

void tree_node_mask::operator()(const pointer_type*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, pointer_type::type_node);
}

void tree_node_mask::operator()(const real_cst*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, real_cst::cst_node);
}

void tree_node_mask::operator()(const real_type*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, real_type::type_node);
}

void tree_node_mask::operator()(const record_type*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, record_type::type_node);
}

void tree_node_mask::operator()(const reference_type*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, reference_type::type_node);
}

void tree_node_mask::operator()(const result_decl*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, result_decl::decl_node);
}

void tree_node_mask::operator()(const gimple_return*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, gimple_return::gimple_node);
}

void tree_node_mask::operator()(const return_stmt*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const scope_ref*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, scope_ref::expr_node);
}

void tree_node_mask::operator()(const ssa_name*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const statement_list*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const string_cst*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, string_cst::cst_node);
}

void tree_node_mask::operator()(const gimple_switch*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, gimple_switch::gimple_node);
}

void tree_node_mask::operator()(const target_expr*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, target_expr::expr_node);
}

void tree_node_mask::operator()(const lut_expr*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, lut_expr::expr_node);
}

void tree_node_mask::operator()(const template_decl*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, template_decl::decl_node);
}

void tree_node_mask::operator()(const template_parm_index*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const tree_list*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, tree_list::WeightedNode);
}

void tree_node_mask::operator()(const tree_vec*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const try_block*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const type_decl*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, type_decl::decl_node);
}

void tree_node_mask::operator()(const union_type*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, union_type::type_node);
}

void tree_node_mask::operator()(const var_decl*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, var_decl::decl_node);
   SET_VISIT_INDEX(mask, var_decl::attr);
}

void tree_node_mask::operator()(const vector_cst*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, vector_cst::cst_node);
}

void tree_node_mask::operator()(const type_argument_pack*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, type_argument_pack::type_node);
}

void tree_node_mask::operator()(const nontype_argument_pack*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, nontype_argument_pack::expr_node);
}

void tree_node_mask::operator()(const type_pack_expansion*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, type_pack_expansion::type_node);
}

void tree_node_mask::operator()(const expr_pack_expansion*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, expr_pack_expansion::expr_node);
}

void tree_node_mask::operator()(const vector_type*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, vector_type::type_node);
}

void tree_node_mask::operator()(const target_mem_ref*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, target_mem_ref::WeightedNode);
}

void tree_node_mask::operator()(const target_mem_ref461*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, target_mem_ref461::WeightedNode);
}

void tree_node_mask::operator()(const bloc*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const gimple_while*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, gimple_while::gimple_node);
}

void tree_node_mask::operator()(const gimple_for*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, gimple_for::gimple_while);
}

void tree_node_mask::operator()(const gimple_multi_way_if*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, gimple_multi_way_if::gimple_node);
}

void tree_node_mask::operator()(const gimple_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, gimple_pragma::gimple_node);
}

void tree_node_mask::operator()(const null_node*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const omp_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const omp_for_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, omp_for_pragma::omp_pragma);
}

void tree_node_mask::operator()(const omp_simd_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, omp_simd_pragma::omp_pragma);
}

void tree_node_mask::operator()(const omp_declare_simd_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, omp_declare_simd_pragma::omp_pragma);
}

void tree_node_mask::operator()(const omp_target_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, omp_target_pragma::omp_pragma);
}

void tree_node_mask::operator()(const omp_task_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, omp_task_pragma::omp_pragma);
}

void tree_node_mask::operator()(const omp_critical_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, omp_critical_pragma::omp_pragma);
}

void tree_node_mask::operator()(const omp_parallel_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, omp_parallel_pragma::omp_pragma);
}

void tree_node_mask::operator()(const omp_sections_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, omp_sections_pragma::omp_pragma);
}

void tree_node_mask::operator()(const omp_parallel_sections_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, omp_parallel_sections_pragma::omp_pragma);
}

void tree_node_mask::operator()(const omp_section_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, omp_section_pragma::omp_pragma);
}

void tree_node_mask::operator()(const map_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const call_hw_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, call_hw_pragma::map_pragma);
}

void tree_node_mask::operator()(const call_point_hw_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, call_point_hw_pragma::map_pragma);
}

void tree_node_mask::operator()(const issue_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const profiling_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
}

void tree_node_mask::operator()(const blackbox_pragma*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, blackbox_pragma::issue_pragma);
}

void tree_node_mask::operator()(const statistical_profiling*, unsigned int& mask)
{
   mask = NO_VISIT;
   SET_VISIT_INDEX(mask, statistical_profiling::profiling_pragma);
}
